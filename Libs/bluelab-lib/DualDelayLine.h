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
//  DualDelayLine.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__DualDelayLine__
#define __BL_Bat__DualDelayLine__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// From DelayObj4
//
class DualDelayLine
{
public:
    DualDelayLine(BL_FLOAT sampleRate, BL_FLOAT maxDelay, BL_FLOAT delay);
    
    virtual ~DualDelayLine();
    
    void Reset(BL_FLOAT sampleRate);
    
    WDL_FFT_COMPLEX ProcessSample(WDL_FFT_COMPLEX sample);
    
protected:
    void Init();
    
    BL_FLOAT mSampleRate;
    
    // Delay in seconds
    // Delay is in [-maxDelay/2, maxDelay/2], and so can be negative
    BL_FLOAT mDelay;
    BL_FLOAT mMaxDelay;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mDelayLine;
    
    long mReadPos;
    long mWritePos;
};

#endif /* defined(__BL_Bat__DualDelayLine__) */
