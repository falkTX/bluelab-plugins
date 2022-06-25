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
//  DelayObj.h
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#ifndef __BL_Precedence__DelayObj__
#define __BL_Precedence__DelayObj__

#include "IPlug_include_in_plug_hdr.h"

// Delay line obj
// Manages clicks:
// - when changing the delay when playing
// - when restarting after a reset
class DelayObj
{
public:
    DelayObj(int maxDelaySamples, BL_FLOAT sampleRate);
    
    virtual ~DelayObj();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetDelay(int delay);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &inBuf);

    void GetSamples(const WDL_TypedBuf<BL_FLOAT> &inBuf,
                    WDL_TypedBuf<BL_FLOAT> *outBuf);
    
    void ConsumeBuffer();

protected:
    int mMaxDelaySamples;
    
    BL_FLOAT mSampleRate;
    
    int mDelay;
    int mPrevDelay;
    
    bool mPrevReset;
    
    WDL_TypedBuf<BL_FLOAT> mDelayLine;
    WDL_TypedBuf<BL_FLOAT> mPrevDelayLine;
};

#endif /* defined(__BL_Precedence__DelayObj__) */
