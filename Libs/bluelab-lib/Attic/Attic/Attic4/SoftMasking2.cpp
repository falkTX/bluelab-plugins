//
//  SoftMasking2.cpp
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#include <BLUtils.h>

#include "SoftMasking2.h"


// Should be 1 in the original algorithm
#define MIXTURE_SUB 1 //0

// Fill the history progressively
// (instead of filling it all with the first value at the beginning)
#define PROGRESSIVE_HISTORY 0 //1


SoftMasking2::SoftMasking2(int historySize)
{
    mHistorySize = historySize;
}

SoftMasking2::~SoftMasking2() {}

void
SoftMasking2::Reset()
{
    mMixtureHistory.clear();
    mHistory.clear();
}

void
SoftMasking2::SetHistorySize(int size)
{
    mHistorySize = size;
    
    Reset();
}

// Process over time
void
SoftMasking2::Process(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                      const WDL_TypedBuf<BL_FLOAT> &magns,
                      WDL_TypedBuf<BL_FLOAT> *softMask)
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
#if !PROGRESSIVE_HISTORY
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
#else
    mMixtureHistory.push_back(mixtureMagnsSub);
    mHistory.push_back(magns);
    
    while (mMixtureHistory.size() > mHistorySize)
        mMixtureHistory.pop_front();
    while (mHistory.size() > mHistorySize)
        mHistory.pop_front();
#endif
    
    WDL_TypedBuf<BL_FLOAT> varianceMixture;
    ComputeVariance(mMixtureHistory, &varianceMixture);
    
    WDL_TypedBuf<BL_FLOAT> varianceMagns;
    ComputeVariance(mHistory, &varianceMagns);
    
    // Create the mask
    if (mMixtureHistory.empty()) // Just in case
        return;
    
    softMask->Resize(mMixtureHistory[0].GetSize());
    for (int i = 0; i < softMask->GetSize(); i++)
    {
        BL_FLOAT mix = varianceMixture.Get()[i];
        BL_FLOAT magn = varianceMagns.Get()[i];
        
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
        
        softMask->Get()[i] = maskVal;
    }
}

#if 0
// TODO: for optimization, avoid many access to deque
void
SoftMasking2::ComputeVariance(deque<WDL_TypedBuf<BL_FLOAT> > &history,
                              WDL_TypedBuf<BL_FLOAT> *outVariance)
{
    if (history.empty())
        return;
    
    outVariance->Resize(history[0].GetSize());
                     
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
            outVariance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        BL_FLOAT variance = 0.0;
        for (int j = 0; j < history.size(); j++)
        {
            BL_FLOAT val = history[j].Get()[i];
            
            variance += (val - avg)*(val - avg);
        }
        variance /= history.size();
        
        // Result
        outVariance->Get()[i] = variance;
    }
}
#endif

// Optimized version
void
SoftMasking2::ComputeVariance(deque<WDL_TypedBuf<BL_FLOAT> > &history,
                              WDL_TypedBuf<BL_FLOAT> *outVariance)
{
    if (history.empty())
        return;
    
    outVariance->Resize(history[0].GetSize());
    
    // Convert deque of vector to vector of vector
    vector<WDL_TypedBuf<BL_FLOAT> > historyVec;
    historyVec.resize(history.size());
    for (int i = 0; i < historyVec.size(); i++)
        historyVec[i] = history[i];
    
    BL_FLOAT invHistorySize = 1.0/historyVec.size();
    for (int i = 0; i < historyVec[0].GetSize(); i++)
    {
        // Compute average over time
        BL_FLOAT avg = 0.0;
        for (int j = 0; j < historyVec.size(); j++)
        {
            BL_FLOAT val = historyVec[j].Get()[i];
            
            avg += val;
        }
        avg *= invHistorySize;
        
        if (historyVec.size() == 1)
        {
            outVariance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        BL_FLOAT variance = 0.0;
        for (int j = 0; j < historyVec.size(); j++)
        {
            BL_FLOAT val = historyVec[j].Get()[i];
            
            variance += (val - avg)*(val - avg);
        }
        variance *= invHistorySize;
        
        // Result
        outVariance->Get()[i] = variance;
    }
}
