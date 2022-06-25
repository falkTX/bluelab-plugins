/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  FastRTConvolver3.cpp
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <Resampler2.h>

#include "FastRTConvolver3.h"

#if !USE_WDL_CONVOLVER
#include <ConvolutionManager.h>
#else
#include "convoengine.h"
#endif


// NOTE: bad, too slow to change buffer size on the fly
//
// On Reaper, when the computer slows down, the samples provided
// is less thant the declared blockSize
#define FIX_VARIABLE_BUFFER_SIZE 0 //1

// NOTE: makes a shift after some time, when playing a short loop
// several times (Reaper)
//
// FIX: On Reaper, at end of selection, the provided samples size
// can be less thant the official blockSize
// In this case, pad the input with zeros to match the blockSize
#define FIX_VARIABLE_BUFFER_SIZE2 0 //1

// GOOD: makes no shift when looping
//
// FIX: On Reaper, at end of selection, the provided samples size
// can be less thant the official blockSize
// In this case, return zeros if no input is available
//
// (crash sometimes if not set)
#define FIX_VARIABLE_BUFFER_SIZE3 1

// Make a small fade when we complete with zeros, to avoid a click
// (for example at the end of a loop)
// NOTE: useless if we managed to compute  0 values
#define FIX_VARIABLE_BUFFER_SIZE3_FADE 0 //1

#define FIX_CRASH_FLSTUDIO_MAC_AU 1

FastRTConvolver3::FastRTConvolver3(int bufferSize, BL_FLOAT sampleRate,
                                   const WDL_TypedBuf<BL_FLOAT> &ir,
                                   Mode mode)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mCurrentIr = ir;
    
    //mBlockSize = BUFFER_SIZE; // 512
    mBlockSize = bufferSize;
    
    mSampleNum = 0;
    
    mMode = mode;
    
#if !USE_WDL_CONVOLVER
    mConvMan = new ConvolutionManager(ir.Get(), ir.GetSize(), bufferSize);
#else
    mConvEngine = new WDL_ConvolutionEngine_Div();
    
    WDL_ImpulseBuffer impulse;
    impulse.SetLength(mCurrentIr.GetSize());
    impulse.SetNumChannels(1);
    impulse.samplerate = mSampleRate;
    impulse.impulses[0] = mCurrentIr;
    
    //int res = mConvEngine->SetImpulse(&impulse, int maxfft_size=0, int known_blocksize=0, int max_imp_size=0, int impulse_offset=0, int latency_allowed=0);

    mConvEngine->SetImpulse(&impulse);
    //mConvEngine->SetImpulse(&impulse, 0, mBlockSize);
#endif
}

FastRTConvolver3::FastRTConvolver3(const FastRTConvolver3 &other)
{
    mBufferSize = other.mBufferSize;
    mSampleRate = other.mSampleRate;
    
    mCurrentIr = other.mCurrentIr;
    
    mBlockSize = other.mBlockSize;
    
    //mSampleNum = other.mSampleNum;
    mSampleNum = 0;
    
    mMode = other.mMode;
    
#if !USE_WDL_CONVOLVER
    mConvMan = new ConvolutionManager(mCurrentIr.Get(), mCurrentIr.GetSize(), mBufferSize);
#else
    mConvEngine = new WDL_ConvolutionEngine_Div();
    
    WDL_ImpulseBuffer impulse;
    impulse.SetLength(mCurrentIr.GetSize());
    impulse.SetNumChannels(1);
    impulse.samplerate = mSampleRate;
    impulse.impulses[0] = mCurrentIr;
    
    mConvEngine->SetImpulse(&impulse);
    //mConvEngine->SetImpulse(&impulse, 0, mBlockSize);
#endif
}

FastRTConvolver3::~FastRTConvolver3()
{
#if !USE_WDL_CONVOLVER
    delete mConvMan;
#else
    delete mConvEngine;
#endif
}

void
FastRTConvolver3::Reset(BL_FLOAT sampleRate, int blockSize)
{
    //mInSamples.Resize(0);
    //mOutSamples.Resize(0);
    mInSamples.Clear();
    mOutSamples.Clear();
    
    mBlockSize = blockSize;
    ComputeBufferSize(blockSize);
    
    mSampleRate = sampleRate;
    
    mSampleNum = 0;
    
#if !USE_WDL_CONVOLVER
    delete mConvMan;
    
    mConvMan = new ConvolutionManager(mCurrentIr.Get(),
                                      mCurrentIr.GetSize(),
                                      mBufferSize);
#else
    mConvEngine->Reset();
    
    WDL_ImpulseBuffer impulse;
    impulse.SetLength(mCurrentIr.GetSize());
    impulse.SetNumChannels(1);
    impulse.samplerate = mSampleRate;
    impulse.impulses[0] = mCurrentIr;
    
    mConvEngine->SetImpulse(&impulse); //, 0, mBlockSize);
    //mConvEngine->SetImpulse(&impulse, 0, mBlockSize);
#endif
}

int
FastRTConvolver3::GetLatency()
{
    int latency = 0;
    
#if !USE_WDL_CONVOLVER
    //int latency = mBlockSize - mBufferSize;
    if (mBlockSize != mBufferSize)
    {
        latency = mBlockSize;
    }
#else
    latency = mConvEngine->GetLatency();
#endif
    
    return latency;
}

void
FastRTConvolver3::SetBufferSize(int bufferSize)
{
    mBufferSize = bufferSize;
    
#if !USE_WDL_CONVOLVER
    if (mConvMan != NULL)
        mConvMan->setBufferSize(bufferSize);
#endif
}

void
FastRTConvolver3::SetIR(const WDL_TypedBuf<BL_FLOAT> &ir)
{
    mCurrentIr = ir;
    
#if !USE_WDL_CONVOLVER
    if (mConvMan != NULL)
        mConvMan->setImpulseResponse(ir.Get(), ir.GetSize());
#else
    mConvEngine->Reset();
    
    WDL_ImpulseBuffer impulse;
    impulse.SetLength(mCurrentIr.GetSize());
    impulse.SetNumChannels(1);
    impulse.samplerate = mSampleRate;
    impulse.impulses[0] = mCurrentIr;
    
    mConvEngine->SetImpulse(&impulse); //, 0, mBlockSize);
    //mConvEngine->SetImpulse(&impulse, 0, mBlockSize);
#endif
}

void
FastRTConvolver3::Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                          WDL_TypedBuf<BL_FLOAT> *result)
{
    // check size
    if (result->GetSize() != samples.GetSize())
        result->Resize(samples.GetSize());
    
#if FIX_VARIABLE_BUFFER_SIZE
    // Check, in case samples size is not the same as the blockSize
    // declared by the host
    ComputeBufferSize(samples.GetSize());
#endif
    
    // Add in samples
    mInSamples.Add(samples.Get(), samples.GetSize());

#if FIX_VARIABLE_BUFFER_SIZE2
    if (samples.GetSize() != mBlockSize)
    {
        int numZeros = mBlockSize - samples.GetSize();
        if (numZeros > 0)
            //BLUtils::PadZerosRight(&mInSamples, numZeros);
            mInSamples.Add(0, numZeros);
    }
#endif
    
    // Process as many samples as possible
    //while(mInSamples.GetSize() >= mBufferSize)
    while(mInSamples.Available() >= mBufferSize)
    {
        // Prepare the buffer to process
        //WDL_TypedBuf<BL_FLOAT> inBuf;
        //inBuf.Add(mInSamples.Get(), mBufferSize);

        WDL_TypedBuf<BL_FLOAT> &inBuf = mTmpBuf0;
        inBuf.Resize(mBufferSize);
        //BLUtils::SetBuf(&inBuf, mInSamples);
        BLUtils::FastQueueToBuf(mInSamples, &inBuf, mBufferSize);
        
        BLUtils::ConsumeLeft(&mInSamples, mBufferSize);
        
        WDL_TypedBuf<BL_FLOAT> &outBuf = mTmpBuf1;
        if (mMode == NORMAL)
        {
            outBuf.Resize(mBufferSize);
            
            // Process
#if !USE_WDL_CONVOLVER
            if (mConvMan != NULL)
            {
                mConvMan->processInput(inBuf.Get());
            
                // Output
                const BL_FLOAT *convo = mConvMan->getOutputBuffer();
                memcpy(outBuf.Get(), convo, outBuf.GetSize()*sizeof(BL_FLOAT));
            }
#else
            BL_FLOAT *inBuffer[1];
            inBuffer[0] = inBuf.Get();
            mConvEngine->Add(inBuffer, inBuf.GetSize(), 1);
            
            int nAvail = MIN(mConvEngine->Avail(inBuf.GetSize()), inBuf.GetSize());
            WDL_FFT_REAL *convo = mConvEngine->Get()[0];
            
            if (nAvail < outBuf.GetSize())
                memset(outBuf.Get(), 0, outBuf.GetSize() - nAvail);
            
            if ((nAvail > 0) && (outBuf.GetSize() > 0) &&
                (nAvail <= outBuf.GetSize()))
                memcpy(&outBuf.Get()[outBuf.GetSize() - nAvail],
                       convo, nAvail*sizeof(BL_FLOAT));
            
            mConvEngine->Advance(nAvail);
#endif
        }
        else // BYPASS
        {
            outBuf = inBuf;
        }
        
        mOutSamples.Add(outBuf.Get(), outBuf.GetSize());
    }
    
    // Manage latency
    int latency = GetLatency();
    
    int numZeros = latency - mSampleNum;
    if (numZeros < 0)
        numZeros = 0;
    
    //
    if (numZeros > samples.GetSize())
        numZeros = samples.GetSize();

    int numOutValues = samples.GetSize() - numZeros;
    
    WDL_TypedBuf<BL_FLOAT> &outRes = mTmpBuf2;
    //if (numZeros > 0)
    //    BLUtils::ResizeFillZeros(&outRes, numZeros);
    outRes.Resize(numZeros + numOutValues);
    if (numZeros > 0)
        memset(outRes.Get(), 0, numZeros*sizeof(BL_FLOAT));
    
    //
    if (numOutValues < 0)
        numOutValues = 0;
    
#if FIX_VARIABLE_BUFFER_SIZE3
    //if (mOutSamples.GetSize() < numOutValues)
    //    numOutValues = mOutSamples.GetSize();
    if (mOutSamples.Available() < numOutValues)
        numOutValues = mOutSamples.Available();
#endif

    //outRes.Add(mOutSamples.Get(), numOutValues);
    WDL_TypedBuf<BL_FLOAT> &tmpBuf = mTmpBuf3;
    tmpBuf.Resize(numOutValues);
    mOutSamples.GetToBuf(0, tmpBuf.Get(), numOutValues);
    memcpy(&outRes.Get()[numZeros],
           tmpBuf.Get(),
           numOutValues*sizeof(BL_FLOAT));
           
    BLUtils::ConsumeLeft(&mOutSamples, numOutValues);
    
    // result
    memcpy(result->Get(), outRes.Get(), numOutValues*sizeof(BL_FLOAT));
    
#if FIX_VARIABLE_BUFFER_SIZE3
    // Fill the rest with zeros if not enough out values are available
    // (can be the case if the blockSize suddently changed)
    if (numOutValues < result->GetSize())
    {
        int numZeros2 = result->GetSize() - numOutValues;
        memset(&result->Get()[numOutValues], 0, numZeros2*sizeof(BL_FLOAT));
        
#if FIX_VARIABLE_BUFFER_SIZE3_FADE
        
#define NUM_SAMPLES_FADE 10
        BLUtils::FadeOut(result, numOutValues - NUM_SAMPLES_FADE, numOutValues);
#endif
    }
#endif
    
    //
    mSampleNum += samples.GetSize();
}

void
FastRTConvolver3::ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                  BL_FLOAT sampleRate, BL_FLOAT respSampleRate)
{
    if (impulseResponse->GetSize() == 0)
        return;
    
    if (respSampleRate != sampleRate)
        // We have to resample the impulse
    {
        if (impulseResponse->GetSize() > 0)
        {
            Resampler2 resampler(sampleRate, respSampleRate);

            WDL_TypedBuf<BL_FLOAT> newImpulse;
            resampler.Resample(impulseResponse, &newImpulse);
            
            ResizeImpulse(&newImpulse);
            
            *impulseResponse = newImpulse;
        }
    }
}

void
FastRTConvolver3::ResampleImpulse2(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                   BL_FLOAT sampleRate, BL_FLOAT respSampleRate,
                                   bool resizeToNextPowerOfTwo)
{
    if (impulseResponse->GetSize() == 0)
        return;
    
    if (respSampleRate != sampleRate)
        // We have to resample the impulse
    {
        if (impulseResponse->GetSize() > 0)
        {
            Resampler2 resampler(sampleRate, respSampleRate);
            
            WDL_TypedBuf<BL_FLOAT> newImpulse;
            resampler.Resample2(impulseResponse, &newImpulse);
            
            if (resizeToNextPowerOfTwo)
                ResizeImpulse(&newImpulse);
            
            *impulseResponse = newImpulse;
        }
    }
}

void
FastRTConvolver3::ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse)
{
    int respSize = impulseResponse->GetSize();
    int newSize = BLUtilsMath::NextPowerOfTwo(respSize);
    int diff = newSize - impulseResponse->GetSize();
        
    impulseResponse->Resize(newSize);
        
    // Fill with zeros if we have grown
    for (int j = 0; j < diff; j++)
    {
        int index = newSize - j - 1;
        if (index < 0)
            continue;
        if (index > impulseResponse->GetSize())
            continue;
        
        impulseResponse->Get()[index] = 0.0;
    }
}

void
FastRTConvolver3::ComputeBufferSize(int blockSize)
{
    // Update buffer size
    int bufferSize = blockSize;
    bufferSize = BLUtilsMath::NextPowerOfTwo(bufferSize);
    if (bufferSize > blockSize)
        bufferSize *= 0.5;
    
    if (bufferSize != mBufferSize)
    {
        mBufferSize = bufferSize;
        
#if !USE_WDL_CONVOLVER
        if (mConvMan != NULL)
            mConvMan->setBufferSize(mBufferSize);
#endif
    }
}
