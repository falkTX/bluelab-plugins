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
//  PhaseCorrectObj.h
//  BL-PitchShift
//
//  Created by Pan on 03/04/18.
//
//

#ifndef __BL_PitchShift__PhaseCorrectObj__
#define __BL_PitchShift__PhaseCorrectObj__

#include "IPlug_include_in_plug_hdr.h"

class PhaseCorrectObj
{
public:
    PhaseCorrectObj(int bufferSize, int oversampling, BL_FLOAT sampleRate);
    
    virtual ~PhaseCorrectObj();

    void Reset(int bufferSize, int oversampling, BL_FLOAT sampleRate);

    void Process(const WDL_TypedBuf<BL_FLOAT> &freqs,
                 WDL_TypedBuf<BL_FLOAT> *ioPhases);

protected:
    BL_FLOAT MapToPi(BL_FLOAT val);
    
    bool mFirstTime;

    int mBufferSize;

    int mOversampling;

    BL_FLOAT mSampleRate;

    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
};

#endif /* defined(__BL_PitchShift__PhaseCorrectObj__) */
