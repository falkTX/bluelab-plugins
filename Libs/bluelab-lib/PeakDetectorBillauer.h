#ifndef PEAK_DETECTOR_BILLAUER_H
#define PEAK_DETECTOR_BILLAUER_H

#include <PeakDetector.h>

// See: http://billauer.co.il/peakdet.html
// And: https://github.com/xuphys/peakdetect/blob/master/peakdetect.c
class PeakDetectorBillauer : public PeakDetector
{
 public:
    PeakDetectorBillauer();
    virtual ~PeakDetectorBillauer();

    void SetThreshold(BL_FLOAT threshold) override;
    
    void DetectPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks,
                     int minIndex = -1, int maxIndex = -1) override;

protected:
    void AdjustPeaksWidth(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks);

    void DBG_TestPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks);
        
    //
    BL_FLOAT mDelta;
};

#endif
