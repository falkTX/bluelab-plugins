//
//  USTWidthAdjuster5.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>
#include <USTProcess.h>

#include <USTStereoWidener.h>
#include <USTCorrelationComputer.h>

#include "USTWidthAdjuster5.h"

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
//#define SKIP_DECIM_SAMPLES 1

// Jump to good with without smooth
#define JUMP_TO_GOOD_WIDTH 1

// TEST
//#define CORRELATION_SMOOTH_COEFF 0.99


USTWidthAdjuster5::USTWidthAdjuster5(BL_FLOAT sampleRate)
{
    mIsEnabled = false;;
    
    mSmoothFactor = MIN_SMOOTH_FACTOR;
    
    mTmpPrevWidth = 0.0;
    
    mUserWidth = 0.0;
    mGoodCorrWidth = 0.0;
    
    mSampleRate = sampleRate;
    
#if USE_CORRELATION_COMPUTER
    mCorrComputer = new USTCorrelationComputer();
#endif
    
    mStereoWidener = new USTStereoWidener();
    
    mPrevCorrelation = 0.0;
}

USTWidthAdjuster5::~USTWidthAdjuster5()
{
#if USE_CORRELATION_COMPUTER
    delete mCorrComputer;
#endif

    delete mStereoWidener;
}

void
USTWidthAdjuster5::Reset(BL_FLOAT sampleRate)
{
    //for (int i = 0; i < 2; i++)
    //{
    //    mSamples[i].Resize(0);
    //}
    
#if JUMP_TO_GOOD_WIDTH // NEW
    mTmpPrevWidth = 0.0;
    mUserWidth = 0.0;
    mGoodCorrWidth = 0.0;
#endif
    
    mPrevCorrelation = 0.0;
    
    mSampleRate = sampleRate;
    
#if USE_CORRELATION_COMPUTER
    mCorrComputer->Reset();
#endif
}

bool
USTWidthAdjuster5::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster5::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster5::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
}

void
USTWidthAdjuster5::SetWidth(BL_FLOAT width)
{
    mUserWidth = width;
    
    mTmpPrevWidth = width;
}

#if 0 // old
void
USTWidthAdjuster5::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
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
#endif

void
USTWidthAdjuster5::Update(BL_FLOAT l, BL_FLOAT r)
{
#if 0 // DEBUG
    //mCorrComputer->SetSmoothCoeff(0.99999999); //bad
    //mCorrComputer->SetSmoothCoeff(0.9999); // Good for 44100Hz
    
    //mCorrComputer->SaveState();
    mCorrComputer->Process(l, r);
    BL_FLOAT corr = mCorrComputer->GetCorrelation();
    //mCorrComputer->RestoreState();
    
    BLDebug::AppendValue("corr.txt", corr);
    
    // BAD: can make disappear out of correlation, if many on correlation was computed before
    //BL_FLOAT corrSmooth = CORRELATION_SMOOTH_COEFF*mPrevCorrelation +
    //                    (1.0 - CORRELATION_SMOOTH_COEFF)*corr;
    //mPrevCorrelation = corrSmooth;
    
    //BLDebug::AppendValue("corr-smooth.txt", corrSmooth);
    
    return;
#endif
    
    BL_FLOAT minWidth = mUserWidth;
    
    // Compute good correclation width until we have consumed enough samples
    BL_FLOAT goodCorrWidth = ComputeFirstGoodCorrWidth(l, r);
    
#if 0 // TEST
    BL_FLOAT normWidth = (goodCorrWidth + 1.0)*0.5;
    normWidth *= 0.5;
    
    goodCorrWidth = normWidth*2.0 - 1.0;
#endif
    
#if 0 //1 // DEBUG
    BLDebug::AppendValue("good-width.txt", goodCorrWidth);
#endif
    
    if (goodCorrWidth < minWidth)
    {
        minWidth = goodCorrWidth;
        
#if JUMP_TO_GOOD_WIDTH
        mTmpPrevWidth = minWidth;
#endif
    }
    
    mGoodCorrWidth = minWidth;
    
    UpdateWidth();
    
    // Update the correlation history
    UpdateCorrelation(l, r);
}

void
USTWidthAdjuster5::UpdateWidth()
{
    BL_FLOAT smoothFactor0 = mSmoothFactor;
    // Change param shape so it will be more progressive
    //smoothFactor0 = BLUtils::ApplyParamShape(smoothFactor0, 100.0);
    
    BL_FLOAT factor = (1.0 - smoothFactor0)*MIN_SMOOTH_FACTOR +
                    smoothFactor0*MAX_SMOOTH_FACTOR;
    
#if !JUMP_TO_GOOD_WIDTH
    BL_FLOAT width = factor*mTmpPrevWidth + (1.0 - factor)*mGoodCorrWidth; //mFixedWidth;
#else
    BL_FLOAT width = factor*mTmpPrevWidth + (1.0 - factor)*mUserWidth;
#endif
    
    mTmpPrevWidth = width;
    
#if 0 //1 // DEBUG
    BLDebug::AppendValue("width.txt", width);
#endif
}

void
USTWidthAdjuster5::UpdateCorrelation(BL_FLOAT l, BL_FLOAT r)
{
    //mStereoWidener->StereoWiden(&l, &r, mTmpPrevWidth);
    
    mCorrComputer->Process(l, r);
}

BL_FLOAT
USTWidthAdjuster5::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mUserWidth;
    
    return mTmpPrevWidth;
}

void
USTWidthAdjuster5::DBG_SetCorrSmoothCoeff(BL_FLOAT smooth)
{
#if USE_CORRELATION_COMPUTER
    mCorrComputer->SetSmoothCoeff(smooth);
#endif
}

#if 0
BL_FLOAT
USTWidthAdjuster5::ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> samples[2],
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
#endif

BL_FLOAT
USTWidthAdjuster5::ComputeCorrelationTmp(BL_FLOAT l, BL_FLOAT r, BL_FLOAT width)
{
    mStereoWidener->StereoWiden(&l, &r, width);
    
    mCorrComputer->SaveState();
    
    mCorrComputer->Process(l, r);
    BL_FLOAT corr = mCorrComputer->GetCorrelation();
    
    mCorrComputer->RestoreState();
    
    return corr;
}

#if 0
BL_FLOAT
USTWidthAdjuster5::ComputeFirstGoodCorrWidth(WDL_TypedBuf<BL_FLOAT> samples[2])
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
#endif

BL_FLOAT
USTWidthAdjuster5::ComputeFirstGoodCorrWidth(BL_FLOAT l, BL_FLOAT r)
{
    // Dichotomic search of the first width value that makes positive correlation
#define WIDTH_EPS 0.001 //0.01
    
    BL_FLOAT width0 = mUserWidth;
    BL_FLOAT width1 = -1.0; // mono
    while(fabs(width0 - width1) > WIDTH_EPS)
    {
        BL_FLOAT corr0 = ComputeCorrelationTmp(l, r, width0);
        BL_FLOAT corr1 = ComputeCorrelationTmp(l, r, width1);
        
        // Avoid infinite loop
        if (corr0*corr1 >= 0.0)
            break;
        
        BL_FLOAT midWidth = (width0 + width1)*0.5;
        BL_FLOAT corrMid = ComputeCorrelationTmp(l, r, midWidth);
        
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
    //return (width0 + width1)*0.5;
}
