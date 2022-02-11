//
//  RebalanceProcessFftObj.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#include <RebalanceMaskPredictor.h>

#include <DbgSpectrogram.h>

#include <BLUtils.h>

#include "RebalanceProcessFftObj.h"

// See Rebalance
#define EPS 1e-10


RebalanceProcessFftObj::RebalanceProcessFftObj(int bufferSize,
                                               RebalanceMaskPredictor *maskPred)
: ProcessObj(bufferSize)
{
    mMaskPred = maskPred;
    
    // Parameters
    mVocal = 0.0;
    mBass = 0.0;
    mDrums = 0.0;
    mOther = 0.0;
    
    mVocalSensitivity = 1.0;
    mBassSensitivity = 1.0;
    mDrumsSensitivity = 1.0;
    mOtherSensitivity = 1.0;
    
    // Global precision (soft/hard)
    mPrecision = 0.0;
    
    mMode = Rebalance::SOFT;
    
    // Spectrogram debug
    mDbgSpectrogramDump = false;
    
    mDbgMixSpectrogram = NULL;
    for (int k = 0; k < 4; k++)
        mDbgSourceSpectrograms[k] = NULL;
    
#if FORCE_SAMPLE_RATE
    mSampleRate = SAMPLE_RATE;
    mPlugSampleRate = SAMPLE_RATE;
    
    InitResamplers();
    
    mRemainingSamples = 0.0;
#endif
}

RebalanceProcessFftObj::~RebalanceProcessFftObj()
{
    if (mDbgMixSpectrogram != NULL)
        delete mDbgMixSpectrogram;
    
    for (int k = 0; k < 4; k++)
    {
        if (mDbgSourceSpectrograms[k] != NULL)
            delete mDbgSourceSpectrograms[k];
    }
}

void
RebalanceProcessFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
#if FORCE_SAMPLE_RATE
    mPlugSampleRate = sampleRate;
    
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
}

void
RebalanceProcessFftObj::Reset()
{
    ProcessObj::Reset();
    
#if FORCE_SAMPLE_RATE
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
}

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObj::ResetResamplers()
{
    mResamplerIn.Reset();
    mResamplerIn.SetRates(mPlugSampleRate, SAMPLE_RATE);
    
    mResamplerOut.Reset();
    mResamplerOut.SetRates(SAMPLE_RATE, mPlugSampleRate);
}
#endif

void
RebalanceProcessFftObj::SetVocal(BL_FLOAT vocal)
{
    mVocal = vocal;
}

void
RebalanceProcessFftObj::SetBass(BL_FLOAT bass)
{
    mBass = bass;
}

void
RebalanceProcessFftObj::SetDrums(BL_FLOAT drums)
{
    mDrums = drums;
}

void
RebalanceProcessFftObj::SetOther(BL_FLOAT other)
{
    mOther = other;
}

void
RebalanceProcessFftObj::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
    mVocalSensitivity = vocalSensitivity;
}

void
RebalanceProcessFftObj::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
    mBassSensitivity = bassSensitivity;
}

void
RebalanceProcessFftObj::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
    mDrumsSensitivity = drumsSensitivity;
}

void
RebalanceProcessFftObj::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
    mOtherSensitivity = otherSensitivity;
}

void
RebalanceProcessFftObj::SetPrecision(BL_FLOAT precision)
{
    mPrecision = precision;
}

void
RebalanceProcessFftObj::SetMode(Rebalance::Mode mode)
{
    mMode = mode;
}

void
RebalanceProcessFftObj::ApplySensitivity(BL_FLOAT *maskVocal, BL_FLOAT *maskBass,
                                         BL_FLOAT *maskDrums, BL_FLOAT *maskOther)
{
    if (*maskVocal < mVocalSensitivity)
        *maskVocal = 0.0;
    
    if (*maskBass < mBassSensitivity)
        *maskBass = 0.0;
    
    if (*maskDrums < mDrumsSensitivity)
        *maskDrums = 0.0;
    
    if (*maskOther < mOtherSensitivity)
        *maskOther = 0.0;
}

void
RebalanceProcessFftObj::DBG_SetSpectrogramDump(bool flag)
{
    mDbgSpectrogramDump = flag;
    
    if (mDbgSpectrogramDump)
    {
        mDbgMixSpectrogram = new DbgSpectrogram(BUFFER_SIZE/(2/**RESAMPLE_FACTOR*/));
        mDbgMixSpectrogram->SetAmpDb(true);
        
        for (int k = 0; k < 4; k++)
            mDbgSourceSpectrograms[k] = new DbgSpectrogram(BUFFER_SIZE/(2/**RESAMPLE_FACTOR*/));
    }
}

void
RebalanceProcessFftObj::DBG_SaveSpectrograms()
{
    if (mDbgSpectrogramDump)
    {
        mDbgMixSpectrogram->SavePPM("pred-mix.ppm");
        
        mDbgSourceSpectrograms[0]->SavePPM("pred-source0.ppm");
        mDbgSourceSpectrograms[1]->SavePPM("pred-source1.ppm");
        mDbgSourceSpectrograms[2]->SavePPM("pred-source2.ppm");
        mDbgSourceSpectrograms[3]->SavePPM("pred-source3.ppm");
    }
}

void
RebalanceProcessFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                         const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
#if DEBUG_LISTEN_BUFFER
    DBG_ListenBuffer(ioBuffer, scBuffer);
    
    return;
#endif
    
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magnsMix;
    WDL_TypedBuf<BL_FLOAT> phasesMix;
    BLUtils::ComplexToMagnPhase(&magnsMix, &phasesMix, mixBuffer);
    
    WDL_TypedBuf<BL_FLOAT> resultMagns;
    
    // Prev code: binary choice: soft or hard
#if 0
    if (mMode == Rebalance::SOFT)
        ComputeMixSoft(&resultMagns, magnsMix);
    else
        ComputeMixHard(&resultMagns, magnsMix);
#endif
    
#if 1
    // New code: interpolate between soft and hard
    WDL_TypedBuf<BL_FLOAT> magnsSoft;
    ComputeMixSoft(&magnsSoft, magnsMix);
    
    WDL_TypedBuf<BL_FLOAT> magnsHard;
    ComputeMixHard(&magnsHard, magnsMix);
    
    BLUtils::Interp(&resultMagns, &magnsSoft, &magnsHard, mPrecision);
#endif
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    BLUtils::MagnPhaseToComplex(&fftSamples, resultMagns, phasesMix);
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}

// GOOD
#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObj::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mPlugSampleRate == SAMPLE_RATE)
        return;
    
    BL_FLOAT sampleRate = mPlugSampleRate;
    
    WDL_ResampleSample *resampledAudio = NULL;
    int desiredSamples = ioBuffer->GetSize(); // Input driven
    
    int numOutSamples = ioBuffer->GetSize()*sampleRate/SAMPLE_RATE; // Input driven
    
    int numSamples = mResamplerOut.ResamplePrepare(desiredSamples, 1, &resampledAudio);
    
#if FIX_ADJUST_OUT_RESAMPLING
    // Compute remaining "parts of sample", due to rounding
    // and re-add it to the number of requested samples
    // FIX: fixes blank frame with sample rate 48000 and buffer size 447
    //
    BL_FLOAT remaining = ((BL_FLOAT)ioBuffer->GetSize())*sampleRate/SAMPLE_RATE - numOutSamples;
    mRemainingSamples += remaining;
    if (mRemainingSamples >= 1.0)
    {
        int addSamples = floor(mRemainingSamples);
        mRemainingSamples -= addSamples;
        
        numOutSamples += addSamples;
    }
#endif
    
    for (int i = 0; i < numSamples; i++)
    {
        if (i >= ioBuffer->GetSize())
            break;
        resampledAudio[i] = ioBuffer->Get()[i];
    }
    
    WDL_TypedBuf<BL_FLOAT> outSamples;
    outSamples.Resize(numOutSamples); // Input driven
    
    int numResampled = mResamplerOut.ResampleOut(outSamples.Get(),
                                                 // Must be exactly the value returned by ResamplePrepare
                                                 // Otherwise the spectrogram could be scaled horizontally
                                                 // (or clicks)
                                                 // Due to flush of the resampler
                                                 numSamples,
                                                 outSamples.GetSize(), 1);
    
    // GOOD
    outSamples.Resize(numResampled);
    
    *ioBuffer = outSamples;
}
#endif

void
RebalanceProcessFftObj::DBG_ListenBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                         const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magnsMix;
    WDL_TypedBuf<BL_FLOAT> phasesMix;
    BLUtils::ComplexToMagnPhase(&magnsMix, &phasesMix, mixBuffer);
    
    RebalanceMaskPredictor::Downsample(&magnsMix, mSampleRate);
    RebalanceMaskPredictor::Upsample(&magnsMix, mSampleRate);
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    BLUtils::MagnPhaseToComplex(&fftSamples, magnsMix, phasesMix);
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}

void
RebalanceProcessFftObj::NormalizeMasks(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                                       WDL_TypedBuf<BL_FLOAT> *maskBass,
                                       WDL_TypedBuf<BL_FLOAT> *maskDrums,
                                       WDL_TypedBuf<BL_FLOAT> *maskOther)
{
    for (int i = 0; i < maskVocal->GetSize(); i++)
    {
        BL_FLOAT vocal = maskVocal->Get()[i];
        BL_FLOAT bass = maskBass->Get()[i];
        BL_FLOAT drums = maskDrums->Get()[i];
        BL_FLOAT other = maskOther->Get()[i];
        
        BL_FLOAT sum = vocal + bass + drums + other;
        
        if (std::fabs(sum) > EPS)
        {
            vocal /= sum;
            bass /= sum;
            drums /= sum;
            other /= sum;
        }
        
        maskVocal->Get()[i] = vocal;
        maskBass->Get()[i] = bass;
        maskDrums->Get()[i] = drums;
        maskOther->Get()[i] = other;
    }
}

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObj::SetPlugSampleRate(BL_FLOAT sampleRate)
{
    mPlugSampleRate = sampleRate;
    
    InitResamplers();
}
#endif

#if USE_SOFT_MASK_N
void
RebalanceProcessFftObj::SetUseSoftMasks(bool flag)
{
    mMaskPred->SetUseSoftMasks(flag);
}
#endif


void
RebalanceProcessFftObj::ComputeMixSoft(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                                       const WDL_TypedBuf<BL_FLOAT> &magnsMix)
{
    BLUtils::ResizeFillZeros(resultMagns, magnsMix.GetSize());
    
    if (mMaskPred == NULL)
        return;
    
    WDL_TypedBuf<BL_FLOAT> maskVocal;
    WDL_TypedBuf<BL_FLOAT> maskBass;
    WDL_TypedBuf<BL_FLOAT> maskDrums;
    WDL_TypedBuf<BL_FLOAT> maskOther;
    
    mMaskPred->GetMaskVocal(&maskVocal);
    mMaskPred->GetMaskBass(&maskBass);
    mMaskPred->GetMaskDrums(&maskDrums);
    mMaskPred->GetMaskOther(&maskOther);
    
    // NEW
    // NOTE: previously, when setting Other sensitivity to 0,
    // there was no change (in soft mode only), than with sensitivity set
    // to 100
    ApplySensitivity(&maskVocal, &maskBass, &maskDrums, &maskOther);
    
    NormalizeMasks(&maskVocal, &maskBass, &maskDrums, &maskOther);
    
    if (mDbgSpectrogramDump)
    {
        mDbgMixSpectrogram->AddLine(magnsMix);
        
        mDbgSourceSpectrograms[0]->AddLine(maskVocal);
        mDbgSourceSpectrograms[1]->AddLine(maskBass);
        mDbgSourceSpectrograms[2]->AddLine(maskDrums);
        mDbgSourceSpectrograms[3]->AddLine(maskOther);
        
        static int count = 0;
        if (count++ == 600)
            DBG_SaveSpectrograms();
    }
    
    // ORIGIN
    //ApplySensitivity(&maskVocal, &maskBass, &maskDrums, &maskOther);
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        BL_FLOAT maskVocal0 = maskVocal.Get()[i];
        BL_FLOAT maskBass0 = maskBass.Get()[i];
        BL_FLOAT maskDrums0 = maskDrums.Get()[i];
        BL_FLOAT maskOther0 = maskOther.Get()[i];
        
        ApplySensitivity(&maskVocal0, &maskBass0,
                         &maskDrums0, &maskOther0);
        
        maskVocal.Get()[i] = maskVocal0;
        maskBass.Get()[i] = maskBass0;
        maskDrums.Get()[i] = maskDrums0;
        maskOther.Get()[i] = maskOther0;
    }
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        BL_FLOAT maskVocal0 = maskVocal.Get()[i];
        BL_FLOAT maskBass0 = maskBass.Get()[i];
        BL_FLOAT maskDrums0 = maskDrums.Get()[i];
        BL_FLOAT maskOther0 = maskOther.Get()[i];
        
#if OTHER_IS_REST
        maskOther0 = 1.0 - (maskVocal0 + maskBass0 + maskDrums0);
#endif
        
        BL_FLOAT vocalCoeff = maskVocal0*mVocal;
        BL_FLOAT bassCoeff = maskBass0*mBass;
        BL_FLOAT drumsCoeff = maskDrums0*mDrums;
        BL_FLOAT otherCoeff = maskOther0*mOther;
        
        BL_FLOAT coeff = vocalCoeff + bassCoeff + drumsCoeff + otherCoeff;
        
        BL_FLOAT val = magnsMix.Get()[i];
        
        resultMagns->Get()[i] = val * coeff;
    }
}

void
RebalanceProcessFftObj::ComputeMixHard(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                                       const WDL_TypedBuf<BL_FLOAT> &magnsMix)
{
    BLUtils::ResizeFillZeros(resultMagns, magnsMix.GetSize());
    
    if (mMaskPred == NULL)
        return;
    
    WDL_TypedBuf<BL_FLOAT> maskVocal;
    WDL_TypedBuf<BL_FLOAT> maskBass;
    WDL_TypedBuf<BL_FLOAT> maskDrums;
    WDL_TypedBuf<BL_FLOAT> maskOther;
    
    mMaskPred->GetMaskVocal(&maskVocal);
    mMaskPred->GetMaskBass(&maskBass);
    mMaskPred->GetMaskDrums(&maskDrums);
    mMaskPred->GetMaskOther(&maskOther);
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        BL_FLOAT maskVocal0 = maskVocal.Get()[i];
        BL_FLOAT maskBass0 = maskBass.Get()[i];
        BL_FLOAT maskDrums0 = maskDrums.Get()[i];
        BL_FLOAT maskOther0 = maskOther.Get()[i];
        
        // Theshold, just in case (prediction can return negative mask values)
        if (maskVocal0 < 0.0)
            maskVocal0 = 0.0;
        if (maskBass0 < 0.0)
            maskBass0 = 0.0;
        if (maskDrums0 < 0.0)
            maskDrums0 = 0.0;
        if (maskOther0 < 0.0)
            maskOther0 = 0.0;
        
        ApplySensitivity(&maskVocal0, &maskBass0,
                         &maskDrums0, &maskOther0);
        
#if OTHER_IS_REST
        // Normalize
        BL_FLOAT sum = maskVocal0 + maskBass0 + maskDrums0 + maskOther0;
        if (std::fabs(sum) > EPS)
        {
            maskVocal0 /= sum;
            maskBass0 /= sum;
            maskDrums0 /= sum;
            maskOther0 /= sum;
        }
        
        maskOther0 = 1.0 - (maskVocal0 + maskBass0 + maskDrums0);
#endif
        
        // Compute max coeff
        BL_FLOAT maxMask = maskVocal0;
        BL_FLOAT coeffMax = mVocal;
        
        //
        if (maskVocal0 < EPS)
            coeffMax = 0.0;
        
        if (maskBass0 > maxMask)
        {
            maxMask = maskBass0;
            coeffMax = mBass;
        }
        
        if (maskDrums0 > maxMask)
        {
            maxMask = maskDrums0;
            coeffMax = mDrums;
        }
        
        if (maskOther0 > maxMask)
        {
            maxMask = maskOther0;
            coeffMax = mOther;
        }
        
        BL_FLOAT val = magnsMix.Get()[i];
        
        resultMagns->Get()[i] = val * coeffMax;
    }
}

void
RebalanceProcessFftObj::ApplySensitivity(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                                         WDL_TypedBuf<BL_FLOAT> *maskBass,
                                         WDL_TypedBuf<BL_FLOAT> *maskDrums,
                                         WDL_TypedBuf<BL_FLOAT> *maskOther)
{
    for (int i = 0; i < maskVocal->GetSize(); i++)
    {
        BL_FLOAT maskVocal0 = maskVocal->Get()[i];
        BL_FLOAT maskBass0 = maskBass->Get()[i];
        BL_FLOAT maskDrums0 = maskDrums->Get()[i];
        BL_FLOAT maskOther0 = maskOther->Get()[i];
        
        ApplySensitivity(&maskVocal0, &maskBass0,
                         &maskDrums0, &maskOther0);
        
        maskVocal->Get()[i] = maskVocal0;
        maskBass->Get()[i] = maskBass0;
        maskDrums->Get()[i] = maskDrums0;
        maskOther->Get()[i] = maskOther0;
    }
}

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObj::InitResamplers()
{
    mResamplerOut.Reset(); //
    
    // Out
    mResamplerOut.SetMode(true, 1, false, 0, 0);
    mResamplerOut.SetFilterParms();
    
    // NOTE: tested output driven: not better
    mResamplerOut.SetFeedMode(true); // Input driven (GOOD)
    
    // set input and output samplerates
    mResamplerOut.SetRates(SAMPLE_RATE, mPlugSampleRate);
}
#endif
