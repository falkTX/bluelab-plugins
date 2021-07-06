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

    
