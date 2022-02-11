//
//  InfraProcess.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Infra__InfraProcess__
#define __BL_Infra__InfraProcess__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include <PartialTracker3.h>

#include <FftProcessObj16.h>

#define INFRA_PROCESS_PROFILE 0

#if INFRA_PROCESS_PROFILE
#include <BlaTimer.h>
#endif


class PartialTracker3;
class SineSynth;
class FilterIIRLow12dB;

class InfraProcess : public ProcessObj
{
public:
    InfraProcess(int bufferSize,
                BL_FLOAT overlapping, BL_FLOAT oversampling,
                BL_FLOAT sampleRate);
    
    virtual ~InfraProcess();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void ProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetPhantomFreq(BL_FLOAT phantomFreq);
    void SetPhantomMix(BL_FLOAT phantomMix);
    
    void SetSubOrder(int subOrder);
    void SetSubMix(BL_FLOAT subMix);
    
protected:
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void GeneratePhantomPartials(const vector<PartialTracker3::Partial> &partials,
                                 vector<PartialTracker3::Partial> *newPartials);

    void GenerateSubPartials(const vector<PartialTracker3::Partial> &partials,
                             vector<PartialTracker3::Partial> *newPartials);
    
    void IncreaseInitialFreq(WDL_TypedBuf<BL_FLOAT> *result,
                             const WDL_TypedBuf<BL_FLOAT> &magns,
                             const vector<PartialTracker3::Partial> &partials);
    
    void IncreaseAllFreqs(WDL_TypedBuf<BL_FLOAT> *ioBuffer, BL_FLOAT mix);

    
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    PartialTracker3 *mPartialTracker;
    SineSynth *mPhantomSynth;
    SineSynth *mSubSynth;
    
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
    
#if INFRA_PROCESS_PROFILE
    BlaTimer mTimer;
    long mCount;
#endif
};

#endif /* defined(__BL_Infra__InfraProcess__) */
