//
//  SoftMaskingComp3.cpp
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#include <Window.h>

#include <BLUtils.h>

#include "SoftMaskingComp3.h"

// Should be 1 in the original algorithm
#define MIXTURE_SUB 1 //0

// Fill the history progressively
// (instead of filling it all with the first value at the beginning)
#define PROGRESSIVE_HISTORY 0 //1

#define FIX_COMPUTE_VARIANCE 1

// See: https://hal.inria.fr/hal-01881425/document

// NOTE: looks better => leaks less, and fixes residual noise almost as good
#define USE_REAL_EXPECTATION 1 // 0


SoftMaskingComp3::SoftMaskingComp3(int historySize)
{
    mHistorySize = historySize;
    
    mProcessingEnabled = true;
}

SoftMaskingComp3::~SoftMaskingComp3() {}

void
SoftMaskingComp3::Reset()
{
    mMixtureHistory.clear();
    mHistory.clear();
}

void
SoftMaskingComp3::SetHistorySize(int size)
{
    mHistorySize = size;
    
    Reset();
}

int
SoftMaskingComp3::GetHistorySize()
{
    return mHistorySize;
}

void
SoftMaskingComp3::SetProcessingEnabled(bool flag)
{
    mProcessingEnabled = flag;
}

// Process over time
void
SoftMaskingComp3::Process(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *softMask)
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
#if !PROGRESSIVE_HISTORY
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
#else
    mMixtureHistory.push_back(mixtureValuesSub);
    mHistory.push_back(values);
    
    while (mMixtureHistory.size() > mHistorySize)
        mMixtureHistory.pop_front();
    while (mHistory.size() > mHistorySize)
        mHistory.pop_front();
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> sigma2Mixture;
    ComputeSigma2(mMixtureHistory, &sigma2Mixture);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> sigma2Values;
    ComputeSigma2(mHistory, &sigma2Values);
    
    // Create the mask
    if (mMixtureHistory.empty()) // Just in case
        return;
    
    softMask->Resize(mMixtureHistory[0].GetSize());
    for (int i = 0; i < softMask->GetSize(); i++)
    {
        WDL_FFT_COMPLEX mix = sigma2Mixture.Get()[i];
        WDL_FFT_COMPLEX val = sigma2Values.Get()[i];
        
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
        
        softMask->Get()[i] = maskVal;
    }
}

// Process over time
void
SoftMaskingComp3::ProcessCentered(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioMixtureValues,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *ioValues,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *softMask)
{
#define EPS 1e-15
    
    // We must compute the mixture, minus the sound corresponding to the mask
    // See: https://hal.inria.fr/inria-00544949/document
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixtureValuesSub = *ioMixtureValues;
    
#if MIXTURE_SUB
    BLUtils::SubstractValues(&mixtureValuesSub, *ioValues);
#endif
    
    // Manage the history
    //
#if !PROGRESSIVE_HISTORY
    if (mMixtureHistory.empty())
    {
        // Fill the whole history with the current values
        for (int i = 0; i < mHistorySize; i++)
        {
            mMixtureHistory.push_back(mixtureValuesSub);
            mHistory.push_back(*ioValues);
        }
    }
    else
    {
        mMixtureHistory.push_back(mixtureValuesSub);
        mMixtureHistory.pop_front();
        
        mHistory.push_back(*ioValues);
        mHistory.pop_front();
    }
#else
    mMixtureHistory.push_back(mixtureValuesSub);
    mHistory.push_back(values);
    
    while (mMixtureHistory.size() > mHistorySize)
        mMixtureHistory.pop_front();
    while (mHistory.size() > mHistorySize)
        mHistory.pop_front();
#endif
    
    if (mProcessingEnabled)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> sigma2Mixture;
        ComputeSigma2(mMixtureHistory, &sigma2Mixture);
    
        WDL_TypedBuf<WDL_FFT_COMPLEX> sigma2Values;
        ComputeSigma2(mHistory, &sigma2Values);
    
        // Create the mask
        if (mMixtureHistory.empty()) // Just in case
            return;
    
        softMask->Resize(mMixtureHistory[0].GetSize());
        for (int i = 0; i < softMask->GetSize(); i++)
        {
            WDL_FFT_COMPLEX mix = sigma2Mixture.Get()[i];
            WDL_FFT_COMPLEX val = sigma2Values.Get()[i];
        
            WDL_FFT_COMPLEX maskVal;
            maskVal.re = 0.0;
            maskVal.im = 0.0;
        
#if MIXTURE_SUB
            WDL_FFT_COMPLEX mixSum = mix;
        
            // This is the correct formula!
            // var == sigma^2 (standard deviation ^ 2)
            mixSum.re += val.re;
            mixSum.im += val.im;
#endif
        
            if ((std::fabs(mixSum.re) > EPS) ||
                (std::fabs(mixSum.im) > EPS))
            {
                COMP_DIV(val, mixSum, maskVal);
            }
        
            BL_FLOAT maskMagn = COMP_MAGN(maskVal);
            if (maskMagn >  1.0)
            {
                BL_FLOAT maskMagnInv = 1.0/maskMagn;
                maskVal.re *= maskMagnInv;
                maskVal.im *= maskMagnInv;
            }
        
            softMask->Get()[i] = maskVal;
        }
    }
    
    // Even if processing enabled is tru or false,
    // update the data from the history
    
    // Compute the centered values
    if (!mMixtureHistory.empty())
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> mixtureValues = mMixtureHistory[mMixtureHistory.size()/2];
        WDL_TypedBuf<WDL_FFT_COMPLEX> values = mHistory[mHistory.size()/2];
        
        BLUtils::AddValues(&mixtureValues, values);
        
        *ioMixtureValues = mixtureValues;
        *ioValues = values;
    }
}

#if 0
// TODO: for optimization, avoid many access to deque
// NOTE: variance is equal to sigma^2
void
SoftMaskingComp3::ComputeSigma2(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
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

#if !USE_REAL_EXPECTATION
// Optimized version
//
// NOTE: variance is equal to sigma^2
void
SoftMaskingComp3::ComputeSigma2(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                                WDL_TypedBuf<WDL_FFT_COMPLEX> *outSigma2)
{
    if (history.empty())
        return;
    
    outSigma2->Resize(history[0].GetSize());
    
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
    
    WDL_FFT_COMPLEX tmp;
    WDL_FFT_COMPLEX avgConj;
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
            outSigma2->Get()[i] = avg;
            
            continue;
        }
        
#if FIX_COMPUTE_VARIANCE
        // Compute avg^2
        tmp = avg;
        COMP_MULT(tmp, tmp, avg);
        
        //tmp = avg;
        //avgConj.re = avg.re;
        //avgConj.im = -avg.im;
        //COMP_MULT(tmp, avgConj, avg);
#endif
        // Compute variance
        WDL_FFT_COMPLEX variance;
        variance.re = 0.0;
        variance.im = 0.0;
        for (int j = 0; j < historyVec.size(); j++)
        {
            WDL_FFT_COMPLEX val = historyVec[j].Get()[i];
            
#if !FIX_COMPUTE_VARIANCE
            val.re -= avg.re;
            val.im -= avg.im;
#endif
            
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
            
#if FIX_COMPUTE_VARIANCE
            square.re -= avg.re;
            square.im -= avg.im;
#endif
            
            variance.re += square.re;
            variance.im += square.im;
        }
        variance.re *= invHistorySize;
        variance.im *= invHistorySize;
        
        // Result
        outSigma2->Get()[i] = variance;
    }
}
#endif

#if USE_REAL_EXPECTATION
// Optimized version
//
// NOTE: variance is equal to sigma^2
void
SoftMaskingComp3::ComputeSigma2(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                                WDL_TypedBuf<WDL_FFT_COMPLEX> *outSigma2)
{
    if (history.empty())
        return;
    
    outSigma2->Resize(history[0].GetSize());
    
    // Convert deque of vector to vector of vector
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > historyVec;
    historyVec.resize(history.size());
    for (int i = 0; i < historyVec.size(); i++)
        historyVec[i] = history[i];
    
    //
    for (int i = 0; i < historyVec[0].GetSize(); i++)
    {
        WDL_FFT_COMPLEX expect;
        expect.re = 0.0;
        expect.im = 0.0;
        
        BL_FLOAT sumProba = 0.0;
        
        if (mWindow.GetSize() != historyVec.size())
        {
             Window::MakeHanning(historyVec.size(), &mWindow);
        }
        
        // Compute expectation
        WDL_FFT_COMPLEX tmp;
        WDL_FFT_COMPLEX valConj;
        for (int j = 0; j < historyVec.size(); j++)
        {
            WDL_FFT_COMPLEX val = historyVec[j].Get()[i];
            
            // See: https://hal.inria.fr/hal-01881425/document
            // |x|^2
            // NOTE: square abs => complex conjugate
            // (better like that !)
            tmp = val;
            valConj.re = val.re;
            valConj.im = -val.im;
            COMP_MULT(tmp, valConj, val);
            
            //tmp = val;
            //COMP_MULT(tmp, tmp, val);
            
            BL_FLOAT p = mWindow.Get()[j];
            
            expect.re += p*val.re;
            expect.im += p*val.im;
            
            sumProba += p;
        }
        
        if (sumProba > EPS)
        {
            expect.re /= sumProba;
            expect.im /= sumProba;
        }
        
        // Result
        outSigma2->Get()[i] = expect;
    }
}
#endif
