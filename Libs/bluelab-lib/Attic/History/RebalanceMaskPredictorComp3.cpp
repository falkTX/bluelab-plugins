//
//  RebalanceMaskPredictorComp3.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#include <string>
using namespace std;

#include <SoftMaskingNComp.h>

#include <BLUtils.h>
#include <PPMFile.h>

#include "RebalanceMaskPredictorComp3.h"

// Export from kerasify or frugally-deep ?
#define USE_MODEL_KF 0 //1

// Export in .h5 format for CompileNN
#define USE_MODEL_CNN 0

// Convert from keras to Caffe, then we have the model files for C++ Caffe
#define USE_MODEL_CAFFE 0

// Darknet models, directly trained inside darknet
#define USE_MODEL_DARKNET 1

#if USE_MODEL_KF
#include <DNNModelKF.h>
#endif

#if USE_MODEL_CNN
#include <DNNModelCNN.h>
#endif

#if USE_MODEL_CAFFE
#include <DNNModelCaffe.h>
#endif

#if USE_MODEL_DARKNET
#include <DNNModelDarknetMc.h>
#endif


// Inverse mel scale  was not applied if RESAMPLE_FACTOR was 1
#define FIX_UPSAMPLE_MEL 1

// Don't predict every mask, but re-use previous predictions and scroll
#define DONT_PREDICT_EVERY_MASK 0 //1
#define MASK_PREDICT_STEP_MOD 16


RebalanceMaskPredictorComp3::RebalanceMaskPredictorComp3(int bufferSize,
                                                         BL_FLOAT overlapping,
                                                         BL_FLOAT oversampling,
                                                         BL_FLOAT sampleRate,
                                                         IGraphics *graphics)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if FORCE_SAMPLE_RATE
    mPlugSampleRate = sampleRate;
#endif
    
    // DNN
    
    mModelNum = 0;
    
#ifndef WIN32
    WDL_String resPath;
    graphics->GetResourceDir(&resPath);
    const char *resourcePath = resPath.Get();
    
    // Vocal
    CreateModel(MODEL_4X, resourcePath, &mModels[0]);
    
    // Bass
    CreateModel(MODEL_8X, resourcePath, &mModels[1]);
    
    // Drums
    CreateModel(MODEL_16X, resourcePath, &mModels[2]);
    
    // Other
    CreateModel(MODEL_32X, resourcePath, &mModels[3]);
    
#else // WIN32
    
#if USE_MODEL_KF
    mModelVocal = new DNNModelKF();
    mModelBass = new DNNModelKF();
    mModelDrums = new DNNModelKF();
    mModelOther = new DNNModelKF();
#endif

#if USE_MODEL_CNN
    mModelVocal = new DNNModelCNN();
    mModelBass = new DNNModelCNN();
    mModelDrums = new DNNModelCNN();
    mModelOther = new DNNModelCNN();
#endif

#if USE_MODEL_CAFFE
    mModelVocal = new DNNModelCaffe();
    mModelBass = new DNNModelCaffe();
    mModelDrums = new DNNModelCaffe();
    mModelOther = new DNNModelCaffe();
#endif

#if USE_MODEL_DARKNET
    mModelVocal = new DNNModelDarknet();
    mModelBass = new DNNModelDarknet();
    mModelDrums = new DNNModelDarknet();
    mModelOther = new DNNModelDarknet();
#endif
    
    mModelVocal->LoadWin(graphics, MODEL0_ID);
    mModelBass->LoadWin(graphics, MODEL1_ID);
    mModelDrums->LoadWin(graphics, MODEL2_ID);
    mModelOther->LoadWin(graphics, MODEL3_ID);
#endif
    
    for (int i = 0; i < NUM_INPUT_COLS; i++)
    {
        WDL_TypedBuf<BL_FLOAT> col;
        BLUtils::ResizeFillZeros(&col, bufferSize/(2*RESAMPLE_FACTOR));
        
        mMixCols.push_back(col);
    }
    
    for (int i = 0; i < NUM_INPUT_COLS; i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> samples;
        BLUtils::ResizeFillZeros(&samples, bufferSize/(2*RESAMPLE_FACTOR));
        
        mSamplesHistory.push_back(samples);
    }
    
#if FORCE_SAMPLE_RATE
    InitResamplers();
    
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
#if USE_SOFT_MASK_N
    mSoftMasking = new SoftMaskingNComp(SOFT_MASK_HISTO_SIZE);
    mUseSoftMasks = true;
#endif
    
    mMaskPredictStepNum = 0;
}

RebalanceMaskPredictorComp3::~RebalanceMaskPredictorComp3()
{
#if USE_SOFT_MASK_N
    delete mSoftMasking;
#endif
    
    for (int i = 0; i < 4; i++)
        delete mModels[i];
}

void
RebalanceMaskPredictorComp3::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
    
#if FORCE_SAMPLE_RATE
    for (int i = 0; i < 2; i++)
    {
        mResamplers[i].Reset();
        // set input and output samplerates
        mResamplers[i].SetRates(mPlugSampleRate, SAMPLE_RATE);
    }
    
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
#if USE_SOFT_MASK_N
    mSoftMasking->Reset();
#endif
    
    mMaskPredictStepNum = 0;
    mCurrentMasks.clear();
}

void
RebalanceMaskPredictorComp3::Reset(int bufferSize, int overlapping,
                                  int oversampling, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if FORCE_SAMPLE_RATE
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
#if USE_SOFT_MASK_N
    mSoftMasking->Reset();
#endif
}

#if 1 // GOOD (even for 192000Hz, block size 64)
#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictorComp3::ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                                   const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer)
{
    if (mPlugSampleRate == SAMPLE_RATE)
        return;
    
    BL_FLOAT sampleRate = mPlugSampleRate;
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioSamples->size())
            break;
        
        WDL_ResampleSample *resampledAudio = NULL;
        int desiredSamples = (*ioSamples)[i]->GetSize(); // Input driven
        
        // GOOD
        // Without: spectrogram scalled horizontally
#if FIX_ADJUST_IN_RESAMPLING
        // Adjust
        if (mRemainingSamples[i] >= 1.0)
        {
            int subSamples = floor(mRemainingSamples[i]);
            mRemainingSamples[i] -= subSamples;
            
            desiredSamples -= subSamples;
        }
#endif
        
        int numSamples = mResamplers[i].ResamplePrepare(desiredSamples, 1, &resampledAudio);
        
        for (int j = 0; j < numSamples; j++)
        {
            if (j >= (*ioSamples)[i]->GetSize())
                break;
            resampledAudio[j] = (*ioSamples)[i]->Get()[j];
        }
        
        WDL_TypedBuf<BL_FLOAT> outSamples;
        outSamples.Resize(desiredSamples);
        int numResampled = mResamplers[i].ResampleOut(outSamples.Get(),
                                                      // Must be exactly the value returned by ResamplePrepare
                                                      // Otherwise the spectrogram could be scaled horizontally
                                                      // (or clicks)
                                                      // Due to flush of the resampler
                                                      numSamples,
                                                      outSamples.GetSize(), 1);
        
        // GOOD !
        // Avoid clicks sometimes (for example with 88200Hz and buffer size 447)
        // The numResampled varies around a value, to keep consistency of the stream
        outSamples.Resize(numResampled);
        
        *((*ioSamples)[i]) = outSamples;
        
#if FIX_ADJUST_IN_RESAMPLING
        // Adjust
        //BL_FLOAT remaining = desiredSamples - numResampled*sampleRate/SAMPLE_RATE;
        BL_FLOAT remaining = numResampled*sampleRate/SAMPLE_RATE - desiredSamples;
        mRemainingSamples[i] += remaining;
#endif
    }
}
#endif
#endif

void
RebalanceMaskPredictorComp3::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                                             const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() < 1)
        return;
    
    // Take only the left channel...
    //
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *(*ioFftSamples)[0];
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
#if USE_SOFT_MASK_N
    mCurrentData = fftSamples;
#endif
    
    // Compute the masks
    WDL_TypedBuf<BL_FLOAT> magnsDown = magns;
    Downsample(&magnsDown, mSampleRate);
    
    // mMixCols is filled with zeros at the origin
    mMixCols.push_back(magnsDown);
    mMixCols.pop_front();
    
    WDL_TypedBuf<BL_FLOAT> mixBufHisto;
    ColumnsToBuffer(&mixBufHisto, mMixCols);
    
    ComputeMasksComp(&mMaskVocal, &mMaskBass, &mMaskDrums, &mMaskOther, mixBufHisto);
}

void
RebalanceMaskPredictorComp3::GetMaskVocal(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal)
{
    *maskVocal = mMaskVocal;
}

void
RebalanceMaskPredictorComp3::GetMaskBass(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass)
{
    *maskBass = mMaskBass;
}

void
RebalanceMaskPredictorComp3::GetMaskDrums(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums)
{
    *maskDrums = mMaskDrums;
}

void
RebalanceMaskPredictorComp3::GetMaskOther(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther)
{
    *maskOther = mMaskOther;
}

void
RebalanceMaskPredictorComp3::Downsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                       BL_FLOAT sampleRate)
{
#if FORCE_SAMPLE_RATE
    sampleRate = SAMPLE_RATE;
#endif
    
    BL_FLOAT hzPerBin = sampleRate/(BUFFER_SIZE/2);
    WDL_TypedBuf<BL_FLOAT> bufMel;
    BLUtils::FreqsToMelNorm(&bufMel, *ioBuf, hzPerBin);
    *ioBuf = bufMel;
    
    if (RESAMPLE_FACTOR == 1)
        // No need to downsample
        return;
    
    int newSize = ioBuf->GetSize()/RESAMPLE_FACTOR;
    
    BLUtils::ResizeLinear(ioBuf, newSize);
}

void
RebalanceMaskPredictorComp3::Upsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                     BL_FLOAT sampleRate)
{
#if FORCE_SAMPLE_RATE
    sampleRate = SAMPLE_RATE;
#endif
    
    if (RESAMPLE_FACTOR != 1)
    {
        int newSize = ioBuf->GetSize()*RESAMPLE_FACTOR;
        
        BLUtils::ResizeLinear(ioBuf, newSize);
    }
    
    BL_FLOAT hzPerBin = sampleRate/(BUFFER_SIZE/2);
    WDL_TypedBuf<BL_FLOAT> bufMel;
    BLUtils::MelToFreqsNorm(&bufMel, *ioBuf, hzPerBin);
    *ioBuf = bufMel;
}

#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictorComp3::SetPlugSampleRate(BL_FLOAT sampleRate)
{
    mPlugSampleRate = sampleRate;
    
    InitResamplers();
}
#endif

#if USE_SOFT_MASK_N
void
RebalanceMaskPredictorComp3::SetUseSoftMasks(bool flag)
{
    mUseSoftMasks = flag;
    
    Reset();
}
#endif

void
RebalanceMaskPredictorComp3::SetModelNum(int modelNum)
{
    if ((modelNum < 0) || (modelNum > 3))
        return;
    
    mModelNum = modelNum;
}

void
RebalanceMaskPredictorComp3::AddInputSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &samples)
{
    mSamplesHistory.push_back(samples);
    mSamplesHistory.pop_front();
}

void
RebalanceMaskPredictorComp3::GetCurrentSamples(WDL_TypedBuf<WDL_FFT_COMPLEX> *samples)
{
//NOTE: something seems reversed....
//TODO: check very well that the masks are not reversed,
//    and check mask scrolling direction after
    
#if !DONT_PREDICT_EVERY_MASK
    int colNum = 0; //NUM_OUTPUT_COLS - 1; // WIP
#else
    int colNum = NUM_OUTPUT_COLS - MASK_PREDICT_STEP_MOD;
#endif
    
    if (colNum >= mSamplesHistory.size())
        return;
    
    *samples = mSamplesHistory[colNum];
}

// Split the mask cols, get each mask and upsample it
void
RebalanceMaskPredictorComp3::UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                                   BL_FLOAT sampleRate)
{
#if !FIX_UPSAMPLE_MEL
    if (RESAMPLE_FACTOR == 1)
        // No need to upsample
        return;
#endif
    
    WDL_TypedBuf<BL_FLOAT> result;
    for (int j = 0; j < NUM_OUTPUT_COLS; j++)
    {
        WDL_TypedBuf<BL_FLOAT> mask;
        mask.Resize(BUFFER_SIZE/(2*RESAMPLE_FACTOR));
        
        // Optim
        memcpy(mask.Get(),
               &ioBuf->Get()[j*mask.GetSize()],
               mask.GetSize()*sizeof(BL_FLOAT));
     
        // Need to always "upsample", because upsamples also makes invert mel scale
        Upsample(&mask, sampleRate);
        
        result.Add(mask.Get(), mask.GetSize());
    }
    
    *ioBuf = result;
}

void
RebalanceMaskPredictorComp3::PredictMasks(vector<WDL_TypedBuf<BL_FLOAT> > *masks,
                                          const WDL_TypedBuf<BL_FLOAT> &mixBufHisto,
                                          DNNModelMc *model,
                                          BL_FLOAT sampleRate)
{
    model->Predict(mixBufHisto, masks);
    
    // NEW
    //ReduceMaskCols(&maskBuf);
}

void
RebalanceMaskPredictorComp3::ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                            const deque<WDL_TypedBuf<BL_FLOAT> > &cols)
{
    if (cols.empty())
        return;
    
    buf->Resize(cols.size()*cols[0].GetSize());
    
    for (int j = 0; j < cols.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &col = cols[j];
        for (int i = 0; i < col.GetSize(); i++)
        {
            int bufIndex = i + j*col.GetSize();
            
            buf->Get()[bufIndex] = col.Get()[i];
        }
    }
}

void
RebalanceMaskPredictorComp3::ColumnsToBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                                             const deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &cols)
{
    if (cols.empty())
        return;
    
    buf->Resize(cols.size()*cols[0].GetSize());
    
    for (int j = 0; j < cols.size(); j++)
    {
        const WDL_TypedBuf<WDL_FFT_COMPLEX> &col = cols[j];
        for (int i = 0; i < col.GetSize(); i++)
        {
            int bufIndex = i + j*col.GetSize();
            
            buf->Get()[bufIndex] = col.Get()[i];
        }
    }
}

#if 0 // ORIG
void
RebalanceMaskPredictorComp3::ComputeMasksComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther,
                                             const WDL_TypedBuf<BL_FLOAT> &mixBufHisto)
{
    vector<WDL_TypedBuf<BL_FLOAT> > masks;
    PredictMasks(&masks, mixBufHisto, mModels[mModelNum], mSampleRate);
    
#if 1 // NEW
    for (int i = 0; i < 4; i++)
        UpsamplePredictedMask(&masks[i], mSampleRate);
#endif
        
    WDL_TypedBuf<BL_FLOAT> maskVocalFull = masks[0];
    WDL_TypedBuf<BL_FLOAT> maskBassFull = masks[1];
    WDL_TypedBuf<BL_FLOAT> maskDrumsFull = masks[2];
    WDL_TypedBuf<BL_FLOAT> maskOtherFull = masks[3];
    
#if 1
    // Theshold, just in case (prediction can return negative mask values)
    BLUtils::ClipMin(&maskVocalFull, 0.0);
    BLUtils::ClipMin(&maskBassFull, 0.0);
    BLUtils::ClipMin(&maskDrumsFull, 0.0);
    BLUtils::ClipMin(&maskOtherFull, 0.0);
#endif
    
#if USE_SOFT_MASK_N
    if (mUseSoftMasks)
    {
        ComputeLineMasksSoft(maskVocal, maskVocalFull, maskBass, maskBassFull,
                             maskDrums, maskDrumsFull, maskOther, maskOtherFull);
        
        return;
    }
#endif
    
    // ORIGIN: Not using soft masks
    ComputeLineMaskComp2(maskVocal, maskVocalFull);
    ComputeLineMaskComp2(maskBass, maskBassFull);
    ComputeLineMaskComp2(maskDrums, maskDrumsFull);
    ComputeLineMaskComp2(maskOther, maskOtherFull);
}
#endif

#if 1
void
RebalanceMaskPredictorComp3::ComputeMasksComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                                              WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                                              WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                                              WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther,
                                              const WDL_TypedBuf<BL_FLOAT> &mixBufHisto)
{
    // DEBUG
    mModelNum = 3;
    
#if DONT_PREDICT_EVERY_MASK
    vector<WDL_TypedBuf<BL_FLOAT> > masks;
    if (mMaskPredictStepNum++ % MASK_PREDICT_STEP_MOD == 0)
    {
        vector<WDL_TypedBuf<BL_FLOAT> > masks0;
        PredictMasks(&masks0, mixBufHisto, mModels[mModelNum], mSampleRate);
        
        UpdateCurrentMasksAdd(masks0);
    }
    else
    {
        UpdateCurrentMasksScroll();
    }
    
#if 0 // DEBUG
    PPMFile::SavePPM("mix.ppm", mixBufHisto.Get(),
                     256, 32, 1,
                     255.0*255.0);
    
    PPMFile::SavePPM("vocal.ppm", mCurrentMasks[0].Get(),
                     256, 32, 1,
                     255.0);
    
    {
        vector<WDL_TypedBuf<BL_FLOAT> > masksTmp;
        PredictMasks(&masksTmp, mixBufHisto, mModels[mModelNum], mSampleRate);
        
        PPMFile::SavePPM("vocal-tmp.ppm", masksTmp[0].Get(),
                         256, 32, 1,
                         255.0);
    }
#endif
    
    masks = mCurrentMasks;
#else
    vector<WDL_TypedBuf<BL_FLOAT> > masks;
    PredictMasks(&masks, mixBufHisto, mModels[mModelNum], mSampleRate);
#endif
    
#if 1 // NEW
    for (int i = 0; i < 4; i++)
        UpsamplePredictedMask(&masks[i], mSampleRate);
#endif
    
    WDL_TypedBuf<BL_FLOAT> maskVocalFull = masks[0];
    WDL_TypedBuf<BL_FLOAT> maskBassFull = masks[1];
    WDL_TypedBuf<BL_FLOAT> maskDrumsFull = masks[2];
    WDL_TypedBuf<BL_FLOAT> maskOtherFull = masks[3];
    
#if 1
    // Theshold, just in case (prediction can return negative mask values)
    BLUtils::ClipMin(&maskVocalFull, 0.0);
    BLUtils::ClipMin(&maskBassFull, 0.0);
    BLUtils::ClipMin(&maskDrumsFull, 0.0);
    BLUtils::ClipMin(&maskOtherFull, 0.0);
#endif
    
#if USE_SOFT_MASK_N
    if (mUseSoftMasks)
    {
        ComputeLineMasksSoft(maskVocal, maskVocalFull, maskBass, maskBassFull,
                             maskDrums, maskDrumsFull, maskOther, maskOtherFull);
        
        return;
    }
#endif
    
    // ORIGIN: Not using soft masks
    ComputeLineMaskComp2(maskVocal, maskVocalFull);
    ComputeLineMaskComp2(maskBass, maskBassFull);
    ComputeLineMaskComp2(maskDrums, maskDrumsFull);
    ComputeLineMaskComp2(maskOther, maskOtherFull);
}
#endif

void
RebalanceMaskPredictorComp3::ComputeLineMaskComp2(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskResult,
                                                 const WDL_TypedBuf<BL_FLOAT> &maskSource)
{
    int numFreqs = BUFFER_SIZE/2;
    //int numFreqs = BUFFER_SIZE/(2*RESAMPLE_FACTOR);
    
    int numCols = NUM_OUTPUT_COLS;
    
    maskResult->Resize(numFreqs);
    
    for (int i = 0; i < numFreqs; i++)
    {
        WDL_FFT_COMPLEX avg;
        avg.re = 0.0;
        avg.im = 0.0;
        
        for (int j = 0; j < numCols; j++)
        {
            int idx = i + j*numFreqs;
            
            BL_FLOAT m = maskSource.Get()[idx];
            avg.re += m;
        }
        
        avg.re /= numCols;
        
        maskResult->Get()[i] = avg;
    }
}

void
RebalanceMaskPredictorComp3::ComputeLineMaskComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskResult,
                                                const WDL_TypedBuf<WDL_FFT_COMPLEX> &maskSource)
{
    int numFreqs = BUFFER_SIZE/2;
    int numCols = NUM_OUTPUT_COLS;
    
    maskResult->Resize(numFreqs);
    
    for (int i = 0; i < numFreqs; i++)
    {
        WDL_FFT_COMPLEX avg;
        avg.re = 0.0;
        avg.im = 0.0;
        
        for (int j = 0; j < numCols; j++)
        {
            int idx = i + j*numFreqs;
            
            WDL_FFT_COMPLEX m = maskSource.Get()[idx];
            avg.re += m.re;
            avg.im += m.im;
        }
        
        avg.re /= numCols;
        avg.im /= numCols;
        
        maskResult->Get()[i] = avg;
    }
}

#if USE_SOFT_MASK_N
void
RebalanceMaskPredictorComp3::ComputeLineMask2(WDL_TypedBuf<BL_FLOAT> *maskResult,
                                              const WDL_TypedBuf<BL_FLOAT> &maskSource)
{
    int numFreqs = BUFFER_SIZE/2;
    int numCols = NUM_OUTPUT_COLS;
    
#if !DONT_PREDICT_EVERY_MASK
    int colNum = numCols - 1;
#else
    int colNum = numCols - MASK_PREDICT_STEP_MOD;
#endif
    
    maskResult->Resize(numFreqs);
    
    for (int i = 0; i < numFreqs; i++)
    {
        // Take the last column
        int idx = i + colNum*numFreqs;
        
        BL_FLOAT m = maskSource.Get()[idx];
        
        maskResult->Get()[i] = m;
    }
}

void
RebalanceMaskPredictorComp3::ComputeLineMasksSoft(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                                                 const WDL_TypedBuf<BL_FLOAT> &maskVocalFull,
                                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                                                 const WDL_TypedBuf<BL_FLOAT> &maskBassFull,
                                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                                                 const WDL_TypedBuf<BL_FLOAT> &maskDrumsFull,
                                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther,
                                                 const WDL_TypedBuf<BL_FLOAT> &maskOtherFull)
{
    WDL_TypedBuf<BL_FLOAT> predictMaskVocal;
    WDL_TypedBuf<BL_FLOAT> predictMaskBass;
    WDL_TypedBuf<BL_FLOAT> predictMaskDrums;
    WDL_TypedBuf<BL_FLOAT> predictMaskOther;
    
    // Compute the lines
    ComputeLineMask2(&predictMaskVocal, maskVocalFull);
    ComputeLineMask2(&predictMaskBass, maskBassFull);
    ComputeLineMask2(&predictMaskDrums, maskDrumsFull);
    ComputeLineMask2(&predictMaskOther, maskOtherFull);
    
    const WDL_TypedBuf<WDL_FFT_COMPLEX> &mix = mCurrentData;
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > estimData;
    estimData.push_back(mix);
    estimData.push_back(mix);
    estimData.push_back(mix);
    estimData.push_back(mix);
    
    // Multiply the masks by the magnitudes, to have the estimation
    BLUtils::MultValues(&estimData[0], predictMaskVocal);
    BLUtils::MultValues(&estimData[1], predictMaskBass);
    BLUtils::MultValues(&estimData[2], predictMaskDrums);
    BLUtils::MultValues(&estimData[3], predictMaskOther);
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > softMasks;
    mSoftMasking->Process(mix, estimData, &softMasks);
    
    *maskVocal = softMasks[0];
    *maskBass = softMasks[1];
    *maskDrums = softMasks[2];
    *maskOther = softMasks[3];
}
#endif

#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictorComp3::InitResamplers()
{
    // In
    
    for (int i = 0; i < 2; i++)
    {
        mResamplers[i].Reset(); //
        
        mResamplers[i].SetMode(true, 1, false, 0, 0);
        mResamplers[i].SetFilterParms();
        
        // GOOD !
        // Set input driven
        // (because output driven has a bug when downsampling:
        // the first samples are bad)
        mResamplers[i].SetFeedMode(true);
        
        // set input and output samplerates
        mResamplers[i].SetRates(mPlugSampleRate, SAMPLE_RATE);
    }
}
#endif

void
RebalanceMaskPredictorComp3::CreateModel(const char *modelFileName,
                                         const char *resourcePath,
                                         DNNModelMc **model)
{
#if USE_MODEL_KF
    *model = new DNNModelKF();
#endif

#if USE_MODEL_CNN
    *model = new DNNModelCNN();
#endif

#if USE_MODEL_CAFFE
    *model = new DNNModelCaffe();
#endif

#if USE_MODEL_DARKNET
    *model = new DNNModelDarknetMc();
#endif
    
    (*model)->Load(modelFileName, resourcePath);
}

void
RebalanceMaskPredictorComp3::ReduceMaskCols(WDL_TypedBuf<BL_FLOAT> *ioMask)
{
    // Method 1: take only the last column
    int index = NUM_OUTPUT_COLS - 1;
    
    // Method2: take the middle
    //int index = NUM_OUTPUT_COLS/2;
    
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(ioMask->GetSize()/NUM_OUTPUT_COLS);
    
    memcpy(result.Get(),
           &ioMask->Get()[result.GetSize()*index],
           result.GetSize()*sizeof(BL_FLOAT));
    
    *ioMask = result;
}

void
RebalanceMaskPredictorComp3::UpdateCurrentMasksAdd(const vector<WDL_TypedBuf<BL_FLOAT> > &newMasks)
{
    if (mCurrentMasks.size() != newMasks.size())
        mCurrentMasks.resize(newMasks.size());
    
    // Resize/init current masks if necessary
    for (int i = 0; i < mCurrentMasks.size(); i++)
    {
        if (mCurrentMasks[i].GetSize() != newMasks[i].GetSize())
        {
            mCurrentMasks[i].Resize(newMasks[i].GetSize());
            BLUtils::FillAllZero(&mCurrentMasks[i]);
        }
    }
    
    // Copy the masks over
    for (int i = 0; i < mCurrentMasks.size(); i++)
    {
        mCurrentMasks[i] = newMasks[i];
    }
}

void
RebalanceMaskPredictorComp3::UpdateCurrentMasksScroll()
{
    int numFreqs = BUFFER_SIZE/(2*RESAMPLE_FACTOR);
    int numCols = NUM_OUTPUT_COLS;
    
    for (int i = 0; i < mCurrentMasks.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &mask = mCurrentMasks[i];
        for (int k = 0; k < numCols - 1; k++)
        {
            for (int j = 0; j < numFreqs; j++)
            {
                // Scroll
                mask.Get()[j + k*numFreqs] = mask.Get()[j + (k + 1)*numFreqs];
            }
            
            // Set the last value to 0
            for (int j = 0; j < numFreqs; j++)
                mask.Get()[j + (numCols - 1)*numFreqs] = 0.0;
        }
    }
}
