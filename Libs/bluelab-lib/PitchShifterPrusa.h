#ifndef PITCH_SHIFTER_PRUSA_H
#define PITCH_SHIFTER_PRUSA_H

#include <vector>
using namespace std;

// Implementation of:
// "Phase Vocoder Done Right" by Zdenek Prusa and Nicki Holighaus.
// with improvements from:
// "Pitch-shifting algorithm design and applications in music" by THÉO ROYER
// (fix for high freqs using oversampling + pre-echo reduction for drums)
class PitchShiftPrusaFftObj;
class FftProcessObj16;
class StereoPhasesProcess;
class PitchShifterPrusa
{
 public:
    PitchShifterPrusa();
    virtual ~PitchShifterPrusa();

    void Reset(BL_FLOAT sampleRate, int blockSize);

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out);
    
    void SetNumChannels(int nchans) {}
    void SetFactor(BL_FLOAT factor);
    // 0, 1, 2 or 3
    void SetQuality(int quality);

    void SetTransBoost(BL_FLOAT transBoost) {}

    int ComputeLatency(int blockSize);
    
protected:
    void InitFft(BL_FLOAT sampleRate);

    //
    BL_FLOAT mSampleRate;
    int mOversampling;

    // Shift factor
    BL_FLOAT mFactor;
    
    PitchShiftPrusaFftObj *mPitchObjs[2];
    
    FftProcessObj16 *mFftObj;
    
    StereoPhasesProcess *mPhasesProcess;
};

#endif
