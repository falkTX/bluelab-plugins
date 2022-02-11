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
