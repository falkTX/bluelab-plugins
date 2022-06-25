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
//  USTWidthAdjuster8.h
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __UST__USTWidthAdjuster8__
#define __UST__USTWidthAdjuster8__

#include <vector>
#include <deque>

#ifndef M_PI
#define M_PI 3.14159265358979323846264
#endif

using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// More intuitive smoothing

// USTWidthAdjuster3:
// - FIX buffer size problem
// - Decimation to manage high sample rates
//
// - USTWidthAdjuster4: manage to avoid any correlation problem
// anytime (test with SPAN mid/side + SideMinder)
//
// - USTWidthAdjuster5: update the width and check at each sample
// (instead of at each buffer)
// (not working well, debug...)
//
// - USTWidthAdjuster6: Use USTCorrelationComputer2
// - USTWidthAdjuster7: Try to fix, and to be closer to SideMinder
// - USTWidthAdjuster8: use real compression on correlation, then find corresponding width
// Tests to try to get good smooth width after FindGoodCorrWidth() (not really convincing)
// Debug/WIP
//
class USTCorrelationComputer2;
class USTStereoWidener;
class CParamSmooth;
class CMAParamSmooth;
class CMAParamSmooth2;
class KalmanParamSmooth;

class InstantCompressor;

class USTWidthAdjuster8
{
public:
    USTWidthAdjuster8(BL_FLOAT sampleRate);
    
    virtual ~USTWidthAdjuster8();
    
    void Reset(BL_FLOAT sampleRate);
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(BL_FLOAT smoothFactor);
    void SetWidth(BL_FLOAT width);
    
    void Update(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetLimitedWidth() const;
    
protected:
    void UpdateWidth(BL_FLOAT width);
    
    BL_FLOAT ComputeFirstGoodCorrWidth(BL_FLOAT targetCorrelation);
    BL_FLOAT ComputeCorrelationAux(BL_FLOAT width);
    
    void ComputeHistorySize(BL_FLOAT sampleRate);
    
    //
    BL_FLOAT CorrToComp(BL_FLOAT corr);
    BL_FLOAT CompToCorr(BL_FLOAT sample);
    
    //
    bool mIsEnabled;
    
    BL_FLOAT mSmoothFactor;
    

    // Tmp width, for smoothing
    BL_FLOAT mLimitedWidth;

    // Width smooth
    //CParamSmooth *mWidthSmoother;
    CMAParamSmooth2 *mWidthSmoother;
    //KalmanParamSmooth *mWidthSmootherKalman;
    
    // Width from the user
    BL_FLOAT mUserWidth;
    
    BL_FLOAT mSampleRate;
    
    // Current detector for out of correlation
    USTCorrelationComputer2 *mCorrComputer;
    
    // Tmp correlation computer, for finding the fist good width
    USTCorrelationComputer2 *mCorrComputerAux;
    
    USTStereoWidener *mStereoWidener;
    
    BL_FLOAT mPrevCorrelation;
    
    deque<BL_FLOAT> mSamplesHistory[2];
    long mHistorySize;
    
    // Compression algorithm
    InstantCompressor *mComp;
    
    // For debugging
    USTCorrelationComputer2 *mCorrComputerDbg;
};

#endif /* defined(__UST__USTWidthAdjuster8__) */
