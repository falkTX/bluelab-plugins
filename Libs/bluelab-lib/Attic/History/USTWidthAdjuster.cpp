//
//  USTWidthAdjuster.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>
#include <USTProcess.h>

#include "USTWidthAdjuster.h"

#define NUM_SAMPLES 256


USTWidthAdjuster::USTWidthAdjuster()
{
    mIsEnabled = false;;
    
    mReleaseMillis = 0;
    
    mFixedWidth = 0.0;
    mGoodCorrWidth = 0.0;
    
    mPrevTime = UpTime::GetUpTime();
}

USTWidthAdjuster::~USTWidthAdjuster() {}

void
USTWidthAdjuster::Reset()
{
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Resize(0);
    }
}

bool
USTWidthAdjuster::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster::SetReleaseMillis(BL_FLOAT release)
{
    mReleaseMillis = release;
}

void
USTWidthAdjuster::SetWidth(BL_FLOAT width)
{
    mFixedWidth = width;
}

void
USTWidthAdjuster::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (!mIsEnabled)
        return;
    
    if (samples.size() != 2)
        return;
    
    // Add the samples
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Add(samples[i].Get(), samples[i].GetSize());
        
        int numToConsume = mSamples[i].GetSize() - NUM_SAMPLES;
        if (numToConsume > 0)
            BLUtils::ConsumeLeft(&mSamples[i], numToConsume);
    }
        
    BL_FLOAT goodCorrWidth = ComputeFirstGoodCorrWidth();
    
    mGoodCorrWidth = goodCorrWidth;
    
    mPrevTime = UpTime::GetUpTime();
}

BL_FLOAT
USTWidthAdjuster::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mFixedWidth;
    
    unsigned long long currentTime = UpTime::GetUpTime();
    unsigned long long elapsed = currentTime - mPrevTime;
    
    BL_FLOAT t = ((BL_FLOAT)elapsed)/mReleaseMillis;
    if (t > 1.0)
        t = 1.0;
    
    BL_FLOAT width = (1.0 - t)*mGoodCorrWidth + t*mFixedWidth;
    
    return width;
}

BL_FLOAT
USTWidthAdjuster::ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> mSamples[2],
                                     BL_FLOAT width)
{
    WDL_TypedBuf<BL_FLOAT> samplesCopy[2] = { mSamples[0], mSamples[1] };
    vector<WDL_TypedBuf<BL_FLOAT> * > samplesCopyVec;
    samplesCopyVec.push_back(&samplesCopy[0]);
    samplesCopyVec.push_back(&samplesCopy[1]);

    // #bl-iplug2
    //USTProcess::StereoWiden(&samplesCopyVec, width);
    StereoWidenProcess::StereoWiden(&samplesCopyVec, width);
    
    BL_FLOAT corr = USTProcess::ComputeCorrelation(samplesCopy);

    return corr;
}

BL_FLOAT
USTWidthAdjuster::ComputeFirstGoodCorrWidth()
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
