//
//  SamplesToSpectrogram.h
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef __BL_Reverb__SamplesToSpectrogram__
#define __BL_Reverb__SamplesToSpectrogram__

#ifdef IGRAPHICS_NANOVG

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class BLSpectrogram4;
class SimpleSpectrogramFftObj;
class FftProcessObj16;

class SamplesToSpectrogram
{
public:
    SamplesToSpectrogram(BL_FLOAT sampleRate);
    
    virtual ~SamplesToSpectrogram();
    
    void Reset(BL_FLOAT sampleRate);
    
    int GetBufferSize();
    
    void SetSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    BLSpectrogram4 *GetSpectrogram();
    
protected:
    FftProcessObj16 *mFftObj;
    SimpleSpectrogramFftObj *mSpectrogramFftObj;
    
    BLSpectrogram4 *mSpectrogram;
    
    BL_FLOAT mSampleRate;
};

#endif

#endif /* defined(__BL_Reverb__SamplesToSpectrogram__) */
