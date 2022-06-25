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
//  OnsetDetector.h
//  BL-OnsetDetect
//
//  Created by applematuer on 8/2/20.
//
//

#ifndef __BL_OnsetDetect__OnsetDetector__
#define __BL_OnsetDetect__OnsetDetector__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Onset detection for guitar
// See: https://core.ac.uk/download/pdf/45290303.pdf

class OnsetDetector
{
public:
    OnsetDetector();
    
    virtual ~OnsetDetector();
    
    // Threshold between 0 and 1
    // Used to compute J (J = thrs*N)
    //
    // Recommended value: 0.94 (94%)
    //
    void SetThreshold(BL_FLOAT threshold);
    
    // To be called from the "fft callback"
    //
    // (e.g buffer size 2048, overlap 8 (near 90%))
    void Detect(const WDL_TypedBuf<BL_FLOAT> &magns);
    
    // To be called from the audio callback, to get the onset value
    // computed from the buffer (e.g 2048)
    // We get 1 onset value for a fft buffer
    BL_FLOAT GetCurrentOnsetValue();
    
protected:
    BL_FLOAT mThreshold;
    
    BL_FLOAT mCurrentOnsetValue;
};

#endif /* defined(__BL_OnsetDetect__OnsetDetector__) */
