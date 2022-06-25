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
//  DelayLinePhaseShift2.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__DelayLinePhaseShift2__
#define __BL_Bat__DelayLinePhaseShift2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

class DelayLinePhaseShift2
{
public:
    DelayLinePhaseShift2(BL_FLOAT sampleRate, int bufferSize, int binNum,
                        BL_FLOAT maxDelay, BL_FLOAT delay);
    
    virtual ~DelayLinePhaseShift2();
    
    void Reset(BL_FLOAT sampleRate);
    
    WDL_FFT_COMPLEX ProcessSample(WDL_FFT_COMPLEX sample);
    
    static void DBG_Test(int bufferSize, BL_FLOAT sampleRate,
                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &samples);
    
protected:
    void Init();
    
    BL_FLOAT mSampleRate;
    int mBufferSize;
    
    int mBinNum;
    
    // Delay in seconds
    // Delay is in [-maxDelay/2, maxDelay/2], and so can be negative
    BL_FLOAT mDelay;
    BL_FLOAT mMaxDelay;
    
    BL_FLOAT mDPhase;
    
    // Friend for optimization
    friend class SourceLocalisationSystem3;
    WDL_FFT_COMPLEX mPhaseShiftComplex;
};

#endif /* defined(__BL_Bat__DelayLinePhaseShift2__) */
