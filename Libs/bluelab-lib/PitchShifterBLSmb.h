#ifndef PITCH_SHIFTER_BL_SMB_H
#define PITCH_SHIFTER_BL_SMB_H

#include <vector>
using namespace std;

#include <PitchShifterInterface.h>

// Original code
// Moved from plugin main PitchShift.cpp to an external class
class PostTransientFftObj3;
class PitchShiftFftObj3;
class FftProcessObj16;
class StereoPhasesProcess;
class PitchShifterBLSmb : public PitchShifterInterface
{
 public:
    PitchShifterBLSmb();
    virtual ~PitchShifterBLSmb();

    void Reset(BL_FLOAT sampleRate, int blockSize) override;

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out) override;
    
    void SetNumChannels(int nchans) override {}
    void SetFactor(BL_FLOAT factor) override;
    // 0, 1, 2 or 3
    void SetQuality(int quality) override;

    void SetTransBoost(BL_FLOAT transBoost) override;

    int ComputeLatency(int blockSize) override;
    
protected:
    void InitFft(BL_FLOAT sampleRate);

    //
    BL_FLOAT mSampleRate;
    int mOversampling;

    // Shift factor
    BL_FLOAT mFactor;
    
    // FIX: was WDL_TypedBuf
    //vector<PitchShiftFftObj3 *> mPitchObjs;
    PitchShiftFftObj3 *mPitchObjs[2];
    
#if DEBUG_PITCH_OBJ
    FftProcessObj16 *mFftObj;
#else
    PostTransientFftObj3 *mFftObj;
#endif
    
    StereoPhasesProcess *mPhasesProcess;
};

#endif
