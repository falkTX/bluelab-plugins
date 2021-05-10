#ifndef PITCH_SHIFTER_JH_H
#define PITCH_SHIFTER_JH_H

#include <vector>
using namespace std;

class PitchShiftJhOversampObj;
class PitchShifterJh
{
 public:
    PitchShifterJh();
    virtual ~PitchShifterJh();

    void Reset(BL_FLOAT sampleRate, int blockSize);

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out);
    
    void SetNumChannels(int nchans) {}
    void SetFactor(BL_FLOAT factor);
    // 0, 1, 2 or 3
    void SetQuality(int quality);

    void SetTransBoost(BL_FLOAT transBoost) {}
    int ComputeLatency(int blockSize) { return 0; }
    
 protected:
    friend class PitchShiftJhOversampObj;
    void ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                        vector<WDL_TypedBuf<BL_FLOAT> > *out);
    
    //int mNumChannels;

    PitchShiftJhOversampObj *mOversampObj;

    BL_FLOAT mSampleRate;
    int mOversampling;

    int mJhOversampling;
    
    BL_FLOAT mShift;

    void *mState;
};

#endif
