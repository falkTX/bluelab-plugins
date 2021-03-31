//
//  RebalanceProcessFftObjComp4.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#include <RebalanceMaskPredictor8.h>
#include <RebalanceMaskProcessor.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>
#include <Scale.h>

#include <SoftMaskingComp4.h>

#include <BLSpectrogram4.h>
#include <SpectrogramDisplayScroll3.h>

#include <Rebalance_defs.h>

#include "RebalanceProcessFftObjComp4.h"

// Post normalize, so that when everything is set to default, the plugin is transparent
#define POST_NORMALIZE 1

#define SPECTRO_NUM_COLS 2048/4 //64

#define SOFT_MASKING_HISTO_SIZE 8

RebalanceProcessFftObjComp4::
RebalanceProcessFftObjComp4(int bufferSize, int oversampling,
                            BL_FLOAT sampleRate,
                            RebalanceMaskPredictor8 *maskPred,
                            int numInputCols,
                            int softMaskHistoSize)
: ProcessObj(bufferSize)
{
    mSampleRate = sampleRate;
    
    mMaskPred = maskPred;
    
    mNumInputCols = numInputCols;

    mSpectrogram = new BLSpectrogram4(sampleRate,
                                      SPECTRO_NUM_COLS/*bufferSize/4*/, -1);
    mSpectroDisplay = NULL;
    
    mScale = new Scale();
    
    ResetSamplesHistory();
    
    // Soft masks
    mSoftMasking = new SoftMaskingComp4(bufferSize, oversampling,
                                        SOFT_MASKING_HISTO_SIZE);

    mMaskProcessor = new RebalanceMaskProcessor();
    
    ResetMixColsComp();
}

RebalanceProcessFftObjComp4::~RebalanceProcessFftObjComp4()
{
    if (mSoftMasking != NULL)
        delete mSoftMasking;

    delete mMaskProcessor;
    
    delete mScale;
    delete mSpectrogram;
}

void
RebalanceProcessFftObjComp4::Reset(int bufferSize, int oversampling,
                                   int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);

    mSampleRate = sampleRate;
    
    if (mSoftMasking != NULL)
        mSoftMasking->Reset(bufferSize, oversampling);
    
    mMaskPred->Reset();
    
    ResetSamplesHistory();
    ResetMixColsComp();
    
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = mBufferSize/(32/mOverlapping);
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = sampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    mSpectrogram->Reset(sampleRate,
                        SPECTRO_NUM_COLS/*bufferSize/4*/, numCols);
}

void
RebalanceProcessFftObjComp4::Reset()
{    
    mSoftMasking->Reset();
   
    mMaskPred->Reset();
    
    ResetSamplesHistory();
    ResetMixColsComp();
    
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = mBufferSize/(32/mOverlapping);
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = mSampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    mSpectrogram->Reset(mSampleRate,
                        SPECTRO_NUM_COLS/*mBufferSize/4*/, numCols);
}

BLSpectrogram4 *
RebalanceProcessFftObjComp4::GetSpectrogram()
{
    return mSpectrogram;
}

void
RebalanceProcessFftObjComp4::
SetSpectrogramDisplay(SpectrogramDisplayScroll3 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
RebalanceProcessFftObjComp4::SetVocal(BL_FLOAT vocal)
{
    mMaskProcessor->SetVocalMix(vocal);
}

void
RebalanceProcessFftObjComp4::SetBass(BL_FLOAT bass)
{
    mMaskProcessor->SetBassMix(bass);
}

void
RebalanceProcessFftObjComp4::SetDrums(BL_FLOAT drums)
{
    mMaskProcessor->SetDrumsMix(drums);
}

void
RebalanceProcessFftObjComp4::SetOther(BL_FLOAT other)
{
    mMaskProcessor->SetOtherMix(other);
}

void
RebalanceProcessFftObjComp4::SetVocalSensitivity(BL_FLOAT vocal)
{
    mMaskProcessor->SetVocalSensitivity(vocal);
}

void
RebalanceProcessFftObjComp4::SetBassSensitivity(BL_FLOAT bass)
{
    mMaskProcessor->SetBassSensitivity(bass);
}

void
RebalanceProcessFftObjComp4::SetDrumsSensitivity(BL_FLOAT drums)
{
    mMaskProcessor->SetDrumsSensitivity(drums);
}

void
RebalanceProcessFftObjComp4::SetOtherSensitivity(BL_FLOAT other)
{
    mMaskProcessor->SetOtherSensitivity(other);
}

void
RebalanceProcessFftObjComp4::SetContrast(BL_FLOAT contrast)
{
    mMaskProcessor->SetContrast(contrast);
}

void
RebalanceProcessFftObjComp4::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                                const WDL_TypedBuf<BL_FLOAT> &phases)
{
    // Simple add
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
    else
        mSpectrogram->AddLine(magns, phases);
}

void
RebalanceProcessFftObjComp4::
ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
#if PROCESS_SIGNAL_DB
    WDL_TypedBuf<BL_FLOAT> magns0;
    WDL_TypedBuf<BL_FLOAT> phases0;
    
    BLUtilsComp::ComplexToMagnPhase(&magns0, &phases0, mixBuffer);
    
    for (int i = 0; i < magns0.GetSize(); i++)
    {
        BL_FLOAT val = magns0.Get()[i];
        val = mScale->ApplyScale(Scale::DB, val,
                                 (BL_FLOAT)PROCESS_SIGNAL_MIN_DB, (BL_FLOAT)0.0);
        magns0.Get()[i] = val;
    }

    BLUtilsComp::MagnPhaseToComplex(&mixBuffer, magns0, phases0);
#endif
    
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

    WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES];
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        mMaskPred->GetMask(i, &masks[i]);

    // Adjust and apply mask
    WDL_TypedBuf<WDL_FFT_COMPLEX> result;
    BLUtils::ResizeFillZeros(&result, mixBuffer.GetSize());

    // TODO: use tmp buffer
    WDL_TypedBuf<BL_FLOAT> mask;
    mMaskProcessor->Process(masks, &mask);
    
    //ApplyMask(mixBuffer, &result, mask);

    // TODO: use tmp buffers
    result = mixBuffer;
    ApplySoftMasking(&result, mask);
    
#if PROCESS_SIGNAL_DB
    WDL_TypedBuf<BL_FLOAT> magns1;
    WDL_TypedBuf<BL_FLOAT> phases1;
    BLUtilsComp::ComplexToMagnPhase(&magns1, &phases1, result);
    
    for (int i = 0; i < magns1.GetSize(); i++)
    {
        BL_FLOAT val = magns1.Get()[i];
        val = mScale->ApplyScaleInv(Scale::DB, val,
                                    (BL_FLOAT)PROCESS_SIGNAL_MIN_DB, (BL_FLOAT)0.0);
        
        // Noise floor
        BL_FLOAT db = BLUtils::AmpToDB(val);
        if (db < PROCESS_SIGNAL_MIN_DB + 1)
            val = 0.0;
        
        magns1.Get()[i] = val;
    }

    AddSpectrogramLine(magns1, phases1);
    
    BLUtilsComp::MagnPhaseToComplex(&result, magns1, phases1);
#endif

    // TODO: tmp buffers / memory optimization
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = result;
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    
    BLUtilsFft::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}

void
RebalanceProcessFftObjComp4::ResetSamplesHistory()
{
    mSamplesHistory.clear();
    
    for (int i = 0; i < mNumInputCols; i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> samples;
        BLUtils::ResizeFillZeros(&samples, mBufferSize/2);
        
        mSamplesHistory.push_back(samples);
    }
}

void
RebalanceProcessFftObjComp4::
ApplyMask(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inData,
          WDL_TypedBuf<WDL_FFT_COMPLEX> *outData,
          const WDL_TypedBuf<BL_FLOAT> &mask)
{
    // TODO: implement method in BLUtils: multvalues(in, out, mask) in complex
    *outData = inData;
    BLUtils::MultValues(outData, mask);
}

void
RebalanceProcessFftObjComp4::ResetMixColsComp()
{
    mMixColsComp.clear();
    
    for (int i = 0; i < mNumInputCols; i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> col;
        BLUtils::ResizeFillZeros(&col, mBufferSize/2);
        
        mMixColsComp.push_back(col);
    }
}

void
RebalanceProcessFftObjComp4::ApplySoftMasking(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioData,
                                              const WDL_TypedBuf<BL_FLOAT> &mask)
{
    // TODO: use tmp buffersd
    WDL_TypedBuf<WDL_FFT_COMPLEX> softMaskedResult;
    mSoftMasking->ProcessCentered(ioData, mask, &softMaskedResult);
            
    if (mSoftMasking->IsProcessingEnabled())
        *ioData = softMaskedResult;
}
