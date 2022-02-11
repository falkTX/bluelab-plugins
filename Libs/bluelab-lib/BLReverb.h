//
//  BLReverb.h
//  BL-ReverbDepth
//
//  Created by applematuer on 8/31/20.
//
//

#ifndef BL_ReverbDepth_BLReverb_h
#define BL_ReverbDepth_BLReverb_h

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class BLReverb
{
public:
    virtual ~BLReverb();
    
    virtual BLReverb *Clone() const = 0;
    
    virtual void Reset(BL_FLOAT sampleRate, int blockSize) = 0;
    
    // Mono
    virtual void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR) = 0;
    
    // Stereo
    virtual void Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR) = 0;
    
    //
    void GetIRs(WDL_TypedBuf<BL_FLOAT> irs[2], int size);
};

#endif
