#ifndef PITCH_SHIFTER_INTERFACE_H
#define PITCH_SHIFTER_INTERFACE_H

class PitchShifterInterface
{
 public:
    virtual void Reset(BL_FLOAT sampleRate, int blockSize) = 0;

    virtual void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                         vector<WDL_TypedBuf<BL_FLOAT> > *out) = 0;
    
    virtual void SetNumChannels(int nchans) = 0;
    virtual void SetFactor(BL_FLOAT factor) = 0;
    // 0, 1, 2 or 3
    virtual void SetQuality(int quality) = 0;

    virtual void SetTransBoost(BL_FLOAT transBoost) = 0;

    virtual int ComputeLatency(int blockSize) = 0;
};

#endif
