//
//  SASFrame6.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#include <SASViewerProcess.h>

#include <FftProcessObj16.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>

#include <Scale.h>

#include "SASFrame6.h"

SASFrame6::SASPartial::SASPartial()
{
    mFreq = 0.0;
    mAmp = 0.0;
    mPhase = 0.0;

    mLinkedId = -1;
    mState = Partial::ALIVE;
    mWasAlive = false;
}

SASFrame6::SASPartial::SASPartial(const SASPartial &other)
{
    mFreq = other.mFreq;
    mAmp = other.mAmp;
    mPhase = other.mPhase;

    mLinkedId = other.mLinkedId;
    mState = other.mState;
    mWasAlive = other.mWasAlive;
}

SASFrame6::SASPartial::SASPartial(const Partial &other)
{
    mFreq = other.mFreq;
    mAmp = other.mAmp;
    mPhase = other.mPhase;

    mLinkedId = other.mLinkedId;
    mState = other.mState;
    mWasAlive = other.mWasAlive;
}

SASFrame6::SASPartial::~SASPartial() {}

bool
SASFrame6::SASPartial::AmpLess(const SASPartial &p1, const SASPartial &p2)
{
    return (p1.mAmp < p2.mAmp);
}

SASFrame6::SASFrame6()
{
    mAmplitude = 0.0;
    mFrequency = 0.0;

    mOnsetDetected = false;
    
    mAmpFactor = 1.0;
    mFreqFactor = 1.0;
    mColorFactor = 1.0;
    mWarpingFactor = 1.0;
}

SASFrame6::SASFrame6(int bufferSize, BL_FLOAT sampleRate,
                     int overlapping, int freqRes)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mOverlapping = overlapping;
    mFreqRes = freqRes;
    
    mAmplitude = 0.0;
    mFrequency = 0.0;

    mOnsetDetected = false;
    
    mAmpFactor = 1.0;
    mFreqFactor = 1.0;
    mColorFactor = 1.0;
    mWarpingFactor = 1.0;
}

SASFrame6::~SASFrame6() {}

void
SASFrame6::Reset(BL_FLOAT sampleRate)
{
    mPartials.clear();
    
    mNoiseEnvelope.Resize(0);
    mColor.Resize(0);
    mWarping.Resize(0);
}

void
SASFrame6::Reset(int bufferSize, int oversampling,
                 int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = oversampling;

    mFreqRes = freqRes;
    
    Reset(sampleRate);
}

void
SASFrame6::SetNoiseEnvelope(const WDL_TypedBuf<BL_FLOAT> &noiseEnv)
{
    mNoiseEnvelope = noiseEnv;
}

void
SASFrame6::GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv) const
{
    *noiseEnv = mNoiseEnvelope;
}

BL_FLOAT
SASFrame6::GetAmplitude() const
{
    return mAmplitude*mAmpFactor;
}

BL_FLOAT
SASFrame6::GetFrequency() const
{
    return mFrequency*mFreqFactor;
}

void
SASFrame6::GetColor(WDL_TypedBuf<BL_FLOAT> *color) const
{
    *color = mColor;

    // Sigmoid
    BL_FLOAT a = mColorFactor*0.5;
    if (a < BL_EPS)
        a = BL_EPS;
    if (a > 1.0 - BL_EPS)
        a = 1.0 - BL_EPS;

    BLUtilsMath::ApplySigmoid(color, a);
}

void
SASFrame6::GetWarping(WDL_TypedBuf<BL_FLOAT> *warping) const
{
    *warping = mWarping;

    BLUtils::AddValues(warping, (BL_FLOAT)-1.0);
    BLUtils::MultValues(warping, mWarpingFactor);
    BLUtils::AddValues(warping, (BL_FLOAT)1.0);
}

void
SASFrame6::SetAmplitude(BL_FLOAT amp)
{
    mAmplitude = amp;
}

void
SASFrame6::SetFrequency(BL_FLOAT freq)
{
    mFrequency = freq;
}

void
SASFrame6::SetColor(const WDL_TypedBuf<BL_FLOAT> &color)
{    
    mColor = color;
}

void
SASFrame6::SetWarping(const WDL_TypedBuf<BL_FLOAT> &warping)
{    
    mWarping = warping;
}

void
SASFrame6::SetWarpingInv(const WDL_TypedBuf<BL_FLOAT> &warpingInv)
{    
    mWarpingInv = warpingInv;
}

void
SASFrame6::GetWarpingInv(WDL_TypedBuf<BL_FLOAT> *warpingInv) const
{
    *warpingInv = mWarpingInv;
}

void
SASFrame6::SetPartials(const vector<Partial> &partials)
{
    mPartials.resize(partials.size());
    // Convert
    for (int i = 0; i < mPartials.size(); i++)
        mPartials[i] = partials[i];
}

void
SASFrame6::SetPartials(const vector<SASPartial> &partials)
{
    mPartials = partials;
}

void
SASFrame6::GetPartials(vector<SASPartial> *partials) const
{
    *partials = mPartials;
}

void
SASFrame6::SetOnsetDetected(bool flag)
{
    mOnsetDetected = flag;
}

bool
SASFrame6::GetOnsetDetected() const
{
    return mOnsetDetected;
}

void
SASFrame6::SetAmpFactor(BL_FLOAT factor)
{
    mAmpFactor = factor;
}

void
SASFrame6::SetFreqFactor(BL_FLOAT factor)
{
    mFreqFactor = factor;
}

void
SASFrame6::SetColorFactor(BL_FLOAT factor)
{
    mColorFactor = factor;
}

void
SASFrame6::SetWarpingFactor(BL_FLOAT factor)
{
    mWarpingFactor = factor;
}

void
SASFrame6::MixFrames(SASFrame6 *result,
                     const SASFrame6 &frame0,
                     const SASFrame6 &frame1,
                     BL_FLOAT t, bool mixFreq)
{
    // Amp
    BL_FLOAT amp0 = frame0.GetAmplitude();
    BL_FLOAT amp1 = frame1.GetAmplitude();
    BL_FLOAT resultAmp = (1.0 - t)*amp0 + t*amp1;
    result->SetAmplitude(resultAmp);
    
    // Freq
    if (mixFreq)
    {
        BL_FLOAT freq0 = frame0.GetFrequency();
        BL_FLOAT freq1 = frame1.GetFrequency();
        BL_FLOAT resultFreq = (1.0 - t)*freq0 + t*freq1;
        
        result->SetFrequency(resultFreq);
    }
    else
    {
        BL_FLOAT freq0 = frame0.GetFrequency();
        result->SetFrequency(freq0);
    }
    
    // Color
    WDL_TypedBuf<BL_FLOAT> color0;
    frame0.GetColor(&color0);
    
    WDL_TypedBuf<BL_FLOAT> color1;
    frame1.GetColor(&color1);
    
    WDL_TypedBuf<BL_FLOAT> resultColor;
    BLUtils::Interp(&resultColor, &color0, &color1, t);
    
    result->SetColor(resultColor);
    
    // Warping
    WDL_TypedBuf<BL_FLOAT> warp0;
    frame0.GetWarping(&warp0);
    
    WDL_TypedBuf<BL_FLOAT> warp1;
    frame1.GetWarping(&warp1);
    
    WDL_TypedBuf<BL_FLOAT> resultWarping;
    BLUtils::Interp(&resultWarping, &warp0, &warp1, t);
    
    result->SetWarping(resultWarping);
    
    // Noise envelope
    WDL_TypedBuf<BL_FLOAT> noise0;
    frame0.GetNoiseEnvelope(&noise0);
    
    WDL_TypedBuf<BL_FLOAT> noise1;
    frame1.GetNoiseEnvelope(&noise1);
    
    WDL_TypedBuf<BL_FLOAT> resultNoise;
    BLUtils::Interp(&resultNoise, &noise0, &noise1, t);
    
    result->SetNoiseEnvelope(resultNoise);
}
