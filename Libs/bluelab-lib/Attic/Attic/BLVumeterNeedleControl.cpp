//
//  BLVumeterNeedleControl.cpp
//  UST-macOS
//
//  Created by applematuer on 9/30/20.
//
//

#include "BLVumeterNeedleControl.h"

BLVumeterNeedleControl::BLVumeterNeedleControl(IRECT &rect,
                                               const IColor &color,
                                               const IColor &bgColor,
                                               float needleDepth,
                                               int paramIdx)
: IControl(rect, paramIdx)
{
    mColor = color;
    mBgColor = bgColor;
    
    mNeedleDepth = needleDepth;
}

BLVumeterNeedleControl::~BLVumeterNeedleControl() {}

void
BLVumeterNeedleControl::Draw(IGraphics& g)
{
    double val = GetValue();
    
    float y = mRECT.H()*(1.0 - val);
    
    IRECT rect = IRECT(mRECT.L, y, mRECT.R, y + mNeedleDepth);
    
    g.FillRect(mColor, rect);
}
