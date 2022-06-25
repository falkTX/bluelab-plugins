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
//  USTCorrelationComputer.h
//  UST
//
//  Created by applematuer on 1/2/20.
//
//

#ifndef __UST__USTCorrelationComputer__
#define __UST__USTCorrelationComputer__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
class USTCorrelationComputer
{
public:
    USTCorrelationComputer();
    
    virtual ~USTCorrelationComputer();
    
    void Reset();
    
    void SetSmoothCoeff(BL_FLOAT smoothCoeff);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> samples[2]);
    
    void Process(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetCorrelation();
    
    // Save and erstore state
    // Usefull when computing prospective correlation
    void SaveState();
    void RestoreState();
    
protected:
    BL_FLOAT mSmoothCoeff;
    
    BL_FLOAT mCurrentL;
    BL_FLOAT mCurrentR;
    
    BL_FLOAT mCurrentL2;
    BL_FLOAT mCurrentR2;
    
    BL_FLOAT mCurrentCorrelation;
    
    // For save state
    BL_FLOAT mCurrentLSave;
    BL_FLOAT mCurrentRSave;
    
    BL_FLOAT mCurrentL2Save;
    BL_FLOAT mCurrentR2Save;
};

#endif /* defined(__UST__USTCorrelationComputer__) */
