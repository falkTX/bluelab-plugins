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
//  FreqAdjustObj.h
//  PitchShift
//
//  Created by Apple m'a Tuer on 30/10/17.
//
//

#ifndef __PitchShift__FreqAdjustObj__
#define __PitchShift__FreqAdjustObj__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "IControl.h"

// See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
class FreqAdjustObj
{
public:
    FreqAdjustObj(int bufferSize, int oversampling, BL_FLOAT sampleRate);
    
    virtual ~FreqAdjustObj();
    
    void Reset();
    
    void ComputeRealFrequencies(const WDL_TypedBuf<BL_FLOAT> &ioPhases, WDL_TypedBuf<BL_FLOAT> *outRealFreqs);
    
    void ComputePhases(WDL_TypedBuf<BL_FLOAT> *ioPhases, const WDL_TypedBuf<BL_FLOAT> &realFreqs);
    
protected:
    static BL_FLOAT MapToPi(BL_FLOAT val);
    
    int mBufferSize;
    int mOversampling;
    BL_FLOAT mSampleRate;
    
    // For correct phase computation
    WDL_TypedBuf<BL_FLOAT> mLastPhases;
    WDL_TypedBuf<BL_FLOAT> mSumPhases;
};

#endif /* defined(__PitchShift__FreqAdjustObj__) */
