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
//  BLVumeterControl.cpp
//  UST-macOS
//
//  Created by applematuer on 9/30/20.
//
//

#include <BLTypes.h>

#include "BLVumeter2SidesControl.h"

#define MIDDLE_SIZE 2

BLVumeter2SidesControl::BLVumeter2SidesControl(IRECT &rect,
                                               const IColor &color,
                                               int paramIdx)
: IControl(rect, paramIdx)
{
    mColor = color;
}

BLVumeter2SidesControl::~BLVumeter2SidesControl() {}

void
BLVumeter2SidesControl::Draw(IGraphics& g)
{
    BL_FLOAT val = GetValue();

    val = 1.0 - val;
    
    // Fill the middle
    BL_FLOAT middle = mRECT.T + mRECT.H()*0.5;
    
    IRECT middleRect = mRECT;
    middleRect.T = middle - MIDDLE_SIZE/2;
    middleRect.B = middle + MIDDLE_SIZE/2;
    g.FillRect(mColor, middleRect);
        
    // Fill the bar
    IRECT rect = mRECT;
    if (val > 0.5)
    {
        rect.T = middle + mRECT.H()*(val - 0.5);
        rect.B = middle;
    }
    else
    {
        rect.T = middle;
        rect.B = middle - mRECT.H()*0.5*(1.0 - val*2.0);
    }
    
    g.FillRect(mColor, rect);
}

void
BLVumeter2SidesControl::SetValue(double value, int valIdx)
{
    if (!IsDisabled())
        IControl::SetValue(value, valIdx);
}

void
BLVumeter2SidesControl::SetValueForce(double value, int valIdx)
{
    IControl::SetValue(value, valIdx);
}
