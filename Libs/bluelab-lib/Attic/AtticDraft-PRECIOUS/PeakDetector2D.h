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
        
        double mIntensity;
        double mArea;
        
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
    
    // Given an image and a list of peaks, return a mask image
    void DetectPeaks(int width, int height, const WDL_TypedBuf<double> &image,
                     const vector<Peak> &peaks, double threshold,
                     WDL_TypedBuf<double> *mask);
    
protected:
    struct Point
    {
        double mX;
        double mY;
        
        double mDirX;
        double mDirY;
        
        double mIntensity;
    };
    
    void DetectPeak(int width, int height, const WDL_TypedBuf<double> &image,
                    const Peak &peak, double threshold,
                    WDL_TypedBuf<double> *mask);
    
    void InitPoints(int width, int height, const WDL_TypedBuf<double> &image,
                    const Peak &peak, vector<Point> *points);
    
    double FindTargetGrayValue(int width, int height,
                               const WDL_TypedBuf<double> &image,
                               const vector<Point> &points);

    int EnlargeCircle(int width, int height,
                      const WDL_TypedBuf<double> &image,
                      double targetValue, double threshold,
                      vector<Point> *points);

    
    void FillMask(int width, int height,
                  WDL_TypedBuf<double> *mask,
                  const Peak &peak,
                  const vector<Point> &points);

    
    // Number of points of the circle
    int mNumPoints;
};

#endif /* defined(__BL_DUET__PeakDetector2D__) */
