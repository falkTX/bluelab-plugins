#ifndef PEAK_DETECTOR_BL_H
#define PEAK_DETECTOR_BL_H

#include <PeakDetector.h>

class PeakDetectorBL : public PeakDetector
{
 public:
    PeakDetectorBL();
    virtual ~PeakDetectorBL();

    void DetectPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks,
                     int minIndex = -1, int maxIndex = -1) override;

 protected:
    bool DiscardInvalidPeaks(const WDL_TypedBuf<BL_FLOAT> &data,
                             int peakIndex, int leftIndex, int rightIndex);
};

#endif
