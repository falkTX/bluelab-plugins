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
//  BLWidthAdjuster9.h
//  BL
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __BL__BLWidthAdjuster9__
#define __BL__BLWidthAdjuster9__

#include <vector>
#include <deque>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846264
#endif

#include <BLTypes.h>

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
// - USTWidthAdjuster8: use real compression on correlation,
// then find corresponding width
// Tests to try to get good smooth width after FindGoodCorrWidth()
// (not really convincing)
// Debug/WIP
//
// - USTWidthAdjuster9: get the gain used in the compressor, and directly apply it to the width
// (simple and looks efficient !)
//
// BLWidthAdjuster: from USTWidthAdjuster9

class BLCorrelationComputer2;
class BLStereoWidener;
class CParamSmooth;
class CMAParamSmooth;
class CMAParamSmooth2;
class KalmanParamSmooth;

class InstantCompressor;

class BLWidthAdjuster
{
public:
    BLWidthAdjuster(BL_FLOAT sampleRate);
    
    virtual ~BLWidthAdjuster();
    
    void Reset(BL_FLOAT sampleRate);
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(BL_FLOAT smoothFactor);
    void SetWidth(BL_FLOAT width);
    
    void Update(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetLimitedWidth() const;
    
protected:
    void UpdateWidth(BL_FLOAT width);
    
    //
    BL_FLOAT CorrToComp(BL_FLOAT corr);
    BL_FLOAT CompToCorr(BL_FLOAT sample);
    
    //
    BL_FLOAT ApplyCompWidth(BL_FLOAT width, BL_FLOAT compGain,
                            BL_FLOAT corr);

    
    //
    bool mIsEnabled;
    
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mSmoothFactor;
    

    // Width from the user
    BL_FLOAT mUserWidth;
    
    // Limited result width
    BL_FLOAT mLimitedWidth;
    
    // Current detector for out of correlation
    BLCorrelationComputer2 *mCorrComputer;
    
    BLStereoWidener *mStereoWidener;
    
    // Compression algorithm
    InstantCompressor *mComp;
};

#endif /* defined(__BL__BLWidthAdjuster__) */
