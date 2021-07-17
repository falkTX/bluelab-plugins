//
//  USTWidthAdjuster9.cpp
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
#include <USTCorrelationComputer3.h>
#include <USTCorrelationComputer4.h>

#include <CParamSmooth.h>
#include <CMAParamSmooth.h>
#include <CMAParamSmooth2.h>
#include <KalmanParamSmooth.h>

#include <InstantCompressor.h>

#include "USTWidthAdjuster9.h"

//
// For USTCorrelationComputer2
//#define CORRELATION_SMOOTH_TIME_MS 100.0

// For USTCorrelationComputer3
//#define CORRELATION_SMOOTH_TIME_MS 50.0
#define CORRELATION_SMOOTH_TIME_MS 20.0 //10.0
//#define CORRELATION_SMOOTH_TIME_MS 20.0

// Compressor
// Smooth attack (not so good)
//#define COMP_ATTACK_TIME 100.0
// Almost instant attack (better)
#define COMP_ATTACK_TIME 0.02

// For USTCorrelationComputer2
//#define COMP_MIN_RELEASE_TIME 100.0
//#define COMP_MAX_RELEASE_TIME 1000.0

// For USTCorrelationComputer3 and 4
//
// Set min release to high value, so we will avoid
// increasing the width too quickly and get out of correlation otherwise
#define COMP_MIN_RELEASE_TIME 1000.0 //500.0
#define COMP_MAX_RELEASE_TIME 2000.0

// Offset in compression, to match with Sideminder
//#define SIDEMINDER_OFFSET 10.0 // Doesnt't work


USTWidthAdjuster9::USTWidthAdjuster9(BL_FLOAT sampleRate)
{
    mIsEnabled = false;
    
    //
    mLimitedWidth = 0.0;
    mUserWidth = 0.0;
    
    //
    
    mSampleRate = sampleRate;
    
    mCorrComputer = new CORRELATION_COMPUTER_CLASS(sampleRate,
                                                   CORRELATION_SMOOTH_TIME_MS);
    
    mCorrComputerDbg = new CORRELATION_COMPUTER_CLASS(sampleRate,
                                                      CORRELATION_SMOOTH_TIME_MS);
    
    mStereoWidener = new USTStereoWidener();
    
    mUserWidth = 0.0;
    
    mComp = new InstantCompressor(sampleRate);
    
    // Attack 10: make stair scales
    // Attack 100: smooth (but reduces less)
    // Release 500: good
    // Release 1000: takes more time to return to original level
    //SetParameters(BL_FLOAT threshold, BL_FLOAT slope, BL_FLOAT  tatt, BL_FLOAT  trel);
    
    mComp->SetParameters(/*49.0*/50.0, // - SIDEMINDER_OFFSET,
                         /*50.0*/100.0, COMP_ATTACK_TIME,
                         1000.0/*500.0*/);
    
    // Knee
    mComp->SetKnee(25.0); // Good
    //mComp->SetKnee(50.0);
    //mComp->SetKnee(75.0);
    
    mSmoothFactor = 0.5;
    SetSmoothFactor(mSmoothFactor);
}

USTWidthAdjuster9::~USTWidthAdjuster9()
{
    delete mCorrComputer;
    
    delete mCorrComputerDbg;
    
    delete mStereoWidener;
    
    delete mComp;
}

void
USTWidthAdjuster9::Reset(BL_FLOAT sampleRate)
{
    mLimitedWidth = mUserWidth;
    
    mSampleRate = sampleRate;
    
    mCorrComputer->Reset(sampleRate);
    
    mCorrComputerDbg->Reset(sampleRate);
    
    mComp->Reset(sampleRate);
}

bool
USTWidthAdjuster9::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster9::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster9::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
    
    BL_FLOAT releaseTime = COMP_MIN_RELEASE_TIME +
        mSmoothFactor*(COMP_MAX_RELEASE_TIME - COMP_MIN_RELEASE_TIME);
    
    mComp->SetRelease(releaseTime);
}

void
USTWidthAdjuster9::SetWidth(BL_FLOAT width)
{
    mUserWidth = width;
    
    // Comment to avoid resetting the limited width to the user width
    // (would not look consistent for the user when width limit is activated)
    //mLimitedWidth = width;
    //Reset(mSampleRate);
}

void
USTWidthAdjuster9::Update(BL_FLOAT l, BL_FLOAT r)
{       
    if (!mIsEnabled)
        return;
    
    // Enlarge and compute correlation
    BL_FLOAT lW = l;
    BL_FLOAT rW = r;
    mStereoWidener->StereoWiden(&lW, &rW, mUserWidth);
    
    // Debug
    ////BL_FLOAT normWidth = (mUserWidth + 1.0)*0.5;
    ////BLDebug::AppendValue("norm-width.txt", normWidth);
    
    // Compute correlation
    //
    mCorrComputer->Process(lW, rW);
    BL_FLOAT corr = mCorrComputer->GetCorrelation();
    
    ////BLDebug::AppendValue("corr0.txt", corr);
    
    // Apply compression
    //
    BL_FLOAT rmsAmp = CorrToComp(corr);
    //BLDebug::AppendValue("rms.txt", rmsAmp);

    mComp->Process(&rmsAmp);
    
    ///BLDebug::AppendValue("corr1.txt", rmsAmp);
    
    // Debug
    //BL_FLOAT corrComp = CompToCorr(rmsAmp);
    //BLDebug::AppendValue("corr1.txt", corrComp);
    
    BL_FLOAT compGain = mComp->GetGain();
    ////BLDebug::AppendValue("comp-gain.txt", compGain);
    
    BL_FLOAT width = mUserWidth;
    width = ApplyCompWidth(width, compGain, corr);
    
    // Update width in any case (for width release)
    UpdateWidth(width);
    
    // Debug
#if 0 //1
    BLDebug::AppendValue("width.txt", mLimitedWidth);
    
    BL_FLOAT lW1 = l;
    BL_FLOAT rW1 = r;
    mStereoWidener->StereoWiden(&lW1, &rW1, mLimitedWidth);
    mCorrComputerDbg->Process(lW1, rW1);
    
    BL_FLOAT corr1 = mCorrComputerDbg->GetCorrelation();
    BLDebug::AppendValue("corr2.txt", corr1);
#endif
}

void
USTWidthAdjuster9::UpdateWidth(BL_FLOAT width)
{
    mLimitedWidth = width;
}

BL_FLOAT
USTWidthAdjuster9::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mUserWidth;
    
    return mLimitedWidth;
}

BL_FLOAT
USTWidthAdjuster9::CorrToComp(BL_FLOAT corr)
{
    BL_FLOAT result = 1.0 - (corr + 1.0)*0.5;
    
    return result;
}

BL_FLOAT
USTWidthAdjuster9::CompToCorr(BL_FLOAT sample)
{
    BL_FLOAT result = (1.0 - sample)*2.0 - 1.0;
    
    return result;
}

// comp-gain "0.94 0.9 0.85 0.79 0.75 0.71 0.68 0.66 0.638 0.622 0.61 0.598"
// theorical width gain "1.0 0.93 0.845 0.775 0.715 0.664 0.62 0.581 0.547 0.516 0.489 0.465"
//
// plot: tan(compGains*c) + thWidthGains(1) - tan(compGains(1)*c)
//
// For USTWidthAdjuster3 and 4
BL_FLOAT
USTWidthAdjuster9::ApplyCompWidth(BL_FLOAT width, BL_FLOAT compGain,
                                  BL_FLOAT corr)
{
    // Cooking recipe from the measured curves
    //BL_FLOAT gainW = std::tan((compGain) - 0.2;
    //BL_FLOAT gainW = std::tan((compGain*0.9) - 0.1;
    
    //BL_FLOAT sideMinderOffset = 0.05; //0.1;
    //BL_FLOAT c = 0.87;
    //BL_FLOAT gainW = std::tan((compGain*c) - 0.19;
    
    //BL_FLOAT c = 0.87;
    //BL_FLOAT gainW = std::tan((compGain*c) - 0.066998;
    
    //BL_FLOAT c = 0.9;
    //BL_FLOAT gainW = std::tan((compGain*c) - 0.129;
    
    // Begin and end matches exactly. Compresse more in the middle.
    //BL_FLOAT c = 0.9;
    //BL_FLOAT gainW = std::tan((compGain*c) - 0.1291911;
    
    // Compress more at the end.
    BL_FLOAT c = 0.92;
    BL_FLOAT gainW = std::tan(compGain*c) - 0.1728954;
    
    //BL_FLOAT c = 0.95;
    //BL_FLOAT gainW = std::tan((compGain*c) - 0.2422003;
    
    //BL_FLOAT c = 0.97;
    //BL_FLOAT gainW = std::tan((compGain*c) - 0.291159;
    
    //BL_FLOAT c = 1.0;
    //BL_FLOAT gainW = std::tan((compGain*c) - 0.3692345;
    
    //
    if (gainW < 0.0)
        gainW = 0.0;
    
    if (gainW > 1.0)
        gainW = 1.0;
    
    //
    BL_FLOAT normWidth = (width + 1.0)*0.5;
    
    normWidth *= gainW;
    
    BL_FLOAT result = (normWidth*2.0) - 1.0;
    
    if (result < -1.0)
        result = -1.0;
    
    return result;
}

#if 0
// Works with USTWidthAdjuster2 (version with atan2())
BL_FLOAT
USTWidthAdjuster9::ApplyCompWidth(BL_FLOAT width, BL_FLOAT compGain,
                                  BL_FLOAT corr)
{
    // Depends on corr => the curve is not smooth at all !
    //BL_FLOAT gainW = std::tan((compGain*alpha0)/std::tan((alpha0);
    //BL_FLOAT alpha0 = ((corr + 1.0)/2.0)*M_PI/2.0;
    
    // Cooking recipe that works well ! :)
    // (the proportionalty between correlation gain and width gain looks preserved).
    BL_FLOAT gainW = std::tan((compGain) - std::tan(((BL_FLOAT)1.0) + 1.0;
    
    if (gainW < 0.0)
        gainW = 0.0;
    
    if (gainW > 1.0)
        gainW = 1.0;
    
    //
    BL_FLOAT normWidth = (width + 1.0)*0.5;
    
    normWidth *= gainW;
    
    BL_FLOAT result = (normWidth*2.0) - 1.0;
    
    if (result < -1.0)
        result = -1.0;
        
    return result;
}
#endif
