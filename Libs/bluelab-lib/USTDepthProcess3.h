//
//  USTDepthProcess3.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTDepthProcess3__
#define __UST__USTDepthProcess3__

#include "IPlug_include_in_plug_hdr.h"

class BLReverbSndF;
class DelayObj4;
class FilterRBJNX;

class BLReverbIR;

class USTStereoWidener;

// From USTDepthProcess2
// - added transparent processing (equivalent to allpass filters)
// - use BLReverb (which uses sndfilter reverb)
// Makes very good result (compared to the reference vendor plugin),
// but takes too much resource (10% CPU)
class USTDepthProcess3
{
public:
    USTDepthProcess3(BL_FLOAT sampleRate);
    
    virtual ~USTDepthProcess3();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
    void BypassProcess(WDL_TypedBuf<BL_FLOAT> samples[2]);
    
protected:
    BLReverbSndF *mReverbs[2];
    
    DelayObj4 *mPreDelays[2];
    
    FilterRBJNX *mLowPassFilters[2];
    
    FilterRBJNX *mLowCutFilters[2];
    
    BLReverbSndF *mBypassReverbs[2];
    
    BLReverbIR *mReverbIRs[2];
    
    USTStereoWidener *mStereoWiden;
};

#endif /* defined(__UST__USTDepthProcess3__) */
