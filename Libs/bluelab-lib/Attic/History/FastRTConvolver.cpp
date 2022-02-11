//
//  FastRTConvolver.cpp
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#include <ConvolutionManager.h>

#include "FastRTConvolver.h"

FastRTConvolver::FastRTConvolver(int bufferSize,
                                 const WDL_TypedBuf<BL_FLOAT> &ir)
{
    mBufferSize = bufferSize;
    mCurrentIr = ir;
    
    mConvMan = new ConvolutionManager(ir.Get(), ir.GetSize(), bufferSize);
}

FastRTConvolver::~FastRTConvolver()
{
    delete mConvMan;
}

void
FastRTConvolver::Reset()
{
    delete mConvMan;
    
    mConvMan = new ConvolutionManager(mCurrentIr.Get(),
                                      mCurrentIr.GetSize(),
                                      mBufferSize);
}

void
FastRTConvolver::SetBufferSize(int bufferSize)
{
    mBufferSize = bufferSize;
    
    mConvMan->setBufferSize(bufferSize);
}

void
FastRTConvolver::SetIR(const WDL_TypedBuf<BL_FLOAT> &ir)
{
    mCurrentIr = ir;
    
    mConvMan->setImpulseResponse(ir.Get(), ir.GetSize());
}

void
FastRTConvolver::Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                         WDL_TypedBuf<BL_FLOAT> *result)
{
    int bufferSize = samples.GetSize();
    // HACK (not well checked)
    bufferSize = BLUtils::NextPowerOfTwo(bufferSize);
    
    if (bufferSize != mBufferSize)
    {
        mBufferSize = bufferSize;
        
        mConvMan->setBufferSize(mBufferSize);
    }
    
    WDL_TypedBuf<BL_FLOAT> samples0 = samples;
    // HACK (not well checked)
    if (samples0.GetSize() < mBufferSize)
        BLUtils::ResizeFillZeros(&samples0, mBufferSize);
    
    mConvMan->processInput(samples0.Get());
    
    result->Resize(samples.GetSize());
    const BL_FLOAT *res = mConvMan->getOutputBuffer();
    
    memcpy(result->Get(), res, samples.GetSize()*sizeof(BL_FLOAT));
}
