#ifndef PEAK_DETECTOR_BILLAUER_H
#define PEAK_DETECTOR_BILLAUER_H

#include <PeakDetector.h>

class PeakDetectorBillauer : public PeakDetector
{
 public:
    PeakDetectorBillauer();
    virtual ~PeakDetectorBillauer();

    void DetectPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks,
                     int minIndex = -1, int maxIndex = -1) override;
};

#endif
