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
//  JReverb.cpp
//  UST
//
//  Created by applematuer on 8/29/20.
//
//

#include <BLTypes.h>
#include <BLUtils.h>

#include "IPlug_include_in_plug_hdr.h"

#include "JReverb.h"

//
class JReverbCombFilter
{
public:
    JReverbCombFilter();
    
    JReverbCombFilter(const JReverbCombFilter &other);
    
    virtual ~JReverbCombFilter();
    
    void SetSize(int size);
    
    void Clear();
    
    BL_FLOAT Process(BL_FLOAT input, BL_FLOAT damp, BL_FLOAT feedbackLevel);
    
    
private:
    WDL_TypedBuf<BL_FLOAT> mBuffer;
    int mBufferSize;
    int mBufferIndex;
    BL_FLOAT mLast;
};

//
JReverbCombFilter::JReverbCombFilter()
{
    mBufferSize = 0;
    mBufferIndex = 0;
    mLast = 0.0;
}

JReverbCombFilter::JReverbCombFilter(const JReverbCombFilter &other)
{
    mBuffer = other.mBuffer;
    mBufferSize = other.mBufferSize;
    mBufferIndex = other.mBufferIndex;
    mLast = other.mLast;
    
    Clear();
}

JReverbCombFilter::~JReverbCombFilter() {}

void
JReverbCombFilter::SetSize(int size)
{
    if (size != mBufferSize)
    {
        mBufferIndex = 0;
        mBuffer.Resize(size);
        mBufferSize = size;
    }
    
    Clear();
}

void
JReverbCombFilter::Clear()
{
    mLast = 0;
    
    BLUtils::FillAllZero(&mBuffer);
}

BL_FLOAT
JReverbCombFilter::Process(BL_FLOAT input, BL_FLOAT damp, BL_FLOAT feedbackLevel)
{
    const BL_FLOAT output = mBuffer.Get()[mBufferIndex];
    mLast = output * (1.0 - damp) + (mLast * damp);
    
    FIX_FLT_DENORMAL(mLast);
    
    BL_FLOAT temp = input + (mLast * feedbackLevel);
    
    FIX_FLT_DENORMAL(temp);
    
    mBuffer.Get()[mBufferIndex] = temp;
    mBufferIndex = (mBufferIndex + 1) % mBufferSize;
    
    return output;
}

//
class JReverbAllPassFilter
{
public:
    JReverbAllPassFilter();
    
    JReverbAllPassFilter(const JReverbAllPassFilter &other);
    
    virtual ~JReverbAllPassFilter();
    
    void SetSize(int size);
    
    void Clear();
    
    BL_FLOAT Process(BL_FLOAT input);
    
private:
    WDL_TypedBuf<BL_FLOAT> mBuffer;
    int mBufferSize;
    int mBufferIndex;
};

//
JReverbAllPassFilter::JReverbAllPassFilter()
{
    mBufferSize = 0;
    mBufferIndex = 0;
}

JReverbAllPassFilter::JReverbAllPassFilter(const JReverbAllPassFilter &other)
{
    mBuffer = other.mBuffer;
    mBufferSize = other.mBufferSize;
    mBufferIndex = other.mBufferIndex;
    
    Clear();
}

JReverbAllPassFilter::~JReverbAllPassFilter() {}

void
JReverbAllPassFilter::SetSize(int size)
{
    if (size != mBufferSize)
    {
        mBufferIndex = 0;
        mBuffer.Resize(size);
        mBufferSize = size;
    }
    
    Clear();
}

void
JReverbAllPassFilter::Clear()
{
    BLUtils::FillAllZero(&mBuffer);
}

BL_FLOAT
JReverbAllPassFilter::Process(BL_FLOAT input)
{
    const BL_FLOAT bufferedValue = mBuffer.Get()[mBufferIndex];
    BL_FLOAT temp = input + (bufferedValue * 0.5);
    
    FIX_FLT_DENORMAL(temp);
    
    mBuffer.Get()[mBufferIndex] = temp;
    mBufferIndex = (mBufferIndex + 1) % mBufferSize;
    
    return bufferedValue - input;
}

//
JReverb::JReverb()
{
    for (int j = 0; j < mNumChannels; ++j)
    {
        for (int i = 0; i < mNumCombs; ++i)
            mComb[j][i] = new JReverbCombFilter();
        
        for (int i = 0; i < mNumAllPasses; ++i)
            mAllPass[j][i] = new JReverbAllPassFilter();
    }
    
    SetParams(JReverbParams());
    SetSampleRate(44100.0);
}

JReverb::JReverb(const JReverb &other)
{
    mParams = other.mParams;
    
    mGain = other.mGain;
    
    for (int i = 0; i < mNumChannels; i++)
    {
        for (int j = 0; j < mNumCombs; j++)
            mComb[i][j] = new JReverbCombFilter(*other.mComb[i][j]);
    }
    
    for (int i = 0; i < mNumChannels; i++)
    {
        for (int j = 0; j < mNumAllPasses; j++)
            mAllPass[i][j] = new JReverbAllPassFilter(*other.mAllPass[i][j]);
    }
    
    mDamping = other.mDamping;
    mFeedback = other.mFeedback;
    mDryGain = other.mDryGain;
    mWetGain1 = other.mWetGain1;
    mWetGain2 = other.mWetGain2;
}

JReverb::~JReverb()
{
    for (int j = 0; j < mNumChannels; ++j)
    {
        for (int i = 0; i < mNumCombs; ++i)
            delete mComb[j][i];
        
        for (int i = 0; i < mNumAllPasses; ++i)
            delete mAllPass[j][i];
    }
}

BLReverb *
JReverb::Clone() const
{
    JReverb *clone = new JReverb(*this);
    
    return clone;
}

void
JReverb::Reset(BL_FLOAT sampleRate, int blockSize)
{
    SetSampleRate(sampleRate);
}

const JReverb::JReverbParams &
JReverb::GetParams() const
{
    return mParams;
}

void
JReverb::SetParams(const JReverbParams &newParams)
{
    const BL_FLOAT wetScaleFactor = 3.0f;
    const BL_FLOAT dryScaleFactor = 2.0f;
    
    const BL_FLOAT wet = newParams.mWetLevel * wetScaleFactor;
    mDryGain = newParams.mDryLevel * dryScaleFactor;
    mWetGain1 =  0.5 * wet * (1.0 + newParams.mWidth);
    mWetGain2 = 0.5 * wet * (1.0 - newParams.mWidth);
            
    mGain = IsFrozen(newParams.mFreezeMode) ? 0.0 : 0.015;
    mParams = newParams;
    
    UpdateDamping();
}

void
JReverb::SetSampleRate(BL_FLOAT sampleRate)
{
    // Origin
    //static const short combTunings[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }; // (at 44100Hz)
    // Niko: avoid the pre-delay before any reverb occurs!
    static const short combTunings[] = { 1, 72, 161, 240, 306, 375, 441, 501 }; // (at 44100Hz)
    
    static const short allPassTunings[] = { 556, 441, 341, 225 };
    const int stereoSpread = 23;
    const int intSampleRate = (int)sampleRate;
    
    for (int i = 0; i < mNumCombs; ++i)
    {
        mComb[0][i]->SetSize((intSampleRate * combTunings[i])/44100);
        mComb[1][i]->SetSize((intSampleRate * (combTunings[i] + stereoSpread))/44100);
    }
            
    for (int i = 0; i < mNumAllPasses; ++i)
    {
        mAllPass[0][i]->SetSize((intSampleRate * allPassTunings[i])/44100);
        mAllPass[1][i]->SetSize((intSampleRate * (allPassTunings[i] + stereoSpread))/44100);
    }
    
    //mDamping.reset (sampleRate, smoothTime);
    // ...
}

void
JReverb::Reset()
{
    for (int j = 0; j < mNumChannels; ++j)
    {
        for (int i = 0; i < mNumCombs; ++i)
            mComb[j][i]->Clear();
                
        for (int i = 0; i < mNumAllPasses; ++i)
            mAllPass[j][i]->Clear();
    }
}

void
JReverb::ProcessStereo(BL_FLOAT *left, BL_FLOAT *right, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        const BL_FLOAT input = (left[i] + right[i]) * mGain;
        BL_FLOAT outL = 0.0;
        BL_FLOAT outR = 0.0;
        
        // Accumulate the comb filters in parallel
        for (int j = 0; j < mNumCombs; ++j)
        {
            outL += mComb[0][j]->Process(input, mDamping, mFeedback);
            outR += mComb[1][j]->Process(input, mDamping, mFeedback);
        }
        
          // Run the allpass filters in series
        for (int j = 0; j < mNumAllPasses; ++j)
        {
            outL = mAllPass[0][j]->Process(outL);
            outR = mAllPass[1][j]->Process(outR);
        }
                
        left[i]  = outL * mWetGain1 + outR * mWetGain2 + left[i]  * mDryGain;
        right[i] = outR * mWetGain1 + outL * mWetGain2 + right[i] * mDryGain;
    }
}

void
JReverb::ProcessMono(BL_FLOAT *samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        const BL_FLOAT input = samples[i] * mGain;
        BL_FLOAT output = 0;
        
        // Accumulate the comb filters in parallel
        for (int j = 0; j < mNumCombs; ++j)
            output += mComb[0][j]->Process(input, mDamping, mFeedback);
        
        // Run the allpass filters in series
        for (int j = 0; j < mNumAllPasses; ++j)
            output = mAllPass[0][j]->Process(output);
                        
        const BL_FLOAT dry  = mDryGain;
        const BL_FLOAT wet1 = mWetGain1;
                        
        samples[i] = output * wet1 + samples[i] * dry;
    }
}

void
JReverb::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR)
{
    *outputL = input;
    *outputR = input;
    
    ProcessStereo(outputL->Get(), outputR->Get(), outputL->GetSize());
}

void
JReverb::Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR)
{
    *outputL = inputs[0];
    *outputR = inputs[1];
    
    ProcessStereo(outputL->Get(), outputR->Get(), outputL->GetSize());
}

bool
JReverb::IsFrozen(BL_FLOAT freezeMode)
{
    return freezeMode >= 0.5;
}
        
void
JReverb::UpdateDamping()
{
    const BL_FLOAT roomScaleFactor = 0.28;
    const BL_FLOAT roomOffset = 0.7;
    const BL_FLOAT dampScaleFactor = 0.4;
            
    if (IsFrozen(mParams.mFreezeMode))
        SetDamping (0.0, 1.0);
    else
        SetDamping(mParams.mDamping * dampScaleFactor,
                   mParams.mRoomSize * roomScaleFactor + roomOffset);
}
        
void
JReverb::SetDamping(BL_FLOAT dampingToUse, BL_FLOAT roomSizeToUse)
{
    mDamping = dampingToUse;
    mFeedback = roomSizeToUse;
}
