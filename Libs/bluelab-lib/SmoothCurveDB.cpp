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
#include <BLDebug.h>

#include "SmoothCurveDB.h"

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
}

SmoothCurveDB::~SmoothCurveDB()
{
    delete mHistogram;
}

void
SmoothCurveDB::Reset(BL_FLOAT sampleRate)
{
    mHistogram->Reset();

    mSampleRate = sampleRate;
}

void
SmoothCurveDB::ClearValues()
{
    mHistogram->Reset();
    
    mCurve->ClearValues();
}

void
SmoothCurveDB::SetValues(const WDL_TypedBuf<BL_FLOAT> &values, bool reset)
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
    mCurve->SetValues5(avgValues, !useFilterBank, !sameScale);
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
#endif
