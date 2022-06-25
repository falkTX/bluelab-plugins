/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <FillTriangle.hpp>

#include "PeakDetector2D.h"

//#define TWO_PI 6.28318530717959

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


PeakDetector2D::PeakDetector2D(int numPoints)
{
    mNumPoints = numPoints;
}

PeakDetector2D::~PeakDetector2D() {}

void
PeakDetector2D::DetectPeaks(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                            const vector<Peak> &peaks, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                            WDL_TypedBuf<BL_FLOAT> *mask)
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
PeakDetector2D::DetectPeaksNoOverlap(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                                     const vector<Peak> &peaks, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                                     WDL_TypedBuf<BL_FLOAT> *mask)
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
    WDL_TypedBuf<BL_FLOAT> overlapPrevMask;
    overlapPrevMask.Resize(mask->GetSize());
   
    // This mask is a mask with the new contours filling progressively
    // contour by contour
    WDL_TypedBuf<BL_FLOAT> overlapNewMask;
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
PeakDetector2D::DetectPeak(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                           const Peak &peak, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                           WDL_TypedBuf<BL_FLOAT> *mask)
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
PeakDetector2D::InitPoints(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
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
PeakDetector2D::FindTargetGrayValue(int width, int height,
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
PeakDetector2D::EnlargeCircle(int width, int height,
                              const WDL_TypedBuf<BL_FLOAT> &image,
                              const Peak &peak,
                              BL_FLOAT targetValue,
                              BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                              vector<Point> *points,
                              const WDL_TypedBuf<BL_FLOAT> *overlapPrevMask,
                              const WDL_TypedBuf<BL_FLOAT> *overlapNewMask)
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
            
            //if (val < peakIntensity*INTENSITY_RATIO)
#if !USE_THRESHOLD_WIDTH
            if (val < threshold*INTENSITY_RATIO)
                break;
#else
            //if (val < thresholdWidth)
            //    break;
            
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
                        BL_FLOAT maskVal = overlapPrevMask->Get()[x1 + y1*width];
                
                        // If the mask value is set, and the mask value is another peak value...
                        // stop grow this point
                        if ((maskVal > EPS) && (std::fabs(maskVal - peak.mId) > EPS))
                            break;
                    }
                    
                    if (overlapNewMask != NULL)
                    {
                        BL_FLOAT maskVal = overlapNewMask->Get()[x1 + y1*width];
                        
                        // If the mask value is set, and the mask value is another peak value...
                        // stop grow this point
                        if ((maskVal > EPS) && (std::fabs(maskVal - peak.mId) > EPS))
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
PeakDetector2D::DrawPeakContour(int width, int height,
                                WDL_TypedBuf<BL_FLOAT> *mask,
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

void
PeakDetector2D::FillPeakContour(int width, int height,
                                WDL_TypedBuf<BL_FLOAT> *mask,
                                const Peak &peak,
                                const vector<Point> &points)
{
    //FillPeakContourFloodFill(width, height, mask, peak, points);
    //FillPeakContourTriangles(width, height, mask, peak, points);
    
    FillPeakContourTrianglesOptim(width, height, mask, peak, points);
}

void
PeakDetector2D::FillPeakContourTriangles(int width, int height,
                                         WDL_TypedBuf<BL_FLOAT> *mask,
                                         const Peak &peak,
                                         const vector<Point> &points)
{
#if HACK_DRAW_AROUND_FILL
    DrawPeakContour(width, height, mask, peak, points);
#endif
    
    int bbox[2][2];
    ComputeBoundingBox(points, bbox);
    
    // Check that the bounding box doesn't go outside image
    ClipBoundingBox(bbox, width, height);
    
    // Simple point in convex polygon test based on cross product
    // doesn't work. (We have non convex polygons sometimes).
    //
    // So decompose in triangles and test point in triangle.
    //
    for (int i = bbox[0][0]; i <= bbox[1][0]; i++)
    {
        for (int j = bbox[0][1]; j <= bbox[1][1]; j++)
        {
            bool inside = false;
            for (int k = 0; k < points.size(); k++)
            {
                const Point &p0 = points[k];
                const Point &p1 = points[(k + 1) % points.size()];
                
                BL_FLOAT peakCenter[2] = { (BL_FLOAT)peak.mX, (BL_FLOAT)peak.mY };
                
                bool insideTriangle = PointInsideTriangle(p0, p1, peakCenter, i, j);
                if (insideTriangle)
                {
                    inside = true;
                    
                    break;
                }
            }
            
            if (inside)
            {
                mask->Get()[i + j*width] = peak.mId;
            }
        }
    }
    
#if HACK_DRAW_AROUND_FILL
    // When drawing countour around the filled region,
    // there are sometimes 1 pixel that stays at 0 in the middle,
    // close to the frontier.
    //
    // So fill these holes!
    //
    FillMaskHoles(width, height, mask, bbox, peak);
#endif
}

void
PeakDetector2D::FillPeakContourTrianglesOptim(int width, int height,
                                              WDL_TypedBuf<BL_FLOAT> *mask,
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
        
        FillTriangle<BL_FLOAT>::Fill(width, height, mask, v0, v1, v2, peak.mId);
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
PeakDetector2D::FillMaskHoles(int width, int height,
                              WDL_TypedBuf<BL_FLOAT> *mask,
                              const int bbox[2][2], const Peak &peak)
{
#define NUM_NEIGHBORS 3 //2
    
    WDL_TypedBuf<BL_FLOAT> maskCopy = *mask;
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
                        BL_FLOAT val0 = maskCopy.Get()[x + y*width];
                        //if (std::fabs(val0 - peak.mId) < EPS)
                        if ((int)val0 == peak.mId)
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

// Method: point in convex polygon, using cross product
bool
PeakDetector2D::PointInsideTriangle(const Point &p0, const Point &p1, BL_FLOAT p2[2],
                                    int x, int y)
{
    // Triangle
    BL_FLOAT triangle[3][2] = { { p0.mX, p0.mY }, { p1.mX, p1.mY }, { p2[0], p2[1] } };
    
    // Sign counters
    int numNeg = 0;
    int numPos = 0;
    for (int i = 0; i < 3; i++)
    {
        // p1 - p0
        BL_FLOAT v0[2] = { triangle[(i + 1) % 3][0] - triangle[i][0],
                         triangle[(i + 1) % 3][1] - triangle[i][1] };
        
        // p - p0
        BL_FLOAT v1[2] = { x - triangle[i][0],
                         y - triangle[i][1] };
        
        BL_FLOAT cross = v0[0]*v1[1] - v0[1]*v1[0];
        
        if (cross < 0.0)
            numNeg++;
        if (cross > 0.0)
            numPos++;
        
        if ((numNeg > 0) && (numPos > 0))
            return false;
    }
    
    return true;
}

// Not used
// Makes some artifacts with very small contours.
// (the flood fill stays blocked sometimes, in very small
// details of the contour).
void
PeakDetector2D::FillPeakContourFloodFill(int width, int height,
                                         WDL_TypedBuf<BL_FLOAT> *mask,
                                         const Peak &peak,
                                         const vector<Point> &points)
{
    DrawPeakContour(width, height, mask, peak, points);
    
    int bbox[2][2];
    ComputeBoundingBox(points, bbox);
    
    // Check that the bounding box doesn't go outside image
    ClipBoundingBox(bbox, width, height);
    
    int floodX = peak.mX;
    int floodY = peak.mY;
    
#if 1 // 0
      // Fixes some wrong cases
    if ((peak.mX >= 0) && (peak.mX < width) &&
        (peak.mY >= 0) && (peak.mY < height))
    {
        BL_FLOAT val = mask->Get()[peak.mX + peak.mY*width];
        if (std::fabs(val - peak.mId) < EPS)
            // The starting point already has the contour color
        {
            // Debug
            //((Peak *)&peak)->mId = 0;
            //DrawPeakContour(width, height, mask, peak, points);
            
            // Set the starting point to 0 color
            // Works a little
            //mask->Get()[peak.mX + peak.mY*width] = 0.0;
            
            // Take the center of the bounding box
            // Works sometimes, but not every time
            //floodX = (bbox[0][0] + bbox[1][0])*0.5;
            //floodY = (bbox[0][1] + bbox[1][1])*0.5;
            
            // Take the centroid
            // Works more often but not wlaways
            BL_FLOAT floodXf;
            BL_FLOAT floodYf;
            //ComputeCentroid(points, &floodXf, &floodYf);
            ComputeCentroidMask(width, height, mask, peak.mId, &floodXf, &floodYf);
            
            floodX = bl_round(floodXf);
            floodY = bl_round(floodYf);
        }
    }
#endif
    
    FloodFill(width, height, mask, floodX, floodY, peak.mId, bbox);
}

// Use flood fill with a bounding box, to avoid huge recursion in case of failure
void
PeakDetector2D::FloodFill(int width, int height, WDL_TypedBuf<BL_FLOAT> *mask,
                          int x, int y, BL_FLOAT col, const int bbox[2][2])
{
    //if ((x >= 0) && (x < width) &&
    //    (y >= 0) && (y < height))
    if ((x >= bbox[0][0]) && (y >= bbox[0][1]) &&
        (x <= bbox[1][0]) && (y <= bbox[1][1]))
    {
        BL_FLOAT val = mask->Get()[x + y*width];
        //if (val <= 0.0)
        if (std::fabs(val - col) > EPS)
        {
            mask->Get()[x + y*width] = col;
            
            FloodFill(width, height, mask, x - 1, y, col, bbox);
            FloodFill(width, height, mask, x + 1, y, col, bbox);
            FloodFill(width, height, mask, x, y - 1, col, bbox);
            FloodFill(width, height, mask, x, y + 1, col, bbox);
        }
    }
}

void
PeakDetector2D::ComputeBoundingBox(const vector<Point> &points, int bbox[2][2])
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
PeakDetector2D::ClipBoundingBox(int bbox[2][2], int width, int height)
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

void
PeakDetector2D::ComputeCentroid(const vector<Point> &points,
                                BL_FLOAT *x, BL_FLOAT *y)
{
    *x = 0.0;
    *y = 0.0;
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        *x += p.mX;
        *y += p.mY;
    }
    
    if (!points.empty())
    {
        *x = *x/points.size();
        *y = *y/points.size();
    }
}

void
PeakDetector2D::ComputeCentroidMask(int width, int height,
                                    const WDL_TypedBuf<BL_FLOAT> *mask,
                                    int peakId, BL_FLOAT *x, BL_FLOAT *y)
{
    *x = 0.0;
    *y = 0.0;
    
    int numPoints = 0;
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT col = mask->Get()[i + j*width];
            
            if (std::fabs(col - peakId) < EPS)
            {
                *x += i;
                *y += j;
                
                numPoints++;
            }
        }
    }
    
    if (numPoints > 0)
    {
        *x = *x / numPoints;
        *y = *y / numPoints;
    }
}
