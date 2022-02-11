//
//  SASFrame2.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SASFrame2__
#define __BL_SASViewer__SASFrame2__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

// SASFrame2: from SASFrame
//
// Use PartialToFreq compute everything for
// frequency in a sperate object
//
// Use PartialToFreq2 (improved algorithm)
//class PartialsToFreq2;
//class PartialsToFreqCepstrum;
class PartialsToFreq4;
class FreqAdjustObj3;
class WavetableSynth;
class SASFrame2
{
public:
    enum SynthMode
    {
        FFT,
        OSC
    };
    
    class SASPartial
    {
    public:
        SASPartial();
        
        SASPartial(const SASPartial &other);
        
        virtual ~SASPartial();
        
        static bool AmpLess(const SASPartial &p1, const SASPartial &p2);

    public:
        BL_FLOAT mFreq;
        BL_FLOAT mAmpDB;
        BL_FLOAT mPhase;
    };
    
    SASFrame2(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    virtual ~SASFrame2();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetSynthMode(enum SynthMode mode);
    
    void SetPartials(const vector<PartialTracker3::Partial> &partials);
    
    // Get
    BL_FLOAT GetAmplitudeDB() const;
    BL_FLOAT GetFrequency() const;
    void GetColor(WDL_TypedBuf<BL_FLOAT> *color) const;
    void GetNormWarping(WDL_TypedBuf<BL_FLOAT> *warping) const;
    
    // Set
    void SetAmplitudeDB(BL_FLOAT amp);
    void SetFrequency(BL_FLOAT freq);
    void SetColor(const WDL_TypedBuf<BL_FLOAT> &color);
    void SetNormWarping(const WDL_TypedBuf<BL_FLOAT> &warping);
    
    
    void ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeSamplesWin(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void SetPitch(BL_FLOAT pitch);
    
    void SetHarmonicSoundFlag(bool flag);
    
    bool ComputeSamplesFlag();
    bool ComputeSamplesWinFlag();
    
protected:
    void ComputeSamplesPartials(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeSamplesSAS(WDL_TypedBuf<BL_FLOAT> *samples);
    // Optim
    void ComputeSamplesSAS2(WDL_TypedBuf<BL_FLOAT> *samples);
    // Avoid clicks
    void ComputeSamplesSAS3(WDL_TypedBuf<BL_FLOAT> *samples);
    // Optim
    void ComputeSamplesSAS4(WDL_TypedBuf<BL_FLOAT> *samples);
    // Optim
    void ComputeSamplesSAS5(WDL_TypedBuf<BL_FLOAT> *samples);
    BL_FLOAT GetColor(const WDL_TypedBuf<BL_FLOAT> &color, BL_FLOAT binIdx);

    
    void ComputeFftSAS(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeFftSASFreqAdjust(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesSASTable(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesSASTable2(WDL_TypedBuf<BL_FLOAT> *samples); // Better interpolation
    void ComputeSamplesSASOverlap(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void Compute();
    
    // Compute steps
    //
    
    void ComputeAmplitude();
    
    void ComputeFrequency();
    
    void ComputeColor();
    void ComputeColorAux();
    
    void ComputeNormWarping();
    void ComputeNormWarpingAux();
    
    // Simple version
    BL_FLOAT ApplyNormWarping(BL_FLOAT freq);
    BL_FLOAT ApplyColor(BL_FLOAT freq);

    // Versions to interpolate over time
    BL_FLOAT ApplyNormWarping(BL_FLOAT freq, BL_FLOAT t);
    BL_FLOAT ApplyColor(BL_FLOAT freq, BL_FLOAT t);
    
    bool FindPartial(BL_FLOAT freq);

    void GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t);
    
    int FindPrevPartialIdx(int currentPartialIdx);

    void GetSASPartial(SASPartial *result, int index, BL_FLOAT t);
    
    // Estimate the fundamental frequency
    BL_FLOAT TWMEstimate(const vector<PartialTracker3::Partial> &partials);
    BL_FLOAT ComputeTWMError(const vector<PartialTracker3::Partial> &partials,
                           BL_FLOAT testFreq);
    BL_FLOAT ComputeErrorK(const PartialTracker3::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT Amax);
    BL_FLOAT ComputeErrorN(const PartialTracker3::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT Amax);
    BL_FLOAT GetNearestHarmonic(BL_FLOAT freq, BL_FLOAT refFreq);
    
    
    //
    SynthMode mSynthMode;
    
    // Tracked partials
    BL_FLOAT mPrevAmplitudeDB;
    vector<PartialTracker3::Partial> mPartials;
    vector<PartialTracker3::Partial> mPrevPartials;
    
    BL_FLOAT mAmplitudeDB;
    
    BL_FLOAT mFrequency;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    WDL_TypedBuf<BL_FLOAT> mColor;
    WDL_TypedBuf<BL_FLOAT> mNormWarping;
    
    // Must keep the prev values, to interpolate over time
    // when generating the samples
    WDL_TypedBuf<BL_FLOAT> mPrevColor;
    WDL_TypedBuf<BL_FLOAT> mPrevNormWarping;
    
    vector<SASPartial> mSASPartials;
    vector<SASPartial> mPrevSASPartials;
    
    BL_FLOAT mPitch;
    
    //PartialsToFreqCepstrum *mPartialsToFreq;
    PartialsToFreq4 *mPartialsToFreq;
    
    FreqAdjustObj3 *mFreqObj;
    
    // For sample synth with table
    WavetableSynth *mTableSynth;
};

#endif /* defined(__BL_SASViewer__SASFrame2__) */
