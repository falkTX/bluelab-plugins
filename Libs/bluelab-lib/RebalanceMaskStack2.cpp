//
//  RebalanceMaskStack2.cpp
//  BL-Rebalance
//
//  Created by applematuer on 6/14/20.
//
//

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "RebalanceMaskStack2.h"

#define VARIANCE2_HISTORY_SIZE 8

// GOOD
#define OPTIM_STACK_WEIGHTED_AVG 1


RebalanceMaskStack2::RebalanceMaskStack2(int width, int stackDepth)
{
    mWidth = width;
    
    mStackDepth = stackDepth;
}

RebalanceMaskStack2::~RebalanceMaskStack2() {}

void
RebalanceMaskStack2::Reset()
{
    mStack.clear();
    
    mLastHistory.clear();
    mAvgHistory.clear();
}

void
RebalanceMaskStack2::AddMask(const WDL_TypedBuf<BL_FLOAT> &mask)
{
    deque<WDL_TypedBuf<BL_FLOAT> > q;
    BufferToQue(&q, mask, mWidth);
    
    AddMask(q);
}

void
RebalanceMaskStack2::GetMaskAvg(WDL_TypedBuf<BL_FLOAT> *mask)
{
    deque<WDL_TypedBuf<BL_FLOAT> > q;
    GetMaskAvg(&q);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::GetMaskVariance(WDL_TypedBuf<BL_FLOAT> *mask)
{
    deque<WDL_TypedBuf<BL_FLOAT> > q;
    GetMaskVariance(&q);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::GetMaskWeightedAvg(WDL_TypedBuf<BL_FLOAT> *mask,
                                        int index)
{
    deque<WDL_TypedBuf<BL_FLOAT> > q;
    GetMaskWeightedAvg(&q, index);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::GetMaskVariance2(WDL_TypedBuf<BL_FLOAT> *mask)
{
    deque<WDL_TypedBuf<BL_FLOAT> > q;
    GetMaskVariance2(&q);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::AddMask(const deque<WDL_TypedBuf<BL_FLOAT> > &mask)
{
    if (mask.empty())
        return;
    
    // First, shift all the previous masks in the stack...
    WDL_TypedBuf<BL_FLOAT> undefinedLine;
    undefinedLine.Resize(mask[0].GetSize());
    BLUtils::FillAllValue(&undefinedLine, (BL_FLOAT)-1.0);

    // Scroll
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
RebalanceMaskStack2::GetMaskAvg(deque<WDL_TypedBuf<BL_FLOAT> > *mask)
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
RebalanceMaskStack2::GetMaskVariance(deque<WDL_TypedBuf<BL_FLOAT> > *mask)
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

#if 0 // Consume a lot of resource
void
RebalanceMaskStack2::GetMaskWeightedAvg(deque<WDL_TypedBuf<BL_FLOAT> > *mask)
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
#endif

// OPTIM
// Avoid many access to deque
void
RebalanceMaskStack2::GetMaskWeightedAvg(deque<WDL_TypedBuf<BL_FLOAT> > *mask,
                                        int index)
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
    
    int numValues = mask->size()*(*mask)[0].GetSize();
    
    WDL_TypedBuf<BL_FLOAT> sum;
    sum.Resize(numValues);
    BLUtils::FillAllZero(&sum);
    
    WDL_TypedBuf<BL_FLOAT> sumWeights;
    sumWeights.Resize(numValues);
    BLUtils::FillAllZero(&sumWeights);
    
    // Compute the sums
    //
    for (int k = 0; k < mStack.size(); k++)
    {
        const deque<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];
        
        BL_FLOAT w = 1.0;
        if (mStack.size() > 1)
        {
            w = ((BL_FLOAT)k)/(mStack.size() - 1);
            
            // ?
            //w = 1.0 - w;
        }
        
        for (int i = 0; i < mask->size(); i++)
        {
#if OPTIM_STACK_WEIGHTED_AVG
            if ((index >= 0) && (i != index))
                continue;
#endif
            const WDL_TypedBuf<BL_FLOAT> &line0 = mask0[i];
            
            for (int j = 0; j < line0.GetSize(); j++)
            {
                int index = i + j*mask->size();
                
                BL_FLOAT val = line0.Get()[j];
                if (val > 0.0)
                {
                    sum.Get()[index] += val*w;
                    sumWeights.Get()[index] += w;
                }
            }
        }
    }
    
    // Update the mask with avg
    for (int i = 0; i < mask->size(); i++)
    {
#if OPTIM_STACK_WEIGHTED_AVG
        if ((index >= 0) && (i != index))
            continue;
#endif
        
        WDL_TypedBuf<BL_FLOAT> &line0 = (*mask)[i];
        
        for (int j = 0; j < line0.GetSize(); j++)
        {
            int index = i + j*mask->size();
            if (sumWeights.Get()[index] > 0.0)
            {
                BL_FLOAT avg = sum.Get()[index]/sumWeights.Get()[index];
        
                line0.Get()[j] = avg;
            }
        }
    }
}

// Not so bad, but simple avg is better
void
RebalanceMaskStack2::GetMaskVariance2(deque<WDL_TypedBuf<BL_FLOAT> > *mask)
{    
#define EPS 1e-15
    
    mask->resize(0);
    
    if (mStack.empty())
        return;
    if (mStack[0].empty())
        return;
    
    *mask = mStack[mStack.size() - 1];
    
    // Last value
    WDL_TypedBuf<BL_FLOAT> mask0;
    QueToBuffer(&mask0, *mask);
    
    mLastHistory.push_back(mask0);
    if (mLastHistory.size() > VARIANCE2_HISTORY_SIZE)
        mLastHistory.pop_front();
    
    // Avg
    WDL_TypedBuf<BL_FLOAT> avg0;
    GetMaskAvg(&avg0);
    
    //
    WDL_TypedBuf<BL_FLOAT> maskDiv = mask0;
    BL_FLOAT stackSizeInv = 1.0/mStackDepth; //
    BLUtils::MultValues(&maskDiv, stackSizeInv);
    
    WDL_TypedBuf<BL_FLOAT> avgSub = avg0;
    BLUtils::SubstractValues(&avgSub, maskDiv);
    
    BLUtils::ClipMin(&avgSub, (BL_FLOAT)0.0);
    
    //
    mAvgHistory.push_back(avgSub);
    if (mAvgHistory.size() > VARIANCE2_HISTORY_SIZE)
        mAvgHistory.pop_front();
    
    for (int i = 0; i < mask0.GetSize(); i++)
    {
        BL_FLOAT varAvg = ComputeVariance(mAvgHistory, i);
        BL_FLOAT varLast = ComputeVariance(mLastHistory, i);
        
        BL_FLOAT coeff = 1.0;
        if (std::fabs(varAvg) > EPS)
            coeff = varLast/(varAvg + varLast);
        
        mask0.Get()[i] = avg0.Get()[i]*coeff;
    }
    
    BLUtils::ClipMax(&mask0, (BL_FLOAT)1.0);
    
    BufferToQue(mask, mask0, mWidth);
}

BL_FLOAT
RebalanceMaskStack2::ComputeVariance(const deque<WDL_TypedBuf<BL_FLOAT> > &history, int index)
{
    vector<BL_FLOAT> col;
    for (int k = 0; k < history.size(); k++)
    {
        BL_FLOAT val = history[k].Get()[index];
                
        if (val > 0.0)
            col.push_back(val);
    }
    
    if (col.size() == 1)
        return col[0];
    
    BL_FLOAT var = BLUtilsMath::ComputeVariance(col);
    //var *= col.size();
    
    return var;
}

void
RebalanceMaskStack2::GetLineAvg(WDL_TypedBuf<BL_FLOAT> *line, int lineNum)
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
            
            const WDL_TypedBuf<BL_FLOAT> &line = mask[lineNum];
                
            BL_FLOAT val = line.Get()[i];
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

void
RebalanceMaskStack2::BufferToQue(deque<WDL_TypedBuf<BL_FLOAT> > *que,
                                 const WDL_TypedBuf<BL_FLOAT> &buffer,
                                 int width)
{
    int height = buffer.GetSize()/width;
    
    que->resize(height);
    for (int j = 0; j < height; j++)
    {
        WDL_TypedBuf<BL_FLOAT> line;
        line.Add(&buffer.Get()[j*width], width);
        (*que)[j] = line;
    }
}

void
RebalanceMaskStack2::QueToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                 const deque<WDL_TypedBuf<BL_FLOAT> > &cols)
{
    if (cols.empty())
        return;
    
    buf->Resize(cols.size()*cols[0].GetSize());
    
    for (int j = 0; j < cols.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &col = cols[j];
        for (int i = 0; i < col.GetSize(); i++)
        {
            int bufIndex = i + j*col.GetSize();
            
            buf->Get()[bufIndex] = col.Get()[i];
        }
    }
}
