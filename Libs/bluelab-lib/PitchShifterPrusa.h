#ifndef PITCH_SHIFTER_PRUSA_H
#define PITCH_SHIFTER_PRUSA_H

#include <vector>
using namespace std;

#include <PitchShifterInterface.h>

// Implementation of:
// "Phase Vocoder Done Right" by Zdenek Prusa and Nicki Holighaus.
// with improvements from:
// "Pitch-shifting algorithm design and applications in music" by THÉO ROYER
// (fix for high freqs using oversampling + pre-echo reduction for drums)
class PitchShiftPrusaFftObj;
class FftProcessObj16;
class StereoPhasesProcess;
class PitchShifterPrusa : public PitchShifterInterface
{
 public:
    PitchShifterPrusa();
    virtual ~PitchShifterPrusa();

    void Reset(BL_FLOAT sampleRate, int blockSize) override;

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out) override;
    
    void SetNumChannels(int nchans) override {}
    void SetFactor(BL_FLOAT factor) override;
    // Set the buffer size: 0 or 1
    void SetQuality(int quality) override;

    void SetTransBoost(BL_FLOAT transBoost) override {}

    int ComputeLatency(int blockSize) override;
    
protected:
    void InitFft(BL_FLOAT sampleRate);

    //
    BL_FLOAT mSampleRate;
    // Not used anymore
    // It smeared traisients when increasing it
    // Change buffer size instead, to get better defined frequancies
    int mOversampling;

    // 2048 -> 4096: makes better defined frequencies
    int mBufferSize;
    
    // Shift factor
    BL_FLOAT mFactor;
    
    PitchShiftPrusaFftObj *mPitchObjs[2];
    
    FftProcessObj16 *mFftObj;
    
    StereoPhasesProcess *mPhasesProcess;
};

#endif
