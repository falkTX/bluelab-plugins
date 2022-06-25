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
//  SMVProcessXComputer.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputer__
#define __BL_SoundMetaViewer__SMVProcessXComputer__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class Axis3D;

class SMVProcessXComputer
{
public:
    virtual ~SMVProcessXComputer() {};
    
    virtual void Reset(BL_FLOAT sampleRate) = 0;
    
    // Return the x coordinates if not polar.
    // Otherwise return the normalized direction in result (x, y).
    virtual void ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                          const WDL_TypedBuf<BL_FLOAT> magns[2],
                          const WDL_TypedBuf<BL_FLOAT> phases[2],
                          WDL_TypedBuf<BL_FLOAT> *resultX,
                          WDL_TypedBuf<BL_FLOAT> *resultY = NULL,
                          bool *isPolar = NULL,
                          BL_FLOAT polarCenter[2] = NULL,
                          bool *isScalable = NULL) = 0;
    
    virtual Axis3D *CreateAxis() = 0;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputer__) */
