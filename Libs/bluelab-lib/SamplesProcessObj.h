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
//  SamplesProcessObj.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__SamplesProcessObj__
#define __BL_PitchShift__SamplesProcessObj__

#include "FftProcessObj13.h"

// Same as FftProcessObj, but just nothing on with the Fft
//
// This is used to bufferize samples as done with FftProcessObj,
// to keep the synchronization of necessary

class SamplesProcessObj : public FftProcessObj13
{
public:
    SamplesProcessObj(int bufferSize, int overlapping)
        : FftProcessObj13(bufferSize, overlapping, 1,
                          // Use no window, to get the data exactly similar
                          AnalysisMethodNone, SynthesisMethodNone,
                          // Use Hanning, to have identity after the sum
                          //AnalysisMethodWindow, SynthesisMethodWindow,
                          false, false, false) {}
    
    virtual ~SamplesProcessObj() {}
    
protected:
    // Does nothing by default
    //
    // In this way, we skeeze the all the Fft stuffs
    virtual void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &inBuffer,
                                  const WDL_TypedBuf<BL_FLOAT> &inScBuffer,
                                  WDL_TypedBuf<BL_FLOAT> *outBuffer)
    {
        // Just bypass
        *outBuffer = inBuffer;
    }
};

#endif /* defined(__BL_PitchShift__SamplesProcessObj__) */
