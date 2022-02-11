//
//  RebalanceMaskPredictorComp4.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#include <string>
using namespace std;

#include <BLUtils.h>
#include <PPMFile.h>

#include "RebalanceMaskPredictorComp4.h"

// Darknet
#include <DNNModelDarknetMc.h>

#include <RebalanceMaskStack2.h>


// See Rebalance
#define EPS 1e-10

// Inverse mel scale  was not applied if RESAMPLE_FACTOR was 1
#define FIX_UPSAMPLE_MEL 1

// Keep to 0 for the moment, will set it to 1 if we re-train the model (and re-generate the data)
#define FIX_MEL_DOWNSAMPLING 0
// NOTE: the change is very small with or without (checked with bass mask)
#define FIX_MEL_UPSAMPLING 0 //1

// With 100, the slope is very steep
// (but the sound seems similar to 10).
#define MAX_GAMMA 10.0 //20.0 //100.0

#define DEFAULT_MODEL_NUM 3

// GOOD!
// Less pumping with it!
// When using gamma=10, separation is better (but more gating)
#define USE_MASK_STACK 1
#define USE_MASK_STACK_METHOD2 1
//#define MASK_STACK_DEPTH NUM_INPUT_COLS
#define MASK_STACK_DEPTH NUM_INPUT_COLS/2

#define OPTIM_UPSAMPLE 1


RebalanceMaskPredictorComp4::RebalanceMaskPredictorComp4(int bufferSize,
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
    
    mModelNum = DEFAULT_MODEL_NUM;
    
    //
    mPredictModulo = 0;
    mDontPredictEveryStep = false;
    
    // Parameters
    for (int i = 0; i < 4; i++)
        mSensitivities[i] = 1.0;
    
    // Masks contrasts, relative one to each other (soft/hard)
    mMasksContrast = 0.0;
    
#if USE_MASK_STACK
    for (int i = 0; i < 4; i++)
        mMaskStacks[i] = new RebalanceMaskStack2(BUFFER_SIZE/(2*RESAMPLE_FACTOR),
                                                 MASK_STACK_DEPTH);
#endif
    
#ifndef WIN32
    WDL_String resPath;
    //graphics->GetResourceDir(&resPath);
    //const char *resourcePath = resPath.Get();
   const char *resourcePath = graphics->GetSharedResourcesSubPath();
    
    // Vocal
    CreateModel(MODEL_4X, resourcePath, &mModels[0]);
    
    // Bass
    CreateModel(MODEL_8X, resourcePath, &mModels[1]);
    
    // Drums
    CreateModel(MODEL_16X, resourcePath, &mModels[2]);
    
    // Other
    CreateModel(MODEL_32X, resourcePath, &mModels[3]);
    
#else // WIN32
    mModels[0] = new DNNModelDarknet();
    mModels[1] = new DNNModelDarknet();
    mModels[2] = new DNNModelDarknet();
    mModels[3] = new DNNModelDarknet();
    
    mModels[0]->LoadWin(graphics, MODEL0_ID, WEIGHTS0_ID);
    mModels[1]->LoadWin(graphics, MODEL1_ID, WEIGHTS1_ID);
    mModels[2]->LoadWin(graphics, MODEL2_ID, WEIGHTS2_ID);
    mModels[3]->LoadWin(graphics, MODEL3_ID, WEIGHTS3_ID);
#endif
    
    InitMixCols();
    
#if FORCE_SAMPLE_RATE
    InitResamplers();
    
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
    mMaskPredictStepNum = 0;
}

RebalanceMaskPredictorComp4::~RebalanceMaskPredictorComp4()
{
    for (int i = 0; i < 4; i++)
        delete mModels[i];
    
#if USE_MASK_STACK
    for (int i = 0; i < 4; i++)
        delete mMaskStacks[i];
#endif
}

void
RebalanceMaskPredictorComp4::Reset()
{
    MultichannelProcess::Reset();
    
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
    
    mMaskPredictStepNum = 0;
    mCurrentMasks.clear();
    
    // NEW
    InitMixCols();
    
    for (int i = 0; i < 4; i++)
        mMaskStacks[i]->Reset();
}

void
RebalanceMaskPredictorComp4::Reset(int bufferSize, int overlapping,
                                   int oversampling, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, overlapping,
                               oversampling, sampleRate);
    
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    // NEW
    Reset();
}

// GOOD (even for 192000Hz, block size 64)
#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictorComp4::ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
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

void
RebalanceMaskPredictorComp4::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
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
    
    //
    
    // Compute the masks
    WDL_TypedBuf<BL_FLOAT> magnsDown = magns;
    Downsample(&magnsDown, mSampleRate);
    
    // mMixCols is filled with zeros at the origin
    mMixCols.push_back(magnsDown);
    mMixCols.pop_front();
    
    WDL_TypedBuf<BL_FLOAT> mixBufHisto;
    ColumnsToBuffer(&mixBufHisto, mMixCols);
    
    WDL_TypedBuf<BL_FLOAT> masks[4];
    ComputeMasks(masks, mixBufHisto);
    
    // NEW
    // NOTE: previously, when setting Other sensitivity to 0,
    // there was no change (in soft mode only), than with sensitivity set
    // to 100
    ApplySensitivity(masks);
    
    NormalizeMasks(masks);
    
    ApplyMasksContrast(masks);
    
    for (int i = 0; i < 4; i++)
        mMasks[i] = masks[i];
}

void
RebalanceMaskPredictorComp4::GetMask(int index, WDL_TypedBuf<BL_FLOAT> *mask)
{
    if ((index < 0) || (index > 3))
        return;
        
    *mask = mMasks[index];
}

void
RebalanceMaskPredictorComp4::Downsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                        BL_FLOAT sampleRate)
{
#if FORCE_SAMPLE_RATE
    sampleRate = SAMPLE_RATE;
#endif
    
    BL_FLOAT hzPerBin = sampleRate/(BUFFER_SIZE/2);
    WDL_TypedBuf<BL_FLOAT> bufMel;
    
#if !FIX_MEL_DOWNSAMPLING
    BLUtils::FreqsToMelNorm(&bufMel, *ioBuf, hzPerBin);
#else
    BLUtils::FreqsToMelNorm2(&bufMel, *ioBuf, hzPerBin);
#endif
    
    *ioBuf = bufMel;
    
    if (RESAMPLE_FACTOR == 1)
        // No need to downsample
        return;
    
    int newSize = ioBuf->GetSize()/RESAMPLE_FACTOR;
    
    BLUtils::ResizeLinear(ioBuf, newSize);
}

void
RebalanceMaskPredictorComp4::Upsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
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
    
#if !FIX_MEL_UPSAMPLING
    BLUtils::MelToFreqsNorm(&bufMel, *ioBuf, hzPerBin);
#else
    BLUtils::MelToFreqsNorm2(&bufMel, *ioBuf, hzPerBin);
#endif
    
    *ioBuf = bufMel;
}

#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictorComp4::SetPlugSampleRate(BL_FLOAT sampleRate)
{
    mPlugSampleRate = sampleRate;
    
    InitResamplers();
}
#endif

void
RebalanceMaskPredictorComp4::SetModelNum(int modelNum)
{
    if ((modelNum < 0) || (modelNum > 3))
        return;
    
    mModelNum = modelNum;
}

void
RebalanceMaskPredictorComp4::SetPredictModuloNum(int moduloNum)
{
    switch(moduloNum)
    {
        case 0:
        {
            mPredictModulo = 0;
            mDontPredictEveryStep = false;
        }
        break;
            
        case 1:
        {
            mPredictModulo = 4;
            mDontPredictEveryStep = true;
        }
        break;
            
        case 2:
        {
            mPredictModulo = 8;
            mDontPredictEveryStep = true;
        }
            break;
            
        case 3:
        {
            mPredictModulo = 16;
            mDontPredictEveryStep = true;
        }
        break;
            
        default:
            break;
    }
}

void
RebalanceMaskPredictorComp4::SetQuality(Quality quality)
{
    // NOTE: modulo is "reversed".
    // 0 means faster, means modulo 16
    //
    switch(quality)
    {
        case QUALITY_0_0:
        {
            SetModelNum(0);
            SetPredictModuloNum(3);
        }
        break;
            
        case QUALITY_1_0:
        {
            SetModelNum(1);
            SetPredictModuloNum(3);
        }
        break;
            
        case QUALITY_1_1:
        {
            SetModelNum(1);
            SetPredictModuloNum(2);
        }
        break;
        
        case QUALITY_1_3:
        {
            SetModelNum(1);
            SetPredictModuloNum(0);
        }
        break;
    
        case QUALITY_2_3:
        {
            SetModelNum(2);
            SetPredictModuloNum(0);
        }
        break;
        
        case QUALITY_3_3:
        {
            SetModelNum(3);
            SetPredictModuloNum(0);
        }
        break;
            
        default:
            break;
    }
}

int
RebalanceMaskPredictorComp4::GetHistoryIndex()
{
    if (!mDontPredictEveryStep)
    {
        int colNum = NUM_OUTPUT_COLS/2;
        return colNum;
    }
    else
    {
        // Limit the latency
        int colNum = NUM_OUTPUT_COLS - mPredictModulo;
    
        // Do not take the border line...
        //int colNum = NUM_OUTPUT_COLS - mPredictModulo - mPredictModulo/2;
        
        return colNum;
    }

    return 0;
}

int
RebalanceMaskPredictorComp4::GetLatency()
{
    int histoIndex = GetHistoryIndex();
    
    int numBuffers = NUM_OUTPUT_COLS - histoIndex;
    
    // GOOD!
    numBuffers = numBuffers - 1;
    if (numBuffers < 0)
        numBuffers = 0;
    
    int latency = (numBuffers*mBufferSize)/mOverlapping;
    
    return latency;
}

void
RebalanceMaskPredictorComp4::SetMasksContrast(BL_FLOAT contrast)
{
    mMasksContrast = contrast;
}

// Split the mask cols, get each mask and upsample it
void
RebalanceMaskPredictorComp4::UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                                   BL_FLOAT sampleRate, int numCols)
{
#if !FIX_UPSAMPLE_MEL
    if (RESAMPLE_FACTOR == 1)
        // No need to upsample
        return;
#endif
    
    WDL_TypedBuf<BL_FLOAT> result;
    for (int j = 0; j < numCols; j++)
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
RebalanceMaskPredictorComp4::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
    mSensitivities[0] = vocalSensitivity;
}

void
RebalanceMaskPredictorComp4::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
    mSensitivities[1] = bassSensitivity;
}

void
RebalanceMaskPredictorComp4::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
    mSensitivities[2] = drumsSensitivity;
}

void
RebalanceMaskPredictorComp4::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
    mSensitivities[3] = otherSensitivity;
}

void
RebalanceMaskPredictorComp4::ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
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
RebalanceMaskPredictorComp4::ColumnsToBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
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

void
RebalanceMaskPredictorComp4::ComputeMasks(WDL_TypedBuf<BL_FLOAT> masks[4],
                                          const WDL_TypedBuf<BL_FLOAT> &mixBufHisto)
{
    WDL_TypedBuf<BL_FLOAT> masks0[4];
    
    if (mDontPredictEveryStep)
    {
        if (mMaskPredictStepNum++ % mPredictModulo == 0)
        {
            vector<WDL_TypedBuf<BL_FLOAT> > masks0v;
            mModels[mModelNum]->Predict(mixBufHisto, &masks0v);
            for (int i = 0; i < 4; i++)
                masks0[i] = masks0v[i];
        
            UpdateCurrentMasksAdd(masks0v);
        }
        else
        {
            UpdateCurrentMasksScroll();
        }
    
        for (int k = 0; k < 4; k++)
            masks0[k] = mCurrentMasks[k];
    }
    else
    {
        vector<WDL_TypedBuf<BL_FLOAT> > masks0v;
        mModels[mModelNum]->Predict(mixBufHisto, &masks0v);
    
        for (int i = 0; i < 4; i++)
            masks0[i] = masks0v[i];
    }
    
#if USE_MASK_STACK
    for (int i = 0; i < 4; i++)
    {
        mMaskStacks[i]->AddMask(masks0[i]);
        
#if !USE_MASK_STACK_METHOD2
        mMaskStacks[i]->GetMaskAvg(&masks0[i]);
#else
        // Seems good!
        int index = GetHistoryIndex();
        mMaskStacks[i]->GetMaskWeightedAvg(&masks0[i], index);
#endif
    }
#endif
    
#if !OPTIM_UPSAMPLE
    for (int i = 0; i < 4; i++)
        UpsamplePredictedMask(&masks0[i], mSampleRate, NUM_OUTPUT_COLS);
    
    // Theshold, just in case (prediction can return negative mask values)
    for (int i = 0; i < 4; i++)
        BLUtils::ClipMin(&masks0[i], 0.0);
    
    ComputeLineMasks(masks, masks0, BUFFER_SIZE/2);
#else
    ComputeLineMasks(masks, masks0, BUFFER_SIZE/(2*RESAMPLE_FACTOR));
    
    for (int i = 0; i < 4; i++)
        UpsamplePredictedMask(&masks[i], mSampleRate, 1);
    
    // Theshold, just in case (prediction can return negative mask values)
    for (int i = 0; i < 4; i++)
        BLUtils::ClipMin(&masks[i], 0.0);
#endif
}

void
RebalanceMaskPredictorComp4::ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
                                             const WDL_TypedBuf<BL_FLOAT> &maskSource,
                                             int numFreqs)
{
    int colNum = GetHistoryIndex();
    
    maskResult->Resize(numFreqs);
    
    for (int i = 0; i < numFreqs; i++)
    {
        int idx = i + colNum*numFreqs;
        
        BL_FLOAT m = maskSource.Get()[idx];
        
        maskResult->Get()[i] = m;
    }
}

void
RebalanceMaskPredictorComp4::ComputeLineMasks(WDL_TypedBuf<BL_FLOAT> masksResult[4],
                                              const WDL_TypedBuf<BL_FLOAT> masksSource[4],
                                              int numFreqs)
{
    // Compute the lines
    for (int i = 0; i < 4; i++)
        ComputeLineMask(&masksResult[i], masksSource[i], numFreqs);
}

#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictorComp4::InitResamplers()
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
RebalanceMaskPredictorComp4::CreateModel(const char *modelFileName,
                                         const char *resourcePath,
                                         DNNModelMc **model)
{
    *model = new DNNModelDarknetMc();
    
    (*model)->Load(modelFileName, resourcePath);
}

void
RebalanceMaskPredictorComp4::UpdateCurrentMasksAdd(const vector<WDL_TypedBuf<BL_FLOAT> > &newMasks)
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
RebalanceMaskPredictorComp4::UpdateCurrentMasksScroll()
{
    int numFreqs = BUFFER_SIZE/(2*RESAMPLE_FACTOR);
    int numCols = NUM_OUTPUT_COLS;
    
    for (int i = 0; i < mCurrentMasks.size(); i++)
    {
        for (int k = 0; k < numCols - 1; k++)
        {
            for (int j = 0; j < numFreqs; j++)
            {
                // Scroll
                mCurrentMasks[i].Get()[j + k*numFreqs] = mCurrentMasks[i].Get()[j + (k + 1)*numFreqs];
            }
        }
        
        // Set the last value to 0
        for (int j = 0; j < numFreqs; j++)
            mCurrentMasks[i].Get()[j + (numCols - 1)*numFreqs] = 0.0;
    }
}

void
RebalanceMaskPredictorComp4::ApplySensitivity(WDL_TypedBuf<BL_FLOAT> masks[4])
{
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        BL_FLOAT vals[4];
        for (int j = 0; j < 4; j++)
            vals[j] = masks[j].Get()[i];
        
        ApplySensitivity(vals);
        
        for (int j = 0; j < 4; j++)
            masks[j].Get()[i] = vals[j];
    }
}

void
RebalanceMaskPredictorComp4::ApplySensitivity(BL_FLOAT masks[4])
{
    for (int i = 0; i < 4; i++)
    {
        if (masks[i] < mSensitivities[i])
        {
            masks[i] = 0.0;
        }
    }
}

void
RebalanceMaskPredictorComp4::NormalizeMasks(WDL_TypedBuf<BL_FLOAT> masks[4])
{
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        BL_FLOAT vals[4];
        
        // Init
        for (int j = 0; j < 4; j++)
            vals[j] = masks[j].Get()[i];
        
        BL_FLOAT sum = vals[0] + vals[1] + vals[2] + vals[3];
        
        if (sum > EPS)
        {
            BL_FLOAT tvals[4];
            
            for (int j = 0; j < 4; j++)
            {
                tvals[j] = vals[j]/sum;
                
                masks[j].Get()[i] = tvals[j];
            }
        }
    }
}

// GOOD!
// NOTE: With gamma=10, it is almost like keeping only the maximum value
//
void
RebalanceMaskPredictorComp4::ApplyMasksContrast(WDL_TypedBuf<BL_FLOAT> masks[4])
{
#define EPS 1e-15
    
    vector<MaskContrastStruct> mc;
    mc.resize(4);
    
    BL_FLOAT gamma = 1.0 + mMasksContrast*(MAX_GAMMA - 1.0);
    
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        // Fill the structure
        for (int k = 0; k < 4; k++)
        {
            mc[k].mMaskId = k;
            mc[k].mValue = masks[k].Get()[i];
        }
        
        // Sort
        sort(mc.begin(), mc.end(), MaskContrastStruct::ValueSmaller);
        
        // Normalize
        BL_FLOAT minValue = mc[0].mValue;
        BL_FLOAT maxValue = mc[3].mValue;
        
        if (std::fabs(maxValue - minValue) < EPS)
            continue;
        
        for (int k = 0; k < 4; k++)
        {
            mc[k].mValue = (mc[k].mValue - minValue)/(maxValue - minValue);
        }
        
        // Apply gamma
        // See: https://www.researchgate.net/figure/Gamma-curves-where-X-represents-the-normalized-pixel-intensity_fig1_280851965
        for (int k = 0; k < 4; k++)
        {
	  mc[k].mValue = std::pow(mc[k].mValue, gamma);
        }
        
        // Un-normalize
        for (int k = 0; k < 4; k++)
        {
            mc[k].mValue *= maxValue;
        }
        
        // Result
        for (int k = 0; k < 4; k++)
        {
            masks[mc[k].mMaskId].Get()[i] = mc[k].mValue;
        }
    }
}

void
RebalanceMaskPredictorComp4::InitMixCols()
{
    mMixCols.clear();
    
    for (int i = 0; i < NUM_INPUT_COLS; i++)
    {
        WDL_TypedBuf<BL_FLOAT> col;
        BLUtils::ResizeFillZeros(&col, mBufferSize/(2*RESAMPLE_FACTOR));
        
        mMixCols.push_back(col);
    }
}
