#ifndef SAS_FRAME_ANA_H
#define SAS_FRAME_ANA_H

#include <bl_queue.h>

class SASFrame6;
class PartialsToFreq7;
class OnsetDetector;
class SASFrameAna
{
 public:
    SASFrameAna(int bufferSize, int oversampling,
                int freqRes, BL_FLOAT sampleRate);
    virtual ~SASFrameAna();

    void Reset(BL_FLOAT sampleRate);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate);
    
    void Reset();

    //
    void SetTimeSmoothNoiseCoeff(BL_FLOAT coeff);
    
    // Keep input magns (will be used to compute frequency)
    void SetInputData(const WDL_TypedBuf<BL_FLOAT> &magns,
                      const WDL_TypedBuf<BL_FLOAT> &phases);

    // Non filtered partials
    void SetRawPartials(const vector<Partial> &partials);
    
    // De-normalized partials
    void SetPartials(const vector<Partial> &partials);

    void Compute(SASFrame6 *frame);
    
protected:
    // Compute steps
    //
    void ComputeNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv);
        
    BL_FLOAT ComputeAmplitude();
    BL_FLOAT ComputeFrequency();
    void ComputeColor(WDL_TypedBuf<BL_FLOAT> *color, BL_FLOAT freq);
    void ComputeColorAux(WDL_TypedBuf<BL_FLOAT> *color, BL_FLOAT freq);
    void ComputeWarping(WDL_TypedBuf<BL_FLOAT> *warping,
                        WDL_TypedBuf<BL_FLOAT> *warpingInv,
                        BL_FLOAT freq);

    // If inverse is true, then compute inverse warping
    void ComputeWarpingAux(WDL_TypedBuf<BL_FLOAT> *warping,
                           BL_FLOAT freq, bool inverse = false);

    bool ComputeOnset();

    void NormalizePartials(const WDL_TypedBuf<BL_FLOAT> &warpingInv,
                           const WDL_TypedBuf<BL_FLOAT> &color);
        
    void ProcessMusicalNoise(WDL_TypedBuf<BL_FLOAT> *noise);
    void SmoothNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noise);
    void TimeSmoothNoise(WDL_TypedBuf<BL_FLOAT> *noise);
    
    //
    
    // Fill everything after the last partial with value
    void FillLastValues(WDL_TypedBuf<BL_FLOAT> *values,
                        const vector<Partial> &partials, BL_FLOAT val);

    // Fill everything bfore the first partial with value
    void FillFirstValues(WDL_TypedBuf<BL_FLOAT> *values,
                         const vector<Partial> &partials, BL_FLOAT val);

    void LinkPartialsIdx(vector<Partial> *partials0,
                         vector<Partial> *partials1);

    // Get the partials which are alive
    // (this avoid getting garbage partials that would never be associated)
    bool GetAlivePartials(vector<Partial> *partials);

    void KeepOnlyPartials(const vector<Partial> &partials,
                          WDL_TypedBuf<BL_FLOAT> *magns);

    //
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    int mFreqRes;
    
    // Input signal, not processed
    WDL_TypedBuf<BL_FLOAT> mInputMagns;
    WDL_TypedBuf<BL_FLOAT> mInputPhases;
    
    // HACK
    deque<WDL_TypedBuf<BL_FLOAT> > mInputMagnsHistory;

    // Noise
    //
    WDL_TypedBuf<BL_FLOAT> mSmoothWinNoise;
    
    // For SmoothNoiseEnvelopeTime()
    BL_FLOAT mTimeSmoothNoiseCoeff;
    WDL_TypedBuf<BL_FLOAT> mPrevNoiseEnvelope;
    
    // For ComputeMusicalNoise()
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mPrevNoiseMasks;

    vector<Partial> mRawPartials;
    
    // Not normalized
    vector<Partial> mPartials;
    vector<Partial> mPrevPartials;

    //PartialsToFreq5 *mPartialsToFreq;
    PartialsToFreq7 *mPartialsToFreq;

    OnsetDetector *mOnsetDetector;

    // Used to compute frequency
    BL_FLOAT mPrevFrequency;

    // Must keep the prev values, to interpolate over time
    // when generating the samples
    WDL_TypedBuf<BL_FLOAT> mPrevColor;
    WDL_TypedBuf<BL_FLOAT> mPrevWarping;
    WDL_TypedBuf<BL_FLOAT> mPrevWarpingInv;
    
    //
    struct PartialAux
    {
        BL_FLOAT mFreq;
        BL_FLOAT mWarping;
    };

private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    vector<Partial> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
};

#endif
