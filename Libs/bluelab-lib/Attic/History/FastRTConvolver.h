//
//  FastRTConvolver.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__FastRTConvolver__
#define __UST__FastRTConvolver__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Zero latency and fast convolver
class ConvolutionManager;
class FastRTConvolver
{
public:
    FastRTConvolver(int bufferSize, const WDL_TypedBuf<BL_FLOAT> &ir);
    
    virtual ~FastRTConvolver();
    
    void Reset();
    
    void SetBufferSize(int bufferSize);
    
    void SetIR(const WDL_TypedBuf<BL_FLOAT> &ir);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> *result);
    
protected:
    int mBufferSize;
    WDL_TypedBuf<BL_FLOAT> mCurrentIr;
    
    ConvolutionManager *mConvMan;
};

#endif /* defined(__UST__FastRTConvolver__) */
