#ifndef PITCH_SHIFTER_PV_H
#define PITCH_SHIFTER_PV_H

#include <vector>
using namespace std;

//#include <PhaseVocoder-DSP/PitchShifter.h>

class PitchShiftPVFftObj;
class PitchShifterPV
{
 public:
    PitchShifterPV();
    virtual ~PitchShifterPV();

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
    void Init(BL_FLOAT sampleRate);

    //
    BL_FLOAT mSampleRate;
    int mOversampling;

    // Shift factor
    BL_FLOAT mFactor;

    //
    //stekyne::PitchShifter *mPitchObjs[2];
};

#endif
