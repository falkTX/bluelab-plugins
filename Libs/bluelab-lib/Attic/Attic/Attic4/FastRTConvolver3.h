//
//  FastRTConvolver3.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__FastRTConvolver3__
#define __UST__FastRTConvolver3__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// From FastRTConvolver
// - manage buffer sizes not power of two
// => makes buffering and a bit latency
//
// FastRTConvolver3: from FastRTConvolver2
// - possibility to use WDL convolver
//

// Use WDL fast convolver instead of ConvolutionManager
//
// NOTE: almost the same performances (seem to be no real gain...)
//
// TODO: need to be well tested
#define USE_WDL_CONVOLVER 0 //1

// Zero latency and fast convolver
class ConvolutionManager;
class WDL_ConvolutionEngine_Div;

class FastRTConvolver3
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
    
    FastRTConvolver3(int bufferSize, BL_FLOAT sampleRate,
                     const WDL_TypedBuf<BL_FLOAT> &ir,
                     Mode mode = NORMAL);
    
    FastRTConvolver3(const FastRTConvolver3 &other);
    
    virtual ~FastRTConvolver3();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    void SetBufferSize(int bufferSize);
    
    int GetLatency();
    
    void SetIR(const WDL_TypedBuf<BL_FLOAT> &ir);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> *result);
    
protected:
    void ComputeBufferSize(int blockSize);

    //
    BL_FLOAT mSampleRate;
    
    int mBufferSize;
    WDL_TypedBuf<BL_FLOAT> mCurrentIr;
    
#if !USE_WDL_CONVOLVER
    ConvolutionManager *mConvMan;
#else
    WDL_ConvolutionEngine_Div *mConvEngine;
#endif
    
    // For bufferizing
    WDL_TypedBuf<BL_FLOAT> mInSamples;
    WDL_TypedBuf<BL_FLOAT> mOutSamples;
    
    int mBlockSize;
    
    long mSampleNum;
    
    Mode mMode;
};

#endif /* defined(__UST__FastRTConvolver3__) */
