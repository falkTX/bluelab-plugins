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
//  Hrtf.h
//  Spatializer
//
//  Created by Pan on 02/12/17.
//
//

#ifndef __Spatializer__Hrtf__
#define __Spatializer__Hrtf__

#include "IPlug_include_in_plug_hdr.h"

#include <KemarHrtf.h>

#define HRTF_NUM_AZIM 360

class HRTF
{
public:
    HRTF(BL_FLOAT sampleRate);
    
    virtual ~HRTF();
    
    BL_FLOAT GetSampleRate();
    
    bool GetImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                            BL_FLOAT elev, BL_FLOAT azim, int chan);
    
    // Get interpolated impulse response
    bool GetImpulseResponseInterp(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                                  BL_FLOAT elev, BL_FLOAT azim, int chan,
                                  BL_FLOAT delay, int respSize, bool *overFlag);
    
    // Get the inpulse response, with a delay
    //
    // For that, "zero-phase" the impulse response,
    // then shift it in time according to delay
    //
    // delay is in seconds
    //
    // respSize is the resulting impulse response size
    // (which should be greater than the response,
    // for example if response size is 128, we may choose
    // respSize = 1024, which allows a delay corresponding to about 6m)
    //
    // overFlag is set to true if the delay is too long and we go over the buffer limit
    bool GetImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                            BL_FLOAT elev, BL_FLOAT azim, int chan,
                            BL_FLOAT delay, int respSize, bool *overFlag);
    
protected:
    bool GetNearestImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                                   int elevId, int azimId, int chan);
    
    bool GetNearestImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                                   int elevId, int azimId,
                                   int *elevIdFound, int *azimIdFound,
                                   int elevDir, int azimDir, int chan);
    
    bool GetImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                            int elevId, int azimId, int chan);
    
    // Elevation, azimuth, channel
    WDL_TypedBuf<BL_FLOAT> *mImpulseResponses[NUM_ELEV][HRTF_NUM_AZIM][2];
    
    // Temporary
private:
    friend class KemarHRTF;
    
    void SetImpulseResponse(const WDL_TypedBuf<BL_FLOAT> *response,
                            int elevId, int azimId, int chan);
    
    long FindZeroPhaseIndex(const WDL_TypedBuf<BL_FLOAT> *response);
    
    void ApplyDelay(WDL_TypedBuf<BL_FLOAT> *result,
                    const WDL_TypedBuf<BL_FLOAT> *response,
                    BL_FLOAT delay, int respSize, bool *overFlag);
    
    BL_FLOAT mSampleRate;
};


#endif /* defined(__Spatializer__Hrtf__) */
