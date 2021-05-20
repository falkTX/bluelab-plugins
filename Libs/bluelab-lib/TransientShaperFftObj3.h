//
//  TransientShaperObj2.h
//  BL-Shaper
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_Shaper__TransientShaperFftObj3__
#define __BL_Shaper__TransientShaperFftObj3__

//#include <FifoDecimator.h>
//#include <FifoDecimator2.h> // NEW iPlug2
#include <FifoDecimator3.h> // NEW Shaper with BLScanDisplay
#include <FftProcessObj16.h>

// Detection of "s" and "p" is good at 44100Hz
// but at 88200Hz, it is not so good (worse selaration)
//
// (maybe this is because we don't have enough significant data
// at 88200, to capture correctly a sufficient par of each transient)
//
// So force resample to 44100Hz for transient detection
//
// NOTE: the signal is also resampled to 44100Hz, then upsampled
// back to 88200Hz
// This seems to make Nyquist problems in high freqs
// (and this is not good for someone working with 88200Hz,
// this would loose quality in its workflow)
//
// GOOD: but for "s" and "p" transients (like with 44100Hz)
// But decrease the signal resolution
#define FORCE_SAMPLE_RATE 0 // NOT USED
#define SAMPLE_RATE 44100.0

#if FORCE_SAMPLE_RATE
#include "../../WDL/resample.h"
#endif

// Avoid downsampling and re-upsampling the signal
// that will be sent output
// (to keep original quality)
// Doesn't work well (because badly aligned ?)
#define FORCE_SAMPLE_RATE_KEEP_QUALITY 0 // NOT USED

// Do not separate "s" well
#define USE_TRANSIENT_DETECT_2 0 //1 ORIGIN

#define USE_TRANSIENT_DETECT_3 0 // TEST
// NEW: use pyramid to resample and stay aligned
// Worked at higher sample rates
#define USE_TRANSIENT_DETECT_4 0 // TEST
#define USE_TRANSIENT_DETECT_5 0 // TEST (drafty)

// Managed well 88200Hz, with buffer size 4096
#define USE_TRANSIENT_DETECT_6 1 // NEW


// TransientShaper
//class FifoDecimator;
class TransientLib5;
class TransientShaperFftObj3 : public ProcessObj
{
public:
    TransientShaperFftObj3(int bufferSize, int oversampling,
                           int freqRes, BL_FLOAT sampleRate,
                           int decimNumPoints, BL_FLOAT decimFactor,
                           bool doApplyTransients);
    
    virtual ~TransientShaperFftObj3();

    void Reset();

    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
#if FORCE_SAMPLE_RATE
    void ProcessInputSamplesPre(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                const WDL_TypedBuf<BL_FLOAT> *scBuffer);
#endif

    void SetPrecision(BL_FLOAT precision);
    
    void SetSoftHard(BL_FLOAT softHard);
    
    void SetFreqAmpRatio(BL_FLOAT ratio);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
#if !FORCE_SAMPLE_RATE_KEEP_QUALITY
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                              WDL_TypedBuf<BL_FLOAT> *scBuffer);
#endif
    
#if FORCE_SAMPLE_RATE
    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
#endif

    //
    void ProcessSamplesBufferEnergy(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                    const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    void GetCurrentTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness);
    
    void ApplyTransientness(WDL_TypedBuf<BL_FLOAT> *ioSamples,
                            const WDL_TypedBuf<BL_FLOAT> &currentTransientness);
    
    // Used for GUI
    void SetTrackIO(int maxNumPoints, BL_FLOAT decimFactor,
                    bool trackInput, bool trackOutput,
                    bool trackTransientness);
    
    void GetTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness);

    //
    void GetCurrentInput(WDL_TypedBuf<BL_FLOAT> *outInput);
    void GetCurrentOutput(WDL_TypedBuf<BL_FLOAT> *outOutput);

    bool HasNewData();
    void TouchNewData();
    
protected:
    // NOTE: we can't compute a transientness normalized
    // from the gain of the signal...
    // ... because transientness doesn't depend on the gain
    
    // For old version
    // But the old version is in use ! (better)
    BL_FLOAT ComputeMaxTransientness();

#if FORCE_SAMPLE_RATE
    void InitResamplers();
    void ResetResamplers();
#endif

    BL_FLOAT mSoftHard;
    BL_FLOAT mPrecision;
    
    BL_FLOAT mFreqAmpRatio;
    
    bool mDoApplyTransients;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentTransientness;
    
    // For GUI
    FifoDecimator3 *mTransientness;
    
    // For computing derivative (for amp to trans)
    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
    
#if FORCE_SAMPLE_RATE
    // From Rebalance
    
    WDL_Resampler mResamplerIn;
    WDL_Resampler mResamplerOut;
    
    // ratio of sample remaining after upsampling
    // (to adjust and avoid clicks / blank zones in the output)
    BL_FLOAT mRemainingSamples;
#endif
    
#if FORCE_SAMPLE_RATE_KEEP_QUALITY
    WDL_TypedBuf<BL_FLOAT> mSamplesIn;
    WDL_TypedBuf<BL_FLOAT> mTransientnessBuf;
#endif

    // For tracking
    FifoDecimator3 mInput;
    FifoDecimator3 mOutput;
    
    TransientLib5 *mTransLib;

    bool mHasNewData;
    
private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
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
    WDL_TypedBuf<BL_FLOAT> mTmpBuf13;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf14;
};

#endif /* defined(__BL_Shaper__TransientShaperFftObj3__) */
