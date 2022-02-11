//
//  FilterSincConvoBandPass.cpp
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#include <Window.h>
#include <FastRTConvolver3.h>

#include <BLUtils.h>

#include "FilterSincConvoBandPass.h"

// That was for SincConvoLPFFilter and to be sure we filterd Nyquist correctly
#define NYQUIST_TYPE_LF 0


FilterSincConvoBandPass::FilterSincConvoBandPass()
{
    mConvolver = NULL;
    
    mFL = 0.0;
    mFH = 0.0;
    
    mFilterSize = DEFAULT_FILTER_SIZE;
}

FilterSincConvoBandPass::FilterSincConvoBandPass(const FilterSincConvoBandPass &other)
{
    mConvolver = new FastRTConvolver3(*other.mConvolver);
    
    mFL = other.mFL;
    mFH = other.mFH;
    
    mFilterSize = other.mFilterSize;
    
    mFilterData = other.mFilterData;
}

FilterSincConvoBandPass::~FilterSincConvoBandPass()
{
    if (mConvolver != NULL)
        delete mConvolver;
}

void
FilterSincConvoBandPass::Init(BL_FLOAT fl, BL_FLOAT fh, BL_FLOAT sampleRate, int filterSize)
{
    mFL = fl;
    mFH = fh;
    mFilterSize = filterSize;
    
    mFilterData.Resize(filterSize);
    
    // TODO: adjust well the cut frequency, so that the totally attenuated signal matches exactly fc
    // NOTE: for the moment, this is very approximative, and depends on filterSize
    
    // Low pass
    //
    
#if NYQUIST_TYPE_LF
    // Adjust cut frequency, so that the signal becomes totally attenuated exactly at fc
    //BL_FLOAT bandwidth = (4.0/filterSize)*sampleRate;
    BL_FLOAT bandwidth = (4.6/filterSize)*sampleRate; // 4.6 for Blackman
    
    // Niko
    bandwidth *= 0.5;
    
    fh -= bandwidth;
    if (fh < 0.0)
        fh = 0.0;
#endif
    
    WDL_TypedBuf<BL_FLOAT> filterDataLow;
    ComputeFilter(fh, filterSize, sampleRate, &filterDataLow, false);
    
    // NOTE: the HPF and the LPF center peaks are not exactly the same
    // (1 x value of difference)
    
    // High pass
    WDL_TypedBuf<BL_FLOAT> filterDataHigh;
    ComputeFilter(fl, filterSize, sampleRate, &filterDataHigh, true);

    // Default
    //BLUtils::Convolve(filterDataHigh, filterDataLow, &mFilterData, CONVO_MODE_FULL);
    BLUtils::Convolve(filterDataHigh, filterDataLow, &mFilterData, CONVO_MODE_SAME);
    
    /*BLDebug::DumpData("lpf.txt", filterDataLow);
    BLDebug::DumpData("hpf.txt", filterDataHigh);
    BLDebug::DumpData("conv.txt", mFilterData);*/
                    
    //
    if (mConvolver != NULL)
        delete mConvolver;
    mConvolver = new FastRTConvolver3(filterSize, sampleRate, mFilterData);
}

void
FilterSincConvoBandPass::Reset(BL_FLOAT sampleRate, int blockSize)
{
    if (mConvolver != NULL)
        mConvolver->Reset(sampleRate, blockSize);
    
    Init(mFL, mFH, sampleRate, mFilterSize);
}

int
FilterSincConvoBandPass::GetLatency()
{
    if (mConvolver == NULL)
        return 0;
        
    int latency = mConvolver->GetLatency();
    
    return latency;
}

void
FilterSincConvoBandPass::Process(WDL_TypedBuf<BL_FLOAT> *result,
                                 const WDL_TypedBuf<BL_FLOAT> &samples)
{
    if (mConvolver == NULL)
    {
        *result = samples;
        
        return;
    }
    
    mConvolver->Process(samples, result);
}

void
FilterSincConvoBandPass::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    WDL_TypedBuf<BL_FLOAT> result;
    Process(&result, *ioSamples);
    
    *ioSamples = result;
}

void
FilterSincConvoBandPass::ComputeFilter(BL_FLOAT fc, int filterSize,
                                       BL_FLOAT sampleRate,
                                       WDL_TypedBuf<BL_FLOAT> *filterData,
                                       bool highPass)
{
    BL_FLOAT fcSr = fc/sampleRate;
    Window::MakeNormSincFilter(filterSize, fcSr, filterData);
    
    WDL_TypedBuf<BL_FLOAT> blackman;
    Window::MakeBlackman(filterSize, &blackman);
    
    BLUtils::MultValues(filterData, blackman);
    
    BLUtils::NormalizeFilter(filterData);
    
    // For high pass filter, must do an additional step
    // See: https://tomroelandts.com/articles/how-to-create-a-simple-high-pass-filter
    if (highPass)
    {
        if (filterData->GetSize() == 0)
            return;
     
        // First, revers the filter
        BLUtils::MultValues(filterData, (BL_FLOAT)-1.0);
        
        int index = (filterData->GetSize() - 1)/2;
        filterData->Get()[index] += 1.0;
    }
}
