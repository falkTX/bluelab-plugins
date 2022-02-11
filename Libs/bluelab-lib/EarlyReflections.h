//
//  EarlyReflections.h
//  BL-ReverbDepth
//
//  Created by applematuer on 9/1/20.
//
//

#ifndef __BL_ReverbDepth__EarlyReflections__
#define __BL_ReverbDepth__EarlyReflections__

#include "IPlug_include_in_plug_hdr.h"

//
#define NUM_DELAYS 4

class DelayObj4;
class EarlyReflections
{
public:
    EarlyReflections(BL_FLOAT sampleRate);
    
    EarlyReflections(const EarlyReflections &other);
    
    virtual ~EarlyReflections();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> outRevSamples[2]);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> *outRevSamplesL,
                 WDL_TypedBuf<BL_FLOAT> *outRevSamplesR);
    
    //
    void SetRoomSize(BL_FLOAT roomSize);
    void SetIntermicDist(BL_FLOAT dist);
    void SetNormDepth(BL_FLOAT depth);
    
protected:
    void Init();
    
    //
    BL_FLOAT mSampleRate;
    
    DelayObj4 *mDelays[NUM_DELAYS];
    
    BL_FLOAT mRoomSize;
    BL_FLOAT mIntermicDist;
    BL_FLOAT mNormDepth;
};

#endif /* defined(__BL_ReverbDepth__EarlyReflections__) */
