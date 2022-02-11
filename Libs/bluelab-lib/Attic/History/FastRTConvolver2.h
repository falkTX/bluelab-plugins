//
//  FastRTConvolver2.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__FastRTConvolver2__
#define __UST__FastRTConvolver2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// From FastRTConvolver
// - manage buffer sizes not power of two
// => makes buffering and a bit latency

// Zero latency and fast convolver
class ConvolutionManager;
class FastRTConvolver2
{
public:
    enum Mode
    {
        NORMAL,
        // Used to get exactly the same delay
        // (for stereo channel for example, if we use convolution
        // on only one channel, the second channel will still be synchronized)
        BYPASS
    };
    
    FastRTConvolver2(int bufferSize,
                     const WDL_TypedBuf<BL_FLOAT> &ir,
                     Mode mode = NORMAL);
    
    virtual ~FastRTConvolver2();
    
    void Reset(int blockSize);
    
    void SetBufferSize(int bufferSize);
    
    int GetLatency();
    
    void SetIR(const WDL_TypedBuf<BL_FLOAT> &ir);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> *result);
    
protected:
    void ComputeBufferSize(int blockSize);

    
    int mBufferSize;
    WDL_TypedBuf<BL_FLOAT> mCurrentIr;
    
    ConvolutionManager *mConvMan;
    
    // For bufferizing
    WDL_TypedBuf<BL_FLOAT> mInSamples;
    WDL_TypedBuf<BL_FLOAT> mOutSamples;
    
    int mBlockSize;
    
    long mSampleNum;
    
    Mode mMode;
};

#endif /* defined(__UST__FastRTConvolver2__) */
