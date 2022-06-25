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
//  RebalanceMaskStack.cpp
//  BL-Rebalance
//
//  Created by applematuer on 6/14/20.
//
//

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "RebalanceMaskStack.h"

RebalanceMaskStack::RebalanceMaskStack(int stackDepth)
{
    mStackDepth = stackDepth;
}

RebalanceMaskStack::~RebalanceMaskStack() {}

void
RebalanceMaskStack::AddMask(const deque<WDL_TypedBuf<BL_FLOAT> > &mask)
{
    if (mask.empty())
        return;
    
    // First, shift all the previous masks in the stack...
    WDL_TypedBuf<BL_FLOAT> undefinedLine;
    undefinedLine.Resize(mask[0].GetSize());
    BLUtils::FillAllValue(&undefinedLine, (BL_FLOAT)-1.0);
    
    for (int i = 0; i < mStack.size(); i++)
    {
        deque<WDL_TypedBuf<BL_FLOAT> > &stackLine = mStack[i];
        stackLine.push_back(undefinedLine);
        stackLine.pop_front();
    }
    
    // ... then add the new mask
    mStack.push_back(mask);
    if (mStack.size() > mStackDepth)
        mStack.pop_front();
}

void
RebalanceMaskStack::GetMaskAvg(deque<WDL_TypedBuf<BL_FLOAT> > *mask)
{
    mask->resize(0);
    
    if (mStack.empty())
        return;
    if (mStack[0].empty())
        return;
    
    mask->resize(mStack[0].size());
    for (int i = 0; i < mask->size(); i++)
    {
        (*mask)[i].Resize(mStack[0][0].GetSize());
        BLUtils::FillAllZero(&(*mask)[i]);
    }
    
    for (int i = 0; i < mask->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*mask)[i];
        for (int j = 0; j < line.GetSize(); j++)
        {
            BL_FLOAT sum = 0.0;
            int count = 0;
        
            for (int k = 0; k < mStack.size(); k++)
            {
                const deque<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];
                const WDL_TypedBuf<BL_FLOAT> &line0 = mask0[i];
            
                BL_FLOAT val = line0.Get()[j];
                if (val > 0.0)
                {
                    sum += val;
                    count++;
                }
            }
        
            if (count > 0)
            {
                BL_FLOAT avg = sum/count;
            
                line.Get()[j] = avg;
            }
        }
    }
}

void
RebalanceMaskStack::GetMaskVariance(deque<WDL_TypedBuf<BL_FLOAT> > *mask)
{
    mask->resize(0);
    
    if (mStack.empty())
        return;
    if (mStack[0].empty())
        return;
    
    mask->resize(mStack[0].size());
    for (int i = 0; i < mask->size(); i++)
    {
        (*mask)[i].Resize(mStack[0][0].GetSize());
        BLUtils::FillAllZero(&(*mask)[i]);
    }
    
    // Variance col
    for (int i = 0; i < mask->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*mask)[i];
        for (int j = 0; j < line.GetSize(); j++)
        {
            vector<BL_FLOAT> col;
            
            for (int k = 0; k < mStack.size(); k++)
            {
                const deque<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];
                const WDL_TypedBuf<BL_FLOAT> &line0 = mask0[i];
                
                BL_FLOAT val = line0.Get()[j];
                
                if (val > 0.0)
                    col.push_back(val);
            }
            
            BL_FLOAT var = BLUtilsMath::ComputeVariance(col);
            
            var *= col.size();
            
            line.Get()[j] = var;
        }
    }
}

void
RebalanceMaskStack::GetMaskWeightedAvg(deque<WDL_TypedBuf<BL_FLOAT> > *mask)
{
    mask->resize(0);
    
    if (mStack.empty())
        return;
    if (mStack[0].empty())
        return;
    
    mask->resize(mStack[0].size());
    for (int i = 0; i < mask->size(); i++)
    {
        (*mask)[i].Resize(mStack[0][0].GetSize());
        BLUtils::FillAllZero(&(*mask)[i]);
    }
    
    for (int i = 0; i < mask->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*mask)[i];
        for (int j = 0; j < line.GetSize(); j++)
        {
            BL_FLOAT sum = 0.0;
            BL_FLOAT sumWeights = 0.0;
            
            for (int k = 0; k < mStack.size(); k++)
            {
                const deque<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];
                const WDL_TypedBuf<BL_FLOAT> &line0 = mask0[i];
                
                BL_FLOAT val = line0.Get()[j];
                if (val > 0.0)
                {
                    BL_FLOAT w = 1.0;
                    if (mStack.size() > 1)
                    {
                        w = ((BL_FLOAT)k)/(mStack.size() - 1);
                        
                        // ?
                        //w = 1.0 - w;
                    }
                    
                    sum += val*w;
                    sumWeights += w;
                }
            }
            
            if (sumWeights > 0.0)
            {
                BL_FLOAT avg = sum/sumWeights;
                
                line.Get()[j] = avg;
            }
        }
    }
}

void
RebalanceMaskStack::GetLineAvg(WDL_TypedBuf<BL_FLOAT> *line, int lineNum)
{
    line->Resize(0);
    
    if (mStack.empty())
        return;
    if (mStack[0].empty())
        return;
    if (lineNum > mStack[0].size())
        return;
    
    line->Resize(mStack[0][0].GetSize());
    BLUtils::FillAllZero(line);
    
    for (int i = 0; i < line->GetSize(); i++)
    {
        BL_FLOAT sum = 0.0;
        int count = 0;
        
        for (int j = 0; j < mStack.size(); j++)
        {
            const deque<WDL_TypedBuf<BL_FLOAT> > &mask = mStack[j];
            
            const WDL_TypedBuf<BL_FLOAT> &line0 = mask[lineNum];
                
            BL_FLOAT val = line0.Get()[i];
            if (val > 0.0)
            {
                sum += val;
                count++;
            }
        }
        
        if (count > 0)
        {
            BL_FLOAT avg = sum/count;
            
            line->Get()[i] = avg;
        }
    }
}
