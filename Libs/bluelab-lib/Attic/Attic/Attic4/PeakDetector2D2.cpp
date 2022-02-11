//
//  PeakDetector2D2.cpp
//  BL-DUET
//
//  Created by applematuer on 5/5/20.
//
//

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include <BLUtils.h>

#include <FillTriangle.hpp>

#include "PeakDetector2D2.h"

#define TWO_PI 6.28318530717959

#define INF 1e15
#define EPS 1e-15

#define MAX_PIX_ADVANCE 8

#define STEP 1.0 //0.01 //0.25

// When do we stop growing the circle?
// GOOD with 0.1
#define INTENSITY_RATIO 0.1 //0.25 //0.001 //0.25 //0.1

#define USE_THRESHOLD_WIDTH 1

// At initialization, enlarge the circle by a small epsilon
// Will avoid degenrted cases of the peak center lying exactly
// on the contour.
// (these degenerated cases make the whole bounging box to be filled
// when filling with point inside polygon method)
#define HACK_POINT_INIT 1
#define HACK_POINT_INIT_EPS 0.001

// Draw the contours around the filled areas
// (avoid aving some empty frontiers between contours sometimes
#define HACK_DRAW_AROUND_FILL 1


PeakDetector2D2::PeakDetector2D2(int numPoints)
{
    mNumPoints = numPoints;
}

PeakDetector2D2::~PeakDetector2D2() {}

void
PeakDetector2D2::DetectPeaks(int width, int height,
                             const WDL_TypedBuf<BL_FLOAT> &image,
                             const vector<Peak> &peaks,
                             BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                             WDL_TypedBuf<int> *mask)
{
    mask->Resize(image.GetSize());
    BLUtils::FillAllZero(mask);
    
    for (int i = 0; i < peaks.size(); i++)
    {
        const Peak &p = peaks[i];
        
        DetectPeak(width, height, image, p, threshold, thresholdWidth, mask);
    }
}

// Version with filled masks, to avoid overlap of contours
void
PeakDetector2D2::DetectPeaksNoOverlap(int width, int height,
                                      const WDL_TypedBuf<BL_FLOAT> &image,
                                      const vector<Peak> &peaks,
                                      BL_FLOAT threshold,
                                      BL_FLOAT thresholdWidth,
                                      WDL_TypedBuf<int> *mask)
{
    if (mNumPoints < 3)
        return;
    
    mask->Resize(image.GetSize());
    BLUtils::FillAllZero(mask);
    
    // Init all points
    vector<vector<Point> > points;
    points.resize(peaks.size());
    for (int i = 0; i < peaks.size(); i++)
    {
        const Peak &peak = peaks[i];
        
        InitPoints(width, height, image, peak, &points[i]);
    }
    
    // Overlap masks
    //
    
    // This mask is a mask with all the previous contours filled
    WDL_TypedBuf<int> overlapPrevMask;
    overlapPrevMask.Resize(mask->GetSize());
   
    // This mask is a mask with the new contours filling progressively
    // contour by contour
    WDL_TypedBuf<int> overlapNewMask;
    overlapNewMask.Resize(mask->GetSize());
    
    // Iterate
    while(true)
    {
        // Clear the mask
        BLUtils::FillAllZero(&overlapPrevMask);
        BLUtils::FillAllZero(&overlapNewMask);
        
        int maxNumPointsAdvance = 0;
        
        // Fill all the previous contours in the mask before enlarging
        for (int i = 0; i < peaks.size(); i++)
        {
            const Peak &peak = peaks[i];
            
            FillPeakContour(width, height, &overlapPrevMask, peak, points[i]);
        }
        
        // Advance each circle step by step
        for (int i = 0; i < peaks.size(); i++)
        {
            const Peak &peak = peaks[i];
            
            // First, find how much we grow the circle
            //
        
            // We find the minimum gray value
            // which will be the target gray value (or target height)
            BL_FLOAT targetValue = FindTargetGrayValue(width, height, image, points[i]);
        
            // For each point, we advance until we are just below the target gray value
            int numPointsAdvance = EnlargeCircle(width, height, image,
                                                 peak,
                                                 targetValue,
                                                 threshold, thresholdWidth,
                                                 &points[i],
                                                 &overlapPrevMask, &overlapNewMask);
        
            if (numPointsAdvance > maxNumPointsAdvance)
                maxNumPointsAdvance = numPointsAdvance;
            
            // Fill the overlap mask, in order to let a track of the progression of the circles
            //DrawPeakContour(width, height, &overlapMask, peak, points[i]);
            FillPeakContour(width, height, &overlapNewMask, peak, points[i]);
        }
        
        // We have finished advancing
        if (maxNumPointsAdvance == 0)
            break;
    }
    
    // At the end, fill the masks
    for (int i = 0; i < peaks.size(); i++)
    {
        const Peak &peak = peaks[i];
        
        //DrawPeakContour(width, height, mask, peak, points[i]);
        FillPeakContour(width, height, mask, peak, points[i]);
    }
}

// Version with overlap of contours
// (old version)
void
PeakDetector2D2::DetectPeak(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                            const Peak &peak, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                            WDL_TypedBuf<int> *mask)
{    
    if (mNumPoints < 3)
        return;
    
    vector<Point> points;
    InitPoints(width, height, image, peak, &points);
    
    // Iterate
    while(true)
    {
        // First, find how much we grow the circle
        //
        
        // We find the minimum gray value
        // which will be the target gray value (or target height)
        BL_FLOAT targetValue = FindTargetGrayValue(width, height, image, points);
        
        // For each point, we advance until we are just below the target gray value
        int numPointsAdvance = EnlargeCircle(width, height, image,
                                             peak,
                                             targetValue,
                                             threshold, thresholdWidth,
                                             &points);
        
        // We have finished advancing
        if (numPointsAdvance == 0)
            break;
    }
    
    // At the end, fill the mask
    DrawPeakContour(width, height, mask, peak, points);
}

void
PeakDetector2D2::InitPoints(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                            const Peak &peak, vector<Point> *points)
{
    // Just in case.
    points->clear();
    
    // Compute the peak intensity
    // (just in case it was not already
    BL_FLOAT peakIntensity = image.Get()[peak.mX + peak.mY*width];
    
    // Create the points and the directions
    for (int i = 0; i < mNumPoints; i++)
    {
        Point p;
        p.mX = peak.mX;
        p.mY = peak.mY;
        
        BL_FLOAT t = ((BL_FLOAT)i)/mNumPoints;
        BL_FLOAT angle = t*TWO_PI;
        
        p.mDirX = std::cos(angle);
        p.mDirY = std::sin(angle);
        
        p.mIntensity = peakIntensity;
        
        points->push_back(p);
    }
    
#if HACK_POINT_INIT
    // Grow a little the circle, to avoid degenerated cases,
    // when a point of the circle doesn't evolve, the peak center
    // stays on the contour.
    for (int i = 0; i < mNumPoints; i++)
    {
        Point &p = (*points)[i];
        p.mX += p.mDirX*HACK_POINT_INIT_EPS;
        p.mY += p.mDirY*HACK_POINT_INIT_EPS;
    }
#endif
}

BL_FLOAT
PeakDetector2D2::FindTargetGrayValue(int width, int height,
                                    const WDL_TypedBuf<BL_FLOAT> &image,
                                    const vector<Point> &points)
{
    BL_FLOAT targetValue = INF;
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        int x = p.mX + p.mDirX*STEP;
        int y = p.mY + p.mDirY*STEP;
        
        if ((x >= 0) && (x < width) &&
            (y >= 0) && (y < height))
        {
            BL_FLOAT val = image.Get()[x + y*width];
            if (val < targetValue)
                targetValue = val;
        }
    }
    
    return targetValue;
}

int
PeakDetector2D2::EnlargeCircle(int width, int height,
                               const WDL_TypedBuf<BL_FLOAT> &image,
                               const Peak &peak,
                               BL_FLOAT targetValue,
                               BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                               vector<Point> *points,
                               const WDL_TypedBuf<int> *overlapPrevMask,
                               const WDL_TypedBuf<int> *overlapNewMask)
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
            
            BL_FLOAT val = image.Get()[x + y*width];
            
            if (val <= targetValue)
                break;
            
            // Threshold is max possible intensity
            // Better to limit with the fixed threshold than
            // with the peaks intensities.
            
#if !USE_THRESHOLD_WIDTH
            if (val < threshold*INTENSITY_RATIO)
                break;
#else
            if (val < thresholdWidth*peak.mIntensity)
                break;
#endif
            
            // We increase again
            if (val > p.mIntensity)
                break;
            
            if ((overlapPrevMask != NULL) || (overlapNewMask != NULL))
            {
                // Consider the next position
                int x1 = p.mX + p.mDirX*STEP;
                int y1 = p.mY + p.mDirY*STEP;
                
                if ((x1 >= 0) && (x1 < width) &&
                    (y1 >= 0) && (y1 < height))
                {
                    if (overlapPrevMask != NULL)
                    {
                        int maskVal = overlapPrevMask->Get()[x1 + y1*width];
                
                        // If the mask value is set, and the mask value is another peak value...
                        // stop grow this point
                        //if ((maskVal > EPS) && (std::fabs(maskVal - peak.mId) > EPS))
                        if ((maskVal > 0) && (maskVal != peak.mId)) // (std::fabs(maskVal - peak.mId) > EPS))
                            break;
                    }
                    
                    if (overlapNewMask != NULL)
                    {
                        int maskVal = overlapNewMask->Get()[x1 + y1*width];
                        
                        // If the mask value is set, and the mask value is another peak value...
                        // stop grow this point
                        //if ((maskVal > EPS) && (std::fabs(maskVal - peak.mId) > EPS))
                        //    break;
                        if ((maskVal > 0) && (maskVal != peak.mId)) // (std::fabs(maskVal - peak.mId) > EPS))
                            break;
                    }
                }
            }
            
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
PeakDetector2D2::DrawPeakContour(int width, int height,
                                WDL_TypedBuf<int> *mask,
                                const Peak &peak,
                                const vector<Point> &points)
{
    // First fill the borders (the final circle shape)
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p0 = points[i];
        const Point &p1 = points[(i + 1) % points.size()];
        
        BL_FLOAT v[2] = { p1.mX - p0.mX, p1.mY - p0.mY };
        BL_FLOAT norm = std::sqrt(v[0]*v[0] + v[1]*v[1]);
        
        if (norm > 0.0)
        {
            v[0] /= norm;
            v[1] /= norm;
        }
        
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

// Fill with optimized triangle fill
void
PeakDetector2D2::FillPeakContour(int width, int height,
                                 WDL_TypedBuf<int> *mask,
                                 const Peak &peak,
                                 const vector<Point> &points)
{
#if HACK_DRAW_AROUND_FILL
    DrawPeakContour(width, height, mask, peak, points);
#endif
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p0 = points[i];
        const Point &p1 = points[(i + 1) % points.size()];
        
        int v0[2] = { (int)p0.mX, (int)p0.mY };
        int v1[2] = { (int)p1.mX, (int)p1.mY };
        int v2[2] = { (int)peak.mX, (int)peak.mY };
        
        FillTriangle<int>::Fill(width, height, mask, v0, v1, v2, peak.mId);
    }
    
#if HACK_DRAW_AROUND_FILL
    // When drawing countour around the filled region,
    // there are sometimes 1 pixel that stays at 0 in the middle,
    // close to the frontier.
    //
    // So fill these holes!
    //
    int bbox[2][2];
    ComputeBoundingBox(points, bbox);
    
    // Check that the bounding box doesn't go outside image
    ClipBoundingBox(bbox, width, height);
    
    FillMaskHoles(width, height, mask, bbox, peak);
#endif
}

// Costly
void
PeakDetector2D2::FillMaskHoles(int width, int height,
                               WDL_TypedBuf<int> *mask,
                               const int bbox[2][2], const Peak &peak)
{
#define NUM_NEIGHBORS 3 //2
    
    WDL_TypedBuf<int> maskCopy = *mask;
    for (int i = bbox[0][0]; i <= bbox[1][0]; i++)
    {
        for (int j = bbox[0][1]; j <= bbox[1][1]; j++)
        {
            BL_FLOAT val = mask->Get()[i + j*width];
            if (val > 0.0)
                continue;
            
            int numNeighbors = 0;
            for (int i0 = -1; i0 <= 1; i0++)
            {
                for (int j0 = -1; j0 <= 1; j0++)
                {
                    // 4-connexity ?
                    //if ((i0 != 0) && (j0 != 0))
                    //    continue;
                    
                    int x = i + i0;
                    int y = j + j0;
                    
                    if ((x >= 0) && (x < width) &&
                        (y >= 0) && (y < height))
                    {
                        int val0 = maskCopy.Get()[x + y*width];
                        //if (std::fabs(val0 - peak.mId) < EPS)
                        if (val0 == peak.mId)
                        {
                            numNeighbors++;
                            
                            if (numNeighbors >= NUM_NEIGHBORS)
                                break;
                        }
                    }
                }
                
                if (numNeighbors >= NUM_NEIGHBORS)
                    break;
            }
            
            if (numNeighbors >= NUM_NEIGHBORS)
                mask->Get()[i + j*width] = peak.mId;
        }
    }
}

void
PeakDetector2D2::ComputeBoundingBox(const vector<Point> &points, int bbox[2][2])
{
    bbox[0][0] = 0;
    bbox[0][1] = 0;
    bbox[1][0] = 0;
    bbox[1][1] = 0;
    
    if (points.empty())
        return;
    
    bbox[0][0] = points[0].mX;
    bbox[0][1] = points[0].mY;
    bbox[1][0] = points[0].mX;
    bbox[1][1] = points[0].mY;
    
    for (int i = 1; i < points.size(); i++)
    {
        const Point &p = points[i];
        if (p.mX < bbox[0][0])
            bbox[0][0] = p.mX;
        if (p.mY < bbox[0][1])
            bbox[0][1] = p.mY;
        
        if (p.mX > bbox[1][0])
            bbox[1][0] = p.mX;
        if (p.mY > bbox[1][1])
            bbox[1][1] = p.mY;
    }
}

void
PeakDetector2D2::ClipBoundingBox(int bbox[2][2], int width, int height)
{
    if (bbox[0][0] < 0)
        bbox[0][0] = 0;
    if (bbox[0][1] < 0)
        bbox[0][1] = 0;
    
    if (bbox[1][0] > width - 1)
        bbox[1][0] = width - 1;
    if (bbox[1][1] > height - 1)
        bbox[1][1] = height - 1;
}
