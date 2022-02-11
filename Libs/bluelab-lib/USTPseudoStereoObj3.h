//
//  USTPseudoStereoObj3.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__USTPseudoStereoObj3__
#define __UST__USTPseudoStereoObj3__

#include "IPlug_include_in_plug_hdr.h"

#define DEFAULT_WIDTH 1.0 //2.0 //1.0

// Implementation of
// "Downmix-compatible conversion from mono to stereo
// in time and frequency-domain" - Marco Fink
// See: https://www.researchgate.net/publication/283008516_Downmix-compatible_conversion_from_mono_to_stereo_in_time-_and_frequency-domain
//
// Renamed PseudoStereoObj(2 ??) => USTPseudoStereoObj
// PseudoStereoObj2: from USTPseudoStereoObj
// - use FastRTConvolver2
//
// USTPseudoStereoObj2 => from PseudoStereoObj2 (direct copy)
//
// USTPseudoStereoObj3: from USTPseudoStereoObj2
// - use FastRTConvolver3
//

class FastRTConvolver3;
class DelayObj4;

class USTPseudoStereoObj3
{
public:
    USTPseudoStereoObj3(BL_FLOAT sampleRate, BL_FLOAT width = DEFAULT_WIDTH);
    
    virtual ~USTPseudoStereoObj3();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
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
    
    // Real convolver
    FastRTConvolver3 *mConvolverL;
    
    // Dummy convolver, for latency
    FastRTConvolver3 *mConvolverR;
    
    DelayObj4 *mDelayObj;
};

#endif /* defined(__UST__USTPseudoStereoObj3__) */
