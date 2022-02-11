//
//  USTWidthAdjuster3.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>
#include <USTProcess.h>

#include "USTWidthAdjuster3.h"

#define NUM_SAMPLES 256

#define DECIM_FACTOR 4.0

//#define MIN_SMOOTH_FACTOR 0.0
#define MIN_SMOOTH_FACTOR 0.9997 //0.9995

// High value because it is updated at the sample level
#define MAX_SMOOTH_FACTOR 0.99995 //0.99995 //0.9999

// Smoothing over time does not work well

USTWidthAdjuster3::USTWidthAdjuster3(BL_FLOAT sampleRate)
{
    mIsEnabled = false;;
    
    mSmoothFactor = MIN_SMOOTH_FACTOR;
    mPrevWidth = 0.0;
    
    mFixedWidth = 0.0;
    mGoodCorrWidth = 0.0;
    
    mSampleRate = sampleRate;
}

USTWidthAdjuster3::~USTWidthAdjuster3() {}

void
USTWidthAdjuster3::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Resize(0);
    }
    
    mSampleRate = sampleRate;
}

bool
USTWidthAdjuster3::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster3::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster3::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
}

void
USTWidthAdjuster3::SetWidth(BL_FLOAT width)
{
    mFixedWidth = width;
    
    mPrevWidth = width;
}

void
USTWidthAdjuster3::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (!mIsEnabled)
        return;
    
    if (samples.size() != 2)
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > samplesDecim = samples;
    USTProcess::DecimateSamplesCorrelation(&samplesDecim,
                                           DECIM_FACTOR, mSampleRate);
    
    // Add the samples
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Add(samplesDecim[i].Get(), samplesDecim[i].GetSize());
    }
    
    if (mSamples[0].GetSize() < NUM_SAMPLES)
        // No enough samples
        return;
    
    BL_FLOAT minWidth = mFixedWidth;
    
    // Compute good correclation width until we have consumed enough samples
    while(mSamples[0].GetSize() >= NUM_SAMPLES)
    {
        WDL_TypedBuf<BL_FLOAT> buf[2];
        buf[0].Add(mSamples[0].Get(), NUM_SAMPLES);
        buf[1].Add(mSamples[1].Get(), NUM_SAMPLES);
        
        BL_FLOAT goodCorrWidth = ComputeFirstGoodCorrWidth(buf);
        
        if (goodCorrWidth < minWidth)
            minWidth = goodCorrWidth;
        
        // Consume
        for (int i = 0; i < 2; i++)
        {
            BLUtils::ConsumeLeft(&mSamples[i], NUM_SAMPLES);
        }
    }
    
    mGoodCorrWidth = minWidth;
}

void
USTWidthAdjuster3::Update()
{
    BL_FLOAT smoothFactor0 = mSmoothFactor;
    // Change param shape so it will be more progressive
    //smoothFactor0 = BLUtils::ApplyParamShape(smoothFactor0, 100.0);
    
    BL_FLOAT factor = (1.0 - smoothFactor0)*MIN_SMOOTH_FACTOR +
                    smoothFactor0*MAX_SMOOTH_FACTOR;
    
    BL_FLOAT width = factor*mPrevWidth + (1.0 - factor)*mGoodCorrWidth; //mFixedWidth;
    mPrevWidth = width;
}

BL_FLOAT
USTWidthAdjuster3::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mFixedWidth;
    
    return mPrevWidth;
}

BL_FLOAT
USTWidthAdjuster3::ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                     BL_FLOAT width)
{
    WDL_TypedBuf<BL_FLOAT> samplesCopy[2] = { samples[0], samples[1] };
    vector<WDL_TypedBuf<BL_FLOAT> * > samplesCopyVec;
    samplesCopyVec.push_back(&samplesCopy[0]);
    samplesCopyVec.push_back(&samplesCopy[1]);

    USTProcess::StereoWiden(&samplesCopyVec, width);

    BL_FLOAT corr = USTProcess::ComputeCorrelation(samplesCopy);

    return corr;
}

BL_FLOAT
USTWidthAdjuster3::ComputeFirstGoodCorrWidth(WDL_TypedBuf<BL_FLOAT> samples[2])
{
    // Dichotomic search of the first width value that makes positive correlation
#define WIDTH_EPS 0.01

    BL_FLOAT width0 = mFixedWidth;
    BL_FLOAT width1 = -1.0; // mono
    while(fabs(width0 - width1) > WIDTH_EPS)
    {
        BL_FLOAT corr0 = ComputeCorrelation(samples, width0);
        BL_FLOAT corr1 = ComputeCorrelation(samples, width1);
        
        // Avoid infinite loop
        if (corr0*corr1 >= 0.0)
            break;
        
        BL_FLOAT midWidth = (width0 + width1)*0.5;
        BL_FLOAT corrMid = ComputeCorrelation(samples, midWidth);
        
        if (corrMid >= 0.0)
        {
            width1 = midWidth;
        }
        else
        {
            width0 = midWidth;
        }
    }
    
    return width0;
}
