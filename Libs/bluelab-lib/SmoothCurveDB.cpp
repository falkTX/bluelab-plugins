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
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifdef IGRAPHICS_NANOVG

#include <SmoothAvgHistogramDB.h>
#include <GraphCurve5.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "SmoothCurveDB.h"

// Avoid having a big stack of curves: keep only the last one!
// e.g with block size = 32, there was dozens of stacked curves to process
#define OPTIM_LOCK_FREE 1

SmoothCurveDB::SmoothCurveDB(GraphCurve5 *curve,
                             BL_FLOAT smoothFactor,
                             int size, BL_FLOAT defaultValue,
                             BL_FLOAT minDB, BL_FLOAT maxDB,
                             BL_FLOAT sampleRate)
{
    mMinDB = minDB;
    mMaxDB = maxDB;
    
    mHistogram = new SmoothAvgHistogramDB(size, smoothFactor,
                                          defaultValue, minDB, maxDB);
    
    mCurve = curve;

    mSampleRate = sampleRate;

    mUseLegacyLock = false;
}

SmoothCurveDB::~SmoothCurveDB()
{
    delete mHistogram;
}

void
SmoothCurveDB::Reset(BL_FLOAT sampleRate, BL_FLOAT smoothFactor)
{
    // Smooth factor can change when DAW block size changes
    mHistogram->Reset(smoothFactor);

    mSampleRate = sampleRate;
}

void
SmoothCurveDB::ClearValues()
{
    if (mUseLegacyLock)
    {
        ClearValuesLF();
        
        return;
    }
    
    LockFreeCurve &curve = mTmpBuf6;
    curve.mCommand = LockFreeCurve::CLEAR_VALUES;

    mLockFreeQueues[0].push(curve);
}

void
SmoothCurveDB::ClearValuesLF()
{
    mHistogram->Reset();
    
    mCurve->ClearValues();
}

void
SmoothCurveDB::SetValues(const WDL_TypedBuf<BL_FLOAT> &values, bool reset)
{
    if (mUseLegacyLock)
    {
        SetValuesLF(values, reset);
            
        return;
    }
    
    LockFreeCurve &curve = mTmpBuf4;
    curve.mCommand = LockFreeCurve::SET_VALUES;
    curve.mValues = values;
    curve.mReset = reset;

    // If reset, must keep the command anyway
    if (reset)
        mLockFreeQueues[0].push(curve);
    else
    {
#if !OPTIM_LOCK_FREE
        mLockFreeQueues[0].push(curve);
#else
        bool clearValues = ContainsClearValues(mLockFreeQueues[0]);
        bool reset0 = ContainsCurveReset(mLockFreeQueues[0]);
        
        if (mLockFreeQueues[0].empty())
            mLockFreeQueues[0].push(curve);
        else
        {
            mLockFreeQueues[0].set(0, curve);
        }

        if (reset0)
        {
            LockFreeCurve &c0 = mTmpBuf8;
            mLockFreeQueues[0].get(0, c0);
            c0.mReset = true;
            mLockFreeQueues[0].set(0, c0);
        }
        
        if (clearValues)
        {
            LockFreeCurve &c0 = mTmpBuf7;
            mLockFreeQueues[0].get(0, c0);
            if (c0.mCommand != LockFreeCurve::CLEAR_VALUES)
            {
                // Need to re-add a CLEAR_VALUES command, to not loose it
                LockFreeCurve &curve0 = mTmpBuf6;
                curve0.mCommand = LockFreeCurve::CLEAR_VALUES;
                mLockFreeQueues[0].push(curve0);
            }
        }
#endif
    }
}
    
void
SmoothCurveDB::SetValuesLF(const WDL_TypedBuf<BL_FLOAT> &values, bool reset)
{
    // Add the values
    int histoNumValues = mHistogram->GetNumValues();
    
    WDL_TypedBuf<BL_FLOAT> &values0 = mTmpBuf0;
    values0 = values;

    bool useFilterBank = false;
    
#if 0 // ORIGIN
    if (values0.GetSize() > histoNumValues)
    {
        // Decimate
        BL_FLOAT decimFactor = ((BL_FLOAT)histoNumValues)/values0.GetSize();
        
        WDL_TypedBuf<BL_FLOAT> &decimValues = mTmpBuf1;
        BLUtils::DecimateSamples(&decimValues, values0, decimFactor);
        
        values0 = decimValues;
    }
#endif

#if 1 // Filter banks
    useFilterBank = true;

    if (!reset)
    {
        WDL_TypedBuf<BL_FLOAT> &decimValues = mTmpBuf1;

        Scale::FilterBankType type =
        mCurve->mScale->TypeToFilterBankType(mCurve->mXScale);
        mCurve->mScale->ApplyScaleFilterBank(type, &decimValues, values0,
                                             mSampleRate, histoNumValues);
        
        values0 = decimValues;
    }
#endif
    
    WDL_TypedBuf<BL_FLOAT> &avgValues = mTmpBuf2;
    avgValues = values0;

    // Check if we have the same scale
    bool sameScale = false;
    Scale::Type curveScale;
    BL_GUI_FLOAT curveMinY;
    BL_GUI_FLOAT curveMaxY;
    mCurve->GetYScale(&curveScale, &curveMinY, &curveMaxY);

    if ((curveScale == Scale::DB) &&
        (fabs(curveMinY - mMinDB) < BL_EPS) &&
        (fabs(curveMaxY - mMaxDB) < BL_EPS))
        sameScale = true;
        
    if (!reset)
    {
        mHistogram->AddValues(values0);

        // Process values and update curve
        if (!sameScale)
            mHistogram->GetValues(&avgValues);
        else
            mHistogram->GetValuesDB(&avgValues);
    }
    else
    {
        //mHistogram->SetValues(&values0);
        mHistogram->SetValues(&values0, false);
    }
    
    mCurve->ClearValues();
    
#if 0 // Old version
    for (int i = 0; i < avgValues.GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(avgValues.GetSize() - 1);
        BL_FLOAT val = avgValues.Get()[i];
        mCurve->SetValue(t, val);
    }
#endif
    
#if 0 // New version: avoid the risk to have undefined values
    mCurve->SetValues4(avgValues, !sameScale);
#endif

#if 1 // New version: optimized in GraphCurve5 and Scale
    mCurve->SetValues5LF(avgValues, !useFilterBank, !sameScale);
#endif
}

void
SmoothCurveDB::GetHistogramValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    mHistogram->GetValues(values);
}

void
SmoothCurveDB::GetHistogramValuesDB(WDL_TypedBuf<BL_FLOAT> *values)
{
    mHistogram->GetValuesDB(values);
}

void
SmoothCurveDB::PushData()
{
#if !OPTIM_LOCK_FREE
    mLockFreeQueues[1].push(mLockFreeQueues[0]);
#else
    bool clearValues = ContainsClearValues(mLockFreeQueues[1]);
    bool reset = ContainsCurveReset(mLockFreeQueues[1]);
        
    if (mLockFreeQueues[1].empty())
        mLockFreeQueues[1].push(mLockFreeQueues[0]);
    else
        mLockFreeQueues[1].set(0, mLockFreeQueues[0]);

    if (reset)
    {
        LockFreeCurve &c0 = mTmpBuf8;
        mLockFreeQueues[1].get(0, c0);
        c0.mReset = true;
        mLockFreeQueues[1].set(0, c0);
    }
    
    if (clearValues)
    {
        LockFreeCurve &c0 = mTmpBuf7;
        mLockFreeQueues[1].get(0, c0);
        if (c0.mCommand != LockFreeCurve::CLEAR_VALUES)
        {
            // Need to re-add a CLEAR_VALUES command, to not loose it
            LockFreeCurve &curve0 = mTmpBuf6;
            curve0.mCommand = LockFreeCurve::CLEAR_VALUES;
            mLockFreeQueues[1].push(curve0);
        }
    }
#endif
    
    mLockFreeQueues[0].clear();
}

void
SmoothCurveDB::PullData()
{
#if !OPTIM_LOCK_FREE
    mLockFreeQueues[2].push(mLockFreeQueues[1]);
#else
    bool clearValues = ContainsClearValues(mLockFreeQueues[2]);
    bool reset = ContainsCurveReset(mLockFreeQueues[2]);
    
    if (mLockFreeQueues[2].empty())
        mLockFreeQueues[2].push(mLockFreeQueues[1]);
    else
        mLockFreeQueues[2].set(0, mLockFreeQueues[1]);

    if (reset)
    {
        LockFreeCurve &c0 = mTmpBuf8;
        mLockFreeQueues[2].get(0, c0);
        c0.mReset = true;
        mLockFreeQueues[2].set(0, c0);
    }
    
    if (clearValues)
    {
        LockFreeCurve &c0 = mTmpBuf7;
        mLockFreeQueues[0].get(0, c0);
        if (c0.mCommand != LockFreeCurve::CLEAR_VALUES)
        {
            // Need to re-add a CLEAR_VALUES command, to not loose it
            LockFreeCurve &curve0 = mTmpBuf6;
            curve0.mCommand = LockFreeCurve::CLEAR_VALUES;
            mLockFreeQueues[2].push(curve0);
        }
    }
#endif
    
    mLockFreeQueues[1].clear();
}

void
SmoothCurveDB::ApplyData()
{
    for (int i = 0; i < mLockFreeQueues[2].size(); i++)
    {
        LockFreeCurve &curve = mTmpBuf5;
        mLockFreeQueues[2].get(i, curve);

        if (curve.mCommand == LockFreeCurve::SET_VALUES)
            SetValuesLF(curve.mValues, curve.mReset);
        else if (curve.mCommand == LockFreeCurve::CLEAR_VALUES)
            ClearValuesLF();
    }

    mLockFreeQueues[2].clear();
}

void
SmoothCurveDB::SetUseLegacyLock(bool flag)
{
    mUseLegacyLock = flag;
}

bool
SmoothCurveDB::ContainsClearValues(/*const*/LockFreeQueue2<LockFreeCurve> &q)
{
    for (int i = 0; i < q.size(); i++)
    {
        LockFreeCurve c;
        q.get(i, c);

        if (c.mCommand == LockFreeCurve::CLEAR_VALUES)
            return true;
    }

    return false;
}

bool
SmoothCurveDB::ContainsCurveReset(/*const*/LockFreeQueue2<LockFreeCurve> &q)
{
    for (int i = 0; i < q.size(); i++)
    {
        LockFreeCurve c;
        q.get(i, c);

        if ((c.mCommand == LockFreeCurve::SET_VALUES) && c.mReset)
            return true;
    }
    
    return false;
}

#endif
