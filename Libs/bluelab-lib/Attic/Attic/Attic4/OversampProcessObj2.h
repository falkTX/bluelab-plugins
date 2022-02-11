//
//  OversampProcessObj2.h
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#ifndef __Saturate__OversampProcessObj2__
#define __Saturate__OversampProcessObj2__

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

class Oversampler3;
class Decimator;

// See: https://gist.github.com/kbob/045978eb044be88fe568

// OversampProcessObj: dos not take care of Nyquist when downsampling
//
// OversampProcessObj2: use Decimator to take care of Nyquist when downsampling
// GOOD ! : avoid filtering too much high frequencies
class OversampProcessObj2
{
public:
    OversampProcessObj2(int oversampling, BL_FLOAT sampleRate);
    
    virtual ~OversampProcessObj2();
    
    // Must be called at least once
    void Reset(BL_FLOAT sampleRate);
    
    void Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) = 0;
    
    Oversampler3 *mUpOversampler;
    Decimator *mDecimator;
    
    WDL_TypedBuf<BL_FLOAT> mTmpBuf;
    
    int mOversampling;
};

#endif /* defined(__Saturate__OversampProcessObj2__) */
