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
//  MelScale.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/15/20.
//
//

#include <cmath>

#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <FastMath.h>

extern "C" {
#include <fast-dct-lee.h>
}

#include "MelScale.h"

// Filter bank
MelScale::FilterBank::FilterBank(int dataSize, BL_FLOAT sampleRate, int numFilters)
{
    mDataSize = dataSize;
    mSampleRate = sampleRate;
    mNumFilters = numFilters;
    
    mFilters.resize(mNumFilters);
    for (int i = 0; i < mFilters.size(); i++)
    {
        mFilters[i].mData.Resize(dataSize);
        BLUtils::FillAllZero(&mFilters[i].mData);
        
        mFilters[i].mBounds[0] = -1;
        mFilters[i].mBounds[1] = -1;
    }
}

MelScale::FilterBank::FilterBank()
{
    mDataSize = 0;
    mSampleRate = 0.0;
    mNumFilters = 0;
}

MelScale::FilterBank::~FilterBank() {}

//

// See: http://practicalcryptography.com/miscellaneous/machine-learning/guide-mel-frequency-cepstral-coefficients-mfccs/

MelScale::MelScale() {}

MelScale::~MelScale() {}

BL_FLOAT
MelScale::HzToMel(BL_FLOAT freq)
{
    //BL_FLOAT mel = 2595.0*std::log10((BL_FLOAT)(1.0 + freq/700.0));
    BL_FLOAT mel = 2595.0*FastMath::log10((BL_FLOAT)(1.0 + freq/700.0));
    
    return mel;
}

BL_FLOAT
MelScale::MelToHz(BL_FLOAT mel)
{
    BL_FLOAT hz = 700.0*(FastMath::pow((BL_FLOAT)10.0, (BL_FLOAT)(mel/2595.0)) - 1.0);
    
    return hz;
}

void
MelScale::HzToMel(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                  const WDL_TypedBuf<BL_FLOAT> &magns,
                  BL_FLOAT sampleRate)
{
    // For dB
    resultMagns->Resize(magns.GetSize());
    BLUtils::FillAllZero(resultMagns);
    
    BL_FLOAT maxFreq = sampleRate*0.5;
    if (maxFreq < BL_EPS)
        return;
    
    BL_FLOAT maxMel = HzToMel(maxFreq);
    
    BL_FLOAT melCoeff = maxMel/resultMagns->GetSize();
    BL_FLOAT idCoeff = (1.0/maxFreq)*resultMagns->GetSize();
    
    int resultMagnsSize = resultMagns->GetSize();
    BL_FLOAT *resultMagnsData = resultMagns->Get();
    
    int magnsSize = magns.GetSize();
    BL_FLOAT *magnsData = magns.Get();
    for (int i = 0; i < resultMagnsSize; i++)
    {
        BL_FLOAT mel = i*melCoeff;
        BL_FLOAT freq = MelToHz(mel);
        
        BL_FLOAT id0 = freq*idCoeff;
        
        int id0i = (int)id0;
        
        BL_FLOAT t = id0 - id0i;
        
        if (id0i >= magnsSize)
            continue;
        
        // NOTE: this optim doesn't compute exactly the same thing than the original version
        int id1 = id0i + 1;
        if (id1 >= magnsSize)
            continue;
        
        BL_FLOAT magn0 = magnsData[id0i];
        BL_FLOAT magn1 = magnsData[id1];
        
        BL_FLOAT magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagnsData[i] = magn;
    }
}

void
MelScale::MelToHz(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                  const WDL_TypedBuf<BL_FLOAT> &magns,
                  BL_FLOAT sampleRate)
{
    resultMagns->Resize(magns.GetSize());
    BLUtils::FillAllZero(resultMagns);
    
    BL_FLOAT hzPerBin = sampleRate*0.5/magns.GetSize();
    
    BL_FLOAT maxFreq = sampleRate*0.5;
    BL_FLOAT maxMel = HzToMel(maxFreq);
    
    int resultMagnsSize = resultMagns->GetSize();
    BL_FLOAT *resultMagnsData = resultMagns->Get();
    int magnsSize = magns.GetSize();
    BL_FLOAT *magnsData = magns.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        BL_FLOAT freq = hzPerBin*i;
        BL_FLOAT mel = HzToMel(freq);
        
        BL_FLOAT id0 = (mel/maxMel) * resultMagnsSize;
        
        if ((int)id0 >= magnsSize)
            continue;
        
        BL_FLOAT t = id0 - (int)(id0);
        
        int id1 = id0 + 1;
        if (id1 >= magnsSize)
            continue;
        
        BL_FLOAT magn0 = magnsData[(int)id0];
        BL_FLOAT magn1 = magnsData[id1];
        
        BL_FLOAT magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagnsData[i] = magn;
    }
}

void
MelScale::HzToMelFilter(WDL_TypedBuf<BL_FLOAT> *result,
                        const WDL_TypedBuf<BL_FLOAT> &magns,
                        BL_FLOAT sampleRate, int numFilters)
{
    if ((magns.GetSize() != mHzToMelFilterBank.mDataSize) ||
        (sampleRate != mHzToMelFilterBank.mSampleRate) ||
        (numFilters != mHzToMelFilterBank.mNumFilters))
    {
        CreateFilterBankHzToMel(&mHzToMelFilterBank, magns.GetSize(),
                                sampleRate, numFilters);
    }
    
    ApplyFilterBank(result, magns, mHzToMelFilterBank);
}

void
MelScale::MelToHzFilter(WDL_TypedBuf<BL_FLOAT> *result,
                        const WDL_TypedBuf<BL_FLOAT> &magns,
                        BL_FLOAT sampleRate, int numFilters)
{
    if ((magns.GetSize() != mMelToHzFilterBank.mDataSize) ||
        (sampleRate != mMelToHzFilterBank.mSampleRate) ||
        (numFilters != mMelToHzFilterBank.mNumFilters))
    {
        CreateFilterBankMelToHz(&mMelToHzFilterBank, magns.GetSize(),
                                sampleRate, numFilters);
    }
    
    ApplyFilterBank(result, magns, mMelToHzFilterBank);
}

BL_FLOAT
MelScale::ComputeTriangleAreaBetween(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                                     BL_FLOAT x0, BL_FLOAT x1)
{
    if ((x0 > txmax) || (x1 < txmin))
        return 0.0;
    
    vector<BL_FLOAT> &x = mTmpBuf0;
    //x.push_back(txmin);
    //x.push_back(txmid);
    //x.push_back(txmax);
    //x.push_back(x0);
    //x.push_back(x1);
    x.resize(5);
    x[0] = txmin;
    x[1] = txmid;
    x[2] = txmax;
    x[3] = x0;
    x[4] = x1;
    
    sort(x.begin(), x.end());
    
    BL_FLOAT points[5][2];
    for (int i = 0; i < 5; i++)
    {
        points[i][0] = x[i];
        points[i][1] = ComputeTriangleY(txmin, txmid, txmax, x[i]);
    }
    
    BL_FLOAT area = 0.0;
    for (int i = 0; i < 4; i++)
    {
        // Suppress the cases which are out of [x0, x1] bounds
        if ((points[i][0] >= x1) ||
            (points[i + 1][0] <= x0))
            continue;
            
        BL_FLOAT y0 = points[i][1];
        BL_FLOAT y1 = points[i + 1][1];
        if (y0 > y1)
        {
            BL_FLOAT tmp = y0;
            y0 = y1;
            y1 = tmp;
        }
        
        BL_FLOAT a = (points[i + 1][0] - points[i][0])*(y0 + (y1 - y0)*0.5);
        
        area += a;
    }
    
    return area;
}

BL_FLOAT
MelScale::ComputeTriangleY(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                           BL_FLOAT x)
{
    if (x <= txmin)
        return 0.0;
    if (x >= txmax)
        return 0.0;
    
    if (x <= txmid)
    {
        BL_FLOAT t = (x - txmin)/(txmid - txmin);
        
        return t;
    }
    else // x >= txmid
    {
        BL_FLOAT t = 1.0 - (x - txmid)/(txmax - txmid);
        
        return t;
    }
}

// See: https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html
void
MelScale::CreateFilterBankHzToMel(FilterBank *filterBank, int dataSize,
                                  BL_FLOAT sampleRate, int numFilters)
{    
    filterBank->mDataSize = dataSize;
    filterBank->mSampleRate = sampleRate;
    filterBank->mNumFilters = numFilters;
    
    filterBank->mFilters.resize(filterBank->mNumFilters);
    
    for (int i = 0; i < filterBank->mFilters.size(); i++)
    {
        filterBank->mFilters[i].mData.Resize(dataSize);
        BLUtils::FillAllZero(&filterBank->mFilters[i].mData);
        
        filterBank->mFilters[i].mBounds[0] = -1;
        filterBank->mFilters[i].mBounds[1] = -1;
    }
    
    // Create filters
    //
    BL_FLOAT lowFreqMel = 0.0;
    BL_FLOAT highFreqMel = HzToMel(sampleRate*0.5);
    
    // Compute equally spaced mel values
    WDL_TypedBuf<BL_FLOAT> melPoints;
    melPoints.Resize(numFilters + 2);
    for (int i = 0; i < melPoints.GetSize(); i++)
    {
        // Compute mel value
        BL_FLOAT t = ((BL_FLOAT)i)/(melPoints.GetSize() - 1);
        BL_FLOAT val = lowFreqMel + t*(highFreqMel - lowFreqMel);
        
        melPoints.Get()[i] = val;
    }
    
    // Compute mel points
    WDL_TypedBuf<BL_FLOAT> hzPoints;
    hzPoints.Resize(melPoints.GetSize());
    for (int i = 0; i < hzPoints.GetSize(); i++)
    {
        // Compute hz value
        BL_FLOAT val = melPoints.Get()[i];
        val = MelToHz(val);
        
        hzPoints.Get()[i] = val;
    }
    
    // Compute bin points
    WDL_TypedBuf<BL_FLOAT> bin;
    bin.Resize(hzPoints.GetSize());
    
    BL_FLOAT hzPerBinInv = (dataSize + 1)/(sampleRate*0.5);
    for (int i = 0; i < bin.GetSize(); i++)
    {
        // Compute hz value
        BL_FLOAT val = hzPoints.Get()[i];
        
        // For the new solution that fills holes, do not round or trunk
        val = val*hzPerBinInv;
        
        bin.Get()[i] = val;
    }
    
    // For each destination value
    for (int i = 0; i < dataSize; i++)
    {
        // For each filter
        for (int m = 1; m < numFilters + 1; m++)
        {
            BL_FLOAT fmin = bin.Get()[m - 1]; // left
            BL_FLOAT fmid = bin.Get()[m];     // center
            BL_FLOAT fmax = bin.Get()[m + 1]; // right
            
            //
            filterBank->mFilters[m - 1].mBounds[0] = std::floor(fmin);
            filterBank->mFilters[m - 1].mBounds[1] = std::ceil(fmax);
            
            // Trapezoid
            BL_FLOAT x0 = i;
            if (fmin > x0)
                x0 = fmin;
            
            BL_FLOAT x1 = i + 1;
            if (fmax < x1)
                x1 = fmax;
            
            BL_FLOAT tarea = ComputeTriangleAreaBetween(fmin, fmid, fmax, x0, x1);
            
            // Normalize
            tarea /= (fmid - fmin)*0.5 + (fmax - fmid)*0.5;
            
            filterBank->mFilters[m - 1].mData.Get()[i] += tarea;
        }
    }
}

void
MelScale::CreateFilterBankMelToHz(FilterBank *filterBank, int dataSize,
                                  BL_FLOAT sampleRate, int numFilters)
{    
    filterBank->mDataSize = dataSize;
    filterBank->mSampleRate = sampleRate;
    filterBank->mNumFilters = numFilters;
    
    filterBank->mFilters.resize(filterBank->mNumFilters);
    
    for (int i = 0; i < filterBank->mFilters.size(); i++)
    {
        filterBank->mFilters[i].mData.Resize(dataSize);
        BLUtils::FillAllZero(&filterBank->mFilters[i].mData);
        
        filterBank->mFilters[i].mBounds[0] = -1;
        filterBank->mFilters[i].mBounds[1] = -1;
    }
    
    // Create filters
    //
    BL_FLOAT lowFreqHz = 0.0;
    BL_FLOAT highFreqHz = sampleRate*0.5;
    
    WDL_TypedBuf<BL_FLOAT> hzPoints;
    hzPoints.Resize(numFilters + 2);
    for (int i = 0; i < hzPoints.GetSize(); i++)
    {
        // Compute hz value
        BL_FLOAT t = ((BL_FLOAT)i)/(hzPoints.GetSize() - 1);
        BL_FLOAT val = lowFreqHz + t*(highFreqHz - lowFreqHz);
        
        hzPoints.Get()[i] = val;
    }
    
    // Compute hz points
    WDL_TypedBuf<BL_FLOAT> melPoints;
    melPoints.Resize(hzPoints.GetSize());
    for (int i = 0; i < melPoints.GetSize(); i++)
    {
        // Compute hz value
        BL_FLOAT val = hzPoints.Get()[i];
        val = HzToMel(val);
        melPoints.Get()[i] = val;
    }
    
    // Compute bin points
    WDL_TypedBuf<BL_FLOAT> bin;
    bin.Resize(melPoints.GetSize());
    
    BL_FLOAT maxMel = HzToMel(sampleRate*0.5);
    BL_FLOAT melPerBinInv = (dataSize + 1)/maxMel;
    for (int i = 0; i < bin.GetSize(); i++)
    {
        // Compute mel value
        BL_FLOAT val = melPoints.Get()[i];
        
        // For the new solution that fills holes, do not round or trunk
        val = val*melPerBinInv;
        
        bin.Get()[i] = val;
    }
    
    // For each destination value
    for (int i = 0; i < dataSize; i++)
    {
        // For each filter
        for (int m = 1; m < numFilters /*+ 1*/; m++)
        {
            BL_FLOAT fmin = bin.Get()[m - 1]; // left
            BL_FLOAT fmid = bin.Get()[m];     // center
            BL_FLOAT fmax = bin.Get()[m + 1]; // right
            
            //
            filterBank->mFilters[m /*- 1*/].mBounds[0] = std::floor(fmin);
            filterBank->mFilters[m /*- 1*/].mBounds[1] = std::ceil(fmax);
            
            // Trapezoid
            BL_FLOAT x0 = i;
            if (fmin > x0)
                x0 = fmin;
            
            BL_FLOAT x1 = i + 1;
            if (fmax < x1)
                x1 = fmax;
            
            BL_FLOAT tarea = ComputeTriangleAreaBetween(fmin, fmid, fmax, x0, x1);
            
            // Normalize
            tarea /= (fmid - fmin)*0.5 + (fmax - fmid)*0.5;
            
            filterBank->mFilters[m /*- 1*/].mData.Get()[i] += tarea;
        }
    }
}

#if 0 // Origin
void
MelScale::ApplyFilterBank(WDL_TypedBuf<BL_FLOAT> *result,
                          const WDL_TypedBuf<BL_FLOAT> &magns,
                          const FilterBank &filterBank)
{
    result->Resize(filterBank.mNumFilters);
    BLUtils::FillAllZero(result);
    
    // For each destination value
    for (int i = 0; i < result->GetSize(); i++)
    {
        // For each filter
        for (int m = 0; m < filterBank.mNumFilters; m++)
        {
            const FilterBank::Filter &filter = filterBank.mFilters[m];
            
            // Check roughtly the bounds
            //if ((i + 1 < std::floor(filter.mBounds[0])) ||
            if (((i + 1) < filter.mBounds[0]) ||
                (i > filter.mBounds[1]))
                // Not inside the current filter
                continue;
            
            // Apply the filter value
            BL_FLOAT tarea = filter.mData.Get()[i];
            result->Get()[m] += tarea*magns.Get()[i];
        }
    }
}
#endif

#if 1 // Optimized
// FIX: this optimization goes faster, but also fixes a bug!
// Dump vocal, Energetic Upbeat => the top frequencies looks strange,
// like some drums frequencies repeated.
void
MelScale::ApplyFilterBank(WDL_TypedBuf<BL_FLOAT> *result,
                          const WDL_TypedBuf<BL_FLOAT> &magns,
                          const FilterBank &filterBank)
{
    result->Resize(filterBank.mNumFilters);
    BLUtils::FillAllZero(result);
    
    // For each filter
    for (int m = 0; m < filterBank.mNumFilters; m++)
    {
        const FilterBank::Filter &filter = filterBank.mFilters[m];

        const BL_FLOAT *filterData = filter.mData.Get();
        BL_FLOAT *resultData = result->Get();
        BL_FLOAT *magnsData = magns.Get();
        
        // For each destination value
        for (int i = filter.mBounds[0] - 1; i < filter.mBounds[1] + 1; i++)
        {
            if (i < 0)
                continue;
            if (i >= magns.GetSize())
                continue;
            
            // Apply the filter value
            //BL_FLOAT tarea = filter.mData.Get()[i];
            //result->Get()[m] += tarea*magns.Get()[i];
            resultData[m] += filterData[i]*magnsData[i];
        }
    }
}
#endif

void
MelScale::MelToMFCC(WDL_TypedBuf<BL_FLOAT> *result,
                    const WDL_TypedBuf<BL_FLOAT> &melMagns)
{
    *result = melMagns;
    
    // Apply the log
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT val = result->Get()[i];
        val = std::log(val + 1.0);
        result->Get()[i] = val;
    }
    
    // Apply the dct
    FastDctLee_transform(result->Get(), result->GetSize());
}

// NOTE: calling MelToMFCC() then MFCCToMel() is not totally inversible,
// as it should be (there is a scale added).
// In Scilab, this is inversible (maybe complex numbers ?)
void
MelScale::MFCCToMel(WDL_TypedBuf<BL_FLOAT> *result,
                    const WDL_TypedBuf<BL_FLOAT> &mfccMagns)
{
    *result = mfccMagns;
    
    // Apply the idct
    FastDctLee_inverseTransform(result->Get(), result->GetSize());
    
    // Apply the inverse log log
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT val = result->Get()[i];
        val = std::exp(val) - 1.0;
        result->Get()[i] = val;
    }
}
