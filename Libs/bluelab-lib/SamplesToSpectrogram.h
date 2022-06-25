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
//  SamplesToSpectrogram.h
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef __BL_Reverb__SamplesToSpectrogram__
#define __BL_Reverb__SamplesToSpectrogram__

#ifdef IGRAPHICS_NANOVG

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class BLSpectrogram4;
class SimpleSpectrogramFftObj;
class FftProcessObj16;

class SamplesToSpectrogram
{
public:
    SamplesToSpectrogram(BL_FLOAT sampleRate);
    
    virtual ~SamplesToSpectrogram();
    
    void Reset(BL_FLOAT sampleRate);
    
    int GetBufferSize();
    
    void SetSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    BLSpectrogram4 *GetSpectrogram();
    
protected:
    FftProcessObj16 *mFftObj;
    SimpleSpectrogramFftObj *mSpectrogramFftObj;
    
    BLSpectrogram4 *mSpectrogram;
    
    BL_FLOAT mSampleRate;
};

#endif

#endif /* defined(__BL_Reverb__SamplesToSpectrogram__) */
