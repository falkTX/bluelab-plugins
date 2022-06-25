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
//  USTWidthAdjuster5.h
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __UST__USTWidthAdjuster5__
#define __UST__USTWidthAdjuster5__

#include <vector>
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
class USTCorrelationComputer;
class USTStereoWidener;

class USTWidthAdjuster5
{
public:
    USTWidthAdjuster5(BL_FLOAT sampleRate);
    
    virtual ~USTWidthAdjuster5();
    
    void Reset(BL_FLOAT sampleRate);
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(BL_FLOAT smoothFactor);
    
    void SetWidth(BL_FLOAT width);
    
    //void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);
    
    void Update(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetLimitedWidth() const;
    
    void DBG_SetCorrSmoothCoeff(BL_FLOAT smooth);
    
protected:
    void UpdateWidth();
    
    void UpdateCorrelation(BL_FLOAT l, BL_FLOAT r);

    
    //BL_FLOAT ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> samples[2],
    //                          BL_FLOAT width);

    //BL_FLOAT ComputeFirstGoodCorrWidth(WDL_TypedBuf<BL_FLOAT> samples[2]);
    
    BL_FLOAT ComputeCorrelationTmp(BL_FLOAT l, BL_FLOAT r, BL_FLOAT width);
    BL_FLOAT ComputeFirstGoodCorrWidth(BL_FLOAT l, BL_FLOAT r);
    
    
    bool mIsEnabled;
    
    BL_FLOAT mSmoothFactor;
    
    // Tmp width, for smoothing
    BL_FLOAT mTmpPrevWidth;
    
    // Width from the user
    BL_FLOAT mUserWidth;
    
    // First width with no correlation
    BL_FLOAT mGoodCorrWidth;
    
    //WDL_TypedBuf<BL_FLOAT> mSamples[2];
    
    BL_FLOAT mSampleRate;
    
    USTCorrelationComputer *mCorrComputer;
    
    USTStereoWidener *mStereoWidener;
    
    BL_FLOAT mPrevCorrelation;
};

#endif /* defined(__UST__USTWidthAdjuster5__) */
