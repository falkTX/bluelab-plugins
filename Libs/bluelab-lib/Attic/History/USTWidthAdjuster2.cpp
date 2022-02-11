//
//  USTWidthAdjuster2.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>
#include <USTProcess.h>

#include "USTWidthAdjuster2.h"

#define NUM_SAMPLES 256

//#define MIN_SMOOTH_FACTOR 0.0
#define MIN_SMOOTH_FACTOR 0.9997 //0.9995

// High value because it is updated at the sample level
#define MAX_SMOOTH_FACTOR 0.99995 //0.99995 //0.9999


// Smoothing over time does not work well

USTWidthAdjuster2::USTWidthAdjuster2()
{
    mIsEnabled = false;;
    
    mSmoothFactor = MIN_SMOOTH_FACTOR;
    mPrevWidth = 0.0;
    
    mFixedWidth = 0.0;
    mGoodCorrWidth = 0.0;
}

USTWidthAdjuster2::~USTWidthAdjuster2() {}

void
USTWidthAdjuster2::Reset()
{
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Resize(0);
    }
}

bool
USTWidthAdjuster2::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster2::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster2::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
}

void
USTWidthAdjuster2::SetWidth(BL_FLOAT width)
{
    mFixedWidth = width;
    
    mPrevWidth = width;
}

void
USTWidthAdjuster2::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (!mIsEnabled)
        return;
    
    if (samples.size() != 2)
        return;
    
    // Add the samples
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Add(samples[i].Get(), samples[i].GetSize());
        
        // NOTE: problem here, if buffer size is > NUM_SAMPLES, then
        // many samples as discarded from computation
        //
        // NOTE2: if buffer size is small, this will take a lot of CPU resources
        int numToConsume = mSamples[i].GetSize() - NUM_SAMPLES;
        if (numToConsume > 0)
            BLUtils::ConsumeLeft(&mSamples[i], numToConsume);
    }
        
    BL_FLOAT goodCorrWidth = ComputeFirstGoodCorrWidth();
    
    mGoodCorrWidth = goodCorrWidth;
}

void
USTWidthAdjuster2::Update()
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
USTWidthAdjuster2::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mFixedWidth;
    
    return mPrevWidth;
}

BL_FLOAT
USTWidthAdjuster2::ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> mSamples[2],
                                     BL_FLOAT width)
{
    WDL_TypedBuf<BL_FLOAT> samplesCopy[2] = { mSamples[0], mSamples[1] };
    vector<WDL_TypedBuf<BL_FLOAT> * > samplesCopyVec;
    samplesCopyVec.push_back(&samplesCopy[0]);
    samplesCopyVec.push_back(&samplesCopy[1]);

    USTProcess::StereoWiden(&samplesCopyVec, width);

    BL_FLOAT corr = USTProcess::ComputeCorrelation(samplesCopy);

    return corr;
}

BL_FLOAT
USTWidthAdjuster2::ComputeFirstGoodCorrWidth()
{
    // Dichotomic search of the first width value that makes positive correlation
#define WIDTH_EPS 0.01

    BL_FLOAT width0 = mFixedWidth;
    BL_FLOAT width1 = -1.0; // mono
    while(fabs(width0 - width1) > WIDTH_EPS)
    {
        BL_FLOAT corr0 = ComputeCorrelation(mSamples, width0);
        BL_FLOAT corr1 = ComputeCorrelation(mSamples, width1);
        
        // Avoid infinite loop
        if (corr0*corr1 >= 0.0)
            break;
        
        BL_FLOAT midWidth = (width0 + width1)*0.5;
        BL_FLOAT corrMid = ComputeCorrelation(mSamples, midWidth);
        
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
