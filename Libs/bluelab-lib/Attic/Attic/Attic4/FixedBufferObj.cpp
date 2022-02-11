//
//  FixedBuffedObj.cpp
//  BL-Spatializer
//
//  Created by applematuer on 5/23/19.
//
//

#include <BLUtils.h>

#include "FixedBufferObj.h"

// FIX: This fixes the click when starting playback in Reason (Mac, block size 64)
// That fix finished fixing the problem (even if there were other modifs in othe class)
#define FIX_CLICK_REASON 1

FixedBufferObj::FixedBufferObj(int bufferSize)
{
    mBufferSize = bufferSize;
    
    // Set stereo by default
    // If mono, will fill the second channel with zeros
    mInputs.resize(2);
    mOutputs.resize(2);
    
    mCurrentLatency = mBufferSize;
}

FixedBufferObj::~FixedBufferObj() {}

void
FixedBufferObj::Reset()
{
    for (int i = 0; i < mInputs.size(); i++)
    {
        mInputs[i].Resize(0);
    }
    
    for (int i = 0; i < mOutputs.size(); i++)
    {
        mOutputs[i].Resize(0);
    }
    
    mCurrentLatency = mBufferSize;
}

// NEW
void
FixedBufferObj::Reset(int bufferSize)
{
    mBufferSize = bufferSize;
    
    Reset();
}

void
FixedBufferObj::SetInputs(const vector<WDL_TypedBuf<double> > &buffers)
{
    if (buffers.empty())
        return;
    
    for (int i = 0; i < buffers.size(); i++)
    {
        const WDL_TypedBuf<double> &buf = buffers[i];
        mInputs[i].Add(buf.Get(), buf.GetSize());
    }
    
    if (buffers.size() == 1)
    // Mono
    {
        BLUtils::AddZeros(&mInputs[1], buffers[0].GetSize());
    }
}

bool
FixedBufferObj::GetInputs(vector<WDL_TypedBuf<double> > *buffers)
{
    if (mInputs[0].GetSize() < mBufferSize)
        return false;
    
    if (mInputs[1].GetSize() < mBufferSize)
        return false;
    
    for (int i = 0; i < buffers->size(); i++)
    {
        (*buffers)[i].Resize(mBufferSize);
        memcpy((*buffers)[i].Get(), mInputs[i].Get(), mBufferSize*sizeof(double));
    }
    
    BLUtils::ConsumeLeft(&mInputs[0], mBufferSize);
    BLUtils::ConsumeLeft(&mInputs[1], mBufferSize);
    
    return true;
}

void
FixedBufferObj::ResizeOutputs(vector<WDL_TypedBuf<double> > *buffers)
{
    for (int i = 0; i < buffers->size(); i++)
    {
        WDL_TypedBuf<double> &buf = (*buffers)[i];
        buf.Resize(mBufferSize);
        
#if FIX_CLICK_REASON
        // FIX: This fixes the click when starting playback in Reason (Mac, block size 64)
        // This was due to garbage data in buffer, the values were not initialized
        BLUtils::FillAllZero(&buf);
#endif
    }
}

void
FixedBufferObj::SetOutputs(const vector<WDL_TypedBuf<double> > &buffers)
{
    if (buffers.empty())
        return;
    
    for (int i = 0; i < buffers.size(); i++)
    {
        const WDL_TypedBuf<double> &buf = buffers[i];
        mOutputs[i].Add(buf.Get(), buf.GetSize());
    }
    
    if (buffers.size() == 1)
        // Mono
    {
        BLUtils::AddZeros(&mOutputs[1], buffers[0].GetSize());
    }
}

bool
FixedBufferObj::GetOutputs(vector<WDL_TypedBuf<double> > *buffers, int nFrames)
{
    if ((mOutputs[0].GetSize() < nFrames) ||
        (mOutputs[1].GetSize() < nFrames) ||
        (mCurrentLatency > nFrames))
    {
        for (int i = 0; i < buffers->size(); i++)
        {
            BLUtils::FillAllZero(&(*buffers)[i]);
        }
        
        mCurrentLatency -= nFrames;
        
        if (mCurrentLatency < 0) // Just in case
            mCurrentLatency = 0;
        
        return false;
    }
    
    int nFramesLat = nFrames - mCurrentLatency;
    
    for (int i = 0; i < buffers->size(); i++)
    {
        (*buffers)[i].Resize(nFrames);
        
        BLUtils::FillAllZero(&(*buffers)[i]);
        
        //memcpy((*buffers)[i].Get(), mOutputs[i].Get(), nFrames*sizeof(double));
        memcpy(&(*buffers)[i].Get()[mCurrentLatency], mOutputs[i].Get(), nFramesLat*sizeof(double));
    }
    
    //BLUtils::ConsumeLeft(&mOutputs[0], nFrames);
    //BLUtils::ConsumeLeft(&mOutputs[1], nFrames);
    
    BLUtils::ConsumeLeft(&mOutputs[0], nFramesLat);
    BLUtils::ConsumeLeft(&mOutputs[1], nFramesLat);
    
    mCurrentLatency = 0;
    
    return true;
}
