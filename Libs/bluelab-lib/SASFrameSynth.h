#ifndef SAS_FRAME_SYNTH_H
#define SAS_FRAME_SYNTH_H

#include <BLTypes.h>

#include <SASFrame6.h>
#include <Partial.h>

#include "IPlug_include_in_plug_hdr.h"

#define SYNTH_MAX_NUM_PARTIALS 40 //20 //10 origin: 10, (40 for test bell)

class SASFrameSynth
{
 public:
    enum SynthMode
    {
        // Use raw detected partials and synth using sine and samples
        RAW_PARTIALS = 0,
        // Source partials, denormalized (we re-apply sas params on them later 
        SOURCE_PARTIALS,
        // Resynth using sines and samples
        RESYNTH_PARTIALS
    };

    SASFrameSynth(int bufferSize, int oversampling,
                  int freqRes, BL_FLOAT sampleRate);
    virtual ~SASFrameSynth();

    void Reset(BL_FLOAT sampleRate);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate);

    void SetMinAmpDB(BL_FLOAT ampDB);

    //
    void SetSynthMode(enum SynthMode mode);
    SASFrameSynth::SynthMode GetSynthMode() const;

    void SetSynthEvenPartials(bool flag);
    void SetSynthOddPartials(bool flag);

    void SetHarmoNoiseMix(BL_FLOAT mix);
    BL_FLOAT GetHarmoNoiseMix();
    
    void SetAmpFactor(BL_FLOAT factor);
    void SetFreqFactor(BL_FLOAT factor);
    void SetColorFactor(BL_FLOAT factor);
    void SetWarpingFactor(BL_FLOAT factor);
    
    //
    void AddSASFrame(const SASFrame6 &frame);
    // Get the current SAS frame
    void GetSASFrame(SASFrame6 *frame);
    
    void ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
protected:
    // Keep it for debugging
    void ComputeSamplesPartialsRaw(WDL_TypedBuf<BL_FLOAT> *samples);
     // TODO: delete this
    void ComputeSamplesPartialsSourceNorm(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesPartialsSource(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesPartialsResynth(WDL_TypedBuf<BL_FLOAT> *samples);

    BL_FLOAT GetColor(const WDL_TypedBuf<BL_FLOAT> &color, BL_FLOAT binIdx);
    BL_FLOAT GetWarping(const WDL_TypedBuf<BL_FLOAT> &warping, BL_FLOAT binIdx);

    bool FindPartial(BL_FLOAT freq);
    void GetPartial(SASFrame6::SASPartial *result, int index, BL_FLOAT t);
    
    //
    BL_FLOAT GetFreq(BL_FLOAT freq0, BL_FLOAT freq1, BL_FLOAT t);
    BL_FLOAT GetAmp(BL_FLOAT amp0, BL_FLOAT amp1, BL_FLOAT t);
    BL_FLOAT GetCol(BL_FLOAT col0, BL_FLOAT col1, BL_FLOAT t);

    void SetSASFactors();
    void UpdateSASData();
        
    //
    vector<SASFrame6::SASPartial> mSASPartials;
    vector<SASFrame6::SASPartial> mPrevSASPartials;

    // For "raw" and "source"
    vector<SASFrame6::SASPartial> mPartials;
    vector<SASFrame6::SASPartial> mPrevPartials;
    
    //
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    int mFreqRes;

    //
    BL_FLOAT mMinAmpDB;
    
    //
    SynthMode mSynthMode;
    bool mSynthEvenPartials;
    bool mSynthOddPartials;

    BL_FLOAT mHarmoNoiseMix;
    
    //
    BL_FLOAT mAmplitude;
    BL_FLOAT mPrevAmplitude;
    
    BL_FLOAT mFrequency;
    // For smoothing
    BL_FLOAT mPrevFrequency;

    WDL_TypedBuf<BL_FLOAT> mColor;
    WDL_TypedBuf<BL_FLOAT> mPrevColor;

    WDL_TypedBuf<BL_FLOAT> mWarping;
    WDL_TypedBuf<BL_FLOAT> mPrevWarping;
    
    WDL_TypedBuf<BL_FLOAT> mWarpingInv;
    //
    BL_FLOAT mAmpFactor;
    BL_FLOAT mFreqFactor;
    BL_FLOAT mColorFactor;
    BL_FLOAT mWarpingFactor;

    //
    SASFrame6 mSASFrame;
    SASFrame6 mPrevSASFrame;

private:
    vector<Partial> mTmpBuf0;
};

#endif
