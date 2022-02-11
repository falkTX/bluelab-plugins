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
class FilterSincConvoBandPass;
class EarlyReflections2;

class FilterRBJNX;

class DelayObj4;
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
    
    // To be called from BL-ReverbDepth plugin,
    // or when setting the config
    //
    void SetUseReverbTail(bool flag);
    
    void SetDry(BL_FLOAT dry);
    void SetWet(BL_FLOAT wet);
    void SetRoomSize(BL_FLOAT roomSize);
    void SetWidth(BL_FLOAT width);
    void SetDamping(BL_FLOAT damping);

    void SetUseFilter(bool flag);
    
    //
    void SetUseEarlyReflections(bool flag);
    
    void SetEarlyRoomSize(BL_FLOAT roomSize);
    void SetEarlyIntermicDist(BL_FLOAT dist);
    void SetEarlyNormDepth(BL_FLOAT depth);
    
    void SetEarlyOrder(int order);
    void SetEarlyReflectCoeff(BL_FLOAT reflectCoeff);
    
protected:
    // Stereo - Use 2 reverb ojects
    void ProcessStereoFull(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                           WDL_TypedBuf<BL_FLOAT> *outputL,
                           WDL_TypedBuf<BL_FLOAT> *outputR);
    
    // Stereo - Use 1 reverb object (reverb objects are stereo anyway)
    void ProcessStereoOptim(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                            WDL_TypedBuf<BL_FLOAT> *outputL,
                            WDL_TypedBuf<BL_FLOAT> *outputR);
    
    void LoadConfig(BL_FLOAT config[13]);
    
    //
    BL_FLOAT mSampleRate;
    
    JReverb *mReverbs[2];
    JReverb *mBypassReverbs[2];
    
    DelayObj4 *mPreDelays[2];
    
    USTStereoWidener *mStereoWiden;
    
    // Filters
    FilterSincConvoBandPass *mSincBandFilters[2];
    
    FilterRBJNX *mLowPassFilters[2];
    FilterRBJNX *mLowCutFilters[2];
    
    EarlyReflections2 *mEarlyRef[2];
    
    BL_FLOAT mDryGain;
    
    //
    bool mUseFilter;
    bool mUseEarly;
    
    bool mUseReverbTail;
};

#endif /* defined(__UST__USTDepthProcess4__) */
