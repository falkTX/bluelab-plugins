#ifndef PITCH_SHIFTER_SMB_H
#define PITCH_SHIFTER_SMB_H

#include <vector>
using namespace std;

// Class created just to test the smb algorithm, without any modification
// (static global variables, license not included in source..)
// => should not be distributed
// (was just for testing)
class PitchShiftSmbOversampObj;
class PitchShifterSmb
{
 public:
    PitchShifterSmb();
    virtual ~PitchShifterSmb();

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
    friend class PitchShiftSmbOversampObj;
    void ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                        vector<WDL_TypedBuf<BL_FLOAT> > *out);
    
    //int mNumChannels;

    PitchShiftSmbOversampObj *mOversampObj;

    BL_FLOAT mSampleRate;
    int mOversampling;

    int mSmbOversampling;
    
    BL_FLOAT mShift;
};

#endif
