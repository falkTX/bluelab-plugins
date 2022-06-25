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
 
#ifndef SATURATE_OVER_OBJ_H
#define SATURATE_OVER_OBJ_H

#include <BLTypes.h>

#include <OversampProcessObj.h>

#include "IPlug_include_in_plug_hdr.h"

// For InfraSynth
// NOT USED ?
class SaturateOverObj : public OversampProcessObj
{
public:
    SaturateOverObj(int overlaping, BL_FLOAT sampleRate);
    
    virtual ~SaturateOverObj();
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) override;
    
    void SetRatio(BL_FLOAT ratio);
    
protected:
    void ComputeSaturation(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT ratio);
    
    BL_FLOAT mRatio;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
};

#endif
