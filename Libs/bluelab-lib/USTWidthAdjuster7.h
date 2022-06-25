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
//  USTWidthAdjuster7.h
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __UST__USTWidthAdjuster7__
#define __UST__USTWidthAdjuster7__

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
// (current version for testing and debugging)
class USTCorrelationComputer2;
class USTStereoWidener;
class CParamSmooth;

//class SimpleCompressor;
class InstantCompressor;

class USTWidthAdjuster7
{
public:
    USTWidthAdjuster7(BL_FLOAT sampleRate);
    
    virtual ~USTWidthAdjuster7();
    
    void Reset(BL_FLOAT sampleRate);
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(BL_FLOAT smoothFactor);
    void SetWidth(BL_FLOAT width);
    
    void Update(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetLimitedWidth() const;
    
    //void DBG_SetCorrSmoothCoeff(BL_FLOAT smooth);
    
protected:
    void UpdateWidth();
    
    BL_FLOAT ComputeFirstGoodCorrWidth();
    BL_FLOAT ComputeCorrelationAux(BL_FLOAT width);
    
    void ComputeHistorySize(BL_FLOAT sampleRate);
    
    //
    BL_FLOAT CorrToComp(BL_FLOAT corr);
    BL_FLOAT CompToCorr(BL_FLOAT sample);
    
    //
    bool mIsEnabled;
    
    BL_FLOAT mSmoothFactor;
    

    // Tmp width, for smoothing
    BL_FLOAT mCurrentSmoothWidth;

    // Width smooth
    CParamSmooth *mWidthSmoother;

    
    // Used to avoid jumps in the final width
    // Can happen if we reduce very quickly the stero width
    CParamSmooth *mFinalWidthSmoother;
    
    // Width from the user
    BL_FLOAT mUserWidth;
    
    BL_FLOAT mSampleRate;
    
    // Current detector for out of correlation
    USTCorrelationComputer2 *mCorrComputerNeg;
    
    // Tmp correlation computer, for finding the fist good width
    USTCorrelationComputer2 *mCorrComputerNegAux;
    
    USTStereoWidener *mStereoWidener;
    
    BL_FLOAT mPrevCorrelation;
    
    deque<BL_FLOAT> mSamplesHistory[2];
    long mHistorySize;
    
    // Compression algorithm
    //SimpleCompressor *mComp;
    //vector<BL_FLOAT> mCorrelationHistory;
    InstantCompressor *mComp;
};

#endif /* defined(__UST__USTWidthAdjuster7__) */
