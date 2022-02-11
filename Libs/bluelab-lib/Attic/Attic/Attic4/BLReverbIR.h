//
//  BLReverbIR.h
//  BL-Reverb
//
//  Created by applematuer on 1/17/20.
//
//

#ifndef __BL_Reverb__BLReverbIR__
#define __BL_Reverb__BLReverbIR__

class BLReverb;
class FastRTConvolver3;

class BLReverbIR
{
public:
    BLReverbIR(BLReverb *reverb, BL_FLOAT sampleRate,
               BL_FLOAT IRLengthSeconds);
    
    virtual ~BLReverbIR();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void UpdateIRs();
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &sampsIn,
                 WDL_TypedBuf<BL_FLOAT> *sampsOut0,
                 WDL_TypedBuf<BL_FLOAT> *sampsOut1);
    
protected:
    BL_FLOAT mSampleRate;
    BL_FLOAT mIRLengthSeconds;
    
    BLReverb *mReverb;
    
    FastRTConvolver3 *mConvolvers[2];
};

#endif /* defined(__BL_Reverb__BLReverbIR__) */
