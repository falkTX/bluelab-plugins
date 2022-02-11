//
//  SoftMaskingN.cpp
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#include <BLUtils.h>

#include "SoftMaskingN.h"


// Should be 1 in the original algorithm
#define MIXTURE_SUB 1 //0

//PROGRESSIVE_HISTORY

SoftMaskingN::SoftMaskingN(int historySize)
{
    mHistorySize = historySize;
}

SoftMaskingN::~SoftMaskingN() {}

void
SoftMaskingN::Reset()
{
    mMixtureHistory.clear();
    mHistory.clear();
}

void
SoftMaskingN::SetHistorySize(int size)
{
    mHistorySize = size;
    
    Reset();
}

// Process over time
//void
//SoftMaskingN::Process(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
//                      const WDL_TypedBuf<BL_FLOAT> &magns,
//                      WDL_TypedBuf<BL_FLOAT> *softMask)

// First algorithm: take the masks one by one,
// and process them accrding to the global mix
void
SoftMaskingN::Process(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                      const vector<WDL_TypedBuf<BL_FLOAT> > &estimMagns,
                      vector<WDL_TypedBuf<BL_FLOAT> > *softMasks)
{
#define EPS 1e-15
    
    // Resize if necessary
    if (mMixtureHistory.size() != estimMagns.size())
        mMixtureHistory.resize(estimMagns.size());
    
    if (mHistory.size() != estimMagns.size())
        mHistory.resize(estimMagns.size());
    
    if (softMasks->size() != estimMagns.size())
        softMasks->resize(estimMagns.size());
    
    // See: https://hal.inria.fr/inria-00544949/document
    //
    for (int k = 0; k < estimMagns.size(); k++)
    {
        // Mixture is the whole mixture, minus the sound corresponding to the mask
        WDL_TypedBuf<BL_FLOAT> mixtureMagnsSub = mixtureMagns;
    
#if MIXTURE_SUB
        BLUtils::SubstractValues(&mixtureMagnsSub, estimMagns[k]);
#endif
    
        // Manage the history
        //
        mMixtureHistory[k].push_back(mixtureMagnsSub); // cppcheck alarming warning
        while (mMixtureHistory[k].size() > mHistorySize)
            mMixtureHistory[k].pop_front();
        
        mHistory[k].push_back(estimMagns[k]);
        while (mHistory[k].size() > mHistorySize)
            mHistory[k].pop_front();
    
        WDL_TypedBuf<BL_FLOAT> varianceMixture;
        ComputeVariance(mMixtureHistory[k], &varianceMixture);
    
        WDL_TypedBuf<BL_FLOAT> varianceMagns;
        ComputeVariance(mHistory[k], &varianceMagns);
    
        // Create the mask
        if (mMixtureHistory.empty()) // Just in case
            return;
    
        (*softMasks)[k].Resize(mMixtureHistory[k][0].GetSize());
        for (int i = 0; i < (*softMasks)[k].GetSize(); i++)
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
        
            (*softMasks)[k].Get()[i] = maskVal;
        }
    }
}

// Optimized version
void
SoftMaskingN::ComputeVariance(deque<WDL_TypedBuf<BL_FLOAT> > &history,
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
