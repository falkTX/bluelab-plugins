//
//  AirProcess3.cpp
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <PartialTracker5.h>
#include <SoftMaskingComp4.h>

#include "AirProcess3.h"

// 8 gives more gating, but less musical noise remaining
#define SOFT_MASKING_HISTO_SIZE 8

// Set bin #0 to 0 after soft masking
//
// FIX: fixed output peak at bin 0 when harmo only
// (this was due to noise result not at 0 for bin #0)
#define SOFT_MASKING_FIX_BIN0 1


AirProcess3::AirProcess3(int bufferSize,
                         BL_FLOAT overlapping, BL_FLOAT oversampling,
                         BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    //mBufferSize = bufferSize;
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    
    mSampleRate = sampleRate;
    
    mPartialTracker = new PartialTracker5(bufferSize, sampleRate, overlapping);
    
    mMix = 0.5;

    mUseSoftMasks = false;
    mSoftMaskingComp = new SoftMaskingComp4(bufferSize, overlapping,
                                            SOFT_MASKING_HISTO_SIZE);

    mEnableComputeSum = true;
}

AirProcess3::~AirProcess3()
{
    delete mPartialTracker;
    
    if (mSoftMaskingComp != NULL)
        delete mSoftMaskingComp;
}

void
AirProcess3::Reset()
{
    Reset(mBufferSize, mOverlapping, mFreqRes/*mOversampling*/, mSampleRate);
}

void
AirProcess3::Reset(int bufferSize, int overlapping,
                   int oversampling, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    mSampleRate = sampleRate;
    
    mPartialTracker->Reset(bufferSize, sampleRate);
    
    if (mSoftMaskingComp != NULL)
        mSoftMaskingComp->Reset(bufferSize, overlapping);
}

void
AirProcess3::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples0 = mTmpBuf0;
    fftSamples0 = *ioBuffer0;
    
    // Take half of the complexes
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples = mTmpBuf15;
    BLUtils::TakeHalf(fftSamples0, &fftSamples);
        
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    DetectPartials(magns, phases);
        
    if (mPartialTracker != NULL)
    {
        // "Envelopes"
        //
        
        // Noise "envelope"
        mPartialTracker->GetNoiseEnvelope(&mNoise);        
        mPartialTracker->DenormData(&mNoise);
        
        // Harmonic "envelope"
        mPartialTracker->GetHarmonicEnvelope(&mHarmo);
        mPartialTracker->DenormData(&mHarmo);
        
        BL_FLOAT noiseCoeff;
        BL_FLOAT harmoCoeff;
        BLUtils::MixParamToCoeffs(mMix, &noiseCoeff, &harmoCoeff);

        // Compute harmo mask
        WDL_TypedBuf<BL_FLOAT> &mask = mTmpBuf17;
        // Noise mask
        //ComputeMask(mNoise, mHarmo, &noiseMask);
        // Harmo mask
        ComputeMask(mHarmo, mNoise, &mask);
        
#if SOFT_MASKING_FIX_BIN0
        mask.Get()[0] = 0.0;
#endif
            
        if (!mUseSoftMasks)
        {
            // Use input data, and mask it
            // (do not use directly denormed data => the sound is not good)
            
            // harmo is 0, noise is 1
            WDL_TypedBuf<WDL_FFT_COMPLEX> &maskedResult0 = mTmpBuf20;
            WDL_TypedBuf<WDL_FFT_COMPLEX> &maskedResult1 = mTmpBuf21;

            // Harmo
            maskedResult0 = fftSamples;
            BLUtils::MultValues(&maskedResult0, mask);
            BLUtils::MultValues(&maskedResult0, harmoCoeff);
            
            // Noise
            WDL_TypedBuf<BL_FLOAT> &maskOpposite = mTmpBuf22;
            maskOpposite = mask;
            BLUtils::ComputeOpposite(&maskOpposite);
            
            maskedResult1 = fftSamples;
            BLUtils::MultValues(&maskedResult1, maskOpposite);
            BLUtils::MultValues(&maskedResult1, noiseCoeff);
 
            for (int i = 0; i < fftSamples.GetSize(); i++)
            {
                const WDL_FFT_COMPLEX &h = maskedResult0.Get()[i];
                const WDL_FFT_COMPLEX &n = maskedResult1.Get()[i];
                
                WDL_FFT_COMPLEX &res = fftSamples.Get()[i];
                
                COMP_ADD(h, n, res);
            }

            if (mEnableComputeSum)
            {
                // Keep the sum for later
                BLUtilsComp::ComplexToMagn(&mSum, fftSamples);
            }
            
            BLUtilsFft::FillSecondFftHalf(fftSamples, ioBuffer0);
        }
        else // Use oft masking
        {
            WDL_TypedBuf<WDL_FFT_COMPLEX> &softMaskedResult0 = mTmpBuf18;
            WDL_TypedBuf<WDL_FFT_COMPLEX> &softMaskedResult1 = mTmpBuf19;
            mSoftMaskingComp->ProcessCentered(&fftSamples, mask,
                                              &softMaskedResult0, &softMaskedResult1);
            
            if (mSoftMaskingComp->IsProcessingEnabled())
            {
                // Apply "mix"
                //
                
                // 0 is noise mask
                //BLUtils::MultValues(&softMaskedResult0, noiseCoeff);
                //BLUtils::MultValues(&softMaskedResult1, harmoCoeff);
                
                // 0 is harmo mask
                BLUtils::MultValues(&softMaskedResult0, harmoCoeff);
                BLUtils::MultValues(&softMaskedResult1, noiseCoeff);
                
                // Sum
                fftSamples = softMaskedResult0;
                BLUtils::AddValues(&fftSamples, softMaskedResult1);
            }

            if (mEnableComputeSum)
            {
                // Keep the sum for later
                BLUtilsComp::ComplexToMagn(&mSum, fftSamples);
            }
            
            // Result
            BLUtilsFft::FillSecondFftHalf(fftSamples, ioBuffer0);
        }
    }
}

void
AirProcess3::SetThreshold(BL_FLOAT threshold)
{
    mPartialTracker->SetThreshold(threshold);
}

void
AirProcess3::SetMix(BL_FLOAT mix)
{
    mMix = mix;
}

void
AirProcess3::SetUseSoftMasks(bool flag)
{
    mUseSoftMasks = flag;
}

int
AirProcess3::GetLatency()
{
    if (mUseSoftMasks)
    {
        int latency = mSoftMaskingComp->GetLatency();
   
        return latency;
    }
    
    return 0;
}

void
AirProcess3::GetNoise(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mNoise;
}

void
AirProcess3::GetHarmo(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mHarmo;
}

void
AirProcess3::GetSum(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mSum;
}

void
AirProcess3::SetEnableSum(bool flag)
{
    mEnableComputeSum = flag;
}

void
AirProcess3::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases)
{
    mPartialTracker->SetData(magns, phases);
    
    mPartialTracker->DetectPartials();
    
    // Filter or not ?
    //
    // If we filter and we get zombie partials
    // this would make musical noise
    mPartialTracker->FilterPartials();
    
    mPartialTracker->ExtractNoiseEnvelope();
}

// NOTE: need to take care of very small input values...
void
AirProcess3::ComputeMask(const WDL_TypedBuf<BL_FLOAT> &s0Buf,
                         const WDL_TypedBuf<BL_FLOAT> &s1Buf,
                         WDL_TypedBuf<BL_FLOAT> *s0Mask)
{
    s0Mask->Resize(s0Buf.GetSize());
    BLUtils::FillAllZero(s0Mask);
    
    for (int i = 0; i < s0Buf.GetSize(); i++)
    {
        BL_FLOAT s0 = s0Buf.Get()[i];
        BL_FLOAT s1 = s1Buf.Get()[i];

        BL_FLOAT sum = s0 + s1;
        if (sum > BL_EPS)
        {
            BL_FLOAT m = s0/sum;
            s0Mask->Get()[i] = m;
        }
    }
}
