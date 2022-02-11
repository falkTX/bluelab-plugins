//
//  RebalanceProcessFftObjComp.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#include <RebalanceMaskPredictorComp2.h>

#include <DbgSpectrogram.h>

#include <BLUtils.h>

#include "RebalanceProcessFftObjComp.h"

// See Rebalance
#define EPS 1e-10

// Debug
#define DBG_ONE_MASK 0 //1

RebalanceProcessFftObjComp::RebalanceProcessFftObjComp(int bufferSize,
                                                       RebalanceMaskPredictorComp2 *maskPred)
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

RebalanceProcessFftObjComp::~RebalanceProcessFftObjComp()
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
RebalanceProcessFftObjComp::Reset(int bufferSize, int oversampling,
                                  int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
#if FORCE_SAMPLE_RATE
    mPlugSampleRate = sampleRate;
    
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
}

void
RebalanceProcessFftObjComp::Reset()
{
    ProcessObj::Reset();
    
#if FORCE_SAMPLE_RATE
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
}

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObjComp::ResetResamplers()
{
    mResamplerIn.Reset();
    mResamplerIn.SetRates(mPlugSampleRate, SAMPLE_RATE);
    
    mResamplerOut.Reset();
    mResamplerOut.SetRates(SAMPLE_RATE, mPlugSampleRate);
}
#endif

void
RebalanceProcessFftObjComp::SetVocal(BL_FLOAT vocal)
{
    mVocal = vocal;
}

void
RebalanceProcessFftObjComp::SetBass(BL_FLOAT bass)
{
    mBass = bass;
}

void
RebalanceProcessFftObjComp::SetDrums(BL_FLOAT drums)
{
    mDrums = drums;
}

void
RebalanceProcessFftObjComp::SetOther(BL_FLOAT other)
{
    mOther = other;
}

void
RebalanceProcessFftObjComp::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
    mVocalSensitivity = vocalSensitivity;
}

void
RebalanceProcessFftObjComp::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
    mBassSensitivity = bassSensitivity;
}

void
RebalanceProcessFftObjComp::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
    mDrumsSensitivity = drumsSensitivity;
}

void
RebalanceProcessFftObjComp::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
    mOtherSensitivity = otherSensitivity;
}

void
RebalanceProcessFftObjComp::SetPrecision(BL_FLOAT precision)
{
    mPrecision = precision;
}

void
RebalanceProcessFftObjComp::SetMode(Rebalance::Mode mode)
{
    mMode = mode;
}

void
RebalanceProcessFftObjComp::ApplySensitivity(WDL_FFT_COMPLEX *maskVocal, WDL_FFT_COMPLEX *maskBass,
                                             WDL_FFT_COMPLEX *maskDrums, WDL_FFT_COMPLEX *maskOther)
{
    if (COMP_MAGN(*maskVocal) < mVocalSensitivity)
    {
        maskVocal->re = 0.0;
        maskVocal->im = 0.0;
    }
    
    if (COMP_MAGN(*maskBass) < mBassSensitivity)
    {
        maskBass->re = 0.0;
        maskBass->im = 0.0;
    }
    
    if (COMP_MAGN(*maskDrums) < mDrumsSensitivity)
    {
        maskDrums->re = 0.0;
        maskDrums->im = 0.0;
    }
    
    if (COMP_MAGN(*maskOther) < mOtherSensitivity)
    {
        maskOther->re = 0.0;
        maskOther->im = 0.0;
    }
}

void
RebalanceProcessFftObjComp::DBG_SetSpectrogramDump(bool flag)
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
RebalanceProcessFftObjComp::DBG_SaveSpectrograms()
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
RebalanceProcessFftObjComp::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                             const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
#if DEBUG_LISTEN_BUFFER
    DBG_ListenBuffer(ioBuffer, scBuffer);
    
    return;
#endif
    
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    // Prev code: binary choice: soft or hard
#if 0
    if (mMode == Rebalance::SOFT)
        ComputeMixSoft(&resultMagns, magnsMix);
    else
        ComputeMixHard(&resultMagns, magnsMix);
#endif
    
    
#if 1
    // New code: interpolate between soft and hard
    WDL_TypedBuf<WDL_FFT_COMPLEX> dataSoft;
    ComputeMixSoft(&dataSoft, mixBuffer);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> dataHard;
    ComputeMixHard(&dataHard, mixBuffer);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> resultData;
    BLUtils::Interp(&resultData, &dataSoft, &dataHard, mPrecision);
#endif
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = resultData;
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}

// GOOD
#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObjComp::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
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
RebalanceProcessFftObjComp::DBG_ListenBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                             const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magnsMix;
    WDL_TypedBuf<BL_FLOAT> phasesMix;
    BLUtils::ComplexToMagnPhase(&magnsMix, &phasesMix, mixBuffer);
    
    RebalanceMaskPredictorComp2::Downsample(&magnsMix, mSampleRate);
    RebalanceMaskPredictorComp2::Upsample(&magnsMix, mSampleRate);
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    BLUtils::MagnPhaseToComplex(&fftSamples, magnsMix, phasesMix);
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}

void
RebalanceProcessFftObjComp::NormalizeMasks(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                                           WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                                           WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                                           WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther)
{
    for (int i = 0; i < maskVocal->GetSize(); i++)
    {
        WDL_FFT_COMPLEX vocal = maskVocal->Get()[i];
        WDL_FFT_COMPLEX bass = maskBass->Get()[i];
        WDL_FFT_COMPLEX drums = maskDrums->Get()[i];
        WDL_FFT_COMPLEX other = maskOther->Get()[i];
        
        WDL_FFT_COMPLEX sum;
        sum.re = vocal.re + bass.re + drums.re + other.re;
        sum.im = vocal.im + bass.im + drums.im + other.im;
        
        if (COMP_MAGN(sum) > EPS)
        {
            WDL_FFT_COMPLEX vocal2;
            WDL_FFT_COMPLEX bass2;
            WDL_FFT_COMPLEX drums2;
            WDL_FFT_COMPLEX other2;
            
            COMP_DIV(vocal, sum, vocal2);
            COMP_DIV(bass, sum, bass2);
            COMP_DIV(drums, sum, drums2);
            COMP_DIV(other, sum, other2);
            
            maskVocal->Get()[i] = vocal2;
            maskBass->Get()[i] = bass2;
            maskDrums->Get()[i] = drums2;
            maskOther->Get()[i] = other2;
        }
    }
}

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObjComp::SetPlugSampleRate(BL_FLOAT sampleRate)
{
    mPlugSampleRate = sampleRate;
    
    InitResamplers();
}
#endif

#if USE_SOFT_MASK_N
void
RebalanceProcessFftObjComp::SetUseSoftMasks(bool flag)
{
    mMaskPred->SetUseSoftMasks(flag);
}
#endif


void
RebalanceProcessFftObjComp::ComputeMixSoft(WDL_TypedBuf<WDL_FFT_COMPLEX> *dataResult,
                                           const WDL_TypedBuf<WDL_FFT_COMPLEX> &dataMix)
{
    BLUtils::ResizeFillZeros(dataResult, dataMix.GetSize());
    
    if (mMaskPred == NULL)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskVocal;
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskBass;
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskDrums;
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskOther;
    
    mMaskPred->GetMaskVocal(&maskVocal);
    mMaskPred->GetMaskBass(&maskBass);
    mMaskPred->GetMaskDrums(&maskDrums);
    mMaskPred->GetMaskOther(&maskOther);
    
    // DEBUG
#if 0
    for (int i = 0; i < dataResult->GetSize(); i++)
    {
        WDL_FFT_COMPLEX coeff = maskVocal.Get()[i];
        
        WDL_FFT_COMPLEX val = dataMix.Get()[i];
        
        WDL_FFT_COMPLEX res;
        COMP_MULT(val, coeff, res);
        dataResult->Get()[i] = res;
    }
    return;
#endif
    
#if !DBG_ONE_MASK
    // NEW
    // NOTE: previously, when setting Other sensitivity to 0,
    // there was no change (in soft mode only), than with sensitivity set
    // to 100
    ApplySensitivity(&maskVocal, &maskBass, &maskDrums, &maskOther);
    
    NormalizeMasks(&maskVocal, &maskBass, &maskDrums, &maskOther);
    
#if 0
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
#endif
    
    // ORIGIN
    //ApplySensitivity(&maskVocal, &maskBass, &maskDrums, &maskOther);
    
    for (int i = 0; i < dataResult->GetSize(); i++)
    {
        WDL_FFT_COMPLEX maskVocal0 = maskVocal.Get()[i];
        WDL_FFT_COMPLEX maskBass0 = maskBass.Get()[i];
        WDL_FFT_COMPLEX maskDrums0 = maskDrums.Get()[i];
        WDL_FFT_COMPLEX maskOther0 = maskOther.Get()[i];
        
        ApplySensitivity(&maskVocal0, &maskBass0,
                         &maskDrums0, &maskOther0);
        
        maskVocal.Get()[i] = maskVocal0;
        maskBass.Get()[i] = maskBass0;
        maskDrums.Get()[i] = maskDrums0;
        maskOther.Get()[i] = maskOther0;
    }
#endif
    
    for (int i = 0; i < dataResult->GetSize(); i++)
    {
        WDL_FFT_COMPLEX maskVocal0 = maskVocal.Get()[i];
        WDL_FFT_COMPLEX maskBass0 = maskBass.Get()[i];
        WDL_FFT_COMPLEX maskDrums0 = maskDrums.Get()[i];
        WDL_FFT_COMPLEX maskOther0 = maskOther.Get()[i];
        
        // NOTE: no need to convert this line to complex
        // (don't know how to do this, and OTHER_IS_REST is 0!)
#if OTHER_IS_REST
        maskOther0 = 1.0 - (maskVocal0 + maskBass0 + maskDrums0);
#endif
        
        WDL_FFT_COMPLEX vocalCoeff = maskVocal0;
        vocalCoeff.re *= mVocal;
        vocalCoeff.im *= mVocal;
        
        WDL_FFT_COMPLEX bassCoeff = maskBass0;
        bassCoeff.re *= mBass;
        bassCoeff.im *= mBass;
        
        WDL_FFT_COMPLEX drumsCoeff = maskDrums0;
        drumsCoeff.re *= mDrums;
        drumsCoeff.im *= mDrums;
        
        WDL_FFT_COMPLEX otherCoeff = maskOther0;
        otherCoeff.re *= mOther;
        otherCoeff.im *= mOther;
        
        WDL_FFT_COMPLEX coeff;
        coeff.re = vocalCoeff.re + bassCoeff.re + drumsCoeff.re + otherCoeff.re;
        coeff.im = vocalCoeff.im + bassCoeff.im + drumsCoeff.im + otherCoeff.im;
        
        WDL_FFT_COMPLEX val = dataMix.Get()[i];
        
        WDL_FFT_COMPLEX res;
        COMP_MULT(val, coeff, res);
        dataResult->Get()[i] = res;
    }
}

void
RebalanceProcessFftObjComp::ComputeMixHard(WDL_TypedBuf<WDL_FFT_COMPLEX> *dataResult,
                                           const WDL_TypedBuf<WDL_FFT_COMPLEX> &dataMix)
{
    BLUtils::ResizeFillZeros(dataResult, dataMix.GetSize());
    
    if (mMaskPred == NULL)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskVocal;
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskBass;
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskDrums;
    WDL_TypedBuf<WDL_FFT_COMPLEX> maskOther;
    
    mMaskPred->GetMaskVocal(&maskVocal);
    mMaskPred->GetMaskBass(&maskBass);
    mMaskPred->GetMaskDrums(&maskDrums);
    mMaskPred->GetMaskOther(&maskOther);
    
    for (int i = 0; i < dataResult->GetSize(); i++)
    {
        WDL_FFT_COMPLEX maskVocal0 = maskVocal.Get()[i];
        WDL_FFT_COMPLEX maskBass0 = maskBass.Get()[i];
        WDL_FFT_COMPLEX maskDrums0 = maskDrums.Get()[i];
        WDL_FFT_COMPLEX maskOther0 = maskOther.Get()[i];
        
        ApplySensitivity(&maskVocal0, &maskBass0,
                         &maskDrums0, &maskOther0);
        
        // NOTE: no need to convert this line to complex
        // (don't know how to do this, and OTHER_IS_REST is 0!)
#if OTHER_IS_REST
        // Normalize
        WDL_FFT_COMPLEX sum;
        sum.re = maskVocal0.re + maskBass0.re + maskDrums0.re + maskOther0.re;
        sum.im = maskVocal0.im + maskBass0.im + maskDrums0.im + maskOther0.im;
        
        if (COMP_MAGN(sum) > EPS)
        {
            WDL_FFT_COMPLEX maskVocal1;
            COMP_DIV(maskVocal0, sum, maskVocal1);
            maskVocal0 = maskVocal1;
            
            WDL_FFT_COMPLEX maskBass1;
            COMP_DIV(maskBass0, sum, maskBass1);
            maskBass0 = maskBass1;
            
            WDL_FFT_COMPLEX maskDrums1;
            COMP_DIV(maskDrums0, sum, maskDrums1);
            maskDrums0 = maskDrums1;
            
            WDL_FFT_COMPLEX maskOther1;
            COMP_DIV(maskOther0, sum, maskOther1);
            maskOther0 = maskOther1;
        }
        
        // NOTE: don't know ho to convert this line to complex
        maskOther0 = 1.0 - (maskVocal0 + maskBass0 + maskDrums0);
#endif
        
        // Compute max coeff
        WDL_FFT_COMPLEX maxMask = maskVocal0;
        BL_FLOAT coeffMax = mVocal;
        
        //
        if (COMP_MAGN(maskVocal0) < EPS)
            coeffMax = 0.0;
        
        if (COMP_MAGN(maskBass0) > COMP_MAGN(maxMask))
        {
            maxMask = maskBass0;
            coeffMax = mBass;
        }
        
        if (COMP_MAGN(maskDrums0) > COMP_MAGN(maxMask))
        {
            maxMask = maskDrums0;
            coeffMax = mDrums;
        }
        
        if (COMP_MAGN(maskOther0) > COMP_MAGN(maxMask))
        {
            maxMask = maskOther0;
            coeffMax = mOther;
        }
        
        WDL_FFT_COMPLEX val = dataMix.Get()[i];
        
        WDL_FFT_COMPLEX res = val;
        res.re *= coeffMax;
        res.im *= coeffMax;
        
        dataResult->Get()[i] = res;
    }
}

void
RebalanceProcessFftObjComp::ApplySensitivity(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther)
{
    for (int i = 0; i < maskVocal->GetSize(); i++)
    {
        WDL_FFT_COMPLEX maskVocal0 = maskVocal->Get()[i];
        WDL_FFT_COMPLEX maskBass0 = maskBass->Get()[i];
        WDL_FFT_COMPLEX maskDrums0 = maskDrums->Get()[i];
        WDL_FFT_COMPLEX maskOther0 = maskOther->Get()[i];
        
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
RebalanceProcessFftObjComp::InitResamplers()
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
