/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  USTWidthAdjuster8.cpp
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#include <UpTime.h>
#include <BLUtils.h>
#include <BLDebug.h>
#include <USTProcess.h>

#include <USTStereoWidener.h>
#include <USTCorrelationComputer2.h>

#include <CParamSmooth.h>
#include <CMAParamSmooth.h>
#include <CMAParamSmooth2.h>
#include <KalmanParamSmooth.h>

#include <InstantCompressor.h>

#include "USTWidthAdjuster8.h"


// Smooth
#define MIN_SMOOTH_TIME 100.0 //50.0 //200.0 //1000.0 //100.0 //1000.0
#define MAX_SMOOTH_TIME 100.0 //50.0 //200.0 //1000.0 //100.0 //4000.0

// With a value of 0.01, we are sure to not
// consume many many resources (and freeze the host for a moment),
// when we are in a zone of many correlation mess.
//#define HISTORY_SIZE_COEFF 0.01 // ORIGIN
//#define HISTORY_SIZE_COEFF 0.1 // TEST
#define HISTORY_SIZE_COEFF 0.02 // TEST

//

// With that, that doesn't pump, but SideMider will fix our result more
//#define CORRELATION_SMOOTH_TIME_MS 100.0 // ORIG

// With 20ms, SideMinder does not fix our result after
// (but that pumps)
#define CORRELATION_SMOOTH_TIME_MS 100.0
#define CORRELATION_SMOOTH_TIME_MS2 100.0 // 20.0


USTWidthAdjuster8::USTWidthAdjuster8(BL_FLOAT sampleRate)
{
    mIsEnabled = false;
    
    //
    mLimitedWidth = 0.0;
    mUserWidth = 0.0;
    
    mSmoothFactor = 0.5;
    
    // CMA2 is naturally more smooth !
    // Kalman is less smooth
    // Both have a delay
    
    //mWidthSmoother = new CParamSmooth(MIN_SMOOTH_TIME, sampleRate);
    mWidthSmoother = new CMAParamSmooth2(MIN_SMOOTH_TIME, sampleRate);
    //mWidthSmootherKalman = new KalmanParamSmooth(sampleRate);
    
    SetSmoothFactor(mSmoothFactor);
    
    //
    
    mSampleRate = sampleRate;
    
    mCorrComputer = new USTCorrelationComputer2(sampleRate,
                                                CORRELATION_SMOOTH_TIME_MS);
    
    mCorrComputerAux = new USTCorrelationComputer2(sampleRate,
                                                   CORRELATION_SMOOTH_TIME_MS2);
    
    mCorrComputerDbg = new USTCorrelationComputer2(sampleRate,
                                                   CORRELATION_SMOOTH_TIME_MS);
    
    mStereoWidener = new USTStereoWidener();
    
    mPrevCorrelation = 0.0;
    
    //
    ComputeHistorySize(sampleRate);
    
    mUserWidth = 0.0;
    
    mComp = new InstantCompressor(sampleRate);
    //mComp->SetParameters(BL_FLOAT threshold, BL_FLOAT  slope, BL_FLOAT  tatt, BL_FLOAT  trel);
    //mComp->SetParameters(50.0, 50.0, 10.0, 10.0/*500.0*/); // origin, for tests filter
    
    // Attack 10: make stair scales
    // Attack 100: smooth (but reduces less)
    // Release 500: good
    // Release 1000: takes more time to return to original level
    mComp->SetParameters(50.0, 50.0, 100.0/*10.0*/, 1000.0/*500.0*/); // test, for getting directly the compression gain
    
}

USTWidthAdjuster8::~USTWidthAdjuster8()
{
    delete mCorrComputer;
    delete mCorrComputerAux;
    
    delete mCorrComputerDbg;
    
    delete mStereoWidener;
    
    delete mWidthSmoother;
    //delete mWidthSmootherKalman;
    
    delete mComp;
}

void
USTWidthAdjuster8::Reset(BL_FLOAT sampleRate)
{
    mLimitedWidth = mUserWidth; // NEW
    
    mWidthSmoother->Reset(sampleRate, mUserWidth);
    //mWidthSmootherKalman->Reset(sampleRate, mUserWidth);
    
    mPrevCorrelation = 0.0;
    
    mSampleRate = sampleRate;
    
    mCorrComputer->Reset(sampleRate);
    mCorrComputerAux->Reset(sampleRate);
    
    mCorrComputerDbg->Reset(sampleRate);
    
    for (int i = 0; i < 2; i++)
        mSamplesHistory[i].clear();
    
    ComputeHistorySize(sampleRate);
    
    mComp->Reset(sampleRate);
}

bool
USTWidthAdjuster8::IsEnabled()
{
    return mIsEnabled;
}

void
USTWidthAdjuster8::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
USTWidthAdjuster8::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
    
    BL_FLOAT smoothTime = MIN_SMOOTH_TIME +
                            mSmoothFactor*(MAX_SMOOTH_TIME - MIN_SMOOTH_TIME);
                   
    mWidthSmoother->SetSmoothTimeMs(smoothTime);
    
    //BL_FLOAT mea = 2000.0; // to be used if we use prev smooting before
    //BL_FLOAT mea = 40000.0; // test without prev smoothing (not very good)
    //BL_FLOAT est = mea;
    //mWidthSmootherKalman->SetKalmanParameters(mea, est, /*0.1*/DEFAULT_KF_Q);
}

void
USTWidthAdjuster8::SetWidth(BL_FLOAT width)
{
    mUserWidth = width;
    mLimitedWidth = width;
    
    Reset(mSampleRate); // NEW
}

void
USTWidthAdjuster8::Update(BL_FLOAT l, BL_FLOAT r)
{
#if 0 // DEBUG: check correlation smoothness and threshold
    mCorrComputer->Process(l, r);
    
    BL_FLOAT corr = mCorrComputer->GetCorrelation();
    BLDebug::AppendValue("corr.txt", corr);
    
    BL_FLOAT instCorr = mCorrComputer->GetInstantCorrelation();
    BLDebug::AppendValue("inst-corr.txt", instCorr);
    
    // Manage and dump correlation
    BL_FLOAT rmsAmp = CorrToComp(corr);
    
    //BLDebug::AppendValue("comp0.txt", corrForComp);
    
    mComp->Process(&rmsAmp);
    
    BL_FLOAT corr2 = CompToCorr(rmsAmp);
    BLDebug::AppendValue("comp.txt", corr2);
    
    return;
#endif
   
#if 1
    // Enlarge and compute correlation
    BL_FLOAT lW0 = l;
    BL_FLOAT rW0 = r;
    mStereoWidener->StereoWiden(&lW0, &rW0, mUserWidth);
    
    // Compute correlation
    //
    mCorrComputer->Process(lW0, rW0);
    BL_FLOAT corrDbg = mCorrComputer->GetCorrelation();
    
    BLDebug::AppendValue("corr.txt", corrDbg);
    
    // Manage and dump correlation
    BL_FLOAT rmsAmpDbg = CorrToComp(corrDbg);
    
    //BLDebug::AppendValue("comp0.txt", corrForComp);
    
    mComp->Process(&rmsAmpDbg);
    
    BL_FLOAT compGain = mComp->GetGain();
    BLDebug::AppendValue("gain.txt", compGain);
    
    BL_FLOAT corr2 = CompToCorr(rmsAmpDbg);
    BLDebug::AppendValue("comp.txt", corr2);
    
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
    corr = CompToCorr(rmsAmp);
    
    BL_FLOAT width = mUserWidth;
    
    // Compute width for good correlation
    BL_FLOAT goodCorrWidth = ComputeFirstGoodCorrWidth(corr);
        
    if (goodCorrWidth < width)
    {
        width = goodCorrWidth;
    }
    
    // Update width in any case (for width release)
    UpdateWidth(width);
    
#if 1
    BLDebug::AppendValue("width.txt", mLimitedWidth/*width*/);
    
    mStereoWidener->StereoWiden(&l, &r, mLimitedWidth);
    mCorrComputerDbg->Process(l, r);
    
    BL_FLOAT dbgCorr = mCorrComputerDbg->GetCorrelation();
    BLDebug::AppendValue("dbg-corr.txt", dbgCorr);
#endif
    
#if 0 // DEBUG: check correlation smoothness and threshold
    BL_FLOAT corr = mCorrComputerNeg->GetCorrelation();
    BLDebug::AppendValue("corr.txt", corr);
    
    BL_FLOAT instCorr = mCorrComputerNeg->GetInstantCorrelation();
    BLDebug::AppendValue("inst-corr.txt", instCorr);
#endif
}

void
USTWidthAdjuster8::UpdateWidth(BL_FLOAT width)
{
    mLimitedWidth = mWidthSmoother->Process(width);
    
    //
    //mLimitedWidth = mWidthSmootherKalman->Process(/*mLimitedWidth*/width);
}

BL_FLOAT
USTWidthAdjuster8::GetLimitedWidth() const
{
    if (!mIsEnabled)
        return mUserWidth;
    
    return mLimitedWidth;
}

BL_FLOAT
USTWidthAdjuster8::ComputeCorrelationAux(BL_FLOAT width)
{
    // Width can have changed drastically, reset
    mCorrComputerAux->Reset();
    
    // And feed with prev samples
    for (int i = 0; i < mSamplesHistory[0].size(); i++)
    {
        BL_FLOAT l = mSamplesHistory[0][i];
        BL_FLOAT r = mSamplesHistory[1][i];
        
        BL_FLOAT lW = l;
        BL_FLOAT rW = r;
        mStereoWidener->StereoWiden(&lW, &rW, width);
        mCorrComputerAux->Process(lW, rW);
    }
    
    BL_FLOAT corr = mCorrComputerAux->GetCorrelation();
    
    return corr;
}

BL_FLOAT
USTWidthAdjuster8::ComputeFirstGoodCorrWidth(BL_FLOAT targetCorrelation)
{
    // Around 7 iterations to find
    
    // Dichotomic search of the first width value that makes positive correlation
#define WIDTH_EPS 0.01
    
    BL_FLOAT width0 = mUserWidth;
    BL_FLOAT width1 = -1.0; // mono
    while(fabs(width0 - width1) > WIDTH_EPS)
    {
        BL_FLOAT corr0 = ComputeCorrelationAux(width0);
        BL_FLOAT corr1 = ComputeCorrelationAux(width1);
        
        // Avoid infinite loop
        if (((corr0 < targetCorrelation) && (corr1 < targetCorrelation)) ||
            ((corr0 > targetCorrelation) && (corr1 > targetCorrelation)))
            break;
        
        BL_FLOAT midWidth = (width0 + width1)*0.5;
        BL_FLOAT corrMid = ComputeCorrelationAux(midWidth);
        
        if (corrMid > targetCorrelation)
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
USTWidthAdjuster8::ComputeHistorySize(BL_FLOAT sampleRate)
{
    mHistorySize = mCorrComputer->GetSmoothWindowMs()*0.001*sampleRate;
    mHistorySize *= HISTORY_SIZE_COEFF;
    if (mHistorySize == 0)
        mHistorySize = 1;
}

BL_FLOAT
USTWidthAdjuster8::CorrToComp(BL_FLOAT corr)
{
    BL_FLOAT result = -corr + 1.0;
    
    return result;
}

BL_FLOAT
USTWidthAdjuster8::CompToCorr(BL_FLOAT sample)
{
    BL_FLOAT result = -sample + 1.0;
    
    return result;
}
