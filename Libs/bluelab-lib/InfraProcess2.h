//
//  InfraProcess2.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Infra__InfraProcess2__
#define __BL_Infra__InfraProcess2__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include <PartialTracker5.h>

#include <FftProcessObj16.h>

#define INFRA_PROCESS_PROFILE 0

#if INFRA_PROCESS_PROFILE
#include <BlaTimer.h>
#endif


class PartialTracker5;
class SineSynth3;
class FilterIIRLow12dB;

class InfraProcess2 : public ProcessObj
{
public:
    InfraProcess2(int bufferSize,
                  BL_FLOAT overlapping, BL_FLOAT oversampling,
                  BL_FLOAT sampleRate);
    
    virtual ~InfraProcess2();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void ProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetPhantomFreq(BL_FLOAT phantomFreq);
    void SetPhantomMix(BL_FLOAT phantomMix);
    
    void SetSubOrder(int subOrder);
    void SetSubMix(BL_FLOAT subMix);
    
    void SetAdaptivePhantomFreq(bool flag);
    
    void SetDebug(bool flag);
    
protected:
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void GeneratePhantomPartials(const vector<PartialTracker5::Partial> &partials,
                                 vector<PartialTracker5::Partial> *newPartials);

    void GenerateSubPartials(const vector<PartialTracker5::Partial> &partials,
                             vector<PartialTracker5::Partial> *newPartials);
    
    void IncreaseInitialFreq(WDL_TypedBuf<BL_FLOAT> *result,
                             const WDL_TypedBuf<BL_FLOAT> &magns,
                             const vector<PartialTracker5::Partial> &partials);
    
    void IncreaseAllFreqs(WDL_TypedBuf<BL_FLOAT> *ioBuffer, BL_FLOAT mix);

    void GenerateOscillatorsFft(const WDL_TypedBuf<BL_FLOAT> &samples);
        
    //
    
    PartialTracker5 *mPartialTracker;
    SineSynth3 *mPhantomSynth;
    SineSynth3 *mSubSynth;
    
    BL_FLOAT mPhantomFreq;
    BL_FLOAT mPhantomMix;
    int mSubOrder;
    BL_FLOAT mSubMix;
    
    // For ramps (progressiveny change the parameter)
    BL_FLOAT mPrevPhantomMix;
    BL_FLOAT mPrevSubMix;
    
    // Low pass filter when increasing the original signal
    // but only the low frequencies
    FilterIIRLow12dB *mLowFilter;
    
    // Low pass filter to fix
    // FIX: when generating sub octave, there are high frequencies appearing
    FilterIIRLow12dB *mSubLowFilter;
    
    bool mAdaptivePhantomFreq;
    
#if INFRA_PROCESS_PROFILE
    BlaTimer mTimer;
    long mCount;
#endif
    
    bool mDebug;
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf12;
};

#endif /* defined(__BL_Infra__InfraProcess2__) */
