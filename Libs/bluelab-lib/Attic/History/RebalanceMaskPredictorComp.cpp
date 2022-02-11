//
//  RebalanceMaskPredictorComp.cpp
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

#include "RebalanceMaskPredictorComp.h"

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
#include <DNNModelDarknet.h>
#endif


// Iverse mel scale  was not applied if RESAMPLE_FACTOR was 1
#define FIX_UPSAMPLE_MEL 1

// Debug
#define DBG_DUMP_DATA 0 //1 //0

RebalanceMaskPredictorComp::RebalanceMaskPredictorComp(int bufferSize,
                                                       BL_FLOAT overlapping, BL_FLOAT oversampling,
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
    
#ifndef WIN32
    WDL_String resPath;
    graphics->GetResourceDir(&resPath);
    const char *resourcePath = resPath.Get();
    
    // Vocal
    CreateModel(MODEL_VOCAL, resourcePath, &mModelVocal);
    
    // Bass
    CreateModel(MODEL_BASS, resourcePath, &mModelBass);
    
    // Drums
    CreateModel(MODEL_DRUMS, resourcePath, &mModelDrums);
    
    // Other
    CreateModel(MODEL_OTHER, resourcePath, &mModelOther);
    
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
    
#if FORCE_SAMPLE_RATE
    InitResamplers();
    
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
#if USE_SOFT_MASK_N
    mSoftMasking = new SoftMaskingNComp(SOFT_MASK_HISTO_SIZE);
    mUseSoftMasks = true;
#endif
}

RebalanceMaskPredictorComp::~RebalanceMaskPredictorComp()
{
#if USE_SOFT_MASK_N
    delete mSoftMasking;
#endif
    
    delete mModelVocal;
    delete mModelBass;
    delete mModelDrums;
    delete mModelOther;
}

void
RebalanceMaskPredictorComp::Reset()
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
}

void
RebalanceMaskPredictorComp::Reset(int bufferSize, int overlapping,
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
RebalanceMaskPredictorComp::ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
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
RebalanceMaskPredictorComp::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
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
RebalanceMaskPredictorComp::GetMaskVocal(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal)
{
    *maskVocal = mMaskVocal;
}

void
RebalanceMaskPredictorComp::GetMaskBass(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass)
{
    *maskBass = mMaskBass;
}

void
RebalanceMaskPredictorComp::GetMaskDrums(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums)
{
    *maskDrums = mMaskDrums;
}

void
RebalanceMaskPredictorComp::GetMaskOther(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther)
{
    *maskOther = mMaskOther;
}

void
RebalanceMaskPredictorComp::Downsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
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
RebalanceMaskPredictorComp::Upsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
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
RebalanceMaskPredictorComp::SetPlugSampleRate(BL_FLOAT sampleRate)
{
    mPlugSampleRate = sampleRate;
    
    InitResamplers();
}
#endif

#if USE_SOFT_MASK_N
void
RebalanceMaskPredictorComp::SetUseSoftMasks(bool flag)
{
    mUseSoftMasks = flag;
    
    Reset();
}
#endif

// Split the mask cols, get each mask and upsample it
void
RebalanceMaskPredictorComp::UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf,
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
RebalanceMaskPredictorComp::PredictMask(WDL_TypedBuf<BL_FLOAT> *result,
                                        const WDL_TypedBuf<BL_FLOAT> &mixBufHisto,
                                        DNNModel *model,
                                        BL_FLOAT sampleRate)
{
#if 0
    // TEST: compute spectrogram band 32x256 not every time
    // for optimizing.
    //
    // Result:
    // 4: 100% CPU
    // 8: 75%
    // 16: 60%
    static int count = 0;
    if (count++ % 16/*8*//*16*//*4*/ != 0)
    {
        *result = mixBufHisto;
        
        return;
    }
#endif
    
#if DBG_DUMP_DATA
    //BLDebug::DumpData("mdl-input.txt", input);
    WDL_TypedBuf<BL_FLOAT> normInput = mixBufHisto;
    BLUtils::Normalize(&normInput);
    PPMFile::SavePPM("msk-input.ppm", normInput.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0);
#endif

    
    WDL_TypedBuf<BL_FLOAT> maskBuf;
    model->Predict(mixBufHisto, &maskBuf);
    
    BLUtils::ClipMin(&maskBuf, 0.0);
    
    UpsamplePredictedMask(&maskBuf, sampleRate);
    
    BLUtils::ClipMin(&maskBuf, 0.0);
    
#if DBG_DUMP_DATA
    //BLDebug::DumpData("mdl-input.txt", input);
    WDL_TypedBuf<BL_FLOAT> normOutput = maskBuf;
    BLUtils::Normalize(&normInput);
    PPMFile::SavePPM("msk-output.ppm", normOutput.Get(),
                     1024, NUM_OUTPUT_COLS, 1, 255.0);
#endif
    
    // NEW
    ReduceMaskCols(&maskBuf);
    
    *result = maskBuf;
}

void
RebalanceMaskPredictorComp::ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
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
RebalanceMaskPredictorComp::ComputeMasksComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                                             WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther,
                                             const WDL_TypedBuf<BL_FLOAT> &mixBufHisto)
{
    WDL_TypedBuf<BL_FLOAT> maskVocalFull;
    PredictMask(&maskVocalFull, mixBufHisto, mModelVocal, mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> maskBassFull;
    PredictMask(&maskBassFull, mixBufHisto, mModelBass, mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> maskDrumsFull;
    PredictMask(&maskDrumsFull, mixBufHisto, mModelDrums, mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> maskOtherFull;
    PredictMask(&maskOtherFull, mixBufHisto, mModelOther, mSampleRate);
    
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

void
RebalanceMaskPredictorComp::ComputeLineMaskComp2(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskResult,
                                                 const WDL_TypedBuf<BL_FLOAT> &maskSource)
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
            
            BL_FLOAT m = maskSource.Get()[idx];
            avg.re += m;
        }
        
        avg.re /= numCols;
        
        maskResult->Get()[i] = avg;
    }
}

void
RebalanceMaskPredictorComp::ComputeLineMaskComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskResult,
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
RebalanceMaskPredictorComp::ComputeLineMask2(WDL_TypedBuf<BL_FLOAT> *maskResult,
                                             const WDL_TypedBuf<BL_FLOAT> &maskSource)
{
    int numFreqs = BUFFER_SIZE/2;
    int numCols = NUM_OUTPUT_COLS;
    
    maskResult->Resize(numFreqs);
    
    for (int i = 0; i < numFreqs; i++)
    {
        int idx = i + (numCols - 1)*numFreqs;
        
        BL_FLOAT m = maskSource.Get()[idx];
        
        maskResult->Get()[i] = m;
    }
}

void
RebalanceMaskPredictorComp::ComputeLineMasksSoft(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
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
RebalanceMaskPredictorComp::InitResamplers()
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
RebalanceMaskPredictorComp::CreateModel(const char *modelFileName,
                                        const char *resourcePath,
                                        DNNModel **model)
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
    *model = new DNNModelDarknet();
#endif
    
    (*model)->Load(modelFileName, resourcePath);
}

void
RebalanceMaskPredictorComp::ReduceMaskCols(WDL_TypedBuf<BL_FLOAT> *ioMask)
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
