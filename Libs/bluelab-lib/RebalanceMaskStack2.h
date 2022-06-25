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
//  RebalanceMaskStack2.h
//  BL-Rebalance
//
//  Created by applematuer on 6/14/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskStack2__
#define __BL_Rebalance__RebalanceMaskStack2__

//#include <deque>
//using namespace std;
#include <bl_queue.h>

#include "IPlug_include_in_plug_hdr.h"

// RebalanceMaskStack2: from RebalanceMaskStack
// - changed input and output types
//
class RebalanceMaskStack2
{
public:
    RebalanceMaskStack2(int width, int stackDepth);
    
    virtual ~RebalanceMaskStack2();
    
    void Reset();
    
    void AddMask(const WDL_TypedBuf<BL_FLOAT> &mask);
    
    // Reference
    void GetMaskAvg(WDL_TypedBuf<BL_FLOAT> *mask);
    // May not work..
    void GetMaskVariance(WDL_TypedBuf<BL_FLOAT> *mask);
    // Good
    // If index is not -1, compute only the result for index
    void GetMaskWeightedAvg(WDL_TypedBuf<BL_FLOAT> *mask, int index = -1);
    // Not so bad, but worse
    void GetMaskVariance2(WDL_TypedBuf<BL_FLOAT> *mask);

    // Compute standard deviation, and discard value if greater than stdev
    void GetMaskStdev(WDL_TypedBuf<BL_FLOAT> *mask, int index = -1);
    
    void GetLineAvg(WDL_TypedBuf<BL_FLOAT> *line, int lineNum);
                 
protected:
    //void AddMask(const deque<WDL_TypedBuf<BL_FLOAT> > &mask);
    void AddMask(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &mask);
    
    void GetMaskAvg(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask);
    void GetMaskVariance(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask);
    void GetMaskWeightedAvg(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask, int index = -1);
    void GetMaskVariance2(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask);
    void GetMaskStdev(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask, int index = -1);
    
    void BufferToQue(bl_queue<WDL_TypedBuf<BL_FLOAT> > *que,
                     const WDL_TypedBuf<BL_FLOAT> &buffer,
                     int width);
    
    void QueToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                     const bl_queue<WDL_TypedBuf<BL_FLOAT> > &cols);
    
    // For variance2
    BL_FLOAT ComputeVariance(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &history,
                             int index);


    void DBG_DumpMaskStack();
    
    //
    int mWidth;
    
    int mStackDepth;
    
    //deque<deque<WDL_TypedBuf<BL_FLOAT> > > mStack;
    bl_queue<bl_queue<WDL_TypedBuf<BL_FLOAT> > > mStack;
    
    // For variance2
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mLastHistory;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mAvgHistory;

private:
    // Tmp buffers
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mTmpBuf0;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mTmpBuf1;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mTmpBuf2;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mTmpBuf3;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    vector<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf12;
    vector<BL_FLOAT> mTmpBuf13;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf14;
};

#endif /* defined(__BL_Rebalance__RebalanceMaskStack2__) */
