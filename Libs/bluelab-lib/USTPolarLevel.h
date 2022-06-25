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
//  USTPolarLevel.h
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#ifndef __UST__USTPolarLevel__
#define __UST__USTPolarLevel__

#include "IPlug_include_in_plug_hdr.h"

class USTPolarLevel
{
public:
    USTPolarLevel(BL_FLOAT sampleRate);
    
    virtual ~USTPolarLevel();
    
    void Reset();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                       WDL_TypedBuf<BL_FLOAT> points[2],
                       WDL_TypedBuf<BL_FLOAT> maxPoints[2]);
    
protected:
    void ComputePolarLevelsSourcePos(const WDL_TypedBuf<BL_FLOAT> &rs,
                                     const WDL_TypedBuf<BL_FLOAT> &thetas,
                                     int numBins,
                                     WDL_TypedBuf<BL_FLOAT> *levels);

    
    void ProcessLevels(WDL_TypedBuf<BL_FLOAT> *ioLevels);
    
    // Smooth over time
    WDL_TypedBuf<BL_FLOAT> mPrevPolarsLevels;
    WDL_TypedBuf<BL_FLOAT> mPrevPolarLevelsMax;
    
    // Smooth over space
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mSampleRate;
};

#endif /* defined(__UST__USTPolarLevel__) */
