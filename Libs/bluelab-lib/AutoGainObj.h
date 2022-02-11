//
//  AutoGainObj.hpp
//  BL-AutoGain-macOS
//
//  Created by applematuer on 11/26/20.
//
//

#ifndef AutoGainObj_hpp
#define AutoGainObj_hpp

#include <vector>
using namespace std;

#include <BLTypes.h>
#include <FftProcessObj16.h>

#include "IPlug_include_in_plug_hdr.h"

// NOT USED: Test for Protools
#define SKIP_FIRST_FRAME 0

// FIX: fixes the jump to +12dB when starting playback
// (detected on Protools)
#define FIX_START_JUMP 1

// Use silence threshold on fft data, not on avg
// This make possible to compare the frequencies only when
// both input and sidechain are defined
#define SILENCE_THRS_ALGO2 1

// Legacy method. Not need anymore. And it will remove an obscure parameter
#define USE_LEGACY_SILENCE_THRESHOLD 0

class SmoothAvgHistogram;
class ParamSmoother;
class AutoGainObj : public MultichannelProcess
{
public:
    enum Mode
    {
        BYPASS_WRITE = 0,
        READ = 1
    };
    
    AutoGainObj(int bufferSize, int oversampling,
                int freqRes, BL_FLOAT sampleRate,
                BL_FLOAT minGain, BL_FLOAT maxGain);
    
    virtual ~AutoGainObj();
    
    void Reset() override;
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate) override;
    
    void SetMode(Mode mode);

#if USE_LEGACY_SILENCE_THRESHOLD
    void SetThreshold(BL_FLOAT threshold);
#endif
    
    void SetPrecision(BL_FLOAT precision);
    void SetDryWet(BL_FLOAT dryWet);
    
    BL_FLOAT GetGain();
    void SetGain(BL_FLOAT gain);
    
    void SetScGain(BL_FLOAT gain);
    
    void SetGainSmooth(BL_FLOAT gainSmooth);
    
    //
    void ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // For graph curves
    void GetCurveSignal0(WDL_TypedBuf<BL_FLOAT> *signal0);
    void GetCurveSignal1(WDL_TypedBuf<BL_FLOAT> *signal1);
    void GetCurveResult(WDL_TypedBuf<BL_FLOAT> *result);
    
protected:
    static void ApplyGain(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                          vector<WDL_TypedBuf<BL_FLOAT> > *outSamples,
                          BL_FLOAT gainDB);
    
    static void ApplyGainConstantSc(BL_FLOAT *scConstantValue,
                                    BL_FLOAT gainDB);
    
    static void ApplyDryWet(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                            vector<WDL_TypedBuf<BL_FLOAT> > *outSamples,
                            BL_FLOAT dryWet);
    
    BL_FLOAT ComputeOutGainSpect(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &scIn);
    
    BL_FLOAT ComputeOutGainSpectConstantSc(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                                           BL_FLOAT constantScValue);
    
    BL_FLOAT ComputeOutGainSpectAux(const WDL_TypedBuf<BL_FLOAT> &dbIn,
                                    const WDL_TypedBuf<BL_FLOAT> &dbSc,
                                    BL_FLOAT inGain);
    
    BL_FLOAT ComputeInGainSamples(const WDL_TypedBuf<BL_FLOAT> &monoIn);
    BL_FLOAT ComputeInGainFft(const WDL_TypedBuf<BL_FLOAT> &monoIn);

    // Not used anymore
    //BL_FLOAT ComputeOutGainRMS(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
    //                           const vector<WDL_TypedBuf<BL_FLOAT> > &scIn);
    
    BL_FLOAT ComputeFftGain(const WDL_TypedBuf<BL_FLOAT> &avgIn,
                            const WDL_TypedBuf<BL_FLOAT> &avgSc);

#if USE_LEGACY_SILENCE_THRESHOLD
    BL_FLOAT ComputeFftGain2(const WDL_TypedBuf<BL_FLOAT> &avgIn,
                             const WDL_TypedBuf<BL_FLOAT> &avgSc);
#endif
    
    // Ponderate diff by the volume
    BL_FLOAT ComputeFftGain3(const WDL_TypedBuf<BL_FLOAT> &avgIn,
                             const WDL_TypedBuf<BL_FLOAT> &avgSc);
    
    static void BoundScCurve(WDL_TypedBuf<BL_FLOAT> *curve,
                             BL_FLOAT mindB, BL_FLOAT maxdB,
                             BL_FLOAT scGain);
    
    void UpdateGraphReadMode(const vector<WDL_TypedBuf<BL_FLOAT> > &inData,
                             const vector<WDL_TypedBuf<BL_FLOAT> > &outData);
    
    
    //
#if USE_LEGACY_SILENCE_THRESHOLD
    BL_FLOAT mThreshold;
#endif
    
    BL_FLOAT mPrecision;
    BL_FLOAT mDryWet;
    
    BL_FLOAT mGain;
    BL_FLOAT mScGain;
    
    Mode mMode;
    
    //
    bool mConstantSc;
    BL_FLOAT mScConstantValue;
    
    //
    SmoothAvgHistogram *mAvgHistoIn;
    SmoothAvgHistogram *mAvgHistoScIn;
    
    // For read mode
    SmoothAvgHistogram *mAvgHistoOut;
    
    //
    WDL_TypedBuf<BL_FLOAT> mAWeights;
    
    //
    ParamSmoother *mScSamplesSmoother;
    ParamSmoother *mInSamplesSmoother;
    ParamSmoother *mGainSmoother;
    
    //
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
#if SKIP_FIRST_FRAME
    // Used to ignore the processing until we reach "latency" num samples
    // FIX: fixed Protools when start playing, gain jumps to +12
    unsigned long long mNumSamples;
#endif
    
#if FIX_START_JUMP
    bool mHasJustReset;
#endif
    
    // Data for curves
    WDL_TypedBuf<BL_FLOAT> mCurveSignal0;
    WDL_TypedBuf<BL_FLOAT> mCurveSignal1;
    WDL_TypedBuf<BL_FLOAT> mCurveResult;
    
    BL_FLOAT mMinGain;
    BL_FLOAT mMaxGain;

private:
    // Tmp buffers
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf0;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf1;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf2;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf6;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf12;
    //WDL_TypedBuf<BL_FLOAT> mTmpBuf13;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf14;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf15;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf16;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf17;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf18;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf19;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf20;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf21;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf22;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf23;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf24;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf25;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf26;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf27;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf28;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf29;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf30;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf31;
};

#endif /* AutoGainObj_hpp */
