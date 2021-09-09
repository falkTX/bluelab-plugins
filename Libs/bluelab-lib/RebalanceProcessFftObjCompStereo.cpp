//
//  RebalanceProcessFftObjStereo.cpp
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
#include <SpectrogramDisplayScroll4.h>

#include <Rebalance_defs.h>

#include <StereoWidenProcess.h>

#include "RebalanceProcessFftObjCompStereo.h"

// Post normalize, so that when everything is set to default,
// the plugin is transparent
#define POST_NORMALIZE 1

//#define SPECTRO_NUM_COLS 2048/4 //64
// Reduce a bit, since we have a small graph
#define SPECTRO_HEIGHT 256 //2048/4

#define SOFT_MASKING_HISTO_SIZE 8

#define USE_SOFT_MASKING 1 //0

// Origin: was 0
#define SMOOTH_SPECTRO_DISPLAY 1 // 0

// With this, it will be possible to increase the gain a lot
// when setting mix at 200%
// (Otherwise, the gain effect was very small)
#define SOFT_MASKING_HACK 1

RebalanceProcessFftObjCompStereo::
RebalanceProcessFftObjCompStereo(int bufferSize, int oversampling,
                                 BL_FLOAT sampleRate,
                                 RebalanceMaskPredictor8 *maskPred,
                                 int numInputCols,
                                 int softMaskHistoSize)
{
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mSampleRate = sampleRate;
    
    mMaskPred = maskPred;
    
    mNumInputCols = numInputCols;

    mSpectrogram = new BLSpectrogram4(sampleRate, SPECTRO_HEIGHT, -1);
    
    mSpectroDisplay = NULL;
    
    mScale = new Scale();
    
    ResetSamplesHistory();
    
    // Soft masks
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            mSoftMasking[i][j] = new SoftMaskingComp4(bufferSize, oversampling,
                                                      SOFT_MASKING_HISTO_SIZE);
        }
    }
    
    mMaskProcessor = new RebalanceMaskProcessor();
    
    ResetMixColsComp();

    // Stereo
    //
    mWidthVocal = 0.0;
    mWidthBass = 0.0;
    mWidthDrums = 0.0;
    mWidthOther = 0.0;
    
    mPanVocal = 0.0;
    mPanBass = 0.0;
    mPanDrums = 0.0;
    mPanOther = 0.0;
}

RebalanceProcessFftObjCompStereo::~RebalanceProcessFftObjCompStereo()
{
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (mSoftMasking[i][j] != NULL)
                delete mSoftMasking[i][j];
        }
    }
    
    delete mMaskProcessor;
    
    delete mScale;
    delete mSpectrogram;
}

void
RebalanceProcessFftObjCompStereo::Reset(int bufferSize, int oversampling,
                                        int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mSampleRate = sampleRate;

    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (mSoftMasking[i][j] != NULL)
                mSoftMasking[i][j]->Reset(bufferSize, oversampling);
        }
    }
    
    mMaskPred->Reset();
    
    ResetSamplesHistory();
    ResetMixColsComp();

    ResetMasksHistory();
    ResetSignalHistory();

    ResetRawSignalHistory();
    
    int numCols = ComputeSpectroNumCols();
    mSpectrogram->Reset(sampleRate, SPECTRO_HEIGHT, numCols);
}

void
RebalanceProcessFftObjCompStereo::Reset()
{
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (mSoftMasking[i][j] != NULL)
                mSoftMasking[i][j]->Reset();
        }
    }
    
    mMaskPred->Reset();
    
    ResetSamplesHistory();
    ResetMixColsComp();

    ResetMasksHistory();
    ResetSignalHistory();

    ResetRawSignalHistory();
 
    int numCols = ComputeSpectroNumCols();
    mSpectrogram->Reset(mSampleRate, SPECTRO_HEIGHT, numCols);
}

BLSpectrogram4 *
RebalanceProcessFftObjCompStereo::GetSpectrogram()
{
    return mSpectrogram;
}

void
RebalanceProcessFftObjCompStereo::
SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
RebalanceProcessFftObjCompStereo::SetVocal(BL_FLOAT vocal)
{
    mMaskProcessor->SetVocalMix(vocal);
}

void
RebalanceProcessFftObjCompStereo::SetBass(BL_FLOAT bass)
{
    mMaskProcessor->SetBassMix(bass);
}

void
RebalanceProcessFftObjCompStereo::SetDrums(BL_FLOAT drums)
{
    mMaskProcessor->SetDrumsMix(drums);
}

void
RebalanceProcessFftObjCompStereo::SetOther(BL_FLOAT other)
{
    mMaskProcessor->SetOtherMix(other);
}

void
RebalanceProcessFftObjCompStereo::SetVocalSensitivity(BL_FLOAT vocal)
{
    mMaskProcessor->SetVocalSensitivity(vocal);
}

void
RebalanceProcessFftObjCompStereo::SetBassSensitivity(BL_FLOAT bass)
{
    mMaskProcessor->SetBassSensitivity(bass);
}

void
RebalanceProcessFftObjCompStereo::SetDrumsSensitivity(BL_FLOAT drums)
{
    mMaskProcessor->SetDrumsSensitivity(drums);
}

void
RebalanceProcessFftObjCompStereo::SetOtherSensitivity(BL_FLOAT other)
{
    mMaskProcessor->SetOtherSensitivity(other);
}

void
RebalanceProcessFftObjCompStereo::SetWidthVocal(BL_FLOAT widthVocal)
{
    mWidthVocal = widthVocal;
}

void
RebalanceProcessFftObjCompStereo::SetWidthBass(BL_FLOAT widthBass)
{
    mWidthBass = widthBass;
}

void
RebalanceProcessFftObjCompStereo::SetWidthDrums(BL_FLOAT widthDrums)
{
    mWidthDrums = widthDrums;
}

void
RebalanceProcessFftObjCompStereo::SetWidthOther(BL_FLOAT widthOther)
{
    mWidthOther = widthOther;
}


void
RebalanceProcessFftObjCompStereo::SetPanVocal(BL_FLOAT panVocal)
{
    mPanVocal = panVocal;
}

void
RebalanceProcessFftObjCompStereo::SetPanBass(BL_FLOAT panBass)
{
    mPanBass = panBass;
}

void
RebalanceProcessFftObjCompStereo::SetPanDrums(BL_FLOAT panDrums)
{
    mPanDrums = panDrums;
}

void
RebalanceProcessFftObjCompStereo::SetPanOther(BL_FLOAT panOther)
{
    mPanOther = panOther;
}

void
RebalanceProcessFftObjCompStereo::SetContrast(BL_FLOAT contrast)
{
    mMaskProcessor->SetContrast(contrast);
}

int
RebalanceProcessFftObjCompStereo::GetLatency()
{
#if !USE_SOFT_MASKING
    return 0;
#endif
    
    if (mSoftMasking[0][0] == NULL)
        return 0;

    int latency = mSoftMasking[0][0]->GetLatency();

    return latency;
}
    
void
RebalanceProcessFftObjCompStereo::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                                const WDL_TypedBuf<BL_FLOAT> &phases)
{
    // When disabled: the spectrogram display jitters less
    // even whn much resource is consumed
    // And also for updating whole spectrogram when param change
#if SMOOTH_SPECTRO_DISPLAY
    // Add for smooth scroll
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
#else
    // Simple add
    mSpectrogram->AddLine(magns, phases);
#endif
    
    // For updating whole spectrogram when param change
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->UpdateSpectrogram(true);
}

void
RebalanceProcessFftObjCompStereo::
//ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
//                 const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    // Avoid a crash
    if (ioFftSamples->size() < 2)
        return;
    
    int numCols = ComputeSpectroNumCols();
    
    // Keep input signal history
    for (int i = 0; i < 2; i++)
    {
        if (mRawSignalHistory[i].size() < numCols)
            mRawSignalHistory[i].push_back(*(*ioFftSamples)[i]);
        else
        {
            mRawSignalHistory[i].freeze();
            mRawSignalHistory[i].push_pop(*(*ioFftSamples)[i]);
        }
    }
    
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> *mixBuffer = mTmpBuf24;
    for (int i = 0; i < 2; i++)
        BLUtils::TakeHalf(*(*ioFftSamples)[i], &mixBuffer[i]);
    
#if PROCESS_SIGNAL_DB
    WDL_TypedBuf<BL_FLOAT> *magns0 = mTmpBuf25;
    WDL_TypedBuf<BL_FLOAT> *phases0 = mTmpBuf26;

    for (int i = 0; i < 2; i++)
    {
        BLUtilsComp::ComplexToMagnPhase(&magns0[i], &phases0[i], mixBuffer[i]);

        mScale->ApplyScaleForEach(Scale::DB, &magns0[i],
                                  (BL_FLOAT)PROCESS_SIGNAL_MIN_DB, (BL_FLOAT)0.0);
    
        BLUtilsComp::MagnPhaseToComplex(&mixBuffer[i], magns0[i], phases0[i]);
    }
#endif

    for (int i = 0; i < 2; i++)
    {
        // For soft masks
        // mMixCols is filled with zeros at the origin
        mMixColsComp[i].freeze();
        mMixColsComp[i].push_pop(mixBuffer[i]);
    
        // History, to stay synchronized between input signal and masks
        mSamplesHistory[i].freeze();
        mSamplesHistory[i].push_pop(mixBuffer[i]);
    }

    for (int i = 0; i < 2; i++)
    {
        int histoIndex = mMaskPred->GetHistoryIndex();
        if (histoIndex < mSamplesHistory[i].size())
            mixBuffer[i] = mSamplesHistory[i][histoIndex];
    }
    
    WDL_TypedBuf<BL_FLOAT> *masks = mTmpBuf3;
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        mMaskPred->GetMask(i, &masks[i]);

    for (int i = 0; i < 2; i++)
    {
        // Keep mask and signal histories
        if (mSignalHistory[i].size() < numCols)
        mSignalHistory[i].push_back(mixBuffer[i]);
        else
        {
            mSignalHistory[i].freeze();
            mSignalHistory[i].push_pop(mixBuffer[i]);
        }
    }
    
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        if (mMasksHistory[i].size() < numCols)
            mMasksHistory[i].push_back(masks[i]);
        else
        {
            mMasksHistory[i].freeze();
            mMasksHistory[i].push_pop(masks[i]);
        }
    }
    
    // Adjust and apply mask
    WDL_TypedBuf<WDL_FFT_COMPLEX> *result = mTmpBuf27;
    WDL_TypedBuf<BL_FLOAT> *magns1 = mTmpBuf28;
    WDL_TypedBuf<BL_FLOAT> *phases1 = mTmpBuf29;
    ComputeResult(mixBuffer, masks, result, magns1, phases1);

    WDL_TypedBuf<BL_FLOAT> &monoMagns = mTmpBuf30;
    BLUtils::StereoToMono(&monoMagns, magns1[0], magns1[1]);

    // NOTE: should do something with phases?
    AddSpectrogramLine(monoMagns, *phases1);
    
    // TODO: tmp buffers / memory optimization
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples = mTmpBuf31;
    for (int i = 0; i < 2; i++)
    {
        fftSamples[i] = result[i];
    
        BLUtilsFft::FillSecondFftHalf(fftSamples[i], (*ioFftSamples)[i]);
    }
}

void
RebalanceProcessFftObjCompStereo::ResetSamplesHistory()
{
    for (int j = 0; j < 2; j++)
    {
        mSamplesHistory[j].resize(mNumInputCols);
    
        for (int i = 0; i < mNumInputCols; i++)
        {
            WDL_TypedBuf<WDL_FFT_COMPLEX> &samples = mTmpBuf8;
            samples.Resize(mBufferSize/2);
            BLUtils::FillAllZero(&samples);
            
            mSamplesHistory[j][i] = samples;
        }
    }
}

void
RebalanceProcessFftObjCompStereo::
ApplyMask(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inData,
          WDL_TypedBuf<WDL_FFT_COMPLEX> *outData,
          const WDL_TypedBuf<BL_FLOAT> &mask)
{
    // TODO: implement method in BLUtils: multvalues(in, out, mask) in complex
    *outData = inData;
    BLUtils::MultValues(outData, mask);
}

void
RebalanceProcessFftObjCompStereo::ResetMixColsComp()
{
    for (int j = 0; j < 2; j++)
    {
        mMixColsComp[j].resize(mNumInputCols);
        
        for (int i = 0; i < mNumInputCols; i++)
        {
            WDL_TypedBuf<WDL_FFT_COMPLEX> &col = mTmpBuf9;
            col.Resize(mBufferSize/2);
            BLUtils::FillAllZero(&col);
            
            mMixColsComp[j][i] = col;
        }
    }
}

void
RebalanceProcessFftObjCompStereo::ResetMasksHistory()
{
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        mMasksHistory[i].unfreeze();
        mMasksHistory[i].clear();
    }
}

void
RebalanceProcessFftObjCompStereo::ResetSignalHistory()
{
    for (int i = 0; i < 2; i++)
    {
        mSignalHistory[i].unfreeze();
        mSignalHistory[i].clear();
    }
}

void
RebalanceProcessFftObjCompStereo::ResetRawSignalHistory()
{
    for (int i = 0; i < 2; i++)
    {
        mRawSignalHistory[i].unfreeze();
        mRawSignalHistory[i].clear();
    }
}
    
void
RebalanceProcessFftObjCompStereo::
ApplySoftMasking(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioData,
                 const WDL_TypedBuf<BL_FLOAT> &mask0)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &softMaskedResult = mTmpBuf10;

    WDL_TypedBuf<BL_FLOAT> &mask = mTmpBuf19;
    mask = mask0;

#if SOFT_MASKING_HACK
    // s = input * HM
    // s = (input * alpha) * HM/alpha
    //
    // here, alpha = 4.0
    //
    // This is a hack, but it works well! :)
    //
    BLUtils::MultValues(&mask, (BL_FLOAT)(1.0/4.0));
    BLUtils::MultValues(ioData, (BL_FLOAT)4.0);
#endif
    
    mSoftMasking[0][0]->ProcessCentered(ioData, mask, &softMaskedResult);

#if SOFT_MASKING_HACK
    // Empirical coeff
    //
    // Compared the input hard mask here, and the soft mask inside mSoftMasking
    // When maks i 1.0 here (all at 100%), the soft mask is 0.1
    // So adjust here with a factor 10.0, plus fix the previous gain of 4.0
    //
    // NOTE: this makes the plugin transparent when all is at 100%
    //
    BLUtils::MultValues(&softMaskedResult, (BL_FLOAT)(10.0/4.0));
#endif
    
    // Result
    if (mSoftMasking[0][0]->IsProcessingEnabled())
        *ioData = softMaskedResult;
}

void
RebalanceProcessFftObjCompStereo::
ApplySoftMaskingStereo(WDL_TypedBuf<WDL_FFT_COMPLEX> ioData[2],
                       const WDL_TypedBuf<BL_FLOAT> masks0[NUM_STEM_SOURCES])
{
    WDL_TypedBuf<BL_FLOAT> *masks = mTmpBuf42;
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        masks[i] = masks0[i];

    // Not sure if we still need to use this for stereo...
#if 0 //SOFT_MASKING_HACK
    // s = input * HM
    // s = (input * alpha) * HM/alpha
    //
    // here, alpha = 4.0
    //
    // This is a hack, but it works well! :)
    //
    BLUtils::MultValues(&mask, (BL_FLOAT)(1.0/4.0));
    BLUtils::MultValues(ioData, (BL_FLOAT)4.0);
#endif

    // References to array
    WDL_TypedBuf<WDL_FFT_COMPLEX> (&sourceData)[4][2] = mTmpBuf43;
    WDL_TypedBuf<WDL_FFT_COMPLEX> (&softMaskedResult)[4][2] = mTmpBuf44;
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            sourceData[i][j] = ioData[j];
            
            mSoftMasking[i][j]->ProcessCentered(&sourceData[i][j],
                                                masks[i], &softMaskedResult[i][j]);
        }
    }

    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        ProcessStereo(i, softMaskedResult[i]);
    
    // Not sure if we still need to use this for stereo...
#if 0 //SOFT_MASKING_HACK
    // Empirical coeff
    //
    // Compared the input hard mask here, and the soft mask inside mSoftMasking
    // When maks i 1.0 here (all at 100%), the soft mask is 0.1
    // So adjust here with a factor 10.0, plus fix the previous gain of 4.0
    //
    // NOTE: this makes the plugin transparent when all is at 100%
    //
    BLUtils::MultValues(&softMaskedResult, (BL_FLOAT)(10.0/4.0));
#endif
    
    // Result
    if (mSoftMasking[0][0]->IsProcessingEnabled())
    {
        // Sum the different parts
        BLUtils::FillAllZero(&ioData[0]);
        BLUtils::FillAllZero(&ioData[1]);
        for (int i = 0; i < NUM_STEM_SOURCES; i++)
        {
            BLUtils::AddValues(&ioData[0], softMaskedResult[i][0]);
            BLUtils::AddValues(&ioData[1], softMaskedResult[i][1]);
        }
        
        //*ioData = softMaskedResult;
    }
}

void
RebalanceProcessFftObjCompStereo::ComputeInverseDB(WDL_TypedBuf<BL_FLOAT> *magns)
{
#if 0 // Origin
    for (int i = 0; i < magns->GetSize(); i++)
    {
        BL_FLOAT val = magns->Get()[i];
        val = mScale->ApplyScaleInv(Scale::DB, val,
                                    (BL_FLOAT)PROCESS_SIGNAL_MIN_DB, (BL_FLOAT)0.0);
        
        // Noise floor
        BL_FLOAT db = BLUtils::AmpToDB(val);
        if (db < PROCESS_SIGNAL_MIN_DB + 1)
            val = 0.0;
        
        magns->Get()[i] = val;
    }
#endif

#if 1 // Optimized
    // NOTE: do we need the noise floor test like above?
    mScale->ApplyScaleInvForEach(Scale::DB, magns,
                                 (BL_FLOAT)PROCESS_SIGNAL_MIN_DB, (BL_FLOAT)0.0);
#endif
}

void
RebalanceProcessFftObjCompStereo::RecomputeSpectrogram(bool recomputeMasks)
{
    if (recomputeMasks)
    {
        // Clear all
        ResetSpectrogram();
        
        // Reprocess globally the previous input signal 
        bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > *rawSignalHistoryCopy = mTmpBuf32;
        rawSignalHistoryCopy[0] = mRawSignalHistory[0];
        rawSignalHistoryCopy[1] = mRawSignalHistory[1];

        // The reset the current one (it will be refilled during reocmputation) 
        ResetRawSignalHistory();
            
        for (int i = 0; i < rawSignalHistoryCopy[0].size(); i++)
        {
            WDL_TypedBuf<WDL_FFT_COMPLEX> *signal = mTmpBuf33;
            for (int k = 0; k < 2; k++)
            {
                signal[0] = rawSignalHistoryCopy[k][i];
            }
            
            //
            vector<WDL_TypedBuf<WDL_FFT_COMPLEX> *> &signalVec = mTmpBuf20;
            signalVec.resize(2);
            signalVec[0] = &signal[0];
            signalVec[1] = &signal[1];

            mMaskPred->ProcessInputFft(&signalVec, NULL);

            //
            //ProcessFftBuffer(&signal, NULL);
            ProcessInputFft(&signalVec, NULL);
        }
        
        // We have finished with recompting everything
        return;
    }

    // Be sure to reset!
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (mSoftMasking[i][j] != NULL)
                mSoftMasking[i][j]->Reset(mBufferSize, mOverlapping);
        }
    }
    
    // Keep lines, and add them all at once at the end 
    vector<WDL_TypedBuf<BL_FLOAT> > *magnsVec = mTmpBuf34;
    vector<WDL_TypedBuf<BL_FLOAT> > *phasesVec = mTmpBuf35;

    for (int i = 0; i < 2; i++)
    {
        magnsVec[i].resize(mSignalHistory[i].size());
        phasesVec[i].resize(mSignalHistory[i].size());
    }
    
    for (int i = 0; i < mSignalHistory[0].size(); i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> *signal = mTmpBuf36;
        signal[0] = mSignalHistory[0][i];
        signal[1] = mSignalHistory[1][i];

        WDL_TypedBuf<BL_FLOAT> *masks = mTmpBuf18;

        // Do not recompute masks, re-use current ones
        for (int j = 0; j < NUM_STEM_SOURCES; j++)
            masks[j] = mMasksHistory[j][i];
                
        WDL_TypedBuf<WDL_FFT_COMPLEX> *result = mTmpBuf37;
        WDL_TypedBuf<BL_FLOAT> *magns = mTmpBuf38;
        WDL_TypedBuf<BL_FLOAT> *phases = mTmpBuf39;
        ComputeResult(signal, masks, result, magns, phases);

        for (int k = 0; k < 2; k++)
        {
            magnsVec[k][i] = magns[k];
            phasesVec[k][i] = phases[k];
        }
    }


    vector<WDL_TypedBuf<BL_FLOAT> > &monoMagnsVec = mTmpBuf40;
    monoMagnsVec.resize(magnsVec[0].size());
    for (int i = 0; i < magnsVec[0].size(); i++)
        BLUtils::StereoToMono(&monoMagnsVec[i], magnsVec[0][i], magnsVec[1][i]);
    
    // Add all lines at once at the end
    
    // NOTE: do something with phases?
    mSpectrogram->SetLines(monoMagnsVec, phasesVec[0]);

    if (mSpectroDisplay != NULL)
        mSpectroDisplay->UpdateSpectrogram(true);
}

void
RebalanceProcessFftObjCompStereo::
ComputeResult(const WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer[2],
              const WDL_TypedBuf<BL_FLOAT> masks0[NUM_STEM_SOURCES],
              WDL_TypedBuf<WDL_FFT_COMPLEX> result[2],
              WDL_TypedBuf<BL_FLOAT> resMagns[2],
              WDL_TypedBuf<BL_FLOAT> resPhases[2])
{
    for (int i = 0; i < 2; i++)
    {
        result[i].Resize(mixBuffer[i].GetSize());
        BLUtils::FillAllZero(&result[i]);
    }
    
    WDL_TypedBuf<BL_FLOAT> *masks = mTmpBuf41;
    mMaskProcessor->ProcessSeparate(masks0, masks);
    
#if !USE_SOFT_MASKING
    ApplyMask(mixBuffer, result, mask);
#endif
    
#if USE_SOFT_MASKING
    result[0] = mixBuffer[0];
    result[1] = mixBuffer[1];
    //ApplySoftMasking(result, mask);
    ApplySoftMaskingStereo(result, masks);
#endif

    for (int i = 0; i < 2; i++)
    {
        BLUtilsComp::ComplexToMagnPhase(&resMagns[i], &resPhases[i], result[i]);
        
#if PROCESS_SIGNAL_DB
        ComputeInverseDB(resMagns[i]);
        
        BLUtilsComp::MagnPhaseToComplex(&result[i], resMagns[i], resPhases[i]);
#endif
    }
}

int
RebalanceProcessFftObjCompStereo::ComputeSpectroNumCols()
{
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = mBufferSize/(32/mOverlapping);
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = mSampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    return numCols;
}

// Reset everything except the raw buffered samples
void
RebalanceProcessFftObjCompStereo::ResetSpectrogram()
{
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (mSoftMasking[i][j] != NULL)
                mSoftMasking[i][j]->Reset(mBufferSize, mOverlapping);
        }
    }
    
    mMaskPred->Reset();
    
    ResetSamplesHistory();
    ResetMixColsComp();

    ResetMasksHistory();
    ResetSignalHistory();

    // Don't reset raw samples
    
    int numCols = ComputeSpectroNumCols();
    mSpectrogram->Reset(mSampleRate, SPECTRO_HEIGHT, numCols);
}

void
RebalanceProcessFftObjCompStereo::
ProcessStereo(int partNum, WDL_TypedBuf<WDL_FFT_COMPLEX> ioFftSamples[2])
{
    // Convert to magn/phases
    WDL_TypedBuf<BL_FLOAT> *magns = mTmpBuf45;
    WDL_TypedBuf<BL_FLOAT> *phases = mTmpBuf46;

    BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], ioFftSamples[0]);
    BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], ioFftSamples[1]);

    // Process
    BL_FLOAT widthFactors[NUM_STEM_SOURCES] =
        { mWidthVocal, mWidthBass, mWidthDrums, mWidthOther };

    BL_FLOAT panFactors[NUM_STEM_SOURCES] =
        { mPanVocal, mPanBass, mPanDrums, mPanOther };

    vector<WDL_TypedBuf<BL_FLOAT> * > magnsVec;
    magnsVec.resize(2);
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        magnsVec[0] = &magns[0];
        magnsVec[1] = &magns[1];
        
        // Do not use param smoother
        // (this is why we use StereoWidenProcess and not BLStereoWidener)

        // Width
        StereoWidenProcess::StereoWiden(&magnsVec, widthFactors[i]);

        // Pan
        StereoWidenProcess::Balance(&magnsVec, panFactors[i]);
    }
    
    // Convert back to complex
    BLUtilsComp::MagnPhaseToComplex(&ioFftSamples[0], magns[0], phases[0]);
    BLUtilsComp::MagnPhaseToComplex(&ioFftSamples[1], magns[1], phases[1]);
}
