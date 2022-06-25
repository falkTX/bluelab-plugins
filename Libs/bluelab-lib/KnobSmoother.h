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
 
#ifndef KNOB_SMOOTHER_H
#define KNOB_SMOOTHER_H

#include <BLTypes.h>
#include <ParamSmoother2.h>

#include "IPlug_include_in_plug_hdr.h"
using namespace iplug;


#define DEFAULT_SMOOTHING_TIME_MS 140.0

// More convenient smoothing of params.
// Smooth the normalized param value
// (seems more homogeneous whatever the parameter non normalized value)
// Seems to manage well when param has a pow shape
// (ParamSmoother2 only makes step scales e.g with BL-Sine)
class KnobSmoother
{
public:
    KnobSmoother(Plugin *plug, int paramIdx,
                 BL_FLOAT sampleRate,
                 BL_FLOAT smoothingTimeMs = DEFAULT_SMOOTHING_TIME_MS)
    {
        mPlug = plug;
        mParamIdx = paramIdx;
        
        BL_FLOAT value = plug->GetParam(mParamIdx)->GetNormalized();
        mParamSmoother = new ParamSmoother2(sampleRate, value, smoothingTimeMs);
                                            
    }
    
    virtual ~KnobSmoother()
    {
        delete mParamSmoother;
    }
    
    inline void Reset(BL_FLOAT sampleRate, BL_FLOAT smoothingTimeMs = -1.0)
    {
        mParamSmoother->Reset(sampleRate, smoothingTimeMs);
    }

    inline void ResetToTargetValue()
    {
        BL_FLOAT value = mPlug->GetParam(mParamIdx)->GetNormalized();
        mParamSmoother->ResetToTargetValue(value);
    }
    
    inline BL_FLOAT Process()
    {
        BL_FLOAT value = mPlug->GetParam(mParamIdx)->GetNormalized();
        mParamSmoother->SetTargetValue(value);

        BL_FLOAT value1 = mParamSmoother->Process();
        
        BL_FLOAT result = mPlug->GetParam(mParamIdx)->FromNormalized(value1);
        
        return result;
    }

    inline BL_FLOAT PickCurrentValue()
    {
        BL_FLOAT value1 = mParamSmoother->PickCurrentValue();
        
        BL_FLOAT result = mPlug->GetParam(mParamIdx)->FromNormalized(value1);
        
        return result;
    }
    
protected:
    Plugin *mPlug;

    int mParamIdx;
    
    ParamSmoother2 *mParamSmoother;
};

#endif
