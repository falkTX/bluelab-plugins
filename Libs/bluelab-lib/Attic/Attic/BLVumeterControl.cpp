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
                                   const IColor &bgColor,
                                   int paramIdx)
: IControl(rect, paramIdx)
{
    mColor = color;
    mBgColor = bgColor;
}

BLVumeterControl::~BLVumeterControl() {}

void
BLVumeterControl::Draw(IGraphics& g)
{
    //IRECT bgRect = mRECT;
    //bgRect.B = mRECT.B*(1.0 - val);
    //g.FillRect(mBgColor, bgRect);
    
    double val = GetValue();
    
    /*IRECT rect = mRECT;
    rect.T = mRECT.T*val;
    g.FillRect(mColor, rect);*/
    
    IRECT rect = mRECT;
    rect.T = mRECT.B - mRECT.H()*val;
    g.FillRect(mColor, rect);
    
    /*IRECT bgRect = mRECT;
    bgRect.B = mRECT.B*(1.0 - val);
    g.FillRect(mBgColor, bgRect);*/
}
