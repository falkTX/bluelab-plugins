//
//  USTWidthAdjuster4.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>
#include <USTProcess.h>

#include <CorrelationComputer.h>

#include "USTWidthAdjuster4.h"

#define NUM_SAMPLES 256

#define DECIM_FACTOR 4.0

//#define MIN_SMOOTH_FACTOR 0.0
#define MIN_SMOOTH_FACTOR 0.9997 //0.9995

// High value because it is updated at the sample level
#define MAX_SMOOTH_FACTOR 0.99995 //0.99995 //0.9999

// Smoothing over time does not work well

#define USE_CORRELATION_SIMPLE   0
#define USE_CORRELATION_MIN_MAX  0
#define USE_CORRELATION_COMPUTER 1

// Test
#define SKIP_DECIM_SAMPLES 1

USTWidthAdjuster4::USTWidthAdjuster4(BL_FLOAT sampleRate)
{
    mIsEnabled = false;;
    
    mSmoothFactor = MIN_SMOOTH_FACTOR;
    mPrevWidth = 0.0;
    
    mFixedWidth = 0.0;
    mGoodCorrWidth = 0.0;
    
    mSampleRate = sampleRate;
    
#if USE_CORRELATION_COMPUTER
    mCorrComputer = new CorrelationComputer(0.99);
#endif
}

USTWidthAdjuster4::~USTWidthAdjuster4()
{
#if USE_CORRELATION_COMPUTER
    delete mCorrComputer;
#endif

}

void
USTWidthAdjuster4::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Resize(0);
    }
    
    mSampleRate = sampleRate;
    
#if USE_CORRELATION_COMPUTER
    mCorrComputer->Reset();
#endif
}

bool
USTWidthAdjuster4::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster4::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster4::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
}

void
USTWidthAdjuster4::SetWidth(BL_FLOAT width)
{
    mFixedWidth = width;
    
    mPrevWidth = width;
}

void
USTWidthAdjuster4::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (!mIsEnabled)
        return;
    
    if (samples.size() != 2)
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > samplesDecim = samples;
    
#if !SKIP_DECIM_SAMPLES
    USTProcess::DecimateSamplesCorrelation(&samplesDecim,
                                           DECIM_FACTOR, mSampleRate);
#endif
    
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
USTWidthAdjuster4::Update()
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
USTWidthAdjuster4::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mFixedWidth;
    
    return mPrevWidth;
}

void
USTWidthAdjuster4::DBG_SetCorrSmoothCoeff(BL_FLOAT smooth)
{
#if USE_CORRELATION_COMPUTER
    mCorrComputer->SetSmoothCoeff(smooth);
#endif
}

BL_FLOAT
USTWidthAdjuster4::ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                     BL_FLOAT width)
{
    WDL_TypedBuf<BL_FLOAT> samplesCopy[2] = { samples[0], samples[1] };
    vector<WDL_TypedBuf<BL_FLOAT> * > samplesCopyVec;
    samplesCopyVec.push_back(&samplesCopy[0]);
    samplesCopyVec.push_back(&samplesCopy[1]);

    USTProcess::StereoWiden(&samplesCopyVec, width);

#if USE_CORRELATION_SIMPLE
    BL_FLOAT corr = USTProcess::ComputeCorrelation(samplesCopy);
#endif
    
#if USE_CORRELATION_MIN_MAX
    // Depends a lot on the window size
    // int windowSize = 10; // Detect correlation problem every time
    int windowSize = 50; // Detect correlation problem almost correctly
    
    BL_FLOAT minCorr;
    BL_FLOAT maxCorr;
    USTProcess::ComputeCorrelationMinMax(samplesCopy,
                                         windowSize,
                                         &minCorr, &maxCorr);
    
    BL_FLOAT corr = minCorr;
#endif
    
#if USE_CORRELATION_COMPUTER
    mCorrComputer->Process(samplesCopy);
    BL_FLOAT corr = mCorrComputer->GetCorrelation();
#endif
    
    return corr;
}

BL_FLOAT
USTWidthAdjuster4::ComputeFirstGoodCorrWidth(WDL_TypedBuf<BL_FLOAT> samples[2])
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
    
    return (width0 + width1)*0.5;
}
