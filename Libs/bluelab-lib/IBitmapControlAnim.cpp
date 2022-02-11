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
