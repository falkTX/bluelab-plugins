//
//  SoftMaskingComp.cpp
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include "SoftMaskingComp.h"

// Should be 1 in the original algorithm
#define MIXTURE_SUB 1 //0 //1


SoftMaskingComp::SoftMaskingComp(int historySize)
{
    mHistorySize = historySize;
    
    // Debug
    mFreqWinSize = 4;
}

SoftMaskingComp::~SoftMaskingComp() {}

void
SoftMaskingComp::Reset()
{
    mMixtureHistory.clear();
    mHistory.clear();
}

void
SoftMaskingComp::SetHistorySize(int size)
{
    mHistorySize = size;
    
    Reset();
}

void
SoftMaskingComp::Process(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *mask)
{
    // Best sound quality
    ProcessTime(mixtureValues, values, mask);
    //ProcessFreq(mixtureMagns, magns, mask);
    
    //ProcessTimeFreq(mixtureValues, values, mask);
    
#if 0
    // TEST
    WDL_TypedBuf<WDL_FFT_COMPLEX> mask0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mask1;
    ProcessTime(mixtureValues, values, &mask0);
    ProcessFreq(mixtureValues, values, &mask1);
    
    *mask = mask0;
    
    // Mult
    //BLUtils::MultValues(mask, mask1);
    // Or average
    BLUtils::AddValues(mask, mask1);
    BLUtils::MultValues(mask, 0.5);
#endif
}

void
SoftMaskingComp::ProcessTime(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                             const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> *mask)
{
#define EPS 1e-15
    
    // We must compute the mixture, minus the sound corresponding to the mask
    // See: https://hal.inria.fr/inria-00544949/document
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixtureValuesSub = mixtureValues;
    
#if MIXTURE_SUB
    BLUtils::SubstractValues(&mixtureValuesSub, values);
#endif
    
    // Manage the history
    //
    if (mMixtureHistory.empty())
    {
        // Fill the whole history with the current values
        for (int i = 0; i < mHistorySize; i++)
        {
            mMixtureHistory.push_back(mixtureValuesSub);
            mHistory.push_back(values);
        }
    }
    else
    {
        mMixtureHistory.push_back(mixtureValuesSub);
        mMixtureHistory.pop_front();
        
        mHistory.push_back(values);
        mHistory.pop_front();
    }
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> varMixture;
    ComputeVariance(mMixtureHistory, &varMixture);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> varValues;
    ComputeVariance(mHistory, &varValues);
    
    // Create the mask
    if (mMixtureHistory.empty()) // Just in case
        return;
    
    mask->Resize(mMixtureHistory[0].GetSize());
    for (int i = 0; i < mask->GetSize(); i++)
    {
        WDL_FFT_COMPLEX mix = varMixture.Get()[i];
        WDL_FFT_COMPLEX val = varValues.Get()[i];
        
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
        
        mask->Get()[i] = maskVal;
    }
}

void
SoftMaskingComp::ProcessFreq(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                             const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> *mask)
{
#define EPS 1e-15
    
    // We must compute the mixture, minus the sound corresponding to the mask
    // See: https://hal.inria.fr/inria-00544949/document
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixtureValuesSub = mixtureValues;
    
#if MIXTURE_SUB
    BLUtils::SubstractValues(&mixtureValuesSub, values);
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> varMixture;
    ComputeVariance(mixtureValuesSub, &varMixture);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> varValues;
    ComputeVariance(values, &varValues);
    
    mask->Resize(values.GetSize());
    for (int i = 0; i < mask->GetSize(); i++)
    {
        WDL_FFT_COMPLEX mix = varMixture.Get()[i];
        WDL_FFT_COMPLEX val = varValues.Get()[i];
        
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
        
        mask->Get()[i] = maskVal;
    }
}

void
SoftMaskingComp::ProcessTimeFreq(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *mask)
{
#define EPS 1e-15
    
    // We must compute the mixture, minus the sound corresponding to the mask
    // See: https://hal.inria.fr/inria-00544949/document
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixtureValuesSub = mixtureValues;
    
#if MIXTURE_SUB
    BLUtils::SubstractValues(&mixtureValuesSub, values);
#endif
    
    // Manage the history
    //
    if (mMixtureHistory.empty())
    {
        // Fill the whole history with the current values
        for (int i = 0; i < mHistorySize; i++)
        {
            mMixtureHistory.push_back(mixtureValuesSub);
            mHistory.push_back(values);
        }
    }
    else
    {
        mMixtureHistory.push_back(mixtureValuesSub);
        mMixtureHistory.pop_front();
        
        mHistory.push_back(values);
        mHistory.pop_front();
    }
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> varMixture;
    ComputeVarianceWin(mMixtureHistory, &varMixture);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> varValues;
    ComputeVarianceWin(mHistory, &varValues);
    
    // Create the mask
    if (mMixtureHistory.empty()) // Just in case
        return;
    
    mask->Resize(mMixtureHistory[0].GetSize());
    for (int i = 0; i < mask->GetSize(); i++)
    {
        WDL_FFT_COMPLEX mix = varMixture.Get()[i];
        WDL_FFT_COMPLEX val = varValues.Get()[i];
        
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
        
        mask->Get()[i] = maskVal;
    }
}

// TODO: for optimization, avoid many access to deque
// NOTE: variance is equal to sigma^2
void
SoftMaskingComp::ComputeVariance(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *variance)
{
    if (history.empty())
        return;
    
    variance->Resize(history[0].GetSize());
                     
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
            variance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        WDL_FFT_COMPLEX var;
        var.re = 0.0;
        var.im = 0.0;
        for (int j = 0; j < history.size(); j++)
        {
            WDL_FFT_COMPLEX val = history[j].Get()[i];
            
            val.re -= avg.re;
            val.im -= avg.im;
            
            WDL_FFT_COMPLEX square;
            COMP_MULT(val, val, square);
            
            //val.re *= val.re;
            //val.im *= val.im;
            
            var.re += square.re;
            var.im += square.im;
        }
        var.re /= history.size();
        var.im /= history.size();
        
        // Result
        variance->Get()[i] = var;
    }
}

// Compute the variance over a window
// NOTE: variance is equal to sigma^2
void
SoftMaskingComp::ComputeVariance(const WDL_TypedBuf<WDL_FFT_COMPLEX> &data,
                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *variance)
{
    variance->Resize(data.GetSize());
    
    for (int i = 0; i < data.GetSize(); i++)
    {
        // Compute the average
        WDL_FFT_COMPLEX avg;
        avg.re = 0.0;
        avg.im = 0.0;
        
        int numValues = 0;
        for (int j = -mFreqWinSize/2; j <= mFreqWinSize/2; j++)
        {
            int idx = i + j;
            if (idx < 0)
                idx = 0;
            if (idx > data.GetSize() - 1)
                idx = data.GetSize() - 1;
            
            WDL_FFT_COMPLEX val = data.Get()[idx];
            
            avg.re += val.re;
            avg.im += val.im;
            
            numValues++;
        }
        if (numValues > 0)
        {
            avg.re /= numValues;
            avg.im /= numValues;
        }
        
        if (numValues == 1)
        {
            variance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        WDL_FFT_COMPLEX var;
        var.re = 0.0;
        var.im = 0.0;
        
        int numValues2 = 0;
        for (int j = -mFreqWinSize/2; j <= mFreqWinSize/2; j++)
        {
            int idx = i + j;
            if (idx < 0)
                idx = 0;
            if (idx > data.GetSize() - 1)
                idx = data.GetSize() - 1;
            
            WDL_FFT_COMPLEX val = data.Get()[idx];
            
            val.re -= avg.re;
            val.im -= avg.im;
            
            WDL_FFT_COMPLEX square;
            COMP_MULT(val, val, square);
            
            //val.re *= val.re;
            //val.im *= val.im;
            
            var.re += square.re;
            var.im += square.im;
            
            numValues2++;
        }
        if (numValues2 > 0)
        {
            var.re /= numValues2;
            var.im /= numValues2;
        }
        
        // Result
        variance->Get()[i] = var;
    }
}

// TODO: for optimization, avoid many access to deque
// NOTE: variance equals sigma^2
void
SoftMaskingComp::ComputeVarianceWin(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                                    WDL_TypedBuf<WDL_FFT_COMPLEX> *variance)
{
    if (history.empty())
        return;
    
    variance->Resize(history[0].GetSize());
    
    for (int i = 0; i < history[0].GetSize(); i++)
    {
        // Compute average over time
        WDL_FFT_COMPLEX avg;
        avg.re = 0.0;
        avg.im = 0.0;
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
                WDL_FFT_COMPLEX val = history[j].Get()[idx];
            
                avg.re += val.re;
                avg.im += val.im;
                
                numValues++;
            }
        }
        if (numValues > 0)
        {
            avg.re /= numValues;
            avg.im /= numValues;
        }
        
        if (history.size() == 1)
        {
            variance->Get()[i] = avg;
            
            continue;
        }
        
        // Compute variance
        WDL_FFT_COMPLEX var;
        var.re = 0.0;
        var.im = 0.0;
        
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
                WDL_FFT_COMPLEX val = history[j].Get()[idx];
            
                val.re -= avg.re;
                val.im -= avg.im;
                
                WDL_FFT_COMPLEX square;
                COMP_MULT(val, val, square);
                
                //val.re *= val.re;
                //val.im *= val.im;
                
                var.re += square.re;
                var.im += square.im;

                numValues2++;
            }
        }
        if (numValues2 > 0)
        {
            var.re /= numValues2;
            var.im /= numValues2;
        }
        
        // Result
        variance->Get()[i] = var;
    }
}
