#ifndef PEAK_DETECTOR_BILLAUER_H
#define PEAK_DETECTOR_BILLAUER_H

#include <PeakDetector.h>

// See: http://billauer.co.il/peakdet.html
// And: https://github.com/xuphys/peakdetect/blob/master/peakdetect.c
class PeakDetectorBillauer : public PeakDetector
{
 public:
    // Can use norm dB scale, or else pure log scale 
    PeakDetectorBillauer(BL_FLOAT maxDelta);
    virtual ~PeakDetectorBillauer();

    void SetThreshold(BL_FLOAT threshold) override;
    void SetThreshold2(BL_FLOAT threshold2) override;
    
    void DetectPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks,
                     int minIndex = -1, int maxIndex = -1) override;

protected:
    void SuppressSmallPeaksSimple(const WDL_TypedBuf<BL_FLOAT> &data,
                                  vector<Peak> *peaks);
    void SuppressSmallPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks);
        
    void AdjustPeaksWidth(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks);

    void ComputePeakProminence(const WDL_TypedBuf<BL_FLOAT> &data, Peak *p);
        
    bool DBG_TestPeaks(const WDL_TypedBuf<BL_FLOAT> &data, const vector<Peak> &peaks);
    // Print peak
    void DBG_DumpPeaks(const WDL_TypedBuf<BL_FLOAT> &data, const vector<Peak> &peaks);
    // Dump array, showing peak bounds
    void DBG_DumpPeaksBounds(const WDL_TypedBuf<BL_FLOAT> &data,
                             const vector<Peak> &peaks);
        
    //
    BL_FLOAT mMaxDelta;
    
    BL_FLOAT mDelta;

    BL_FLOAT mThreshold2;
};

#endif
