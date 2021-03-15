//
//  AirProcess2.cpp
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>
#include <DebugGraph.h>

//#include <PartialTracker3.h>
//#include <PartialTracker4.h>
#include <PartialTracker5.h>
#include <SoftMaskingComp3.h>

#include "AirProcess2.h"

#define USE_SOFT_MASKING 1 // 0
// 8 gives more gating, but less musical noise remaining
#define SOFT_MASKING_HISTO_SIZE 8

#define FIX_RESET 1

// Soft masking only on harmo? (for perfs)
#define DISABLE_NOISE_SOFT_MASKING 1

AirProcess2::AirProcess2(int bufferSize,
                         BL_FLOAT overlapping, BL_FLOAT oversampling,
                         BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    //mPartialTracker = new PartialTracker3(bufferSize, sampleRate, overlapping);
    //mPartialTracker = new PartialTracker4(bufferSize, sampleRate, overlapping);
    mPartialTracker = new PartialTracker5(bufferSize, sampleRate, overlapping);
    
    mMix = 0.5;
    mTransientSP = 0.5;
    
    mDebugFreeze = false;
    
    for (int i = 0; i < 2; i++)
    {
        mSoftMaskingComps[i] = NULL;
#if USE_SOFT_MASKING
        mSoftMaskingComps[i] = new SoftMaskingComp3(SOFT_MASKING_HISTO_SIZE);
#endif
    }
    
#if DISABLE_NOISE_SOFT_MASKING
    if (mSoftMaskingComps[0] != NULL)
        mSoftMaskingComps[0]->SetProcessingEnabled(false);
#endif
    
#if AIR_PROCESS_PROFILE
    BlaTimer::Reset(&mTimer, &mCount);
#endif
}

AirProcess2::~AirProcess2()
{
    delete mPartialTracker;
    
    for (int i = 0; i < 2; i++)
    {
        if (mSoftMaskingComps[i] != NULL)
            delete mSoftMaskingComps[i];
    }
}

void
AirProcess2::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
AirProcess2::Reset(int bufferSize, int overlapping, int oversampling,
                   BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if FIX_RESET
    //mPartialTracker->Reset();
    mPartialTracker->Reset(bufferSize, sampleRate);
#endif
    
#if USE_SOFT_MASKING
    for (int i = 0; i < 2; i++)
    {
        if (mSoftMaskingComps[i] != NULL)
            mSoftMaskingComps[i]->Reset();
    }
#endif
}

void
AirProcess2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
#if AIR_PROCESS_PROFILE
    BlaTimer::Start(&mTimer);
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples0 = mTmpBuf0;
    fftSamples0 = *ioBuffer0;
    
    // Take half of the complexes
    //BLUtils::TakeHalf(&fftSamples);
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples = mTmpBuf15;
    BLUtils::TakeHalf(fftSamples0, &fftSamples);
        
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    DetectPartials(magns, phases);
        
    if (mPartialTracker != NULL)
    {
        // "Envelopes"
        
#if 1 // ORIGIN
        // Noise "envelope"
        WDL_TypedBuf<BL_FLOAT> &noise = mTmpBuf3;
        mPartialTracker->GetNoiseEnvelope(&noise);

        // NEW
        mPartialTracker->DenormData(&noise);
#endif
        
        mNoise = noise;
        
#if 0 // TEST: smooth noise, but remove transients
        BLUtils::GenNoise(&phases);
        BLUtils::MultValues(&phases, 2.0*M_PI);
#endif
        
#if 0 // Same result, with 6 ou 8 dB less
        // Noise "envelope"
        WDL_TypedBuf<BL_FLOAT> &noiseEnv = mTmp4;
        mPartialTracker->GetNoiseEnvelope(&noiseEnv);
        
        // Gen noise, and mult by envelope
        WDL_TypedBuf<BL_FLOAT> &noise = mTmp5;
        noise.Resize(noiseEnv.GetSize());
        BLUtils::GenNoise(&noise);
        
        BLUtils::MultValues(&noise, noiseEnv);
#endif
        
        // Harmonic "envelope"
        WDL_TypedBuf<BL_FLOAT> &harmo = mTmpBuf6;
        mPartialTracker->GetHarmonicEnvelope(&harmo);

        // NEW
        mPartialTracker->DenormData(&harmo);
        
        mHarmo = harmo;
        
#if 0   // TEST: try to have exactly the same signal when mix is at 0
        // => dos not give exactly the same signal with mix at 0
        // => maxes musical noise when mix is at -100.0
        harmo = magns;
        BLUtils::SubstractValues(&harmo, noise);
        BLUtils::ClipMin(&harmo, 0.0);
#endif
                
        BL_FLOAT noiseCoeff;
        BL_FLOAT harmoCoeff;
        BLUtils::MixParamToCoeffs(mMix, &noiseCoeff, &harmoCoeff);
     
        // Origin
#if !USE_SOFT_MASKING
        // Result
        WDL_TypedBuf<BL_FLOAT> &newMagns = mTmpBuf7;
        newMagns.Resize(magns.GetSize());
        for (int i = 0; i < newMagns.GetSize(); i++)
        {
            BL_FLOAT n = noise.Get()[i];
            BL_FLOAT h = harmo.Get()[i];
            
            BL_FLOAT val = n*noiseCoeff + h*harmoCoeff;
            newMagns.Get()[i] = val;
        }
        magns = newMagns;
#else // New: soft masking
        
        WDL_TypedBuf<BL_FLOAT> &noiseRatio = mTmpBuf8;
        noiseRatio.Resize(noise.GetSize());
        WDL_TypedBuf<BL_FLOAT> &harmoRatio = mTmpBuf9;
        harmoRatio.Resize(harmo.GetSize());

        int noiseRatioSize = noiseRatio.GetSize();
        BL_FLOAT *noiseRatioData = noiseRatio.Get();
        BL_FLOAT *harmoRatioData = harmoRatio.Get();
        BL_FLOAT *noiseData = noise.Get();
        BL_FLOAT *harmoData = harmo.Get();
        BL_FLOAT *magnsData = magns.Get();
        
        //for (int i = 0; i < noiseRatio.GetSize(); i++)
        for (int i = 0; i < noiseRatioSize; i++)
        {
            //BL_FLOAT n = noise.Get()[i];
            //BL_FLOAT h = harmo.Get()[i];
            BL_FLOAT n = noiseData[i];
            BL_FLOAT h = harmoData[i];
            
            //BL_FLOAT nr = 0.0;
            //if (n + h > BL_EPS)
            //    nr = n/(n + h);
    
            // Warning: the sum of noise + harmo could be greater
            // than the total magn.
            // This is because we fill the holes in the noise.
            //BL_FLOAT m = magns.Get()[i];
            BL_FLOAT m = magnsData[i];
            BL_FLOAT nr = 0.0;
            BL_FLOAT hr = 0.0;
            if (m > BL_EPS)
            {
                //nr = n/m;
                //hr = h/m;

                BL_FLOAT mInv = 1.0/m;
                nr = n*mInv;
                hr = h*mInv;
            }
            
            //noiseRatio.Get()[i] = nr;
            //harmoRatio.Get()[i] = 1.0 - nr;
            noiseRatioData[i] = nr;
            harmoRatioData[i] = 1.0 - nr;
        }
        
        // 0: noise, 1: harmo
        //WDL_TypedBuf<WDL_FFT_COMPLEX> result[2];
        WDL_TypedBuf<WDL_FFT_COMPLEX> *result = mTmpBuf10;
        for (int i = 0; i < 2; i++)
        {
            WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples0 = mTmpBuf11;
            fftSamples0 = fftSamples;
            WDL_TypedBuf<WDL_FFT_COMPLEX> &inMask = mTmpBuf12;
            inMask = fftSamples;
            
            if (i == 0)
                BLUtils::MultValues(&inMask, noiseRatio);
            else
                BLUtils::MultValues(&inMask, harmoRatio);
            
            WDL_TypedBuf<WDL_FFT_COMPLEX> &softMask = mTmpBuf13;
            mSoftMaskingComps[i]->ProcessCentered(&fftSamples0,
                                                  &inMask, &softMask);
        
            if (mSoftMaskingComps[i]->IsProcessingEnabled())
            {
                // We have a shift of the input samples in ProcessCentered(),
                // Must update (shift) input samples to take it into account.
                result[i] = fftSamples0;
                BLUtils::MultValues(&result[i], softMask);
            }
            else // Soft masking disabled
            {
                // In mask, but shifted in time depending on the history of
                // soft masking
                result[i] = inMask;
            }
        }
        
        // Combine the result
        WDL_TypedBuf<WDL_FFT_COMPLEX> &outResult = mTmpBuf14;
        outResult.Resize(result[0].GetSize());

        int outResultSize = outResult.GetSize();
        WDL_FFT_COMPLEX *outResultData = outResult.Get();

        WDL_FFT_COMPLEX *result0Data = result[0].Get();
        WDL_FFT_COMPLEX *result1Data = result[1].Get();

        WDL_FFT_COMPLEX res;
        
        //for (int i = 0; i < outResult.GetSize(); i++)
        for (int i = 0; i < outResultSize; i++)
        {
            //WDL_FFT_COMPLEX n = result[0].Get()[i];
            WDL_FFT_COMPLEX &n = result0Data[i];
            n.re *= noiseCoeff;
            n.im *= noiseCoeff;
            
            //WDL_FFT_COMPLEX h = result[1].Get()[i];
            WDL_FFT_COMPLEX &h = result1Data[i];
            h.re *= harmoCoeff;
            h.im *= harmoCoeff;
            
            //WDL_FFT_COMPLEX res;
            COMP_ADD(n, h, res);
            
            //outResult.Get()[i] = res;
            outResultData[i] = res;
        }
        
        BLUtilsComp::ComplexToMagn(&mSum, outResult);
        
        // Optim: avoid reconverting to magns
        // and return early
        //ioBuffer = outResult;
        //ioBuffer->Resize(ioBuffer->GetSize()*2);
        memcpy(ioBuffer0->Get(), outResult.Get(), outResult.GetSize());
        BLUtilsFft::FillSecondFftHalf(ioBuffer0);
        
        return;
#endif
    }
    
    mSum = magns;
    
    // For noise envelope
    //BLUtils::MagnPhaseToComplex(&ioBuffer, magns, phases);
    //ioBuffer->Resize(ioBuffer->GetSize()*2);

    WDL_TypedBuf<WDL_FFT_COMPLEX> &result = mTmpBuf16;
    BLUtilsComp::MagnPhaseToComplex(&result, magns, phases);
    memcpy(ioBuffer0->Get(), result.Get(),
           result.GetSize()*sizeof(WDL_FFT_COMPLEX));
    BLUtilsFft::FillSecondFftHalf(ioBuffer0);
    
#if AIR_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer, &mCount, "AirProcess-profile.txt", "%ld");
#endif
}

void
AirProcess2::SetThreshold(BL_FLOAT threshold)
{
    mPartialTracker->SetThreshold(threshold);
}

void
AirProcess2::SetMix(BL_FLOAT mix)
{
    mMix = mix;
}

int
AirProcess2::GetLatency()
{
#if USE_SOFT_MASKING
    int latency = (SOFT_MASKING_HISTO_SIZE/2)*(mBufferSize/mOverlapping);
    
    // Hack
    latency *= 0.75;
    
    return latency;
#endif
    
    return 0;
}

void
AirProcess2::GetNoise(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mNoise;
}

void
AirProcess2::GetHarmo(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mHarmo;
}

void
AirProcess2::GetSum(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mSum;
}

void
AirProcess2::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases)
{    
    if (!mDebugFreeze)
        mPartialTracker->SetData(magns, phases);
    
    mPartialTracker->DetectPartials();
    
    // Filter or not ?
    //
    // If we filter and we get zombie partials
    // this would make musical noise
    mPartialTracker->FilterPartials();
    
    mPartialTracker->ExtractNoiseEnvelope();
}
