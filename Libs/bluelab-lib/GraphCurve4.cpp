//
//  GraphCurve.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>
#include "GraphControl11.h"
#include "GraphCurve4.h"

// Fixes a Valgrind uninitialized warning
#define FIX_VALGRIND_UNINIT 1

GraphCurve4::GraphCurve4(int numValues)
{
    mDescription = NULL;
    
    mYdBScale = false;
    mMinY = 0.0;
    mMaxY = 1.0;
    
    SET_COLOR_FROM_INT(mColor, 0, 0, 0, 255);
    mAlpha = 1.0;
    
    mXdBScale = false;
    mMinX = 0.0;
    mMaxX = 1.0;
    
    mPointStyle = false;
    mPointsAsLinesPolar = false;
    mPointsAsLines = false;
    
    mLineWidth = 1.0;
    mPointSize = 1.0;
    
    mBevelFlag = false;
    
    mPointOverlay = false;
    
    mCurveFill = false;
    mFillAlpha = FILL_CURVE_ALPHA;
    mFillAlphaUp = 0.0;
    mFillAlphaUpFlag = false;
    
    // NEW
    SET_COLOR_FROM_INT(mFillColor, 0, 0, 0, 255);
    mFillColorSet = false;
    
    mSingleValueH = false;
    mSingleValueV = false;
    
    mNumValues = numValues;
    mXValues.Resize(mNumValues);
    mYValues.Resize(mNumValues);
    
#if FIX_VALGRIND_UNINIT
    ClearValues();
#endif
    
    FillAllYValues(0.0);
    
    mPushCounter = 0;
    mTmpPushValue = 0.0;
    
    mLimitToBounds = false;
    
    mOptimSameColor = false;
    
    mWeightMultAlpha = true;
    mUseWeightTargetColor = false;
}

GraphCurve4::~GraphCurve4()
{
    if (mDescription != NULL)
        delete []mDescription;
}

void
GraphCurve4::SetDescription(const char *description, int descrColor[4])
{
#define DESCRIPTION_MAX_SIZE 256
    
    if (mDescription != NULL)
        delete []mDescription;
    
    mDescription = new char[DESCRIPTION_MAX_SIZE];
    memset(mDescription, 0, DESCRIPTION_MAX_SIZE*sizeof(char));
    
    sprintf(mDescription, "%s", description);
    
    for (int i = 0; i < 4; i++)
        mDescrColor[i] = descrColor[i];
}

void
GraphCurve4::ResetNumValues(int numValues)
{
    mNumValues = numValues;
    
    mXValues.Resize(mNumValues);
    mYValues.Resize(mNumValues);
    
    BLUtils::FillAllZero(&mXValues);
    BLUtils::FillAllZero(&mYValues);
    
    //BLUtils::ResizeFillZeros(&mXValues, mNumValues);
    //BLUtils::ResizeFillZeros(&mYValues, mNumValues);
}

void
GraphCurve4::ClearValues()
{
    BLUtils::FillAllValue(&mXValues, (BL_GUI_FLOAT)GRAPH_VALUE_UNDEFINED);
    BLUtils::FillAllValue(&mYValues, (BL_GUI_FLOAT)GRAPH_VALUE_UNDEFINED);
}

void
GraphCurve4::SetYScale(bool dBFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY)
{
    mYdBScale = dBFlag;
    mMinY = minY;
    mMaxY = maxY;
}

void
GraphCurve4::FillAllYValues(BL_GUI_FLOAT val)
{
    for (int i = 0; i < mYValues.GetSize(); i++)
    {
        mYValues.Get()[i] = val;
    }
}

void
GraphCurve4::FillAllXValues(BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    for (int i = 0; i < mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(mXValues.GetSize() - 1);
        
        BL_GUI_FLOAT x = minX + t*(maxX - minX);
        
        mXValues.Get()[i] = x;
    }
}

void
GraphCurve4::SetYValues(const WDL_TypedBuf<BL_GUI_FLOAT> *yValues, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    if (mYValues.GetSize() != yValues->GetSize())
        return;
    
    mYValues = *yValues;
    
    mXValues.Resize(0);
    for (int i = 0; i < mYValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(mYValues.GetSize() - 1);
        
        BL_GUI_FLOAT x = minX + t*(maxX - minX);
    
        mXValues.Add(x);
    }
}

void
GraphCurve4::SetValue(BL_GUI_FLOAT t, BL_GUI_FLOAT x, BL_GUI_FLOAT y)
{
    int pos = t*(mNumValues - 1);
    
    if ((pos  < 0) || (pos >= mNumValues))
        return;
    
    mXValues.Get()[pos] = x;
    mYValues.Get()[pos] = y;
}

void
GraphCurve4::PushValue(BL_GUI_FLOAT x, BL_GUI_FLOAT y)
{
    if (mDoSmooth)
        // Smooth Y
    {
        mParamSmoother.SetNewValue(y);
        mParamSmoother.Update();
        y = mParamSmoother.GetCurrentValue();
    }
    
    mXValues.Add(x);
    mYValues.Add(y);
    
    if (mYValues.GetSize() > mNumValues)
    {
        WDL_TypedBuf<BL_GUI_FLOAT> newXValues;
        WDL_TypedBuf<BL_GUI_FLOAT> newYValues;
        for (int i = 1; i < mNumValues + 1; i++)
        {
            BL_GUI_FLOAT x = mXValues.Get()[i];
            BL_GUI_FLOAT y = mYValues.Get()[i];
            
            newXValues.Add(x);
            newYValues.Add(y);
        }
        
        mXValues = newXValues;
        mYValues = newYValues;
    }
}

void
GraphCurve4::NormalizeXValues(BL_GUI_FLOAT maxXValue)
{
    for (int i = 0; i < mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(mNumValues - 1);
        
        mXValues.Get()[i] = t*maxXValue;
    }
}

void
GraphCurve4::SetXScale(bool dbFlag, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    mXdBScale = dbFlag;
    mMinX = minX;
    mMaxX = maxX;
}

void
GraphCurve4::SetPointValues(const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                            const WDL_TypedBuf<BL_GUI_FLOAT> &yValues)
{
    mXValues = xValues;
    mYValues = yValues;
}

void
GraphCurve4::SetLimitToBounds(bool flag)
{
    mLimitToBounds = flag;
}

void
GraphCurve4::SetWeightTargetColor(int color[4])
{
    mUseWeightTargetColor = true;
    
    SET_COLOR_FROM_INT(mWeightTargetColor,
                       color[0], color[1], color[2], color[3]);
}

#endif // IGRAPHICS_NANOVG
