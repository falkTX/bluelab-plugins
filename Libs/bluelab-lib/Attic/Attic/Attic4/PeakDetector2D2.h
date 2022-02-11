//
//  PeakDetector2D2.h
//  BL-DUET
//
//  Created by applematuer on 5/5/20.
//
//

#ifndef __BL_DUET__PeakDetector2D2__
#define __BL_DUET__PeakDetector2D2__

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

// PeakDetector2D2: from PeakDetector2D
// - code clean
// - mask: BL_FLOAT -> int
class PeakDetector2D2
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
    
    PeakDetector2D2(int numPoints);
    
    virtual ~PeakDetector2D2();
    
    // The peak objects have been computed in another class (e.g DUETDeparator)
    
    // Given an image and a list of peaks, return a mask image
    void DetectPeaks(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                     const vector<Peak> &peaks, BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                     WDL_TypedBuf<int> *mask);
    
    // Same as previous, but avoids overlap between peaks regions
    void DetectPeaksNoOverlap(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                              const vector<Peak> &peaks,
                              BL_FLOAT threshold, BL_FLOAT thresholdWidth,
                              WDL_TypedBuf<int> *mask);
    
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
                    WDL_TypedBuf<int> *mask);
    
    void InitPoints(int width, int height, const WDL_TypedBuf<BL_FLOAT> &image,
                    const Peak &peak, vector<Point> *points);
    
    BL_FLOAT FindTargetGrayValue(int width, int height,
                               const WDL_TypedBuf<BL_FLOAT> &image,
                               const vector<Point> &points);

    int EnlargeCircle(int width, int height,
                      const WDL_TypedBuf<BL_FLOAT> &image,
                      const Peak &peak,
                      BL_FLOAT targetValue,
                      BL_FLOAT threshold,
                      BL_FLOAT thresholdWidth,
                      vector<Point> *points,
                      const WDL_TypedBuf<int> *overlapPrevMask = NULL,
                      const WDL_TypedBuf<int> *overlapNewMask = NULL);

    
    void DrawPeakContour(int width, int height,
                         WDL_TypedBuf<int> *mask,
                         const Peak &peak,
                         const vector<Point> &points);
    
    void FillPeakContour(int width, int height,
                         WDL_TypedBuf<int> *mask,
                         const Peak &peak,
                         const vector<Point> &points);
    
    void FillMaskHoles(int width, int height,
                       WDL_TypedBuf<int> *mask,
                       const int bbox[2][2],
                       const Peak &peak);

    void ComputeBoundingBox(const vector<Point> &points, int bbox[2][2]);

    void ClipBoundingBox(int bbox[2][2], int width, int height);

    //
    
    // Number of points of the circle
    int mNumPoints;
};

#endif /* defined(__BL_DUET__PeakDetector2D2__) */
