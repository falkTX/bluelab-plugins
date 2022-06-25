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
//  EarlyReflections.h
//  BL-ReverbDepth
//
//  Created by applematuer on 9/1/20.
//
//

#ifndef __BL_ReverbDepth__EarlyReflections__
#define __BL_ReverbDepth__EarlyReflections__

#include "IPlug_include_in_plug_hdr.h"

//
#define NUM_DELAYS 4

class DelayObj4;
class EarlyReflections
{
public:
    EarlyReflections(BL_FLOAT sampleRate);
    
    EarlyReflections(const EarlyReflections &other);
    
    virtual ~EarlyReflections();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> outRevSamples[2]);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> *outRevSamplesL,
                 WDL_TypedBuf<BL_FLOAT> *outRevSamplesR);
    
    //
    void SetRoomSize(BL_FLOAT roomSize);
    void SetIntermicDist(BL_FLOAT dist);
    void SetNormDepth(BL_FLOAT depth);
    
protected:
    void Init();
    
    //
    BL_FLOAT mSampleRate;
    
    DelayObj4 *mDelays[NUM_DELAYS];
    
    BL_FLOAT mRoomSize;
    BL_FLOAT mIntermicDist;
    BL_FLOAT mNormDepth;
};

#endif /* defined(__BL_ReverbDepth__EarlyReflections__) */
