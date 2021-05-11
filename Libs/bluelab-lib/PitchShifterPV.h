#ifndef PITCH_SHIFTER_PV_H
#define PITCH_SHIFTER_PV_H

#include <vector>
using namespace std;

#include <PhaseVocoder-DSP/PitchShifter.h>
#include <PhaseVocoder-DSP/PeakShifter.h> // TESt

class PitchShiftPVFftObj;
class PitchShifterPV
{
 public:
    PitchShifterPV();
    virtual ~PitchShifterPV();

    void Reset(BL_FLOAT sampleRate, int blockSize);

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out);

    void SetFactor(BL_FLOAT factor);

    int ComputeLatency(int blockSize);
    
    void SetNumChannels(int nchans) {}

    // 0, 1, 2 or 3
    void SetQuality(int quality) {}

    void SetTransBoost(BL_FLOAT transBoost) {}
    
protected:
    //stekyne::PitchShifter<BL_FLOAT> *mPitchObjs[2];
    stekyne::PeakShifter<BL_FLOAT> *mPitchObjs[2];
};

#endif
