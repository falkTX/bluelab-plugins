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
    //deque<WDL_TypedBuf<BL_FLOAT> > q;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > &q = mTmpBuf0;
    q.freeze(); // TEST
    BufferToQue(&q, mask, mWidth);
    
    AddMask(q);
}

void
RebalanceMaskStack2::GetMaskAvg(WDL_TypedBuf<BL_FLOAT> *mask)
{
    bl_queue<WDL_TypedBuf<BL_FLOAT> > &q = mTmpBuf1;
    q.freeze(); // TEST
    GetMaskAvg(&q);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::GetMaskVariance(WDL_TypedBuf<BL_FLOAT> *mask)
{
    bl_queue<WDL_TypedBuf<BL_FLOAT> > &q = mTmpBuf2;
    q.freeze(); // TEST
    GetMaskVariance(&q);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::GetMaskWeightedAvg(WDL_TypedBuf<BL_FLOAT> *mask,
                                        int index)
{
    bl_queue<WDL_TypedBuf<BL_FLOAT> > &q = mTmpBuf3;
    q.freeze(); // TEST
    GetMaskWeightedAvg(&q, index);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::GetMaskVariance2(WDL_TypedBuf<BL_FLOAT> *mask)
{
    bl_queue<WDL_TypedBuf<BL_FLOAT> > &q = mTmpBuf4;
    q.freeze(); // TEST
    GetMaskVariance2(&q);
    QueToBuffer(mask, q);
}

void
RebalanceMaskStack2::AddMask(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &mask)
{
    if (mask.empty())
        return;
    
    // First, shift all the previous masks in the stack...
    WDL_TypedBuf<BL_FLOAT> &undefinedLine = mTmpBuf5;
    undefinedLine.Resize(mask[0].GetSize());
    BLUtils::FillAllValue(&undefinedLine, (BL_FLOAT)-1.0);

    // Scroll
    for (int i = 0; i < mStack.size(); i++)
    {
        bl_queue<WDL_TypedBuf<BL_FLOAT> > &stackLine = mStack[i];
        //stackLine.push_back(undefinedLine);
        //stackLine.pop_front();
        stackLine.freeze();
        stackLine.push_pop(undefinedLine);
    }

    // ... then add the new mask
    if (mStack.size() < mStackDepth)
        mStack.push_back(mask);
    else
    {
        //if (mStack.size() >= mStackDepth)
        mStack.freeze();
        mStack.push_pop(mask);
    }
    //if (mStack.size() > mStackDepth)
    //    mStack.pop_front();
}

void
RebalanceMaskStack2::GetMaskAvg(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask)
{  
    if (mStack.empty())
    {
        mask->resize(0);   

        return;
    }
    
    if (mStack[0].empty())
    {
        mask->resize(0);
        
        return;
    }
    
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
                const bl_queue<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];
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
RebalanceMaskStack2::GetMaskVariance(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask)
{
    if (mStack.empty())
    {
        mask->resize(0);
        
        return;
    }
    
    if (mStack[0].empty())
    {
        mask->resize(0);
        
        return;
    }
    
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
            int colSize = 0;
            for (int k = 0; k < mStack.size(); k++)
            {
                const bl_queue<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];    
                const WDL_TypedBuf<BL_FLOAT> &line0 = mask0[i];
                
                BL_FLOAT val = line0.Get()[j];
                
                if (val > 0.0)
                    colSize++;
            }
            
            vector<BL_FLOAT> &col = mTmpBuf6;
            col.resize(colSize);

            int colIdx = 0;
            for (int k = 0; k < mStack.size(); k++)
            {
                const bl_queue<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];
                
                const WDL_TypedBuf<BL_FLOAT> &line0 = mask0[i];
                
                BL_FLOAT val = line0.Get()[j];
                
                if (val > 0.0)
                    //col.push_back(val);
                    col[colIdx++] = val;
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
    if (mStack.empty())
    {
        mask->resize(0);
        
        return;
    }
    
    if (mStack[0].empty())
    {
        mask->resize(0);
        
        return;
    }
    
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
RebalanceMaskStack2::GetMaskWeightedAvg(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask,
                                        int index)
{    
    if (mStack.empty())
    {
        mask->resize(0);
        
        return;
    }
    
    if (mStack[0].empty())
    {
        mask->resize(0);
        
        return;
    }
    
    mask->resize(mStack[0].size());
    for (int i = 0; i < mask->size(); i++)
    {
        (*mask)[i].Resize(mStack[0][0].GetSize());
        BLUtils::FillAllZero(&(*mask)[i]);
    }
    
    int numValues = mask->size()*(*mask)[0].GetSize();
    
    WDL_TypedBuf<BL_FLOAT> &sum = mTmpBuf7;
    sum.Resize(numValues);
    BLUtils::FillAllZero(&sum);
    
    WDL_TypedBuf<BL_FLOAT> &sumWeights = mTmpBuf8;
    sumWeights.Resize(numValues);
    BLUtils::FillAllZero(&sumWeights);
    
    // Compute the sums
    //
    for (int k = 0; k < mStack.size(); k++)
    {
        const bl_queue<WDL_TypedBuf<BL_FLOAT> > &mask0 = mStack[k];
        
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
            int line0Size = line0.GetSize();
            BL_FLOAT *line0Data = line0.Get();

            int maskSize = mask->size();

            BL_FLOAT *sumData = sum.Get();
            BL_FLOAT *sumWeightsData = sumWeights.Get();
            
            for (int j = 0; j < line0Size; j++)
            {
                //int index = i + j*mask->size();
                int index = i + j*maskSize;
                
                //BL_FLOAT val = line0.Get()[j];
                BL_FLOAT val = line0Data[j];
                if (val > 0.0)
                {
                    sumData[index] += val*w;
                    sumWeightsData[index] += w;
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

        int line0Size = line0.GetSize();
        BL_FLOAT *line0Data = line0.Get();

        int maskSize = mask->size();

        BL_FLOAT *sumData = sum.Get();
        BL_FLOAT *sumWeightsData = sumWeights.Get();
        
        for (int j = 0; j < line0Size; j++)
        {
            int index = i + j*maskSize;
            if (sumWeightsData[index] > 0.0)
            {
                BL_FLOAT avg = sumData[index]/sumWeightsData[index];
        
                line0Data[j] = avg;
            }
        }
    }
}

// Not so bad, but simple avg is better
void
RebalanceMaskStack2::GetMaskVariance2(bl_queue<WDL_TypedBuf<BL_FLOAT> > *mask)
{        
    if (mStack.empty())
    {
        mask->resize(0);
        
        return;
    }
    
    if (mStack[0].empty())
    {
        mask->resize(0);
        
        return;
    }
    
    *mask = mStack[mStack.size() - 1];
    
    // Last value
    WDL_TypedBuf<BL_FLOAT> &mask0 = mTmpBuf9;
    QueToBuffer(&mask0, *mask);

    if (mLastHistory.size() < VARIANCE2_HISTORY_SIZE)
        mLastHistory.push_back(mask0);
    else
    {
        mLastHistory.freeze();
        mLastHistory.push_pop(mask0);
    }
    //if (mLastHistory.size() > VARIANCE2_HISTORY_SIZE)
    //    mLastHistory.pop_front();
    
    // Avg
    WDL_TypedBuf<BL_FLOAT> &avg0 = mTmpBuf10;
    GetMaskAvg(&avg0);
    
    //
    WDL_TypedBuf<BL_FLOAT> &maskDiv = mTmpBuf11;
    maskDiv = mask0;
    BL_FLOAT stackSizeInv = 1.0/mStackDepth; //
    BLUtils::MultValues(&maskDiv, stackSizeInv);
    
    WDL_TypedBuf<BL_FLOAT> &avgSub = mTmpBuf12;
    avgSub = avg0;
    BLUtils::SubstractValues(&avgSub, maskDiv);
    
    BLUtils::ClipMin(&avgSub, (BL_FLOAT)0.0);
    
    //
    if (mAvgHistory.size() < VARIANCE2_HISTORY_SIZE)
        mAvgHistory.push_back(avgSub);
    else
    {
        mAvgHistory.freeze();
        mAvgHistory.push_pop(avgSub);
    }
    //if (mAvgHistory.size() > VARIANCE2_HISTORY_SIZE)
    //    mAvgHistory.pop_front();
    
    for (int i = 0; i < mask0.GetSize(); i++)
    {
        BL_FLOAT varAvg = ComputeVariance(mAvgHistory, i);
        BL_FLOAT varLast = ComputeVariance(mLastHistory, i);
        
        BL_FLOAT coeff = 1.0;
        if (std::fabs(varAvg) > BL_EPS)
            coeff = varLast/(varAvg + varLast);
        
        mask0.Get()[i] = avg0.Get()[i]*coeff;
    }
    
    BLUtils::ClipMax(&mask0, (BL_FLOAT)1.0);
    
    BufferToQue(mask, mask0, mWidth);
}

BL_FLOAT
RebalanceMaskStack2::ComputeVariance(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &history,
                                     int index)
{
    int colSize = 0;
    for (int k = 0; k < history.size(); k++)
    {
        BL_FLOAT val = history[k].Get()[index];
                
        if (val > 0.0)
            colSize++;
    }
    
    vector<BL_FLOAT> &col = mTmpBuf13;
    col.resize(colSize);

    int colIdx = 0;
    for (int k = 0; k < history.size(); k++)
    {
        BL_FLOAT val = history[k].Get()[index];
                
        if (val > 0.0)
            //col.push_back(val);
            col[colIdx++] = val;
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
    if (mStack.empty())
    {
        line->Resize(0);
        
        return;
    }
    
    if (mStack[0].empty())
    {
        line->Resize(0);
        
        return;
    }
    
    if (lineNum > mStack[0].size())
    {
        line->Resize(0);
        
        return;
    }
    
    line->Resize(mStack[0][0].GetSize());
    BLUtils::FillAllZero(line);
    
    for (int i = 0; i < line->GetSize(); i++)
    {
        BL_FLOAT sum = 0.0;
        int count = 0;
        
        for (int j = 0; j < mStack.size(); j++)
        {
            const bl_queue<WDL_TypedBuf<BL_FLOAT> > &mask = mStack[j];
            
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
RebalanceMaskStack2::BufferToQue(bl_queue<WDL_TypedBuf<BL_FLOAT> > *que,
                                 const WDL_TypedBuf<BL_FLOAT> &buffer,
                                 int width)
{
    int height = buffer.GetSize()/width;
    
    que->resize(height);
    for (int j = 0; j < height; j++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = mTmpBuf14;
        //line.Add(&buffer.Get()[j*width], width);
        line.Resize(width);
        memcpy(line.Get(), &buffer.Get()[j*width], width*sizeof(BL_FLOAT));
        
        (*que)[j] = line;
    }
}

void
RebalanceMaskStack2::QueToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                 const bl_queue<WDL_TypedBuf<BL_FLOAT> > &cols)
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
