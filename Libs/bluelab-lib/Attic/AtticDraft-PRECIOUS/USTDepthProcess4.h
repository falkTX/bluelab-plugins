//
//  USTDepthProcess4.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTDepthProcess4__
#define __UST__USTDepthProcess4__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class JReverb;
class SincConvoBandPassFilter;
class EarlyReflections;

class DelayObj4;
class NRBJFilter;
class USTStereoWidener;


// From USTDepthProcess2
// - added transparent processing (equivalent to allpass filters)
// - use BLReverb (which uses sndfilter reverb)
// Makes very good result (compared to the reference vendor plugin),
// but takes too much resource (10% CPU)
//
// USTDepthProcess4: from USTDepthProcess3
// Use JReverb, to try to improve performances
class USTDepthProcess4
{
public:
    USTDepthProcess4(BL_FLOAT sampleRate);
    
    // For BLReverb
    USTDepthProcess4(const USTDepthProcess4 &other);
    
    virtual ~USTDepthProcess4();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
    void BypassProcess(WDL_TypedBuf<BL_FLOAT> samples[2]);
    
    //
    void DBG_SetDry(BL_FLOAT dry);
    void DBG_SetWet(BL_FLOAT wet);
    void DBG_SetRoomSize(BL_FLOAT roomSize);
    void DBG_SetWidth(BL_FLOAT width);
    void DBG_SetDamping(BL_FLOAT damping);

    void DBG_SetUseFilter(bool flag);
    
    //
    void DBG_SetUseEarlyReflections(bool flag);
    
    void DBG_SetEarlyRoomSize(BL_FLOAT roomSize);
    void DBG_SetEarlyIntermicDist(BL_FLOAT dist);
    void DBG_SetEarlyNormDepth(BL_FLOAT depth);
    
protected:
    // Stereo - Use 2 reverb ojects
    void ProcessStereoFull(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                           WDL_TypedBuf<BL_FLOAT> *outputL,
                           WDL_TypedBuf<BL_FLOAT> *outputR);
    
    // Stereo - Use 1 reverb object (reverb objects are stereo anyway)
    void ProcessStereoOptim(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                            WDL_TypedBuf<BL_FLOAT> *outputL,
                            WDL_TypedBuf<BL_FLOAT> *outputR);
    
    
    JReverb *mReverbs[2];
    JReverb *mBypassReverbs[2];
    
    DelayObj4 *mPreDelays[2];
    
    USTStereoWidener *mStereoWiden;
    
    NRBJFilter *mLowPassFilters[2];
    NRBJFilter *mLowCutFilters[2];
    
    SincConvoBandPassFilter *mSincBandFilters[2];
    
    EarlyReflections *mEarlyRef[2];
    
    BL_FLOAT mDryGain;
    
    //
    bool mDbgUseFilter;
    
    bool mDbgUseEarly;
};

#endif /* defined(__UST__USTDepthProcess4__) */
