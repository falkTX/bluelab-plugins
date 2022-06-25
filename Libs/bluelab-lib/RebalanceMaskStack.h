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
//  RebalanceMaskStack.h
//  BL-Rebalance
//
//  Created by applematuer on 6/14/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskStack__
#define __BL_Rebalance__RebalanceMaskStack__

#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class RebalanceMaskStack
{
public:
    RebalanceMaskStack(int stackDepth);
    
    virtual ~RebalanceMaskStack();
    
    void AddMask(const deque<WDL_TypedBuf<BL_FLOAT> > &mask);
    
    void GetMaskAvg(deque<WDL_TypedBuf<BL_FLOAT> > *mask);
    
    void GetMaskVariance(deque<WDL_TypedBuf<BL_FLOAT> > *mask);
    
    void GetMaskWeightedAvg(deque<WDL_TypedBuf<BL_FLOAT> > *mask);
    
    void GetLineAvg(WDL_TypedBuf<BL_FLOAT> *line, int lineNum);
                 
protected:
    int mStackDepth;
    
    deque<deque<WDL_TypedBuf<BL_FLOAT> > > mStack;
};

#endif /* defined(__BL_Rebalance__RebalanceMaskStack__) */
