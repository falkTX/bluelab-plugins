//
//  SamplesDelayer.h
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#ifndef __Transient__SamplesDelayer__
#define __Transient__SamplesDelayer__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

// Delays output depending on input, by nFrames
class SamplesDelayer
{
public:
    SamplesDelayer(int nFrames);
    
    virtual ~SamplesDelayer();
    
    // Return true if output is available
    bool Process(const BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
    void Reset();
    
protected:
    int mNFrames;
    
    WDL_TypedBuf<BL_FLOAT> mSamples;
};

#endif /* defined(__Transient__SamplesDelayer__) */
