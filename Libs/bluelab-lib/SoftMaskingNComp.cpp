//
//  SoftMaskingNComp.cpp
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include "SoftMaskingNComp.h"

// Should be 1 in the original algorithm
#define MIXTURE_SUB 1 //0

SoftMaskingNComp::SoftMaskingNComp(int historySize)
{
    mHistorySize = historySize;
}

SoftMaskingNComp::~SoftMaskingNComp() {}

void
SoftMaskingNComp::Reset()
{
    mMixtureHistory.clear();
    mHistory.clear();
}

void
SoftMaskingNComp::SetHistorySize(int size)
{
    mHistorySize = size;
    
    Reset();
}

// Process over time
void
SoftMaskingNComp::Process(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureData,
                          const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &estimData,
                          vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *softMasks)
{
#define EPS 1e-15
    
    // Resize if necessary
    if (mMixtureHistory.size() != estimData.size())
        mMixtureHistory.resize(estimData.size());
    
    if (mHistory.size() != estimData.size())
        mHistory.resize(estimData.size());
    
    if (softMasks->size() != estimData.size())
        softMasks->resize(estimData.size());
    
    for (int k = 0; k < estimData.size(); k++)
    {
        // We must compute the mixture, minus the sound corresponding to the mask
        // See: https://hal.inria.fr/inria-00544949/document
        WDL_TypedBuf<WDL_FFT_COMPLEX> mixtureValuesSub = mixtureData;
    
#if MIXTURE_SUB
        BLUtils::SubstractValues(&mixtureValuesSub, estimData[k]);
#endif
        
        // Manage the history
        //
        mMixtureHistory[k].push_back(mixtureValuesSub);
        mHistory[k].push_back(estimData[k]);
    
        while (mMixtureHistory[k].size() > mHistorySize)
            mMixtureHistory[k].pop_front();
        while (mHistory[k].size() > mHistorySize)
            mHistory[k].pop_front();
    
        WDL_TypedBuf<WDL_FFT_COMPLEX> varianceMixture;
        ComputeVariance(mMixtureHistory[k], &varianceMixture);
    
        WDL_TypedBuf<WDL_FFT_COMPLEX> varianceValues;
        ComputeVariance(mHistory[k], &varianceValues);
    
        // Create the mask
        if (mMixtureHistory[k].empty()) // Just in case
            return;
    
        (*softMasks)[k].Resize(mMixtureHistory[k][0].GetSize());
        for (int i = 0; i < (*softMasks)[k].GetSize(); i++)
        {
            WDL_FFT_COMPLEX mix = varianceMixture.Get()[i];
            WDL_FFT_COMPLEX val = varianceValues.Get()[i];
        
            WDL_FFT_COMPLEX maskVal;
            maskVal.re = 0.0;
            maskVal.im = 0.0;
        
#if MIXTURE_SUB
            // This is the correct formula!
            // var == sigma^2 (standard deviation ^ 2)
            mix.re += val.re;
            mix.im += val.im;
#endif
        
            if ((std::fabs(mix.re) > EPS) ||
                (std::fabs(mix.im) > EPS))
            {
                COMP_DIV(val, mix, maskVal);
            }
        
            BL_FLOAT maskMagn = COMP_MAGN(maskVal);
            if (maskMagn >  1.0)
            {
                BL_FLOAT maskMagnInv = 1.0/maskMagn;
                maskVal.re *= maskMagnInv;
                maskVal.im *= maskMagnInv;
            }
        
            (*softMasks)[k].Get()[i] = maskVal;
        }
    }
}

#if 0
// TODO: for optimization, avoid many access to deque
// NOTE: variance is equal to sigma^2
void
SoftMaskingNComp::ComputeVariance(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *outVariance)
{
    if (history.empty())
        return;
    
    outVariance->Resize(history[0].GetSize());
                     
    for (int i = 0; i < history[0].GetSize(); i++)
    {
        // Compute average over time
        WDL_FFT_COMPLEX avg;
        avg.re = 0.0;
        avg.im = 0.0;
        
        for (int j = 0; j < history.size(); j++)
        {
            WDL_FFT_COMPLEX val = history[j].Get()[i];
            
            avg.re += val.re;
            avg.im += val.im;
        }
        avg.re /= history.size();
        avg.im /= history.size();
        
        if (history.size() == 1)
        {
            outVariance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        WDL_FFT_COMPLEX variance;
        variance.re = 0.0;
        variance.im = 0.0;
        for (int j = 0; j < history.size(); j++)
        {
            WDL_FFT_COMPLEX val = history[j].Get()[i];
            
            val.re -= avg.re;
            val.im -= avg.im;
            
            // Original version (maks a weird sound...)
            //WDL_FFT_COMPLEX square;
            //COMP_MULT(val, val, square);
            
            // For variance, we must use the absolute square instead!
            // Absolute square of a complex number => mult by complex its conjugate!
            WDL_FFT_COMPLEX valConj;
            valConj.re = val.re;
            valConj.im = -val.im;
            
            WDL_FFT_COMPLEX square;
            COMP_MULT(val, valConj, square);
            
            variance.re += square.re;
            variance.im += square.im;
        }
        variance.re /= history.size();
        variance.im /= history.size();
        
        // Result
        outVariance->Get()[i] = variance;
    }
}
#endif

// Optimized version
//
// NOTE: variance is equal to sigma^2
void
SoftMaskingNComp::ComputeVariance(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *outVariance)
{
    if (history.empty())
        return;
    
    outVariance->Resize(history[0].GetSize());
    
    // Convert deque of vector to vector of vector
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > historyVec;
    historyVec.resize(history.size());
    for (int i = 0; i < historyVec.size(); i++)
        historyVec[i] = history[i];
    
    BL_FLOAT invHistorySize = 1.0/historyVec.size();
    
    //
    WDL_FFT_COMPLEX avg;
    WDL_FFT_COMPLEX valConj;
    WDL_FFT_COMPLEX square;
    //
    for (int i = 0; i < historyVec[0].GetSize(); i++)
    {
        // Compute average over time
        //WDL_FFT_COMPLEX avg;
        avg.re = 0.0;
        avg.im = 0.0;
        
        for (int j = 0; j < historyVec.size(); j++)
        {
            const WDL_FFT_COMPLEX &val = historyVec[j].Get()[i];
            
            avg.re += val.re;
            avg.im += val.im;
        }
        avg.re *= invHistorySize;
        avg.im *= invHistorySize;
        
        if (historyVec.size() == 1)
        {
            outVariance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        WDL_FFT_COMPLEX variance;
        variance.re = 0.0;
        variance.im = 0.0;
        for (int j = 0; j < historyVec.size(); j++)
        {
            WDL_FFT_COMPLEX val = historyVec[j].Get()[i];
            
            val.re -= avg.re;
            val.im -= avg.im;
            
            // Original version (maks a weird sound...)
            //WDL_FFT_COMPLEX square;
            //COMP_MULT(val, val, square);
            
            // For variance, we must use the absolute square instead!
            // Absolute square of a complex number => mult by complex its conjugate!
            //WDL_FFT_COMPLEX valConj;
            valConj.re = val.re;
            valConj.im = -val.im;
            
            //WDL_FFT_COMPLEX square;
            COMP_MULT(val, valConj, square);
            
            variance.re += square.re;
            variance.im += square.im;
        }
        variance.re *= invHistorySize;
        variance.im *= invHistorySize;
        
        // Result
        outVariance->Get()[i] = variance;
    }
}
