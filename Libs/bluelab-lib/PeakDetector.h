#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class PeakDetector
{
 public:
    struct Peak
    {
        int mPeakIndex;
        int mLeftIndex;
        int mRightIndex;

        // Optional
        BL_FLOAT mProminence;

        static bool ProminenceLess(const Peak &p1, const Peak &p2)
        { return (p1.mProminence < p2.mProminence); }
    };

    virtual void SetThreshold(BL_FLOAT threshold) {}
    virtual void SetThreshold2(BL_FLOAT threshold2) {}
    
    virtual void DetectPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks,
                             int minIndex = -1, int maxIndex = -1) = 0;
};

#endif
