//
//  BLWidthAdjuster.cpp
//  
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>

#include <BLStereoWidener.h>
#include <BLCorrelationComputer2.h>

#include <CParamSmooth.h>
#include <CMAParamSmooth.h>
#include <CMAParamSmooth2.h>
#include <KalmanParamSmooth.h>

#include <InstantCompressor.h>

#include "BLWidthAdjuster.h"

//
#define CORRELATION_SMOOTH_TIME_MS 20.0

// Compressor
// Almost instant attack (better)
#define COMP_ATTACK_TIME 0.02

// For BLCorrelationComputer2
//
// Set min release to high value, so we will avoid
// increasing the width too quickly and get out of correlation otherwise
#define COMP_MIN_RELEASE_TIME 1000.0 //500.0
#define COMP_MAX_RELEASE_TIME 2000.0

// Adaptations for width boost
#define FIX_WIDTH_BOOST 1


BLWidthAdjuster::BLWidthAdjuster(BL_FLOAT sampleRate)
{
    mIsEnabled = false;
    
    mLimitedWidth = 0.0;
    mUserWidth = 0.0;
    
    mSampleRate = sampleRate;
    
    mCorrComputer = new BLCorrelationComputer2(sampleRate,
                                               CORRELATION_SMOOTH_TIME_MS);
    
    mStereoWidener = new BLStereoWidener();
    
    mComp = new InstantCompressor(sampleRate);
    
    // Attack 10: make stair scales
    // Attack 100: smooth (but reduces less)
    // Release 500: good
    // Release 1000: takes more time to return to original level
    //
    // SetParameters(BL_FLOAT threshold, BL_FLOAT slope,
    // BL_FLOAT  tatt, BL_FLOAT  trel);

#if !FIX_WIDTH_BOOST
    // Origin: for width [-1, 1]
    mComp->SetParameters(50.0, 100.0, COMP_ATTACK_TIME, 1000.0);
#else
    // New: for width [-1, 5] (width boost)
    mComp->SetParameters(50.0, 200.0, COMP_ATTACK_TIME, 1000.0);
#endif
    
    // Knee
    mComp->SetKnee(25.0);
    
    mSmoothFactor = 0.5;
    SetSmoothFactor(mSmoothFactor);
}

BLWidthAdjuster::~BLWidthAdjuster()
{
    delete mCorrComputer;
    
    delete mStereoWidener;
    
    delete mComp;
}

void
BLWidthAdjuster::Reset(BL_FLOAT sampleRate)
{
    mLimitedWidth = mUserWidth;
    
    mSampleRate = sampleRate;
    
    mCorrComputer->Reset(sampleRate);
    
    mComp->Reset(sampleRate);
}

bool
BLWidthAdjuster::IsEnabled()
{
    return mIsEnabled;
}

void
BLWidthAdjuster::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
BLWidthAdjuster::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
    
    BL_FLOAT releaseTime =
        COMP_MIN_RELEASE_TIME +
        mSmoothFactor*(COMP_MAX_RELEASE_TIME - COMP_MIN_RELEASE_TIME);
    
    mComp->SetRelease(releaseTime);
}

void
BLWidthAdjuster::SetWidth(BL_FLOAT width)
{
    mUserWidth = width;
}

void
BLWidthAdjuster::Update(BL_FLOAT l, BL_FLOAT r)
{       
    if (!mIsEnabled)
        return;
    
    // Enlarge and compute correlation
    BL_FLOAT lW = l;
    BL_FLOAT rW = r;
    mStereoWidener->StereoWiden(&lW, &rW, mUserWidth);
    
    // Compute correlation
    //
    mCorrComputer->Process(lW, rW);
    BL_FLOAT corr = mCorrComputer->GetCorrelation();
    
    // Apply compression
    //
    BL_FLOAT rmsAmp = CorrToComp(corr);
    
    mComp->Process(&rmsAmp);
    
    BL_FLOAT compGain = mComp->GetGain();
    
    BL_FLOAT width = mUserWidth;
    width = ApplyCompWidth(width, compGain, corr);
    
    // Update width in any case (for width release)
    UpdateWidth(width);
}

void
BLWidthAdjuster::UpdateWidth(BL_FLOAT width)
{    
    mLimitedWidth = width;
}

BL_FLOAT
BLWidthAdjuster::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mUserWidth;
    
    return mLimitedWidth;
}

BL_FLOAT
BLWidthAdjuster::CorrToComp(BL_FLOAT corr)
{
    BL_FLOAT result = 1.0 - (corr + 1.0)*0.5;
    
    return result;
}

BL_FLOAT
BLWidthAdjuster::CompToCorr(BL_FLOAT sample)
{
    BL_FLOAT result = (1.0 - sample)*2.0 - 1.0;
    
    return result;
}

BL_FLOAT
BLWidthAdjuster::ApplyCompWidth(BL_FLOAT width, BL_FLOAT compGain,
                                BL_FLOAT corr)
{
#if !FIX_WIDTH_BOOST
    // Compress more at the end.
    BL_FLOAT c = 0.92;
    BL_FLOAT gainW = std::tan(compGain*c) - 0.1728954;
#endif
    
    BL_FLOAT gainW = compGain;
    
    if (gainW < 0.0)
        gainW = 0.0;
    
    if (gainW > 1.0)
        gainW = 1.0;
    
    BL_FLOAT normWidth = (width + 1.0)*0.5;
        
    normWidth *= gainW;
    
    BL_FLOAT result = (normWidth*2.0) - 1.0;
    
    if (result < -1.0)
        result = -1.0;
    
    return result;
}
