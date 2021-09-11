//
//  SoftMaskingNComp4.cpp
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#include <Window.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "SoftMaskingNComp4.h"

// Use fake mask for second mask?
// Fake mask seem to work well, and is optimized
#define USE_FAKE_MASK1 1 // 0

//
SoftMaskingNComp4::HistoryLine::HistoryLine()
{
    mSize = 0;
    mNumMasks = 0;
}

SoftMaskingNComp4::HistoryLine::HistoryLine(const HistoryLine &other)
{
    mSize = other.mSize;
    mNumMasks = other.mNumMasks;

    Resize(mSize, mNumMasks);
    
    mSum = other.mSum;
    mMaskedSquare = other.mMaskedSquare;
}

SoftMaskingNComp4::HistoryLine::~HistoryLine() {}

void
SoftMaskingNComp4::HistoryLine::Resize(int size, int numMasks)
{
    mSize = size;
    mNumMasks = numMasks;
    
    mSum.Resize(mSize);

    mMaskedSquare.resize(mNumMasks);
    for (int i = 0; i < mMaskedSquare.size(); i++)
        mMaskedSquare[i].Resize(mSize);
}

int
SoftMaskingNComp4::HistoryLine::GetSize()
{
    return mSize;
}

int
SoftMaskingNComp4::HistoryLine::GetNumMasks()
{
    return mNumMasks;
}

//

SoftMaskingNComp4::SoftMaskingNComp4(int bufferSize, int overlapping,
                                     int historySize, int numMasks,
                                     bool autoGenerateRestMask)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    
    mHistorySize = historySize;
    mNumMasks = numMasks;
    
    mProcessingEnabled = true;

    mAutoGenerateRestMask = autoGenerateRestMask;
    if (mAutoGenerateRestMask)
        // One additonal hidden mask
        mNumMasks++;
}

SoftMaskingNComp4::~SoftMaskingNComp4() {}

void
SoftMaskingNComp4::Reset(int bufferSize, int overlapping)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;

    // ??
    //mHistorySize = 0;
    //mNumMasks = 0;
    
    Reset();
}

void
SoftMaskingNComp4::Reset()
{    
    mHistory.resize(0);
}

int
SoftMaskingNComp4::GetHistorySize()
{
    return mHistorySize;
}

void
SoftMaskingNComp4::SetProcessingEnabled(bool flag)
{
    mProcessingEnabled = flag;
}

bool
SoftMaskingNComp4::IsProcessingEnabled()
{
    return mProcessingEnabled;
}

int
SoftMaskingNComp4::GetLatency()
{
#if 0 // Version with hack
    int latency = (mHistorySize/2)*(mBufferSize/mOverlapping);
    // Hack (for Air)
    latency *= 0.75;
#endif

    // Correct version
    //
    // In history, we push_back() and pop_front()
    // and we take the index historySize/2
    //

    // The index where we get the data (from the end)
    // (this covers the case of odd and even history size)
    // Index 0 has 0 latency, since we have just added the current data to it.
    int revIndex = (mHistorySize - 1) - mHistorySize/2;
    int latency = revIndex*(mBufferSize/mOverlapping);
    
    return latency;
}

// Process over time
//
// Algo:

// s = input * HM
// n = input * (1.0 - HM)
// SM = s2(s)/(s2(s) + s2(n))
// output = input * SM
//
void
SoftMaskingNComp4::
ProcessCentered(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioSum,
                const vector<WDL_TypedBuf<BL_FLOAT> > &masks0,
                vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *ioMaskedResult)
{
    if (masks0.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > masks = mTmpBuf10;
    masks = masks0;

    // Generate rest mask if necessary
    if (mAutoGenerateRestMask)
    {
        WDL_TypedBuf<BL_FLOAT> &restMask = mTmpBuf11;
        restMask.Resize(masks0[0].GetSize());
        for (int i = 0; i < masks0[0].GetSize(); i++)
        {
            BL_FLOAT sum = 0.0;
            for (int j = 0; j < masks0.size(); j++)
                sum += masks0[j].Get()[i];

            restMask.Get()[i] = 1.0 - sum;
        }

        masks.push_back(restMask);
            
    }
    
    if (masks.size() != mNumMasks)
        // Error
        return;

    /////////
    /*BLDebug::DumpData("mask0.txt", masks[0]);
      BLDebug::DumpData("mask1.txt", masks[1]);
      BLDebug::DumpData("mask2.txt", masks[2]);
      BLDebug::DumpData("mask3.txt", masks[3]);*/
    /////////
    
    ioMaskedResult->resize(mNumMasks);
    
    HistoryLine &newHistoLine = mTmpHistoryLine;
    newHistoLine.Resize(ioSum->GetSize(), mNumMasks);

    newHistoLine.mSum = *ioSum;

    // Optim: compute square history only if enabled
    // Otherwise, fill with zeros
    if (mProcessingEnabled)
    {
        // masked0 = sum*mask
        for (int j = 0; j < masks.size(); j++)
        {
            newHistoLine.mMaskedSquare[j] = *ioSum;
            BLUtils::MultValues(&newHistoLine.mMaskedSquare[j], masks[j]);
        
            // maskd1 = sum - masked0
            // same as: masked1 = sum*(1 - mask)
            //newHistoLine.mMasked1Square = *ioSum;
            //BLUtils::SubstractValues(&newHistoLine.mMasked1Square,
            //                         newHistoLine.mMasked0Square);
            
            // See: https://hal.inria.fr/hal-01881425/document
            // |x|^2
            // NOTE: square abs => complex conjugate
            
            // Compute squares (using complex conjugate)
            BLUtilsComp::ComputeSquareConjugate(&newHistoLine.mMaskedSquare[j]);
            //BLUtilsComp::ComputeSquareConjugate(&newHistoLine.mMasked1Square);
        }
    }
    else // Not enabled, fill history with zeros
    {
        for (int j = 0; j < masks.size(); j++)
        {
            newHistoLine.mMaskedSquare[j].Resize(ioSum->GetSize());
            BLUtils::FillAllZero(&newHistoLine.mMaskedSquare[j]);
        
            //newHistoLine.mMasked1Square.Resize(ioSum->GetSize());
            //BLUtils::FillAllZero(&newHistoLine.mMasked1Square);
        }
    }
    
    // Manage the history
    if (mHistory.empty())
    {
        // Fill the whole history with the first line
        mHistory.resize(mHistorySize);
        mHistory.clear(newHistoLine);
    }
    else
    {
        mHistory.freeze();
        mHistory.push_pop(newHistoLine);
    }
    
    if (mProcessingEnabled)
    {
        vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &sigma2Mask = mTmpBuf0;
        sigma2Mask.resize(mNumMasks);
        for (int i = 0; i < mNumMasks; i++)
        {
            //WDL_TypedBuf<WDL_FFT_COMPLEX> &sigma2Mask1 = mTmpBuf1;
            ComputeSigma2(i, &sigma2Mask[i]);
        }
            
        vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &softMasks = mTmpBuf8;
        softMasks.resize(mNumMasks);

        WDL_TypedBuf<WDL_FFT_COMPLEX> &sum = mTmpBuf9;
        sum.Resize(ioSum->GetSize());
        for (int i = 0; i < ioSum->GetSize(); i++)
        {
            WDL_FFT_COMPLEX &csum = sum.Get()[i];
            csum.re = 0.0;
            csum.im = 0.0;
            
            for (int j = 0; j < mNumMasks; j++)
            {
                const WDL_FFT_COMPLEX &s = sigma2Mask[j].Get()[i];
                    
                // Update sum
                csum.re += s.re;
                csum.im += s.im;
            }
        }
        
        for (int j = 0; j < mNumMasks; j++)
        {
            softMasks[j].Resize(ioSum->GetSize());
            
            //WDL_FFT_COMPLEX *s0Data = sigma2Mask0.Get();
            //WDL_FFT_COMPLEX *s1Data = sigma2Mask1.Get();
        
            // Create the mask
            //if (mHistory[j].empty()) // Just in case
            //    return;

            //WDL_TypedBuf<WDL_FFT_COMPLEX> &softMask0 = mTmpBuf4;
            //softMask0.Resize(mHistory[0].GetSize());

            //int softMask0Size = softMask0.GetSize();
            //WDL_FFT_COMPLEX *softMask0Data = softMask0.Get();
        
            // Compute soft mask 0
            for (int i = 0; i < ioSum->GetSize(); i++)
            {
                WDL_FFT_COMPLEX csum = sum.Get()[i];
                const WDL_FFT_COMPLEX &s = sigma2Mask[j].Get()[i];

                WDL_FFT_COMPLEX &maskVal = softMasks[j].Get()[i];

                maskVal.re = 0.0;
                maskVal.im = 0.0;

                if ((std::fabs(csum.re) > BL_EPS) ||
                    (std::fabs(csum.im) > BL_EPS))
                {
                    COMP_DIV(s, csum, maskVal);
                }
            
                BL_FLOAT maskMagn = COMP_MAGN(maskVal);

                // Limit to 1
                if (maskMagn >  1.0)
                {
                    BL_FLOAT maskMagnInv = 1.0/maskMagn;
                    maskVal.re *= maskMagnInv;
                    maskVal.im *= maskMagnInv;
                }
            
                softMasks[j].Get()[i] = maskVal;
            }

            // Result when enabled
            
            // Apply mask 0
            (*ioMaskedResult)[j] = mHistory[mHistory.size()/2].mSum;
            BLUtils::MultValues(&(*ioMaskedResult)[j], softMasks[j]);
        }

        /////////
        /*WDL_TypedBuf<BL_FLOAT> dbgMask0;
          BLUtilsComp::ComplexToMagn(&dbgMask0, softMasks[0]);
          BLDebug::DumpData("smask0.txt", dbgMask0);
          WDL_TypedBuf<BL_FLOAT> dbgMask1;
          BLUtilsComp::ComplexToMagn(&dbgMask1, softMasks[1]);
          BLDebug::DumpData("smask1.txt", dbgMask1);
          WDL_TypedBuf<BL_FLOAT> dbgMask2;
          BLUtilsComp::ComplexToMagn(&dbgMask2, softMasks[2]);
          BLDebug::DumpData("smask2.txt", dbgMask2);
          WDL_TypedBuf<BL_FLOAT> dbgMask3;
          BLUtilsComp::ComplexToMagn(&dbgMask3, softMasks[3]);
          BLDebug::DumpData("smask3.txt", dbgMask3);
        */
        /////////
    }
    
    // Even if processing enabled is true or false,
    // update the data from the history
    
    // Compute the centered values
    if (!mHistory.empty())
    {
        // Shifted input data
        *ioSum = mHistory[mHistory.size()/2].mSum;
    }
}

// NOTE: variance is equal to sigma^2
void
SoftMaskingNComp4::ComputeSigma2(int maskNum,
                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *outSigma2)
{    
    if (mHistory.empty())
        return;
    
    outSigma2->Resize(mHistory[0].mSum.GetSize());
    
    // Result sum 0
    WDL_TypedBuf<WDL_FFT_COMPLEX> &currentSum = mTmpBuf2;
    currentSum.Resize(mHistory[0].mSum.GetSize());
    BLUtils::FillAllZero(&currentSum);

    WDL_FFT_COMPLEX *currentSumData = currentSum.Get();
    
    // Window
    if (mWindow.GetSize() != mHistory.size())
        Window::MakeHanning(mHistory.size(), &mWindow);
    
    BL_FLOAT sumProba = BLUtils::ComputeSum(mWindow);
    BL_FLOAT sumProbaInv = 0.0;
    if (sumProba > BL_EPS)
        sumProbaInv = 1.0/sumProba;
    
    //
    for (int j = 0; j < mHistory.size(); j++)
    {
        const HistoryLine &line = mHistory[j];
        
        const WDL_TypedBuf<WDL_FFT_COMPLEX> &line0 = line.mMaskedSquare[maskNum];
        int line0Size = line0.GetSize();
        WDL_FFT_COMPLEX *line0Data = line0.Get();
        
        BL_FLOAT p = mWindow.Get()[j];
        
        for (int i = 0; i < line0Size; i++)
        {
            WDL_FFT_COMPLEX &expect = currentSumData[i];
            const WDL_FFT_COMPLEX &val = line0Data[i];
            expect.re += p*val.re;
            expect.im += p*val.im;
        }
    }

    // Divide by sum probas
    if (sumProba > BL_EPS)
        BLUtils::MultValues(&currentSum, sumProbaInv);
    
    // Result
    *outSigma2 = currentSum;
}
