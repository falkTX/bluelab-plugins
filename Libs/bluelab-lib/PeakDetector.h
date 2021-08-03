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
    };

    virtual void SetThreshold(BL_FLOAT threshold) {}
    
    virtual void DetectPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks,
                             int minIndex = -1, int maxIndex = -1) = 0;
};

#endif
