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
//  IBitmapControlAnim.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#include <UpTime.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "IBitmapControlAnim.h"

void
IBitmapControlAnim::Draw(IGraphics &g)
{
    g.DrawBitmap(mBitmap, mRECT, mBitmapNum, &mBlend);
    
    mPrevBitmapNum = mBitmapNum;
}

bool
IBitmapControlAnim::IsDirty()
{
    UpdateBitmapNum();
    
    mDirty = false;
    
    if (mPrevBitmapNum != mBitmapNum)
        mDirty = true;
    
    return mDirty;
}

void
IBitmapControlAnim::UpdateBitmapNum()
{
    unsigned long long ut = UpTime::GetUpTime();
    long long cycleLength = (1.0/mSpeed)*1000;
    
    int coeff = 1;
    if (mPingPong)
        coeff = 2;
    
    double t = (((double)(ut % (cycleLength*coeff)))/cycleLength);
    
    int bitmapNum = t*mBitmap.N();
    
    if (mSinAnim)
    {
        if (mBitmap.N() > 1)
        {
          double t2 = ((double)bitmapNum) / (mBitmap.N() - 1);
            t2 = t2 * M_PI; // Half a cycle

            t2 = (cos(t2 + M_PI) + 1.0) / 2.0;

            bitmapNum = t2 * (mBitmap.N() - 1);
        }
    }
    
    if (bitmapNum >= mBitmap.N())
    {
        bitmapNum = mBitmap.N()*2 - bitmapNum - 1;
    }
    
    // Check the bounds just in case
    if (bitmapNum < 0)
        bitmapNum = 0;
    if (bitmapNum > mBitmap.N() - 1)
        bitmapNum = mBitmap.N() - 1;
    if (bitmapNum < 0)
        bitmapNum = 0;

    mBitmapNum = bitmapNum;
}
