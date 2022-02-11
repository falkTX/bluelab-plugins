//
//  SamplesDelayer.h
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#ifndef __Transient__SamplesDelayer__
#define __Transient__SamplesDelayer__

#include "../../WDL/IPlug/Containers.h"

// Delays output depending on input, by nFrames
class SamplesDelayer
{
public:
    SamplesDelayer(int nFrames);
    
    virtual ~SamplesDelayer();
    
    // Return true if output is available
    bool Process(const double *input, double *output, int nFrames);
    
    void Reset();
    
protected:
    int mNFrames;
    
    WDL_TypedBuf<double> mSamples;
};

#endif /* defined(__Transient__SamplesDelayer__) */
