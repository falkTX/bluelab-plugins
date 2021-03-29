//
//  RebalanceMaskPredictorComp7.cpp
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
//#include <DNNModelDarknetMc.h>
#include <DNNModelDarknet.h>
#include <RebalanceMaskStack2.h>

#include <MelScale.h>

#include "RebalanceMaskPredictorComp7.h"

// See Rebalance
//#define EPS 1e-10

// With 100, the slope is very steep
// (but the sound seems similar to 10).
#define MAX_GAMMA 10.0 //20.0 //100.0

// GOOD!
// Less pumping with it!
// When using gamma=10, separation is better (but more gating)
#define USE_MASK_STACK 1
#define USE_MASK_STACK_METHOD2 1
//#define MASK_STACK_DEPTH NUM_INPUT_COLS
#define MASK_STACK_DEPTH REBALANCE_NUM_SPECTRO_COLS/2

#define SOFT_SENSITIVITY 1 //0

RebalanceMaskPredictorComp7::RebalanceMaskPredictorComp7(int bufferSize,
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
    
    // Parameters
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        mSensitivities[i] = 1.0;
    
    // Masks contrasts, relative one to each other (soft/hard)
    mMasksContrast = 0.0;
    
#if USE_MASK_STACK
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        mMaskStacks[i] = new RebalanceMaskStack2(REBALANCE_NUM_SPECTRO_FREQS,
                                                 //REBALANCE_TARGET_BUFFER_SIZE,
                                                 //mBufferSize/2,
                                                 MASK_STACK_DEPTH);
#endif
    
#ifndef WIN32
    WDL_String resPath;
    BLUtilsPlug::GetFullPlugResourcesPath(plug, &resPath);
    
    const char *resourcePath = resPath.Get();
    
    CreateModel(MODEL_NAME, resourcePath, &mModel);
    
#else // WIN32
    //mModel = new DNNModelDarknetMc();
    mModel = new DNNModelDarknet();
    mModel->LoadWin(graphics, MODEL_FN, WEIGHTS_FN);
#endif
    
    InitMixCols();
    
    mMaskPredictStepNum = 0;
    
    mMelScale = new MelScale();
}

RebalanceMaskPredictorComp7::~RebalanceMaskPredictorComp7()
{
    delete mModel;
    
    delete mMelScale;
}

void
RebalanceMaskPredictorComp7::Reset()
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
RebalanceMaskPredictorComp7::Reset(int bufferSize, int overlapping,
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
RebalanceMaskPredictorComp7::
ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() < 1)
        return;
    
    // Take only the left channel...
    //WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *(*ioFftSamples)[0];
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > monoFftSamples;
    for (int i = 0; i < ioFftSamples->size(); i++)
    {
        monoFftSamples.push_back(*(*ioFftSamples)[i]);
    }
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    BLUtils::StereoToMono(&fftSamples, monoFftSamples);
    
    // Stereo to mono
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    //
    DownsampleHzToMel(&magns);
    
    // mMixCols is filled with zeros at the origin
    mMixCols.push_back(magns);
    mMixCols.pop_front();
    
    WDL_TypedBuf<BL_FLOAT> mixBufHisto;
    ColumnsToBuffer(&mixBufHisto, mMixCols);
    
    WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES];
    ComputeMasks(masks, mixBufHisto);
    
    // NEW
    // NOTE: previously, when setting Other sensitivity to 0,
    // there was no change (in soft mode only), than with sensitivity set
    // to 100
    ApplySensitivity(masks);
    
    NormalizeMasks(masks);
    
    ApplyMasksContrast(masks);
    
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        UpdsampleMelToHz(&masks[i]);
        
        mMasks[i] = masks[i];
    }
}

bool
RebalanceMaskPredictorComp7::IsMaskAvailable()
{
    if (mMasks[0].GetSize() == 0)
        return false;
    
    return true;
}

void
RebalanceMaskPredictorComp7::GetMask(int index, WDL_TypedBuf<BL_FLOAT> *mask)
{
    if ((index < 0) || (index >= NUM_STEM_SOURCES))
        return;
        
    *mask = mMasks[index];
}

void
RebalanceMaskPredictorComp7::SetPredictModuloNum(int moduloNum)
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
RebalanceMaskPredictorComp7::GetHistoryIndex()
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
RebalanceMaskPredictorComp7::GetLatency()
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
RebalanceMaskPredictorComp7::SetMasksContrast(BL_FLOAT contrast)
{
    mMasksContrast = contrast;
}

#if 0
void
RebalanceMaskPredictorComp7::SetDbgThreshold(BL_FLOAT thrs)
{
    mModel->SetDbgThreshold(thrs);
}
#endif

void
RebalanceMaskPredictorComp7::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
#if !SENSIVITY_IS_SCALE
    mSensitivities[0] = vocalSensitivity;
#else
    mModel->SetMaskScale(0, vocalSensitivity);
#endif
}

void
RebalanceMaskPredictorComp7::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
#if !SENSIVITY_IS_SCALE
    mSensitivities[1] = bassSensitivity;
#else
    mModel->SetMaskScale(1, bassSensitivity);
#endif
}

void
RebalanceMaskPredictorComp7::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
#if !SENSIVITY_IS_SCALE
    mSensitivities[2] = drumsSensitivity;
#else
    mModel->SetMaskScale(2, drumsSensitivity);
#endif
}

void
RebalanceMaskPredictorComp7::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
#if !SENSIVITY_IS_SCALE
    mSensitivities[3] = otherSensitivity;
#else
    mModel->SetMaskScale(3, otherSensitivity);
#endif
}

void
RebalanceMaskPredictorComp7::
ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                const deque<WDL_TypedBuf<BL_FLOAT> > &cols)
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
RebalanceMaskPredictorComp7::
ColumnsToBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                const deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &cols)
{
    if (cols.empty())
        return;
    
    buf->Resize((int)(cols.size()*cols[0].GetSize()));
    
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
RebalanceMaskPredictorComp7::
ComputeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES],
             const WDL_TypedBuf<BL_FLOAT> &mixBufHisto)
{
    WDL_TypedBuf<BL_FLOAT> masks0[NUM_STEM_SOURCES];
    if (mDontPredictEveryStep)
    {
        if (mMaskPredictStepNum++ % mPredictModulo == 0)
        {
            vector<WDL_TypedBuf<BL_FLOAT> > masks0v;
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
        vector<WDL_TypedBuf<BL_FLOAT> > masks0v;
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
RebalanceMaskPredictorComp7::ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
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
RebalanceMaskPredictorComp7::
ComputeLineMasks(WDL_TypedBuf<BL_FLOAT> masksResult[NUM_STEM_SOURCES],
                 const WDL_TypedBuf<BL_FLOAT> masksSource[NUM_STEM_SOURCES],
                 int numFreqs)
{
    // Compute the lines
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        ComputeLineMask(&masksResult[i], masksSource[i], numFreqs);
}

void
RebalanceMaskPredictorComp7::CreateModel(const char *modelFileName,
                                         const char *resourcePath,
                                         //DNNModelMc **model)
                                         DNNModel2 **model)
{
    //*model = new DNNModelDarknetMc();
    *model = new DNNModelDarknet();
    
    (*model)->Load(modelFileName, resourcePath);
}

void
RebalanceMaskPredictorComp7::
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
RebalanceMaskPredictorComp7::UpdateCurrentMasksScroll()
{
    //int numFreqs = mBufferSize/2;
    int numFreqs = REBALANCE_NUM_SPECTRO_FREQS;
    int numCols = mNumSpectroCols;
    
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
RebalanceMaskPredictorComp7::
ApplySensitivity(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES])
{
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        BL_FLOAT vals[NUM_STEM_SOURCES];
        for (int j = 0; j < NUM_STEM_SOURCES; j++)
            vals[j] = masks[j].Get()[i];
        
#if !SOFT_SENSITIVITY
        ApplySensitivityHard(vals);
#else
        ApplySensitivitySoft(vals);
#endif
        
        for (int j = 0; j < NUM_STEM_SOURCES; j++)
            masks[j].Get()[i] = vals[j];
    }
}

void
RebalanceMaskPredictorComp7::ApplySensitivityHard(BL_FLOAT masks[NUM_STEM_SOURCES])
{
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        if (masks[i] < (1.0 - mSensitivities[i]))
        {
            masks[i] = 0.0;
        }
    }
}

void
RebalanceMaskPredictorComp7::ApplySensitivitySoft(BL_FLOAT masks[NUM_STEM_SOURCES])
{
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        masks[i] *= mSensitivities[i];
    }
}

void
RebalanceMaskPredictorComp7::
NormalizeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES])
{
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        BL_FLOAT vals[NUM_STEM_SOURCES];
        
        // Init
        for (int j = 0; j < NUM_STEM_SOURCES; j++)
            vals[j] = masks[j].Get()[i];
        
        BL_FLOAT sum = vals[0] + vals[1] + vals[2] + vals[3];
        
        if (sum > BL_EPS)
        {
            BL_FLOAT tvals[NUM_STEM_SOURCES];
            
            for (int j = 0; j < NUM_STEM_SOURCES; j++)
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
RebalanceMaskPredictorComp7::
ApplyMasksContrast(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES])
{
    vector<MaskContrastStruct> mc;
    mc.resize(NUM_STEM_SOURCES);
    
    BL_FLOAT gamma = 1.0 + mMasksContrast*(MAX_GAMMA - 1.0);
    
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        // Fill the structure
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mMaskId = k;
            mc[k].mValue = masks[k].Get()[i];
        }
        
        // Sort
        sort(mc.begin(), mc.end(), MaskContrastStruct::ValueSmaller);
        
        // Normalize
        BL_FLOAT minValue = mc[0].mValue;
        BL_FLOAT maxValue = mc[3].mValue;
        
        if (std::fabs(maxValue - minValue) < BL_EPS)
            continue;
        
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mValue = (mc[k].mValue - minValue)/(maxValue - minValue);
        }
        
        // Apply gamma
        // See: https://www.researchgate.net/figure/Gamma-curves-where-X-represents-the-normalized-pixel-intensity_fig1_280851965
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mValue = std::pow(mc[k].mValue, gamma);
        }
        
        // Un-normalize
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mValue *= maxValue;
        }
        
        // Result
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            masks[mc[k].mMaskId].Get()[i] = mc[k].mValue;
        }
    }
}

void
RebalanceMaskPredictorComp7::InitMixCols()
{
    mMixCols.clear();
    
    for (int i = 0; i < mNumSpectroCols; i++)
    {
        WDL_TypedBuf<BL_FLOAT> col;
        BLUtils::ResizeFillZeros(&col,
                                 REBALANCE_NUM_SPECTRO_FREQS);
                                 //mBufferSize/2);
        
        mMixCols.push_back(col);
    }
}

void
RebalanceMaskPredictorComp7::DownsampleHzToMel(WDL_TypedBuf<BL_FLOAT> *ioMagns)
{
#if REBALANCE_USE_MEL_FILTER_METHOD
    // Origin, use filters
    int numMelBins = REBALANCE_NUM_SPECTRO_FREQS;
    WDL_TypedBuf<BL_FLOAT> melMagnsFilters = *ioMagns;
    mMelScale->HzToMelFilter(&melMagnsFilters, *ioMagns, mSampleRate, numMelBins);
    *ioMagns = melMagnsFilters;
#else 
    // Quick method
    BLUtils::ResizeLinear(ioMagns, REBALANCE_NUM_SPECTRO_FREQS);
    WDL_TypedBuf<BL_FLOAT> melMagnsFilters = *ioMagns;
    MelScale::HzToMel(&melMagnsFilters, *ioMagns, mSampleRate);
    *ioMagns = melMagnsFilters;
#endif
    
#if 0 // DEBUG
    BLDebug::DumpData("hz0.txt", *ioMagns);
    
    WDL_TypedBuf<BL_FLOAT> melMagns = *ioMagns;
    MelScale::HzToMel(&melMagns, *ioMagns, mSampleRate);
    BLDebug::DumpData("mel0.txt", melMagns);
    
    WDL_TypedBuf<BL_FLOAT> hzMagns0;
    MelScale::MelToHz(&hzMagns0, melMagns, mSampleRate);
    BLDebug::DumpData("hz1.txt", hzMagns0);
    
    int numMelBins = ioMagns->GetSize();
    WDL_TypedBuf<BL_FLOAT> melMagnsFilters = *ioMagns;
    mMelScale->HzToMelFilter(&melMagnsFilters, *ioMagns, mSampleRate, numMelBins);
    BLDebug::DumpData("mel1.txt", melMagnsFilters);
    
    // TEST
    WDL_TypedBuf<BL_FLOAT> hzMagns1;
    MelScale::MelToHz(&hzMagns1, melMagnsFilters, mSampleRate);
    BLDebug::DumpData("hz2.txt", hzMagns1);
    
    WDL_TypedBuf<BL_FLOAT> hzMagns2;
    mMelScale->MelToHzFilter(&hzMagns2, melMagnsFilters, mSampleRate, numMelBins);
    BLDebug::DumpData("hz3.txt", hzMagns2);
#endif
}

void
RebalanceMaskPredictorComp7::UpdsampleMelToHz(WDL_TypedBuf<BL_FLOAT> *ioMagns)
{
#if REBALANCE_USE_MEL_FILTER_METHOD
    // Origin method with filters
    int numFreqBins = mBufferSize/2;
    WDL_TypedBuf<BL_FLOAT> hzMagnsFilters = *ioMagns;
    mMelScale->MelToHzFilter(&hzMagnsFilters, *ioMagns, mSampleRate, numFreqBins);
    *ioMagns = hzMagnsFilters;
#else
    // Quick method
    BLUtils::ResizeLinear(ioMagns, mBufferSize/2);
    WDL_TypedBuf<BL_FLOAT> melMagnsFilters = *ioMagns;
    MelScale::MelToHz(&melMagnsFilters, *ioMagns, mSampleRate);
    *ioMagns = melMagnsFilters;
#endif
}
