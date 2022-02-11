//
//  USTPseudoStereoObj.cpp
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#include <limits.h>

#include <randomsequence.h>

#include <FastRTConvolver.h>
#include <USTProcess.h>
#include <FFTProcessObj16.h>
#include <DelayObj4.h>

#include <BLUtils.h>

#include "USTPseudoStereoObj.h"

// Default value, will be modified in FastRTConvolver
// depending on host buffer size
#define BUFFER_SIZE 512

// For 44100Hz
#define IR_SIZE 2048

// Use delay for difference like in the paper ?
#define USE_DELAY 0

// Fix the gain loss after converting mono to stereo
// (see pan law principle)
// => Now the mono to stereo process is transparent in term of gain
//
// Gain in dB
#define GAIN_ADJUST_DB 6.0 //3.0


USTPseudoStereoObj::USTPseudoStereoObj(BL_FLOAT sampleRate, BL_FLOAT width)
{
    //
    FftProcessObj16::Init();
    
    mSampleRate = sampleRate;
    mWidth = width;
    
    SetIRSize(sampleRate);
    
    WDL_TypedBuf<BL_FLOAT> ir;
    GenerateIR(&ir);
    
    mConvolver = new FastRTConvolver(BUFFER_SIZE, ir);
 
#if USE_DELAY
    mDelayObj = new DelayObj4(1);
    UpdateDelay();
#endif
}

USTPseudoStereoObj::~USTPseudoStereoObj()
{
    delete mConvolver;
    
#if USE_DELAY
    delete mDelayObj;
#endif
}

void
USTPseudoStereoObj::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    SetIRSize(sampleRate);
    
    WDL_TypedBuf<BL_FLOAT> ir;
    GenerateIR(&ir);
    mConvolver->SetIR(ir);
 
#if USE_DELAY
    UpdateDelay();
#endif
}
                              
void
USTPseudoStereoObj::SetWidth(BL_FLOAT width)
{
    mWidth = width;
}

void
USTPseudoStereoObj::ProcessSamples(const WDL_TypedBuf<BL_FLOAT> &sampsIn,
                                 WDL_TypedBuf<BL_FLOAT> *sampsOutL,
                                 WDL_TypedBuf<BL_FLOAT> *sampsOutR)
{
    sampsOutL->Resize(sampsIn.GetSize());
    mConvolver->Process(sampsIn, sampsOutL);
    
    // TEST: simple substraction without delay
    //*sampsOutR = sampsIn;
    //BLUtils::SubstractValues(sampsOutR, *sampsOutL);
    
    // Substract with delay (see paper, "group delay")
    sampsOutR->Resize(sampsIn.GetSize());
    for (int i = 0; i < sampsIn.GetSize(); i++)
    {
        BL_FLOAT samp = sampsIn.Get()[i];
        
#if !USE_DELAY
        BL_FLOAT sampD = samp;
#else
        BL_FLOAT sampD = mDelayObj->ProcessSample(samp);
#endif
        BL_FLOAT sampL = sampsOutL->Get()[i];
        BL_FLOAT sampR = sampD - sampL;
        
        sampsOutR->Get()[i] = sampR;
    }
    
    // Adjust the gain so the processing is transparent
    // when doing nothing
    // (otherwise the sound would be lower)
    AdjustGain(sampsOutL);
    AdjustGain(sampsOutR);
}

void
USTPseudoStereoObj::ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec)
{
    if (samplesVec->empty())
        return;
    
    if (samplesVec->size() != 2)
        return;
    
    USTProcess::StereoToMono(samplesVec);
    
    WDL_TypedBuf<BL_FLOAT> mono = (*samplesVec)[0];
    ProcessSamples(mono, &(*samplesVec)[0], &(*samplesVec)[1]);
    
    // No need, if we want to adjut the default width,
    // change the DEFAULT_WIDTH macro in this class
#if 0
    // Adjust the width by default
#define WIDTH_ADJUST 1.0
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(&(*samplesVec)[0]);
    samples.push_back(&(*samplesVec)[1]);
    
    USTProcess::StereoWiden(&samples, WIDTH_ADJUST);
#endif
}

void
USTPseudoStereoObj::GenerateIR(WDL_TypedBuf<BL_FLOAT> *ir)
{
    ir->Resize(mIRSize);
    
    // Use a random number class
    // So we always have the same seed, without changing the system seed
    // (so the result sound will be always the same)
    unsigned int seedBase = 2345677898907;
    RandomSequenceOfUnique rnd(seedBase, seedBase + 1);
    
    // Generate the random number vector
    WDL_TypedBuf<BL_FLOAT> R;
    R.Resize(ir->GetSize());
    for (int i = 0; i < R.GetSize(); i++)
    {
        unsigned int r = rnd.next();
        
        BL_FLOAT rd = ((BL_FLOAT)r)/UINT_MAX;
        
        rd = (rd - 0.5)*2.0;
        
        R.Get()[i] = rd;
    }
    
    // Compute Hl
    WDL_TypedBuf<BL_FLOAT> Hl;
    Hl.Resize(ir->GetSize());
    for (int i = 0; i < Hl.GetSize(); i++)
    {
        BL_FLOAT wr = mWidth*mWidth*R.Get()[i];
        BL_FLOAT at = std::atan(wr);
        
        BL_FLOAT h = 1.0/2.0 + (1.0/M_PI)*at;
        
        Hl.Get()[i] = h;
    }
    
    // Compute Fft-1(Hl)
    WDL_TypedBuf<BL_FLOAT> hl;
    FftProcessObj16::ComputeInverseFft(Hl, &hl, true);
    
    *ir = hl;
    
    // BAD: shifts the pan to the left
    //NormalizeIR(ir);
}

void
USTPseudoStereoObj::SetIRSize(BL_FLOAT sampleRate)
{
    mIRSize = IR_SIZE*(sampleRate/44100.0);
    mIRSize = BLUtils::NextPowerOfTwo(mIRSize);
}

void
USTPseudoStereoObj::UpdateDelay()
{
#if USE_DELAY
    BL_FLOAT delay = (mIRSize - 1)*0.5;
    
    mDelayObj->SetDelay(delay);
#endif
}

void
USTPseudoStereoObj::NormalizeIR(WDL_TypedBuf<BL_FLOAT> *ir)
{
    BL_FLOAT sum = BLUtils::ComputeSum(*ir);
    
#define EPS 1e-10
    if (std::fabs(sum) > EPS)
    {
        BLUtils::MultValues(ir, 1.0/sum);
    }
}

void
USTPseudoStereoObj::AdjustGain(WDL_TypedBuf<BL_FLOAT> *samples)
{
    BL_FLOAT gain = BLUtils::DBToAmp(GAIN_ADJUST_DB);
    
    BLUtils::MultValues(samples, gain);
}
