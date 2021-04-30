//
//  RebalanceMaskPredictor8.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#include <string>
using namespace std;

#include <BLUtils.h>
#include <BLUtilsPlug.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

// Darknet
#include <DNNModelDarknet.h>
#include <RebalanceMaskStack2.h>

#include <MelScale.h>
#include <Scale.h>

#include <Rebalance_defs.h>

#include "RebalanceMaskPredictor8.h"

#define USE_MASK_STACK 1
#define USE_MASK_STACK_METHOD2 1

#define MASK_STACK_DEPTH REBALANCE_NUM_SPECTRO_COLS/2

RebalanceMaskPredictor8::RebalanceMaskPredictor8(int bufferSize,
                                                 BL_FLOAT overlapping,
                                                 BL_FLOAT sampleRate,
                                                 int numSpectroCols,
                                                 const IPluginBase &plug,
                                                 IGraphics &graphics)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    
    mSampleRate = sampleRate;
    
    mNumSpectroCols = numSpectroCols;
    
    //
    mPredictModulo = 0;
    mDontPredictEveryStep = false;
    
#if USE_MASK_STACK
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        mMaskStacks[i] = new RebalanceMaskStack2(REBALANCE_NUM_SPECTRO_FREQS,
                                                 MASK_STACK_DEPTH);
#endif

    mModel = NULL;
    
#ifndef WIN32
    WDL_String resPath;
    BLUtilsPlug::GetFullPlugResourcesPath(plug, &resPath);
    
    const char *resourcePath = resPath.Get();
    
    CreateModel(MODEL_NAME, resourcePath, &mModel);
    
#else // WIN32
    mModel = new DNNModelDarknet();
    mModel->LoadWin(graphics, MODEL_FN, WEIGHTS_FN);
#endif
    
    InitMixCols();
    
    mMaskPredictStepNum = 0;
    
    mMelScale = new MelScale();
    mScale = new Scale();
}

RebalanceMaskPredictor8::~RebalanceMaskPredictor8()
{
    if (mModel != NULL)
        delete mModel;
    
    delete mMelScale;
    delete mScale;
}

void
RebalanceMaskPredictor8::Reset()
{
    MultichannelProcess::Reset();
    
    mMaskPredictStepNum = 0;
    mCurrentMasks.clear();
    
    // NEW
    InitMixCols();
    
#if USE_MASK_STACK
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        mMaskStacks[i]->Reset();
    }
#endif
}

void
RebalanceMaskPredictor8::Reset(int bufferSize, int overlapping,
                               int oversampling, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, overlapping,
                               oversampling, sampleRate);
    
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    
    mSampleRate = sampleRate;
    
    Reset();
}

void
RebalanceMaskPredictor8::
ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() < 1)
        return;
    
    // Take only the left channel...
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &stereoFftSamples = mTmpBuf0;
    stereoFftSamples.resize(ioFftSamples->size());
    for (int i = 0; i < ioFftSamples->size(); i++)
    {
        //monoFftSamples.push_back(*(*ioFftSamples)[i]);
        stereoFftSamples[i] = *(*ioFftSamples)[i];
    }
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples0 = mTmpBuf1;
    BLUtils::StereoToMono(&fftSamples0, stereoFftSamples);
    
    // Stereo to mono
    
    // Take half of the complexes
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples = mTmpBuf16;
    BLUtils::TakeHalf(fftSamples0, &fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf3;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    //
    DownsampleHzToMel(&magns);
    
    // mMixCols is filled with zeros at the origin
    
    //mMixCols.push_back(magns);
    //mMixCols.pop_front();
    mMixCols.push_pop(magns);
    
    WDL_TypedBuf<BL_FLOAT> &mixBufHisto = mTmpBuf4;
    ColumnsToBuffer(&mixBufHisto, mMixCols);
    
    //WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES];
    WDL_TypedBuf<BL_FLOAT> *masks = mTmpBuf5;
    ComputeMasks(masks, mixBufHisto);
    
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        UpsampleMelToHz(&masks[i]);
        
        mMasks[i] = masks[i];
    }
}

bool
RebalanceMaskPredictor8::IsMaskAvailable()
{
    if (mMasks[0].GetSize() == 0)
        return false;
    
    return true;
}

void
RebalanceMaskPredictor8::GetMask(int index, WDL_TypedBuf<BL_FLOAT> *mask)
{
    if ((index < 0) || (index >= NUM_STEM_SOURCES))
        return;
        
    *mask = mMasks[index];
}

void
RebalanceMaskPredictor8::SetPredictModuloNum(int moduloNum)
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

int
RebalanceMaskPredictor8::GetHistoryIndex()
{
    if (!mDontPredictEveryStep)
    {
        int colNum = mNumSpectroCols/2;
        return colNum;
    }
    else
    {
        // Limit the latency
        int colNum = mNumSpectroCols - mPredictModulo;
        
        return colNum;
    }

    return 0;
}

int
RebalanceMaskPredictor8::GetLatency()
{
    int histoIndex = GetHistoryIndex();
    int numBuffers = mNumSpectroCols - histoIndex;
    
    numBuffers = numBuffers - 1;
    if (numBuffers < 0)
        numBuffers = 0;
    
    int latency = (numBuffers*mBufferSize)/mOverlapping;
    
    return latency;
}

void
RebalanceMaskPredictor8::
ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                //const deque<WDL_TypedBuf<BL_FLOAT> > &cols)
                const bl_queue<WDL_TypedBuf<BL_FLOAT> > &cols)
{
    if (cols.empty())
        return;
    
    buf->Resize((int)cols.size()*cols[0].GetSize());
    
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
RebalanceMaskPredictor8::
ComputeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES],
             const WDL_TypedBuf<BL_FLOAT> &mixBufHisto)
{
    //WDL_TypedBuf<BL_FLOAT> masks0[NUM_STEM_SOURCES];
    WDL_TypedBuf<BL_FLOAT> *masks0 = mTmpBuf6;
    if (mDontPredictEveryStep)
    {
        if (mMaskPredictStepNum++ % mPredictModulo == 0)
        {
            vector<WDL_TypedBuf<BL_FLOAT> > &masks0v = mTmpBuf7;
            mModel->Predict(mixBufHisto, &masks0v);
            for (int i = 0; i < NUM_STEM_SOURCES; i++)
                masks0[i] = masks0v[i];
        
            UpdateCurrentMasksAdd(masks0v);
        }
        else
        {
            UpdateCurrentMasksScroll();
        }
    
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
            masks0[k] = mCurrentMasks[k];
    }
    else
    {
        vector<WDL_TypedBuf<BL_FLOAT> > &masks0v = mTmpBuf8;
        mModel->Predict(mixBufHisto, &masks0v);
        
        for (int i = 0; i < NUM_STEM_SOURCES; i++)
            masks0[i] = masks0v[i];
    }
    
#if USE_MASK_STACK
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
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
    
    ComputeLineMasks(masks, masks0,
                     REBALANCE_NUM_SPECTRO_FREQS);
    
    // Theshold, just in case (prediction can return negative mask values)
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        BLUtils::ClipMin(&masks[i], (BL_FLOAT)0.0);
}

void
RebalanceMaskPredictor8::ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
                                         const WDL_TypedBuf<BL_FLOAT> &maskSource,
                                         int numFreqs)
{
    maskResult->Resize(numFreqs);
    BLUtils::FillAllZero(maskResult);

    if (maskSource.GetSize() == 0)
        return;
    
    int colNum = GetHistoryIndex();
    for (int i = 0; i < numFreqs; i++)
    {
        int idx = i + colNum*numFreqs;
        
        BL_FLOAT m = maskSource.Get()[idx];
        
        maskResult->Get()[i] = m;
    }
}

void
RebalanceMaskPredictor8::
ComputeLineMasks(WDL_TypedBuf<BL_FLOAT> masksResult[NUM_STEM_SOURCES],
                 const WDL_TypedBuf<BL_FLOAT> masksSource[NUM_STEM_SOURCES],
                 int numFreqs)
{
    // Compute the lines
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        ComputeLineMask(&masksResult[i], masksSource[i], numFreqs);
}

void
RebalanceMaskPredictor8::CreateModel(const char *modelFileName,
                                     const char *resourcePath,
                                     DNNModel2 **model)
{
    *model = new DNNModelDarknet();
    
    (*model)->Load(modelFileName, resourcePath);
}

void
RebalanceMaskPredictor8::
UpdateCurrentMasksAdd(const vector<WDL_TypedBuf<BL_FLOAT> > &newMasks)
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
RebalanceMaskPredictor8::UpdateCurrentMasksScroll()
{
    int numFreqs = REBALANCE_NUM_SPECTRO_FREQS;
    int numCols = mNumSpectroCols;
    
    for (int i = 0; i < mCurrentMasks.size(); i++)
    {
        for (int k = 0; k < numCols - 1; k++)
        {
            for (int j = 0; j < numFreqs; j++)
            {
                // Scroll
                mCurrentMasks[i].Get()[j + k*numFreqs] =
                    mCurrentMasks[i].Get()[j + (k + 1)*numFreqs];
            }
        }
        
        // Set the last value to 0
        for (int j = 0; j < numFreqs; j++)
            mCurrentMasks[i].Get()[j + (numCols - 1)*numFreqs] = 0.0;
    }
}

void
RebalanceMaskPredictor8::InitMixCols()
{
    //mMixCols.clear();
    mMixCols.resize(mNumSpectroCols);
    
    for (int i = 0; i < mNumSpectroCols; i++)
    {
        WDL_TypedBuf<BL_FLOAT> &col = mTmpBuf9;
        BLUtils::ResizeFillZeros(&col, REBALANCE_NUM_SPECTRO_FREQS);
        
        //mMixCols.push_back(col);
        mMixCols[i] = col;
    }
}

void
RebalanceMaskPredictor8::DownsampleHzToMel(WDL_TypedBuf<BL_FLOAT> *ioMagns)
{
#if REBALANCE_MEL_METHOD_FILTER
    // Origin, use filters
    int numMelBins = REBALANCE_NUM_SPECTRO_FREQS;
    WDL_TypedBuf<BL_FLOAT> &melMagnsFilters = mTmpBuf10;
    melMagnsFilters = *ioMagns;
    mMelScale->HzToMelFilter(&melMagnsFilters, *ioMagns, mSampleRate, numMelBins);
    *ioMagns = melMagnsFilters;
#endif

#if REBALANCE_MEL_METHOD_SCALE
    int numMelBins = REBALANCE_NUM_SPECTRO_FREQS;

    WDL_TypedBuf<BL_FLOAT> &scaledData = mTmpBuf11;
    Scale::FilterBankType type = mScale->TypeToFilterBankType(Scale::MEL_FILTER);
    mScale->ApplyScaleFilterBank(type, &scaledData, *ioMagns,
                                 mSampleRate, numMelBins);
    *ioMagns = scaledData;
#endif
    
#if REBALANCE_MEL_METHOD_SIMPLE
    // Quick method
    BLUtils::ResizeLinear(ioMagns, REBALANCE_NUM_SPECTRO_FREQS);
    WDL_TypedBuf<BL_FLOAT> &melMagnsFilters = mTmpBuf12;
    melMagnsFilters = *ioMagns;
    MelScale::HzToMel(&melMagnsFilters, *ioMagns, mSampleRate);
    *ioMagns = melMagnsFilters;
#endif
}

void
RebalanceMaskPredictor8::UpsampleMelToHz(WDL_TypedBuf<BL_FLOAT> *ioMagns)
{
#if REBALANCE_MEL_METHOD_FILTER
    // Origin method with filters
    int numFreqBins = mBufferSize/2;
    WDL_TypedBuf<BL_FLOAT> &hzMagnsFilters = mTmpBuf13;
    hzMagnsFilters = *ioMagns;
    mMelScale->MelToHzFilter(&hzMagnsFilters, *ioMagns, mSampleRate, numFreqBins);
    *ioMagns = hzMagnsFilters;
#endif

#if REBALANCE_MEL_METHOD_SCALE
    int numFreqBins = mBufferSize/2;
    
    WDL_TypedBuf<BL_FLOAT> &scaledData = mTmpBuf14;
    Scale::FilterBankType type = mScale->TypeToFilterBankType(Scale::MEL_FILTER);
    mScale->ApplyScaleFilterBankInv(type, &scaledData, *ioMagns,
                                    mSampleRate, numFreqBins);
    *ioMagns = scaledData;
#endif
    
#if REBALANCE_MEL_METHOD_SIMPLE
    // Quick method
    BLUtils::ResizeLinear(ioMagns, mBufferSize/2);
    WDL_TypedBuf<BL_FLOAT> &melMagnsFilters = mTmpBuf15;
    melMagnsFilters = *ioMagns;
    MelScale::MelToHz(&melMagnsFilters, *ioMagns, mSampleRate);
    *ioMagns = melMagnsFilters;
#endif
}
