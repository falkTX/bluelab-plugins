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
//  PeakDetector2D.h
//  BL-DUET
//
//  Created by applematuer on 5/5/20.
//
//

#ifndef __BL_DUET__PeakDetector2D__
#define __BL_DUET__PeakDetector2D__

// Peak detection using the custom "rubber" algorithm
//
// Take the maximum, put a circle of radius 0 at the top,
// then make the circle descend the peak until we reach the bottom of the peak.
// The circle will enlarge as long as it goes down the peak.
// We take as basis the current height (or grayscale level).
// The shape of the circle will change a bit in order to match the peak.
//
// (maybe there is already a know algorithm for that, I don't know...)
//
class PeakDetector2D
{
public:
    struct Peak
    {
        int mX;
        int mY;
        
        int mId;
        
        BL_FLOAT mIntensity;
        BL_FLOAT mArea;
        
        //
        static bool IntensityGreater(const Peak &p0, const Peak &p1)
        {
            return (p0.mIntensity > p1.mIntensity);
        }
        
        static bool AreaGreater(const Peak &p0, const Peak &p1)
        {
            return (p0.mArea > p1.mArea);
        }
    };
    
    PeakDetector2D(int numPoints);
    
    virtual ~PeakDetector2D();
    
    // The peak objects have been computed in another class (e.g DUETDeparator)
    
    // Given an image and a list of peaks, return a mask image
    void DetectPeaks(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                     const vector<Peak> &peaks, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                     WDL_TypedBuf<BL_FLOAT> *mask);
    
    // Same as previous, but avoids overlap between peaks regions
    void DetectPeaksNoOverlap(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                              const vector<Peak> &peaks, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                              WDL_TypedBuf<BL_FLOAT> *mask);
    
protected:
    struct Point
    {
        BL_FLOAT mX;
        BL_FLOAT mY;
        
        BL_FLOAT mDirX;
        BL_FLOAT mDirY;
        
        BL_FLOAT mIntensity;
    };
    
    void DetectPeak(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                    const Peak &peak, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                    WDL_TypedBuf<BL_FLOAT> *mask);
    
    void InitPoints(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                    const Peak &peak, vector<Point> *points);
    
    BL_FLOAT FindTargetGrayValue(int width, int height,
                               const WDL_TypedBuf<BL_FLOAT> &image,
                               const vector<Point> &points);

    int EnlargeCircle(int width, int height,
                      const WDL_TypedBuf<BL_FLOAT> &image,
                      const Peak &peak,
                      BL_FLOAT targetValue,
                      BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                      vector<Point> *points,
                      const WDL_TypedBuf<BL_FLOAT> *overlapPrevMask = NULL,
                      const WDL_TypedBuf<BL_FLOAT> *overlapNewMask = NULL);

    
    void DrawPeakContour(int width, int height,
                         WDL_TypedBuf<BL_FLOAT> *mask,
                         const Peak &peak,
                         const vector<Point> &points);
    
    void FillPeakContour(int width, int height,
                         WDL_TypedBuf<BL_FLOAT> *mask,
                         const Peak &peak,
                         const vector<Point> &points);

    void FillPeakContourTriangles(int width, int height,
                                  WDL_TypedBuf<BL_FLOAT> *mask,
                                  const Peak &peak,
                                  const vector<Point> &points);
    
    void FillPeakContourTrianglesOptim(int width, int height,
                                      WDL_TypedBuf<BL_FLOAT> *mask,
                                      const Peak &peak,
                                      const vector<Point> &points);
    
    bool PointInsideTriangle(const Point &p0, const Point &p1, BL_FLOAT p2[2], int x, int y);
    
    void FillMaskHoles(int width, int height,
                       WDL_TypedBuf<BL_FLOAT> *mask,
                       const int bbox[2][2], const Peak &peak);

    
    void FillPeakContourFloodFill(int width, int height,
                                  WDL_TypedBuf<BL_FLOAT> *mask,
                                  const Peak &peak,
                                  const vector<Point> &points);

    void FloodFill(int width, int height, WDL_TypedBuf<BL_FLOAT> *mask,
                   int x, int y, BL_FLOAT col, const int bbox[2][2]);

    void ComputeBoundingBox(const vector<Point> &points, int bbox[2][2]);

    void ClipBoundingBox(int bbox[2][2], int width, int height);

    void ComputeCentroid(const vector<Point> &points,
                         BL_FLOAT *x, BL_FLOAT *y);
    void ComputeCentroidMask(int width, int height,
                             const WDL_TypedBuf<BL_FLOAT> *mask,
                             int peakId, BL_FLOAT *x, BL_FLOAT *y);
    
    // Number of points of the circle
    int mNumPoints;
};

#endif /* defined(__BL_DUET__PeakDetector2D__) */
