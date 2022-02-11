//
//  USTPseudoStereoObj.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__USTPseudoStereoObj__
#define __UST__USTPseudoStereoObj__

#include "IPlug_include_in_plug_hdr.h"

#define DEFAULT_WIDTH 1.0 //2.0 //1.0

// Implementation of
// "Downmix-compatible conversion from mono to stereo
// in time and frequency-domain" - Marco Fink
// See: https://www.researchgate.net/publication/283008516_Downmix-compatible_conversion_from_mono_to_stereo_in_time-_and_frequency-domain
//
// Renamed PseudoStereoObj2 => USTPseudoStereoObj

class FastRTConvolver;
class DelayObj4;

class USTPseudoStereoObj
{
public:
    USTPseudoStereoObj(BL_FLOAT sampleRate, BL_FLOAT width = DEFAULT_WIDTH);
    
    virtual ~USTPseudoStereoObj();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetWidth(BL_FLOAT width);
    
    void ProcessSamples(const WDL_TypedBuf<BL_FLOAT> &sampsIn,
                        WDL_TypedBuf<BL_FLOAT> *sampsOut0,
                        WDL_TypedBuf<BL_FLOAT> *sampsOut1);
    
    void ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec);
    
protected:
    void GenerateIR(WDL_TypedBuf<BL_FLOAT> *ir);
    
    void SetIRSize(BL_FLOAT sampleRate);
    
    void UpdateDelay();
    
    void NormalizeIR(WDL_TypedBuf<BL_FLOAT> *ir);
    
    void AdjustGain(WDL_TypedBuf<BL_FLOAT> *samples);
    
    //
    int mIRSize;
    
    BL_FLOAT mSampleRate;
    BL_FLOAT mWidth;
    
    FastRTConvolver *mConvolver;
    
    DelayObj4 *mDelayObj;
};

#endif /* defined(__UST__USTPseudoStereoObj__) */
