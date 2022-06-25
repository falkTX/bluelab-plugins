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
//  BLReverbSndF.cpp
//  UST
//
//  Created by applematuer on 1/16/20.
//
//

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>

#include <BLUtils.h>

#include <BLReverbSndF.h>

#define DUMMY_BLOCK_SIZE 512


BLReverbSndF::BLReverbSndF(BL_FLOAT sampleRate, bool optim)
{
    mSampleRate = sampleRate;
    
    mOptim = optim;
    
    // Init with SF_REVERB_PRESET_MEDIUMROOM2
    mOversampFactor = 2;
    
    mEarlyAmount = 0.50;
    mEarlyWet = -8.0;
    mEarlyDry = -8;
    mEarlyFactor = 1.2;
    mEarlyWidth = 0.6;
    
    mWidth = 0.9;
    mWet  = -8;
    mWander = 0.30;
    mBassBoost = 0.10;
    mSpin = 0.4;
    mInputLPF = 18000;
    mBassLPF = 300;
    mDampLPF = 10000;
    
    mOutputLPF = 18000;
    mRT60 = 1.2;
    mDelay = 0.016;    
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

BLReverbSndF::BLReverbSndF(const BLReverbSndF &other)
{
    // With this, we make the copy without copying
    // the previous internal buffers of sf_reverb_state_st
    //
    // (this avoids having small modifications at each update of the
    // spectrogram in BLReverbViewer)
    //
    mSampleRate = other.mSampleRate;
    mOversampFactor = other.mOversampFactor;
    mEarlyAmount = other.mEarlyAmount;
    mEarlyWet = other.mEarlyWet;
    mEarlyDry = other.mEarlyDry;
    mEarlyFactor = other.mEarlyFactor;
    mEarlyWidth = other.mEarlyWidth;
    mWidth = other.mWidth;
    mWet = other.mWet;
    mWander = other.mWander;
    mBassBoost = other.mBassBoost;
    mSpin = other.mSpin;
    mInputLPF = other.mInputLPF;
    mBassLPF = other.mBassLPF;
    mDampLPF = other.mDampLPF;
    mOutputLPF = other.mOutputLPF;
    mRT60 = other.mRT60;
    mDelay = other.mDelay;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

BLReverbSndF::~BLReverbSndF() {}

BLReverb *
BLReverbSndF::Clone() const
{
    BLReverb *clone = new BLReverbSndF(*this);
    
    return clone;
}

void
BLReverbSndF::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    
    int optim = mOptim ? 1 : 0;
    
#if !USE_ONLY_EARLY
    sf_advancereverb(&mRev, mSampleRate,
                     mOversampFactor,
                     mEarlyAmount,
                     mEarlyWet,
                     mEarlyDry,
                     mEarlyFactor,
                     mEarlyWidth,
                     mWidth,
                     mWet,
                     mWander,
                     mBassBoost,
                     mSpin,
                     mInputLPF,
                     mBassLPF,
                     mDampLPF,
                     mOutputLPF,
                     mRT60,
                     mDelay,
                     optim);
#else
    sf_earlyref(&mRev, mSampleRate, mEarlyFactor, mEarlyWidth);
#endif
}

void
BLReverbSndF::SetOversampFactor(int factor)
{
    mOversampFactor = factor;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetEarlyAmount(BL_FLOAT amount)
{
    mEarlyAmount = amount;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetEarlyWet(BL_FLOAT wet)
{
    mEarlyWet = wet;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetEarlyDry(BL_FLOAT dry)
{
    mEarlyDry = dry;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetEarlyFactor(BL_FLOAT factor)
{
    mEarlyFactor = factor;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetEarlyWidth(BL_FLOAT width)
{
    mEarlyWidth = width;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetWidth(BL_FLOAT width)
{
    mWidth = width;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetWet(BL_FLOAT wet)
{
    mWet = wet;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetWander(BL_FLOAT wander)
{
    mWander = wander;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetBassBoost(BL_FLOAT boost)
{
    mBassBoost = boost;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetSpin(BL_FLOAT spin)
{
    mSpin = spin;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetInputLPF(BL_FLOAT lpf)
{
    mInputLPF = lpf;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetBassLPF(BL_FLOAT lpf)
{
    mBassLPF = lpf;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetDampLPF(BL_FLOAT damp)
{
    mDampLPF = damp;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetOutputLPF(BL_FLOAT lpf)
{
    mOutputLPF = lpf;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetRT60(BL_FLOAT rt60)
{
    mRT60 = rt60;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::SetDelay(BL_FLOAT delay)
{
    mDelay = delay;
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                  WDL_TypedBuf<BL_FLOAT> *outputL,
                  WDL_TypedBuf<BL_FLOAT> *outputR)
{
    int size = input.GetSize();
 
    outputL->Resize(size);
    outputR->Resize(size);
    
    sf_sample_st *input0;
    input0 = (sf_sample_st *)malloc(sizeof(sf_sample_st)*size);
    for (int i = 0; i < size; i++)
    {
        // Mono
        BL_FLOAT val = input.Get()[i];
        FIX_FLT_DENORMAL(val);
        input0[i].L = val;
        input0[i].R = val;
    }
    
    sf_sample_st *output0;
    output0 = (sf_sample_st *)malloc(sizeof(sf_sample_st)*size);
    
#if !USE_ONLY_EARLY
    sf_reverb_process(&mRev, size, input0, output0);
#else
    sf_earlyref_process(&mRev, size, input0, output0);
#endif
    
    for (int i = 0; i < size; i++)
    {
        outputL->Get()[i] = output0[i].L;
        outputR->Get()[i] = output0[i].R;
    }
}

void
BLReverbSndF::Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                  WDL_TypedBuf<BL_FLOAT> *outputL,
                  WDL_TypedBuf<BL_FLOAT> *outputR)
{
    int size = inputs[0].GetSize();
    
    outputL->Resize(size);
    outputR->Resize(size);

    sf_sample_st *input0;
    input0 = (sf_sample_st *)malloc(sizeof(sf_sample_st)*size);
    for (int i = 0; i < size; i++)
    {
        // Stereo
        BL_FLOAT valL = inputs[0].Get()[i];
        BL_FLOAT valR = inputs[1].Get()[i];
        
        input0[i].L = valL;
        input0[i].R = valR;
    }
    
    sf_sample_st *output0;
    output0 = (sf_sample_st *)malloc(sizeof(sf_sample_st)*size);
    
#if !USE_ONLY_EARLY
    sf_reverb_process(&mRev, size, input0, output0);
#else
    sf_earlyref_process(&mRev, size, input0, output0);
#endif
    
    for (int i = 0; i < size; i++)
    {
        outputL->Get()[i] = output0[i].L;
        outputR->Get()[i] = output0[i].R;
    }
}

void
BLReverbSndF::ApplyPreset(BL_FLOAT preset[])
{
    mOversampFactor = preset[0];
    mEarlyAmount = preset[1];
    mEarlyWet = preset[2];
    mEarlyDry = preset[3];
    mEarlyFactor = preset[4];
    mEarlyWidth = preset[5];
    mWidth = preset[6];
    mWet = preset[7];
    mWander = preset[8];
    mBassBoost = preset[9];
    mSpin = preset[10];
    mInputLPF = preset[11];
    mBassLPF = preset[12];
    mDampLPF = preset[13];
    mOutputLPF = preset[14];
    mRT60 = preset[15];
    mDelay = preset[16];
    
    Reset(mSampleRate, DUMMY_BLOCK_SIZE);
}

void
BLReverbSndF::DumpPreset()
{
    fprintf(stderr, "{ %d, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g }\n",
            mOversampFactor, mEarlyAmount, mEarlyWet, mEarlyDry, mEarlyFactor,
            mEarlyWidth, mWidth, mWet, mWander, mBassBoost, mSpin, mInputLPF,
            mBassLPF, mDampLPF, mOutputLPF, mRT60, mDelay);
}
