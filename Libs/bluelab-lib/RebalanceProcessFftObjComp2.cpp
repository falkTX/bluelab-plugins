//
//  RebalanceProcessFftObjComp2.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

//#include <RebalanceMaskPredictorComp4.h>
#include <RebalanceMaskPredictorComp5.h>

#include <DbgSpectrogram.h>

#include <BLUtils.h>
#include <BLUtilsFft.h>
#include <BLUtilsComp.h>

#include <SoftMaskingNComp.h>

#include "RebalanceProcessFftObjComp2.h"

// Post normalize, so that when everything is set to default, the plugin is transparent
#define POST_NORMALIZE 1

RebalanceProcessFftObjComp2::RebalanceProcessFftObjComp2(int bufferSize,
                                                         MASK_PREDICTOR_CLASS *maskPred)
: ProcessObj(bufferSize)
{
    mMaskPred = maskPred;
    
    //mMode = RebalanceMode::SOFT;
    
#if FORCE_SAMPLE_RATE
    mSampleRate = SAMPLE_RATE;
    mPlugSampleRate = SAMPLE_RATE;
    
    InitResamplers();
    
    mRemainingSamples = 0.0;
#endif
    
    ResetSamplesHistory();
    
    // Soft masks
    mSoftMasking = new SoftMaskingNComp(SOFT_MASK_HISTO_SIZE);
    mUseSoftMasks = true;
    
    ResetMixColsComp();
    
    // Mix parameters
    for (int i = 0; i < 4; i++)
        mMixes[i] = 0.0;
}

RebalanceProcessFftObjComp2::~RebalanceProcessFftObjComp2()
{
    delete mSoftMasking;
}

void
RebalanceProcessFftObjComp2::Reset(int bufferSize, int oversampling,
                                   int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
#if FORCE_SAMPLE_RATE
    mPlugSampleRate = sampleRate;
    
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
    
    mSoftMasking->Reset();
    
    // NEW
    mMaskPred->Reset();
    
    // NEW
    ResetSamplesHistory();
    ResetMixColsComp();
}

void
RebalanceProcessFftObjComp2::Reset()
{
    //ProcessObj::Reset();
    
#if FORCE_SAMPLE_RATE
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
    
    mSoftMasking->Reset();
    
    // NEW
    mMaskPred->Reset();
    
    // NEW
    ResetSamplesHistory();
    ResetMixColsComp();
}

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObjComp2::ResetResamplers()
{
    mResamplerIn.Reset();
    mResamplerIn.SetRates(mPlugSampleRate, SAMPLE_RATE);
    
    mResamplerOut.Reset();
    mResamplerOut.SetRates(SAMPLE_RATE, mPlugSampleRate);
}
#endif

void
RebalanceProcessFftObjComp2::SetVocal(BL_FLOAT vocal)
{
    mMixes[0] = vocal;
}

void
RebalanceProcessFftObjComp2::SetBass(BL_FLOAT bass)
{
    mMixes[1] = bass;
}

void
RebalanceProcessFftObjComp2::SetDrums(BL_FLOAT drums)
{
    mMixes[2] = drums;
}

void
RebalanceProcessFftObjComp2::SetOther(BL_FLOAT other)
{
    mMixes[3] = other;
}

void
RebalanceProcessFftObjComp2::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
    mMaskPred->SetVocalSensitivity(vocalSensitivity);
}

void
RebalanceProcessFftObjComp2::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
    mMaskPred->SetBassSensitivity(bassSensitivity);
}

void
RebalanceProcessFftObjComp2::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
    mMaskPred->SetDrumsSensitivity(drumsSensitivity);
}

void
RebalanceProcessFftObjComp2::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
    mMaskPred->SetOtherSensitivity(otherSensitivity);
}

void
RebalanceProcessFftObjComp2::SetMasksContrast(BL_FLOAT contrast)
{
    mMaskPred->SetMasksContrast(contrast);
}

void
RebalanceProcessFftObjComp2::SetMode(RebalanceMode mode)
{
    mMode = mode;
}

void
RebalanceProcessFftObjComp2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                             const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    // For soft masks
    // mMixCols is filled with zeros at the origin
    mMixColsComp.push_back(mixBuffer);
    mMixColsComp.pop_front();
    
    // History, to stay synchronized between input signal and masks
    mSamplesHistory.push_back(mixBuffer);
    mSamplesHistory.pop_front();
    
    int histoIndex = mMaskPred->GetHistoryIndex();
    if (histoIndex < mSamplesHistory.size())
        mixBuffer = mSamplesHistory[histoIndex];
    
    // Interpolate between soft and hard
    // TODO: smooth gamma between soft and hard
    WDL_TypedBuf<WDL_FFT_COMPLEX> dataSoft;
    ComputeMix(&dataSoft, mixBuffer);
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = dataSoft;
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    
    BLUtilsFft::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}

// GOOD
#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObjComp2::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
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

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObjComp2::SetPlugSampleRate(BL_FLOAT sampleRate)
{
    mPlugSampleRate = sampleRate;
    
    InitResamplers();
}
#endif

void
RebalanceProcessFftObjComp2::SetUseSoftMasks(bool flag)
{
    mUseSoftMasks = flag;
    
    Reset();
}

// Previously named ComputeMixSoft()
void
RebalanceProcessFftObjComp2::ComputeMix(WDL_TypedBuf<WDL_FFT_COMPLEX> *dataResult,
                                        const WDL_TypedBuf<WDL_FFT_COMPLEX> &dataMix)
{
    BLUtils::ResizeFillZeros(dataResult, dataMix.GetSize());
    
    if (mMaskPred == NULL)
        return;
    
    WDL_TypedBuf<BL_FLOAT> masks0[4];
    for (int i = 0; i < 4; i++)
        mMaskPred->GetMask(i, &masks0[i]);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> masks[4];
    ApplySoftMasks(masks, masks0);
    
#if POST_NORMALIZE
    NormalizeMasks(masks);
#endif
    
    // Must apply mix after soft masks,
    // because if mix param is > 1,
    // soft masks will not manage well if mask is > 1
    // (mask will not have the same spectrogram "shape" at the end when > 1)
    //
    ApplyMix(masks);
    
    for (int i = 0; i < dataResult->GetSize(); i++)
    {
        // Mask values
        WDL_FFT_COMPLEX coeffs[4];
        for (int j = 0; j < 4; j++)
            coeffs[j] = masks[j].Get()[i];
        
        // NOTE: no need to convert this line to complex
        // (don't know how to do this, and OTHER_IS_REST is 0!)
#if OTHER_IS_REST
        coeffs[3] = 1.0 - (coeffs[0] + coeffs[1] + coeffs[2]);
#endif
        
        // Final coeff
        WDL_FFT_COMPLEX coeff;
        coeff.re = coeffs[0].re + coeffs[1].re + coeffs[2].re + coeffs[3].re;
        coeff.im = coeffs[0].im + coeffs[1].im + coeffs[2].im + coeffs[3].im;
        
        WDL_FFT_COMPLEX val = dataMix.Get()[i];
        
        // Result
        WDL_FFT_COMPLEX res;
        COMP_MULT(val, coeff, res);
        dataResult->Get()[i] = res;
    }
}

#if FORCE_SAMPLE_RATE
void
RebalanceProcessFftObjComp2::InitResamplers()
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

void
RebalanceProcessFftObjComp2::ApplySoftMasks(WDL_TypedBuf<WDL_FFT_COMPLEX> masksResult[4],
                                            const WDL_TypedBuf<BL_FLOAT> masksSource[4])
{
    if (masksSource[0].GetSize() == 0)
        return;
    
    //
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > softMasks;
    if (mUseSoftMasks)
    {
        // Use history and soft masking
        int histoIndex = mMaskPred->GetHistoryIndex();
        if (histoIndex >= mMixColsComp.size())
            return;
        WDL_TypedBuf<WDL_FFT_COMPLEX> mix = mMixColsComp[histoIndex];
        
        vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > estimData;
        estimData.push_back(mix);
        estimData.push_back(mix);
        estimData.push_back(mix);
        estimData.push_back(mix);
        
        for (int i = 0; i < 4; i++)
            BLUtils::MultValues(&estimData[i], masksSource[i]);
        
        mSoftMasking->Process(mix, estimData, &softMasks);
    }
    else
    {
        // Simply convert masks to complex
        softMasks.resize(4);
        for (int i = 0; i < 4; i++)
            softMasks[i].Resize(masksSource[i].GetSize());
        
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < softMasks[i].GetSize(); j++)
            {
                softMasks[i].Get()[j].re = masksSource[i].Get()[j];
                softMasks[i].Get()[j].im = 0.0;
            }
        }
    }
    
    for (int i = 0; i < 4; i++)
        masksResult[i] = softMasks[i];
}

void
RebalanceProcessFftObjComp2::CompDiv(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *estim,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> &mix)
{
    for (int k = 0; k < 4; k++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> &est = (*estim)[k];
        for (int i = 0; i < est.GetSize(); i++)
        {
            WDL_FFT_COMPLEX est0 = est.Get()[i];
            WDL_FFT_COMPLEX mix0 = mix.Get()[i];
            
            WDL_FFT_COMPLEX res;
            COMP_DIV(est0, mix0, res);
            
            est.Get()[i] = res;
        }
    }
}

void
RebalanceProcessFftObjComp2::ResetSamplesHistory()
{
    mSamplesHistory.clear();
    
    for (int i = 0; i < REBALANCE_NUM_SPECTRO_COLS; i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> samples;
        BLUtils::ResizeFillZeros(&samples, mBufferSize/2);
        
        mSamplesHistory.push_back(samples);
    }
}

void
RebalanceProcessFftObjComp2::ResetMixColsComp()
{
    mMixColsComp.clear();
    
    for (int i = 0; i < REBALANCE_NUM_SPECTRO_COLS; i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> col;
        BLUtils::ResizeFillZeros(&col, mBufferSize/2);
        
        mMixColsComp.push_back(col);
    }
}

void
RebalanceProcessFftObjComp2::ApplyMix(WDL_FFT_COMPLEX masks[4])
{
    // Apply mix
    for (int j = 0; j < 4; j++)
    {
        masks[j].re *= mMixes[j];
        masks[j].im *= mMixes[j];
    }
}

void
RebalanceProcessFftObjComp2::ApplyMix(WDL_TypedBuf<WDL_FFT_COMPLEX> masks[4])
{
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        WDL_FFT_COMPLEX vals[4];
        for (int j = 0; j < 4; j++)
            vals[j] = masks[j].Get()[i];
        
        ApplyMix(vals);
        
        for (int j = 0; j < 4; j++)
            masks[j].Get()[i] = vals[j];
    }
}

void
RebalanceProcessFftObjComp2::NormalizeMasks(WDL_TypedBuf<WDL_FFT_COMPLEX> masks[4])
{
    WDL_FFT_COMPLEX vals[4];
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        for (int k = 0; k < 4; k++)
        {
            vals[k] = masks[k].Get()[i];
        }
        
        NormalizeMaskVals(vals);
        
        for (int k = 0; k < 4; k++)
        {
            masks[k].Get()[i] = vals[k];
        }
    }
}

void
RebalanceProcessFftObjComp2::NormalizeMaskVals(WDL_FFT_COMPLEX maskVals[4])
{
#define EPS 1e-15
    
    // Compute sum magns
    BL_FLOAT sumMagns = 0.0;
    for (int k = 0; k < 4; k++)
    {
        const WDL_FFT_COMPLEX &val = maskVals[k];
        BL_FLOAT magn = COMP_MAGN(val);
        
        sumMagns += magn;
    }
    
    if (sumMagns < EPS)
        return;
    
    BL_FLOAT invSum = 1.0/sumMagns;
    
    for (int k = 0; k < 4; k++)
    {
        WDL_FFT_COMPLEX val = maskVals[k];
        
        val.re *= invSum;
        val.im *= invSum;
        
        maskVals[k] = val;
    }
}
