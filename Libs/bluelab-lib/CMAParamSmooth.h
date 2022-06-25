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
//  CMAParamSmooth.h
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#ifndef __UST__CMAParamSmooth__
#define __UST__CMAParamSmooth__

#include <deque>
using namespace std;

#include <BLTypes.h>

class CMAParamSmooth
{
public:
    CMAParamSmooth(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate);
    
    virtual ~CMAParamSmooth();
    
    void Reset(BL_FLOAT samplingRate);
    void Reset(BL_FLOAT samplingRate, BL_FLOAT val);
    
    void SetSmoothTimeMs(BL_FLOAT smoothingTimeMs);
    
    BL_FLOAT Process(BL_FLOAT inVal);
    
private:
    BL_FLOAT mSmoothingTimeMs;
    BL_FLOAT mSampleRate;
    
    int mWindowSize;
    
    BL_FLOAT mCurrentAvg;
    
    deque<BL_FLOAT> mPrevValues;
};

#endif /* defined(__UST__CMAParamSmooth__) */
