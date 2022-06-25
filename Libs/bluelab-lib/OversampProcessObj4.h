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
//  OversampProcessObj4.h
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#ifndef __Saturate__OversampProcessObj4__
#define __Saturate__OversampProcessObj4__

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

#define WDL_BESSEL_FILTER_ORDER 8
#define WDL_BESSEL_DENORMAL_AGGRESSIVE
//#include "../../../besselfilter.h"
#include "besselfilter.h"

class FilterRBJNX;

// Only works with 1
#define NUM_CASCADE_FILTERS 1 //6

// See: https://gist.github.com/kbob/045978eb044be88fe568

// OversampProcessObj: dos not take care of Nyquist when downsampling
//
// OversampProcessObj2: use Decimator to take care of Nyquist when downsampling
// GOOD ! : avoid filtering too much high frequencies
//
// OversampProcessObj3: from OversampProcessObj2
// - use Oversampler4 (for fix click)
// - finally, adapted the code from OversampProcessObj (which looks better than OversampProcessObj2)

// NOTE: a problem was detected during UST Clipper (with Oversampler4)
// When using on a pure sine wave, clipper gain 18dB
// => there are very light vertical bars on the spectrogram, every ~2048 sample
// (otherwise it looks to work well)
//
// OversampProcessObj4: from OversampProcessObj3
// Use Tale bessel filter instead of Oversampler4 (which uses WDL_Resampler)
// The result should have less defects, and the CPU consumption should be better.
// NOTE: in fact, the result is not good (aliasing etc...)
class OversampProcessObj4
{
public:
    OversampProcessObj4(int oversampling, BL_FLOAT sampleRate,
                        bool filterNyquist);
    
    virtual ~OversampProcessObj4();
    
    // Must be called at least once
    void Reset(BL_FLOAT sampleRate);
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
protected:
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) = 0;
    
    WDL_BesselFilterCoeffs mAntiAlias[NUM_CASCADE_FILTERS];
	WDL_BesselFilterStage mUpsample[NUM_CASCADE_FILTERS];
    WDL_BesselFilterStage mDownsample[NUM_CASCADE_FILTERS];
    
    FilterRBJNX *mFilter;
    
    int mOversampling;
};

#endif /* defined(__Saturate__OversampProcessObj4__) */
