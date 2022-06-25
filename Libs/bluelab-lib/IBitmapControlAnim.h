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
//  IBitmapControlAnim.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#ifndef IBitmapControlAnim_h
#define IBitmapControlAnim_h

#include <IControl.h>

using namespace iplug::igraphics;

// From IBitmapControlAnim2
class IBitmapControlAnim : public IBitmapControl
{
public:
    IBitmapControlAnim(int x, int y, const IBitmap &bitmap, int paramIdx,
                        double speed, bool pingPong, bool sinAnim,
                        EBlend blend = EBlend::Default)
    : IBitmapControl(x, y, bitmap, paramIdx, blend)
    {
        mSpeed = speed;
        mPingPong = pingPong;
        mSinAnim = sinAnim;
        
        mBitmapNum = 0;
        mPrevBitmapNum = 0;
    }
    
    virtual ~IBitmapControlAnim() {}
    
    void Draw(IGraphics &g) override;
    
    bool IsDirty() override;
    
protected:
    void UpdateBitmapNum();
    
    //
    double mSpeed;
    bool mPingPong;
    bool mSinAnim;
    
    int mBitmapNum;
    int mPrevBitmapNum;
};


#endif /* IBitmapControlAnim */
