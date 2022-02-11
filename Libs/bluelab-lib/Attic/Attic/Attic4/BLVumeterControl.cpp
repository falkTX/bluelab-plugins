//
//  BLVumeterControl.cpp
//  UST-macOS
//
//  Created by applematuer on 9/30/20.
//
//

#include "BLVumeterControl.h"

BLVumeterControl::BLVumeterControl(IRECT &rect,
                                   const IColor &color,
                                   int paramIdx)
: IControl(rect, paramIdx)
{
    mColor = color;
}

BLVumeterControl::~BLVumeterControl() {}

void
BLVumeterControl::Draw(IGraphics& g)
{
    double val = GetValue();
    
    IRECT rect = mRECT;
    rect.T = mRECT.B - mRECT.H()*val;
    g.FillRect(mColor, rect);
}
