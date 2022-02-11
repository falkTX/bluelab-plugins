//
//  USTWidthAdjuster9.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <Utils.h>
#include <USTProcess.h>

#include <USTStereoWidener.h>
#include <USTCorrelationComputer2.h>

#include <CParamSmooth.h>
#include <CMAParamSmooth.h>
#include <CMAParamSmooth2.h>
#include <KalmanParamSmooth.h>

#include <InstantCompressor.h>

#include <Debug.h>

#include "USTWidthAdjuster9.h"

//
#define CORRELATION_SMOOTH_TIME_MS 100.0
// FIX_GOOD_TRACK_COMPRESS
//#define CORRELATION_SMOOTH_TIME_MS 10.0

// Compressor
//#define COMP_ATTACK_TIME 100.0
// FIX_GOOD_TRACK_COMPRESS
#define COMP_ATTACK_TIME 0.02 //1.0 // 100.0

#define COMP_MIN_RELEASE_TIME 100.0
#define COMP_MAX_RELEASE_TIME 1000.0

#if 0
TODO:
- Now, the new correlation is well bounded / but sideminder still clips
=> check after (width and other)

- use knee ? (starting for example at 0.45, then 100% slope at 0.5 ?)
- TODO: check of we keep threshold at 0.49 or is we move to 0.5 (or more around)
#endif

USTWidthAdjuster9::USTWidthAdjuster9(double sampleRate)
{
    mIsEnabled = false;
    
    //
    mLimitedWidth = 0.0;
    mUserWidth = 0.0;
    
    //
    
    mSampleRate = sampleRate;
    
    mCorrComputer = new USTCorrelationComputer2(sampleRate,
                                                CORRELATION_SMOOTH_TIME_MS);
    
    mCorrComputerDbg = new USTCorrelationComputer2(sampleRate,
                                                   CORRELATION_SMOOTH_TIME_MS);
    
    mStereoWidener = new USTStereoWidener();
    
    mUserWidth = 0.0;
    
    mComp = new InstantCompressor(sampleRate);
    
    // Attack 10: make stair scales
    // Attack 100: smooth (but reduces less)
    // Release 500: good
    // Release 1000: takes more time to return to original level
    //SetParameters(double threshold, double slope, double  tatt, double  trel);
    
    //mComp->SetParameters(50.0, 50.0, COMP_ATTACK_TIME, 1000.0/*500.0*/); // test, for getting directly the compression gain
    
    //FIX_GOOD_TRACK_COMPRESS
    mComp->SetParameters(49.0/*50.0*/, /*50.0*/100.0/*0.0*/, COMP_ATTACK_TIME, 1000.0/*500.0*/);
    
    // TEST
    //mComp->SetKnee(25.0);
    mComp->SetKnee(50.0);
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
USTWidthAdjuster9::Reset(double sampleRate)
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
USTWidthAdjuster9::SetSmoothFactor(double factor)
{
    mSmoothFactor = factor;
    
    double releaseTime = COMP_MIN_RELEASE_TIME +
                            mSmoothFactor*(COMP_MAX_RELEASE_TIME - COMP_MIN_RELEASE_TIME);
    
    mComp->SetRelease(releaseTime);
}

void
USTWidthAdjuster9::SetWidth(double width)
{
    mUserWidth = width;
    
    // Comment to avoid resetting the limited width to the user width
    // (would not look consistent for the user when width limit is activated)
    //mLimitedWidth = width;
    //Reset(mSampleRate);
}

void
USTWidthAdjuster9::Update(double l, double r)
{   
#if 0
    // Enlarge and compute correlation
    double lW0 = l;
    double rW0 = r;
    mStereoWidener->StereoWiden(&lW0, &rW0, mUserWidth);
    
    // Compute correlation
    //
    mCorrComputer->Process(lW0, rW0);
    double corrDbg = mCorrComputer->GetCorrelation();
    
    Debug::AppendValue("corr.txt", corrDbg);
    
    // Manage and dump correlation
    double rmsAmpDbg = CorrToComp(corrDbg);
    
    //Debug::AppendValue("comp0.txt", corrForComp);
    
    mComp->Process(&rmsAmpDbg);
    
    double compGain = mComp->GetGain();
    Debug::AppendValue("gain.txt", compGain);
    
    double corr2 = CompToCorr(rmsAmpDbg);
    Debug::AppendValue("comp.txt", corr2);
    
    return;
#endif
    
    if (!mIsEnabled)
        return;
    
    // Enlarge and compute correlation
    double lW = l;
    double rW = r;
    mStereoWidener->StereoWiden(&lW, &rW, mUserWidth);
    
    // Compute correlation
    //
    mCorrComputer->Process(lW, rW);
    double corr = mCorrComputer->GetCorrelation();
    
    // // Debug::AppendValue("corr0.txt", corr);
    
    // Apply compression
    //
    double rmsAmp = CorrToComp(corr);
    //Debug::AppendValue("rms.txt", rmsAmp);
    
#if 1 // Debug
    for (int i = 0; i < 100; i++)
    {
        double t = ((double)i)/99.0;
    
        mComp->Process(&t);
        
        Debug::AppendValue("t.txt", t);
    }
    
    exit(0);
#endif

    mComp->Process(&rmsAmp);
    
    //Debug::AppendValue("comp-rms.txt", rmsAmp);
    
    // Debug
    //double corrComp = CompToCorr(rmsAmp);
    // // Debug::AppendValue("comp-corr.txt", corrComp);
    
    double compGain = mComp->GetGain();
    //if (compGain < 0.0)
    //    compGain = 0.0;
    //Debug::AppendValue("comp-gain.txt", compGain);
    
    double width = mUserWidth;
    width = ApplyCompWidth(width, compGain, corr);
    
    // Update width in any case (for width release)
    UpdateWidth(width);
    
    // Debug
#if 0 //1
    Debug::AppendValue("width.txt", mLimitedWidth);
    
    double lW1 = l;
    double rW1 = r;
    mStereoWidener->StereoWiden(&lW1, &rW1, mLimitedWidth);
    mCorrComputerDbg->Process(lW1, rW1);
    
    double corr1 = mCorrComputerDbg->GetCorrelation();
    Debug::AppendValue("corr1.txt", corr1);
#endif
}

void
USTWidthAdjuster9::UpdateWidth(double width)
{
    mLimitedWidth = width;
}

double
USTWidthAdjuster9::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mUserWidth;
    
    return mLimitedWidth;
}

double
USTWidthAdjuster9::CorrToComp(double corr)
{
    //double result = -corr + 1.0;
    
    // FIX_GOOD_TRACK_COMPRESS
    double result = 1.0 - (corr + 1.0)*0.5;
    
    return result;
}

double
USTWidthAdjuster9::CompToCorr(double sample)
{
    //double result = -sample + 1.0;
    
    // FIX_GOOD_TRACK_COMPRESS
    double result = (1.0 - sample)*2.0 - 1.0;
    
    return result;
}

double
USTWidthAdjuster9::ApplyCompWidth(double width, double compGain,
                                  double corr)
{
    // Naive implementation, with x2 on the gain effect (not working)
    //
    //double gain = 1.0 - compGain;
    //gain *= 2.0; // Factor !
    //gain = 1.0 - gain;
    
    // Good method, convert correlation gain compression to width gain compression
    //
#if 0 // corr is not used if we use the cooking recipe that works well (below)
#define EPS 1e-10
    // Check bounds (for later tan()), and return relevant values
    if (corr < -1.0 + EPS)
        return 0.0;
    if (corr > 1.0 - EPS)
        return 1.0;
#endif
    
    // Depends on corr => the curve is not smooth at all !
    //double gainW = tan(compGain*alpha0)/tan(alpha0);
    //double alpha0 = ((corr + 1.0)/2.0)*M_PI/2.0;
    
    // Cooking recipe that works well ! :)
    // (the proportionalty between correlation gain and width gain looks preserved).
    double gainW = tan(compGain) - tan(1.0) + 1.0;
    
    if (gainW < 0.0)
        gainW = 0.0;
    
    if (gainW > 1.0)
        gainW = 1.0;
    
    //
    double normWidth = (width + 1.0)*0.5;
    
    normWidth *= gainW;
    
    double result = (normWidth*2.0) - 1.0;
    
    if (result < -1.0)
        result = -1.0;
        
    return result;
}
