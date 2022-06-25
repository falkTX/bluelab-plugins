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
//  FilterBank.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/15/20.
//
//

#include <cmath>

#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLDebug.h>

#include <FastMath.h>

#include "FilterBank.h"

// When using for example a log scale, there is aliasing on the lower values
// (big stairs effect). This is because the triangle filters are smaller than 1 bin.
// And also because several successive triangles match exactly the same bin.
// So this makes pure horizontal series of values (plateau).
//
// To avoid this, grow the triangles that are too small on the left
// and on the right. It works to fix aliasing, because we use normalization
// based on triangles areas in floating format. So we have a good continuity.
#define FIX_ALIASING_LOW_FREQS 1
#define FIX_ALIASING_MIN_TRIANGLE_WIDTH 2.0

// Filter bank
FilterBank::FilterBankObj::FilterBankObj(int dataSize,
                                         BL_FLOAT sampleRate,
                                         int numFilters)
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

FilterBank::FilterBankObj::FilterBankObj()
{
    mDataSize = 0;
    mSampleRate = 0.0;
    mNumFilters = 0;
}

FilterBank::FilterBankObj::~FilterBankObj() {}

//

// See: http://practicalcryptography.com/miscellaneous/machine-learning/guide-mel-frequency-cepstral-coefficients-mfccs/

FilterBank::FilterBank(Scale::Type targetScaleType)
{
    mTargetScaleType = targetScaleType;

    mScale = new Scale();
}

FilterBank::~FilterBank()
{
    delete mScale;
}

void
FilterBank::HzToTarget(WDL_TypedBuf<BL_FLOAT> *result,
                       const WDL_TypedBuf<BL_FLOAT> &magns,
                       BL_FLOAT sampleRate, int numFilters)
{
    if ((magns.GetSize() != mHzToTargetFilterBank.mDataSize) ||
        (sampleRate != mHzToTargetFilterBank.mSampleRate) ||
        (numFilters != mHzToTargetFilterBank.mNumFilters))
    {
        CreateFilterBankHzToTarget(&mHzToTargetFilterBank, magns.GetSize(),
                                   sampleRate, numFilters);
    }
    
    ApplyFilterBank(result, magns, mHzToTargetFilterBank);
}

void
FilterBank::TargetToHz(WDL_TypedBuf<BL_FLOAT> *result,
                       const WDL_TypedBuf<BL_FLOAT> &magns,
                       BL_FLOAT sampleRate, int numFilters)
{
    if ((magns.GetSize() != mTargetToHzFilterBank.mDataSize) ||
        (sampleRate != mTargetToHzFilterBank.mSampleRate) ||
        (numFilters != mTargetToHzFilterBank.mNumFilters))
    {
        CreateFilterBankTargetToHz(&mTargetToHzFilterBank, magns.GetSize(),
                                   sampleRate, numFilters);
    }
    
    ApplyFilterBank(result, magns, mTargetToHzFilterBank);
}

BL_FLOAT
FilterBank::ComputeTriangleAreaBetween(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                                       BL_FLOAT x0, BL_FLOAT x1)
{
    if ((x0 > txmax) || (x1 < txmin))
        return 0.0;
    
    vector<BL_FLOAT> &x = mTmpBuf0;
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
FilterBank::ComputeTriangleY(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
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
FilterBank::CreateFilterBankHzToTarget(FilterBankObj *filterBank, int dataSize,
                                       BL_FLOAT sampleRate, int numFilters)
{
    // Clear previous
    filterBank->mFilters.resize(0);

    // Init
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
    BL_FLOAT lowFreqTarget = 0.0;
    //BL_FLOAT highFreqTarget = mScale->ApplyScale(mTargetScaleType, sampleRate*0.5);
    BL_FLOAT highFreqTarget = ApplyScale(sampleRate*0.5, 0.0, sampleRate*0.5);
    
    // Compute equally spaced target values
    WDL_TypedBuf<BL_FLOAT> targetPoints;
    targetPoints.Resize(numFilters + 2);
    for (int i = 0; i < targetPoints.GetSize(); i++)
    {
        // Compute target value
        BL_FLOAT t = ((BL_FLOAT)i)/(targetPoints.GetSize() - 1);
        BL_FLOAT val = lowFreqTarget + t*(highFreqTarget - lowFreqTarget);
        
        targetPoints.Get()[i] = val;
    }
    
    // Compute target points
    WDL_TypedBuf<BL_FLOAT> hzPoints;
    hzPoints.Resize(targetPoints.GetSize());
    for (int i = 0; i < hzPoints.GetSize(); i++)
    {
        // Compute hz value
        BL_FLOAT val = targetPoints.Get()[i];
        //val = mScale->ApplyScaleInv(mTargetScaleType, val);
        val = ApplyScaleInv(val, 0.0, sampleRate*0.5);
        
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
    
    // For each filter
    for (int m = 1; m < numFilters + 1; m++)
    {
        BL_FLOAT fmin = bin.Get()[m - 1]; // left
        BL_FLOAT fmid = bin.Get()[m];     // center
        BL_FLOAT fmax = bin.Get()[m + 1]; // right

#if FIX_ALIASING_LOW_FREQS
        FixSmallTriangles(&fmin, &fmax, dataSize);
#endif
            
        //
        filterBank->mFilters[m - 1].mBounds[0] = std::floor(fmin);
        filterBank->mFilters[m - 1].mBounds[1] = std::ceil(fmax);
        
        // Keep floating values
        //filterBank->mFilters[m - 1].mBounds[0] = fmin;
        //filterBank->mFilters[m - 1].mBounds[1] = fmax;

        // Check upper bound
        if (filterBank->mFilters[m - 1].mBounds[1] > dataSize - 1)
            filterBank->mFilters[m - 1].mBounds[1] = dataSize - 1;
        
        for (int i = filterBank->mFilters[m - 1].mBounds[0];
             i <= filterBank->mFilters[m - 1].mBounds[1]; i++)
        {
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
FilterBank::CreateFilterBankTargetToHz(FilterBankObj *filterBank, int dataSize,
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
    WDL_TypedBuf<BL_FLOAT> targetPoints;
    targetPoints.Resize(hzPoints.GetSize());
    for (int i = 0; i < targetPoints.GetSize(); i++)
    {
        // Compute hz value
        BL_FLOAT val = hzPoints.Get()[i];
        //val = mScale->ApplyScale(mTargetScaleType, val);
        val = ApplyScale(val, 0.0, sampleRate*0.5);
        
        targetPoints.Get()[i] = val;
    }
    
    // Compute bin points
    WDL_TypedBuf<BL_FLOAT> bin;
    bin.Resize(targetPoints.GetSize());
    
    //BL_FLOAT maxTarget = mScale->ApplyScale(mTargetScaleType, sampleRate*0.5);
    BL_FLOAT maxTarget = ApplyScale(sampleRate*0.5, 0.0, sampleRate*0.5);
    BL_FLOAT targetPerBinInv = (dataSize + 1)/maxTarget;
    for (int i = 0; i < bin.GetSize(); i++)
    {
        // Compute target value
        BL_FLOAT val = targetPoints.Get()[i];
        
        // For the new solution that fills holes, do not round or trunk
        val = val*targetPerBinInv;
        
        bin.Get()[i] = val;
    }
    
    // For each filter
    for (int m = 1; m < numFilters; m++)
    {
        BL_FLOAT fmin = bin.Get()[m - 1]; // left
        BL_FLOAT fmid = bin.Get()[m];     // center
        BL_FLOAT fmax = bin.Get()[m + 1]; // right

#if FIX_ALIASING_LOW_FREQS
        FixSmallTriangles(&fmin, &fmax, dataSize);
#endif
            
        //
        filterBank->mFilters[m].mBounds[0] = std::floor(fmin);
        filterBank->mFilters[m].mBounds[1] = std::ceil(fmax);

        // Check upper bound
        if (filterBank->mFilters[m ].mBounds[1] > dataSize - 1)
            filterBank->mFilters[m].mBounds[1] = dataSize - 1;
        
        for (int i = filterBank->mFilters[m].mBounds[0];
             i <= filterBank->mFilters[m].mBounds[1]; i++)
        {
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
            
            filterBank->mFilters[m].mData.Get()[i] += tarea;
        }
    }
}

// Optimized
//
// FIX: this optimization goes faster, but also fixes a bug!
// Dump vocal, Energetic Upbeat => the top frequencies looks strange,
// like some drums frequencies repeated.
void
FilterBank::ApplyFilterBank(WDL_TypedBuf<BL_FLOAT> *result,
                            const WDL_TypedBuf<BL_FLOAT> &magns,
                            const FilterBankObj &filterBank)
{
    result->Resize(filterBank.mNumFilters);
    BLUtils::FillAllZero(result);
    
    // For each filter
    for (int m = 0; m < filterBank.mNumFilters; m++)
    {
        const FilterBankObj::Filter &filter = filterBank.mFilters[m];

        const BL_FLOAT *filterData = filter.mData.Get();
        BL_FLOAT *resultData = result->Get();
        BL_FLOAT *magnsData = magns.Get();
        
        // For each destination value

        // NOTE: no need to take margin around bounds
        // Bounds are now well set (since anti stairs fix)
        
        //for (int i = filter.mBounds[0] - 1; i < filter.mBounds[1] + 1; i++)
        for (int i = filter.mBounds[0]; i <= filter.mBounds[1]; i++)
        {
            // NOTE: we previously check well bounds
            // so do not check here (may save some resources)

            // NOTE2: after having passed valgrind, these tests look still necessary!
            if (i < 0)
                continue;
            if (i >= magns.GetSize())
                continue;
            
            // Apply the filter value
            resultData[m] += filterData[i]*magnsData[i];
        }
    }
}

void
FilterBank::DBG_DumpFilterBank(const FilterBankObj &filterBank)
{
    for (int m = 0; m < filterBank.mNumFilters; m++)
    {
        const FilterBankObj::Filter &filter = filterBank.mFilters[m];
        
        if (m == 0)
            BLDebug::DumpData("filters.txt", filter.mData);
        else
            BLDebug::AppendData("filters.txt", filter.mData);
        BLDebug::AppendNewLine("filters.txt");

#if 0   // If we want to use this, we must turn the bounds to float,
        // and remove ceil and floor 
        if (m == 0)
            BLDebug::DumpValue("centers.txt",
                               ((BL_FLOAT)(filter.mBounds[1] + filter.mBounds[0]))*0.5);
        else
            BLDebug::AppendValue("centers.txt",
                                 ((BL_FLOAT)(filter.mBounds[1] + filter.mBounds[0]))*0.5);

        if (m == 0)
            BLDebug::DumpValue("widths.txt",
                               ((BL_FLOAT)(filter.mBounds[1] - filter.mBounds[0])));
        else
            BLDebug::AppendValue("widths.txt",
                                 ((BL_FLOAT)(filter.mBounds[1] - filter.mBounds[0])));
#endif
    }
}

BL_FLOAT
FilterBank::ApplyScale(BL_FLOAT val, BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
    BL_FLOAT minTarget = mScale->ApplyScale(mTargetScaleType, 0.0, minFreq, maxFreq);
    BL_FLOAT maxTarget = mScale->ApplyScale(mTargetScaleType, 1.0, minFreq, maxFreq);;

    // Normalize
    val = (val - minFreq)/(maxFreq - minFreq);
    
    // Apply (normalized)
    val = mScale->ApplyScale(mTargetScaleType, val, minFreq, maxFreq);

    // De normalize
    val = val*(maxTarget - minTarget) + minTarget;
    
    return val;
}

BL_FLOAT
FilterBank::ApplyScaleInv(BL_FLOAT val, BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
    BL_FLOAT minTarget = mScale->ApplyScale(mTargetScaleType, 0.0, minFreq, maxFreq);
    BL_FLOAT maxTarget = mScale->ApplyScale(mTargetScaleType, 1.0, minFreq, maxFreq);

    // Normalized
    val = (val - minTarget)/(maxTarget - minTarget);
 
    // Apply (normalized)
    val = mScale->ApplyScaleInv(mTargetScaleType, val, minFreq, maxFreq);

    // De normalize
    val = val*(maxFreq - minFreq) + minFreq;
    
    return val;
}

void
FilterBank::FixSmallTriangles(BL_FLOAT *fmin, BL_FLOAT *fmax, int dataSize)
{
    if (dataSize < FIX_ALIASING_MIN_TRIANGLE_WIDTH)
        return;
    
    // Hard fix: grow by 1 on the left and on the right
    /* *fmin = *fmin - 1.0;
       if (*fmin < 0.0)
       *fmin = 0.0;
       *fmax = *fmax + 1.0;
       if (*fmax > dataSize - 1)
       *fmax = dataSize - 1;*/

    // Smart fix
    if (*fmax - *fmin < FIX_ALIASING_MIN_TRIANGLE_WIDTH)
    {
        BL_FLOAT diff = FIX_ALIASING_MIN_TRIANGLE_WIDTH - (*fmax - *fmin);
        *fmin -= diff*0.5;
        *fmax += diff*0.5;

        if (*fmin < 0.0)
        {
            *fmax += -*fmin;
            *fmin = 0.0;
        }

        if (*fmax > dataSize - 1)
        {
            *fmin -= *fmax - (dataSize - 1);
            *fmax = dataSize - 1;
        }
    }
}
