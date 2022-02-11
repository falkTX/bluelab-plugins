//
//  USTWidthAdjuster6.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>
#include <USTProcess.h>

#include <USTStereoWidener.h>
#include <USTCorrelationComputer2.h>

#include <CParamSmooth.h>

#include "USTWidthAdjuster6.h"

// Smooth
#define MIN_SMOOTH_TIME 1000.0 //2500.0
#define MAX_SMOOTH_TIME 4000.0

// Final smooth, to avoid clicks if width decreases quickly
//#define FINAL_SMOOTH_TIME 100.0 // Pumping effect
//#define FINAL_SMOOTH_TIME 500.0 // Too slow, let pass bad correct with Span
#define FINAL_SMOOTH_TIME 250.0 // No so bad

// Jump to good with without smooth
#define JUMP_TO_GOOD_WIDTH 1

// This is sensivity
// Set to low value to avoid detecting everytime out of correlation
//
//#define CORRELATION_THRESHOLD -0.05
//#define CORRELATION_THRESHOLD -0.08

// After fix
//#define CORRELATION_THRESHOLD -0.3
//#define CORRELATION_THRESHOLD -0.2 // To be like sideminder
#define CORRELATION_THRESHOLD -0.3 // TEST

// NOTE: icreasing CORRELATION_THRESHOLD2 increases the valid width

// DEBUG: for testing for CPU overload in case of many correlation mess
// in a sone
// GOOD: narrow less width in case of problem
// GOOD: with that, sideminder will not detect any correlation problem
#define CORRELATION_THRESHOLD2 CORRELATION_THRESHOLD

// GOOD (narrow more width in case of problem)
//#define CORRELATION_THRESHOLD2 CORRELATION_THRESHOLD*0.5 // GOOD

// Narrows a lot in case of problem
//#define CORRELATION_THRESHOLD2 0.0 //

// Try not to narrow too much
// NOTE: with that, sideminder will detect problems
//#define CORRELATION_THRESHOLD2 CORRELATION_THRESHOLD*2.0

// With a value of 0.01, we are sure to not
// consume many many resources (and freeze the host for a moment),
// when we are in a zone of many correlation mess.
//#define HISTORY_SIZE_COEFF 0.01 // ORIGIN
//#define HISTORY_SIZE_COEFF 0.1 // TEST
#define HISTORY_SIZE_COEFF 0.02 // TEST

// Without this, the sound clicks
#define FINAL_SMOOTH_WIDTH 1

//

// With that, that doesn't pump, but SideMider will fix our result more
//#define CORRELATION_SMOOTH_TIME_MS 100.0 // ORIG

// With 20ms, SideMinder does not fix our result after
// (but that pumps)
#define CORRELATION_SMOOTH_TIME_MS 100.0
#define CORRELATION_SMOOTH_TIME_MS2 20.0


USTWidthAdjuster6::USTWidthAdjuster6(BL_FLOAT sampleRate)
{
    mIsEnabled = false;
    
    //
    mCurrentSmoothWidth = 0.0;
    
    mSmoothFactor = 0.5;
    
    mWidthSmoother = new CParamSmooth(MIN_SMOOTH_TIME, sampleRate);
    SetSmoothFactor(mSmoothFactor);
    
    mFinalWidthSmoother = new CParamSmooth(FINAL_SMOOTH_TIME, sampleRate);
    
    mUserWidth = 0.0;
    
    //
    
    mSampleRate = sampleRate;
    
    mCorrComputerNeg = new USTCorrelationComputer2(sampleRate,
                                                   CORRELATION_SMOOTH_TIME_MS,
                                                   USTCorrelationComputer2::KEEP_ONLY_NEGATIVE);
    
    mCorrComputerNegAux = new USTCorrelationComputer2(sampleRate,
                                                      CORRELATION_SMOOTH_TIME_MS2,
                                                      USTCorrelationComputer2::KEEP_ONLY_NEGATIVE);
    
    mStereoWidener = new USTStereoWidener();
    
    mPrevCorrelation = 0.0;
    
    //
    ComputeHistorySize(sampleRate);
    
    mUserWidth = 0.0; // NEW
}

USTWidthAdjuster6::~USTWidthAdjuster6()
{
    delete mCorrComputerNeg;
    delete mCorrComputerNegAux;
    
    delete mStereoWidener;
    
    delete mWidthSmoother;
    
    delete mFinalWidthSmoother;
}

void
USTWidthAdjuster6::Reset(BL_FLOAT sampleRate)
{
#if JUMP_TO_GOOD_WIDTH
    //mUserWidth = 0.0;
    //mCurrentSmoothWidth = 0.0;
    
    mCurrentSmoothWidth = mUserWidth; // NEW
#endif
    
    mWidthSmoother->Reset(sampleRate, mUserWidth);
    
    mFinalWidthSmoother->Reset(sampleRate, 0.0/*mUserWidth*/);
    
    mPrevCorrelation = 0.0;
    
    mSampleRate = sampleRate;
    
    mCorrComputerNeg->Reset(sampleRate);
    mCorrComputerNegAux->Reset(sampleRate);
    
    for (int i = 0; i < 2; i++)
        mSamplesHistory[i].clear();
    
    ComputeHistorySize(sampleRate);
}

bool
USTWidthAdjuster6::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster6::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster6::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
    
    BL_FLOAT smoothTime = MIN_SMOOTH_TIME +
                        mSmoothFactor*(MAX_SMOOTH_TIME - MIN_SMOOTH_TIME);
                   
    mWidthSmoother->SetSmoothTimeMs(smoothTime);
}

void
USTWidthAdjuster6::SetWidth(BL_FLOAT width)
{
    mUserWidth = width;
    mCurrentSmoothWidth = width;
    
    Reset(mSampleRate); // NEW
}

void
USTWidthAdjuster6::Update(BL_FLOAT l, BL_FLOAT r)
{
#if 0 // DEBUG: check correlation smoothness and threshold
    mCorrComputerNeg->Process(l, r);
    
    BL_FLOAT corr = mCorrComputerNeg->GetCorrelation();
    BLDebug::AppendValue("corr.txt", corr);
    
    BL_FLOAT instCorr = mCorrComputerNeg->GetInstantCorrelation();
    BLDebug::AppendValue("inst-corr.txt", instCorr);
    
    return;
#endif
    
#if 0 //1
    BLDebug::AppendValue("user-width.txt", mUserWidth);
    BLDebug::AppendValue("width.txt", mCurrentSmoothWidth);
#endif
    
    if (!mIsEnabled)
        return;
    
    // Update the history
    //
    mSamplesHistory[0].push_back(l);
    mSamplesHistory[1].push_back(r);
    for (int i = 0; i < 2; i++)
    {
        while(mSamplesHistory[i].size() > mHistorySize)
        {
            mSamplesHistory[i].pop_front();
        }
    }
    
    // Compute current correlation, to see if it is ok
    //
    
    // Enlarge and compute correlation (neg)
    BL_FLOAT currentWidth = GetLimitedWidth();
    
    BL_FLOAT lW = l;
    BL_FLOAT rW = r;
    mStereoWidener->StereoWiden(&lW, &rW, currentWidth);
    mCorrComputerNeg->Process(lW, rW);
    
    BL_FLOAT corrNeg = mCorrComputerNeg->GetCorrelation();
    if (corrNeg < CORRELATION_THRESHOLD)
    {
        BL_FLOAT minWidth = mUserWidth;
    
        // Compute good correclation width until we have consumed enough samples
        BL_FLOAT goodCorrWidth = ComputeFirstGoodCorrWidth();
        
        // DEBUG: uncomment, to check for history size and host freeze
        // and set
        // and set #define CORRELATION_THRESHOLD2 CORRELATION_THRESHOLD
        //goodCorrWidth = mUserWidth;
        
        if (goodCorrWidth < minWidth)
        {
            minWidth = goodCorrWidth;
        
#if JUMP_TO_GOOD_WIDTH
            mCurrentSmoothWidth = minWidth;
            mWidthSmoother->Reset(mSampleRate, mCurrentSmoothWidth);
#endif
        }
        
        // Refresh the current correlation computer
        //
        
        // Width can have changed drastically, reset
        mCorrComputerNeg->Reset();
        
        // And feed with prev samples
        for (int i = 0; i < mSamplesHistory[0].size(); i++)
        {
            BL_FLOAT l0 = mSamplesHistory[0][i];
            BL_FLOAT r0 = mSamplesHistory[1][i];
            
            BL_FLOAT lW0 = l0;
            BL_FLOAT rW0 = r0;
            
            mStereoWidener->StereoWiden(&lW0, &rW0, mCurrentSmoothWidth);
            
            mCorrComputerNeg->Process(lW0, rW0);
        }
    }
    
    // Update width in any case (for width release)
    UpdateWidth();
    
#if 0 //1 // DEBUG: check correlation smoothness and threshold
    BL_FLOAT corr = mCorrComputerNeg->GetCorrelation();
    BLDebug::AppendValue("corr.txt", corr);
    
    BL_FLOAT instCorr = mCorrComputerNeg->GetInstantCorrelation();
    BLDebug::AppendValue("inst-corr.txt", instCorr);
#endif
}

void
USTWidthAdjuster6::UpdateWidth()
{
    mCurrentSmoothWidth = mWidthSmoother->Process(mUserWidth);
    
#if FINAL_SMOOTH_WIDTH
    // NEW
    mCurrentSmoothWidth = mFinalWidthSmoother->Process(mCurrentSmoothWidth);
#endif
}

BL_FLOAT
USTWidthAdjuster6::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mUserWidth;
    
    return mCurrentSmoothWidth;
}

BL_FLOAT
USTWidthAdjuster6::ComputeCorrelationAux(BL_FLOAT width)
{
    // Width can have changed drastically, reset
    mCorrComputerNegAux->Reset();
    
    // And feed with prev samples
    for (int i = 0; i < mSamplesHistory[0].size(); i++)
    {
        BL_FLOAT l = mSamplesHistory[0][i];
        BL_FLOAT r = mSamplesHistory[1][i];
        
        BL_FLOAT lW = l;
        BL_FLOAT rW = r;
        mStereoWidener->StereoWiden(&lW, &rW, width);
        mCorrComputerNegAux->Process(lW, rW);
    }
    
    BL_FLOAT corr = mCorrComputerNegAux->GetCorrelation();
    
    return corr;
}

BL_FLOAT
USTWidthAdjuster6::ComputeFirstGoodCorrWidth()
{
    // Around 7 iterations to find
    
    // Dichotomic search of the first width value that makes positive correlation
//#define WIDTH_EPS 0.001 //0.01
#define WIDTH_EPS 0.01
    
    BL_FLOAT width0 = mUserWidth;
    BL_FLOAT width1 = -1.0; // mono
    while(fabs(width0 - width1) > WIDTH_EPS)
    {
        BL_FLOAT corr0 = ComputeCorrelationAux(width0);
        BL_FLOAT corr1 = ComputeCorrelationAux(width1);
        
        // Avoid infinite loop
        if (((corr0 < CORRELATION_THRESHOLD2) && (corr1 < CORRELATION_THRESHOLD2)) ||
            ((corr0 > CORRELATION_THRESHOLD2) && (corr1 > CORRELATION_THRESHOLD2)))
            break;
        
        BL_FLOAT midWidth = (width0 + width1)*0.5;
        BL_FLOAT corrMid = ComputeCorrelationAux(midWidth);
        
        if (corrMid > CORRELATION_THRESHOLD2)
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

void
USTWidthAdjuster6::ComputeHistorySize(BL_FLOAT sampleRate)
{
    mHistorySize = mCorrComputerNeg->GetSmoothWindowMs()*0.001*sampleRate;
    mHistorySize *= HISTORY_SIZE_COEFF;
    if (mHistorySize == 0)
        mHistorySize = 1;
}
