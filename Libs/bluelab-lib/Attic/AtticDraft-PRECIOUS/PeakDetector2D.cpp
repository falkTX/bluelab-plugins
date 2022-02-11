//
//  PeakDetector2D.cpp
//  BL-DUET
//
//  Created by applematuer on 5/5/20.
//
//

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include <Utils.h>

#include "PeakDetector2D.h"

#define TWO_PI 6.28318530717959

#define MAX_PIX_ADVANCE 8

#define STEP 1.0 //0.01 //0.25 //1.0 //0.25

// GOOD with 0.1
#define INTENSITY_RATIO 0.1 //0.25 //0.001 //0.25 //0.1


PeakDetector2D::PeakDetector2D(int numPoints)
{
    mNumPoints = numPoints;
}

PeakDetector2D::~PeakDetector2D() {}

void
PeakDetector2D::DetectPeaks(int width, int height, const WDL_TypedBuf<double> &image,
                            const vector<Peak> &peaks, double threshold,
                            WDL_TypedBuf<double> *mask)
{
    mask->Resize(image.GetSize());
    Utils::FillAllZero(mask);
    
    for (int i = 0; i < peaks.size(); i++)
    {
        const Peak &p = peaks[i];
        
        DetectPeak(width, height, image, p, threshold, mask);
    }
}

void
PeakDetector2D::DetectPeak(int width, int height, const WDL_TypedBuf<double> &image,
                           const Peak &peak, double threshold,
                           WDL_TypedBuf<double> *mask)
{
#define INF 1e15
    
    if (mNumPoints < 3)
        return;
    
    vector<Point> points;
    InitPoints(width, height, image, peak, &points);
    
    // Debug
    //FillMask(width, height, mask, peak, points);
    
    // Iterate
    while(true)
    {
        // First, find how much we grow the circle
        //
        
        // We find the minimum gray value
        // which will be the target gray value (or target height)
        double targetValue = FindTargetGrayValue(width, height, image, points);
        
        // For each point, we advance until we are just below the target gray value
        int numPointsAdvance = EnlargeCircle(width, height, image,
                                             targetValue, threshold, &points);
        
        // We have finished advancing
        if (numPointsAdvance == 0)
            break;
        
        // Debug
        //FillMask(width, height, mask, peak, points);
    }
    
    // At the end, fill the mask
    FillMask(width, height, mask, peak, points);
}

void
PeakDetector2D::InitPoints(int width, int height, const WDL_TypedBuf<double> &image,
                           const Peak &peak, vector<Point> *points)
{
    // Just in case.
    points->clear();
    
    // Compute the peak intensity
    // (just in case it was not already
    double peakIntensity = image.Get()[peak.mX + peak.mY*width];
    
    // Create the points and the directions
    for (int i = 0; i < mNumPoints; i++)
    {
        Point p;
        p.mX = peak.mX;
        p.mY = peak.mY;
        
        double t = ((double)i)/mNumPoints;
        double angle = t*TWO_PI;
        
        p.mDirX = cos(angle);
        p.mDirY = sin(angle);
        
        p.mIntensity = peakIntensity;
        
        points->push_back(p);
    }
}

double
PeakDetector2D::FindTargetGrayValue(int width, int height,
                                    const WDL_TypedBuf<double> &image,
                                    const vector<Point> &points)
{
    double targetValue = INF;
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        int x = p.mX + p.mDirX*STEP;
        int y = p.mY + p.mDirY*STEP;
        
        if ((x >= 0) && (x < width) &&
            (y >= 0) && (y < height))
        {
            double val = image.Get()[x + y*width];
            if (val < targetValue)
                targetValue = val;
        }
    }
    
    return targetValue;
}

int
PeakDetector2D::EnlargeCircle(int width, int height,
                              const WDL_TypedBuf<double> &image,
                              double targetValue, double threshold,
                              vector<Point> *points)
{
    int numPointsAdvance = 0;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        bool pointAdvanced = false;
        while(true)
        {
            int x = p.mX;
            int y = p.mY;
            
            if ((x < 0) || (x >= width) ||
                (y < 0) || (y >= height))
                break;
            
            double val = image.Get()[x + y*width];
            
            if (val <= targetValue)
                break;
            
            // Threshold is max possible intensity
            // Better to limit with the fixed threshold than
            // with the peaks intensities.
            
            //if (val < peakIntensity*INTENSITY_RATIO)
            if (val < threshold*INTENSITY_RATIO)
                break;
            
            // We increase again
            if (val > p.mIntensity)
                break;
            
            p.mIntensity = val;
            
            // Advance by 1 pixel
            p.mX += p.mDirX*STEP;
            p.mY += p.mDirY*STEP;
            
            pointAdvanced = true;
        }
        
        if (pointAdvanced)
            numPointsAdvance++;
    }

    return numPointsAdvance;
}

void
PeakDetector2D::FillMask(int width, int height,
                         WDL_TypedBuf<double> *mask,
                         const Peak &peak,
                         const vector<Point> &points)
{
    // First fill the borders (the final circle shape)
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p0 = points[i];
        const Point &p1 = points[(i + 1) % points.size()];
        
        double v[2] = { p1.mX - p0.mX, p1.mY - p0.mY };
        double norm = sqrt(v[0]*v[0] + v[1]*v[1]);
        
        if (norm > 0.0)
        {
            v[0] /= norm;
            v[1] /= norm;
        }
        
        //norm = round(norm);
        
        for (int j = 0; j <= (int)norm; j++)
        {
            int x = p0.mX + j*v[0];
            int y = p0.mY + j*v[1];
            
            if ((x >= 0) && (x < width) &&
                (y >= 0) && (y < height))
            {
                mask->Get()[x + y*width] = peak.mId;
            }
        }
    }
}
