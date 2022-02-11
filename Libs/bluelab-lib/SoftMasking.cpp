//
//  SoftMasking.cpp
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#include <BLUtils.h>

#include "SoftMasking.h"

// Should be 1 in the original algorithm
#define MIXTURE_SUB 1 //0 //1

SoftMasking::SoftMasking(int historySize)
{
    mHistorySize = historySize;
    
    // Debug
    mFreqWinSize = 4;
}

SoftMasking::~SoftMasking() {}

void
SoftMasking::Reset()
{
    mMixtureHistory.clear();
    mHistory.clear();
}

void
SoftMasking::SetHistorySize(int size)
{
    mHistorySize = size;
    
    Reset();
}

void
SoftMasking::Process(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                     const WDL_TypedBuf<BL_FLOAT> &magns,
                     WDL_TypedBuf<BL_FLOAT> *mask)
{
    // Best sound quality
    ProcessTime(mixtureMagns, magns, mask);
    
    //ProcessFreq(mixtureMagns, magns, mask);
    
    //ProcessTimeFreq(mixtureMagns, magns, mask);
    
#if 0
    // TEST
    WDL_TypedBuf<BL_FLOAT> mask0;
    WDL_TypedBuf<BL_FLOAT> mask1;
    ProcessTime(mixtureMagns, magns, &mask0);
    ProcessFreq(mixtureMagns, magns, &mask1);
    
    *mask = mask0;
    // Mult
    //BLUtils::MultValues(mask, mask1);
    // Or average
    BLUtils::AddValues(mask, mask1);
    BLUtils::MultValues(mask, 0.5);
#endif
}

void
SoftMasking::ProcessTime(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                         const WDL_TypedBuf<BL_FLOAT> &magns,
                         WDL_TypedBuf<BL_FLOAT> *mask)
{
#define EPS 1e-15
    
    // See: https://hal.inria.fr/inria-00544949/document
    //
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    WDL_TypedBuf<BL_FLOAT> mixtureMagnsSub = mixtureMagns;
    
#if MIXTURE_SUB
    BLUtils::SubstractValues(&mixtureMagnsSub, magns);
#endif
    
    // Manage the history
    //
    if (mMixtureHistory.empty())
    {
        // Fill the whole history with the current values
        for (int i = 0; i < mHistorySize; i++)
        {
            mMixtureHistory.push_back(mixtureMagnsSub);
            mHistory.push_back(magns);
        }
    }
    else
    {
        mMixtureHistory.push_back(mixtureMagnsSub);
        mMixtureHistory.pop_front();
        
        mHistory.push_back(magns);
        mHistory.pop_front();
    }
    
    WDL_TypedBuf<BL_FLOAT> varMixture;
    ComputeVariance(mMixtureHistory, &varMixture);
    
    WDL_TypedBuf<BL_FLOAT> varMagns;
    ComputeVariance(mHistory, &varMagns);
    
    // Create the mask
    if (mMixtureHistory.empty()) // Just in case
        return;
    
    mask->Resize(mMixtureHistory[0].GetSize());
    for (int i = 0; i < mask->GetSize(); i++)
    {
        BL_FLOAT mix = varMixture.Get()[i];
        BL_FLOAT magn = varMagns.Get()[i];
        
        BL_FLOAT maskVal = 0.0;
        
#if MIXTURE_SUB
        if (std::fabs(magn + mix) > EPS)
        {
            maskVal = magn/(magn + mix);
        }
#else
        if (std::fabs(mix) > EPS)
        {
            maskVal = magn/mix;
        }
#endif
        
        if (maskVal >  1.0)
            maskVal = 1.0;
        
        mask->Get()[i] = maskVal;
    }
}

void
SoftMasking::ProcessFreq(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                         const WDL_TypedBuf<BL_FLOAT> &magns,
                         WDL_TypedBuf<BL_FLOAT> *mask)
{
#define EPS 1e-15
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    WDL_TypedBuf<BL_FLOAT> mixtureMagnsSub = mixtureMagns;
    
#if MIXTURE_SUB
    BLUtils::SubstractValues(&mixtureMagnsSub, magns);
#endif
    
    WDL_TypedBuf<BL_FLOAT> varMixture;
    ComputeVariance(mixtureMagnsSub, &varMixture);
    
    WDL_TypedBuf<BL_FLOAT> varMagns;
    ComputeVariance(magns, &varMagns);
    
    mask->Resize(magns.GetSize());
    for (int i = 0; i < mask->GetSize(); i++)
    {
        BL_FLOAT mix = varMixture.Get()[i];
        BL_FLOAT magn = varMagns.Get()[i];
        
        BL_FLOAT maskVal = 0.0;
        
#if MIXTURE_SUB
        if (std::fabs(magn + mix) > EPS)
        {
            maskVal = magn/(magn + mix);
        }
#else
        if (std::fabs(mix) > EPS)
        {
            maskVal = magn/mix;
        }
#endif
        
        if (maskVal >  1.0)
            maskVal = 1.0;
        
        mask->Get()[i] = maskVal;
    }
}

void
SoftMasking::ProcessTimeFreq(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                             const WDL_TypedBuf<BL_FLOAT> &magns,
                             WDL_TypedBuf<BL_FLOAT> *mask)
{
#define EPS 1e-15
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    WDL_TypedBuf<BL_FLOAT> mixtureMagnsSub = mixtureMagns;
    
#if MIXTURE_SUB
    BLUtils::SubstractValues(&mixtureMagnsSub, magns);
#endif
    
    // Manage the history
    //
    if (mMixtureHistory.empty())
    {
        // Fill the whole history with the current values
        for (int i = 0; i < mHistorySize; i++)
        {
            mMixtureHistory.push_back(mixtureMagnsSub);
            mHistory.push_back(magns);
        }
    }
    else
    {
        mMixtureHistory.push_back(mixtureMagnsSub);
        mMixtureHistory.pop_front();
        
        mHistory.push_back(magns);
        mHistory.pop_front();
    }
    
    WDL_TypedBuf<BL_FLOAT> varMixture;
    ComputeVarianceWin(mMixtureHistory, &varMixture);
    
    WDL_TypedBuf<BL_FLOAT> varMagns;
    ComputeVarianceWin(mHistory, &varMagns);
    
    // Create the mask
    if (mMixtureHistory.empty()) // Just in case
        return;
    
    mask->Resize(mMixtureHistory[0].GetSize());
    for (int i = 0; i < mask->GetSize(); i++)
    {
        BL_FLOAT mix = varMixture.Get()[i];
        BL_FLOAT magn = varMagns.Get()[i];
        
        BL_FLOAT maskVal = 0.0;
        
#if MIXTURE_SUB
        if (std::fabs(magn + mix) > EPS)
        {
            maskVal = magn/(magn + mix);
        }
#else
        if (std::fabs(mix) > EPS)
        {
            maskVal = magn/mix;
        }
#endif
        
        if (maskVal >  1.0)
            maskVal = 1.0;
        
        mask->Get()[i] = maskVal;
    }
}

// TODO: for optimization, avoid many access to deque
void
SoftMasking::ComputeVariance(deque<WDL_TypedBuf<BL_FLOAT> > &history,
                             WDL_TypedBuf<BL_FLOAT> *variance)
{
    if (history.empty())
        return;
    
    variance->Resize(history[0].GetSize());
                     
    for (int i = 0; i < history[0].GetSize(); i++)
    {
        // Compute average over time
        BL_FLOAT avg = 0.0;
        for (int j = 0; j < history.size(); j++)
        {
            BL_FLOAT val = history[j].Get()[i];
            
            avg += val;
        }
        avg /= history.size();
        
        if (history.size() == 1)
        {
            variance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        BL_FLOAT var = 0.0;
        for (int j = 0; j < history.size(); j++)
        {
            BL_FLOAT val = history[j].Get()[i];
            
            var += (val - avg)*(val - avg);
        }
        var /= history.size();
        
        // Result
        variance->Get()[i] = var;
    }
}

// Compute the variance over a window
void
SoftMasking::ComputeVariance(const WDL_TypedBuf<BL_FLOAT> &data,
                             WDL_TypedBuf<BL_FLOAT> *variance)
{
    variance->Resize(data.GetSize());
    
    for (int i = 0; i < data.GetSize(); i++)
    {
        // compute the average
        BL_FLOAT avg = 0.0;
        int numValues = 0;
        for (int j = -mFreqWinSize/2; j <= mFreqWinSize/2; j++)
        {
            int idx = i + j;
            if (idx < 0)
                idx = 0;
            if (idx > data.GetSize() - 1)
                idx = data.GetSize() - 1;
            
            BL_FLOAT val = data.Get()[idx];
            
            avg += val;
            numValues++;
        }
        if (numValues > 0)
            avg /= numValues;
        
        if (numValues == 1)
        {
            variance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        BL_FLOAT var = 0.0;
        int numValues2 = 0;
        for (int j = -mFreqWinSize/2; j <= mFreqWinSize/2; j++)
        {
            int idx = i + j;
            if (idx < 0)
                idx = 0;
            if (idx > data.GetSize() - 1)
                idx = data.GetSize() - 1;
            
            BL_FLOAT val = data.Get()[idx];
            
            var += (val - avg)*(val - avg);
            
            numValues2++;
        }
        if (numValues2 > 0)
            var /= numValues2;
        
        // Result
        variance->Get()[i] = var;
    }
}

// TODO: for optimization, avoid many access to deque
void
SoftMasking::ComputeVarianceWin(deque<WDL_TypedBuf<BL_FLOAT> > &history,
                                WDL_TypedBuf<BL_FLOAT> *variance)
{
    if (history.empty())
        return;
    
    variance->Resize(history[0].GetSize());
    
    for (int i = 0; i < history[0].GetSize(); i++)
    {
        // Compute average over time
        BL_FLOAT avg = 0.0;
        int numValues = 0;
        for (int i0 = -mFreqWinSize/2; i0 <= mFreqWinSize/2; i0++)
        {
            int idx = i + i0;
            if (idx < 0)
                idx = 0;
            if (idx > history[0].GetSize() - 1)
                idx = history[0].GetSize() - 1;
            
            for (int j = 0; j < history.size(); j++)
            {
                BL_FLOAT val = history[j].Get()[idx];
            
                avg += val;
                numValues++;
            }
        }
        if (numValues > 0)
        avg /= numValues;
        
        if (history.size() == 1)
        {
            variance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        BL_FLOAT var = 0.0;
        int numValues2 = 0;
        for (int i0 = -mFreqWinSize/2; i0 <= mFreqWinSize/2; i0++)
        {
            int idx = i + i0;
            if (idx < 0)
                idx = 0;
            if (idx > history[0].GetSize() - 1)
                idx = history[0].GetSize() - 1;
            
            for (int j = 0; j < history.size(); j++)
            {
                BL_FLOAT val = history[j].Get()[idx];
            
                var += (val - avg)*(val - avg);
                
                numValues2++;
            }
        }
        if (numValues2 > 0)
            var /= numValues2;
        
        // Result
        variance->Get()[i] = var;
    }
}
