//
//  GraphCurve.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

#include <BLUtils.h>
#include <BLUtilsDecim.h>
#include <BLUtilsPlug.h>
#include <BLUtilsMath.h>

#include "GraphCurve5.h"

#define CURVE_PIXEL_DENSITY 2.0

GraphCurve5::GraphCurve5(int numValues)
{
    mGraph = NULL;
    
    mDescription = NULL;

    mScale  = new Scale();
    
    mYScale = Scale::LINEAR;
    mMinY = 0.0;
    mMaxY = 1.0;
    
    SET_COLOR_FROM_INT(mColor, 0, 0, 0, 255);
    mAlpha = 1.0;
    
    mXScale = Scale::LINEAR;
    mMinX = 0.0;
    mMaxX = 1.0;
    
    mPointStyle = false;
    mPointsAsLinesPolar = false;
    mPointsAsLines = false;
    
    // Set line width to 1.5 by default,
    // because with 1, it is to thin and badly displayed sometimes.
    //mLineWidth = 1.0;
    mLineWidth = 1.5;
    
    mPointSize = 1.0;
    
    //mBevelFlag = false;
    mBevelFlag = true;
    
    mPointOverlay = false;
    
    mCurveFill = false;
    mFillAlpha = FILL_CURVE_ALPHA;
    mFillAlphaUp = 0.0;
    mFillAlphaUpFlag = false;
    
    SET_COLOR_FROM_INT(mFillColor, 0, 0, 0, 255);
    mFillColorSet = false;
    
    mSingleValueH = false;
    mSingleValueV = false;
    
    mNumValues = numValues;
    mXValues.Resize(mNumValues);
    mYValues.Resize(mNumValues);
    
    ClearValues();
    
    FillAllYValues(0.0);
    
    mPushCounter = 0;
    mTmpPushValue = 0.0;
    
    mLimitToBounds = false;
    
    mOptimSameColor = false;
    
    mWeightMultAlpha = true;
    mUseWeightTargetColor = false;
    
    mYScaleFactor = 1.0;
    mAutoAdjustFactor = 1.0;
    
    mViewSize[0] = 256;
    mViewSize[1] = 256;
}

GraphCurve5::~GraphCurve5()
{
    if (mDescription != NULL)
        delete []mDescription;

    delete mScale;
}

void
GraphCurve5::SetViewSize(BL_FLOAT width, BL_FLOAT height)
{
    mViewSize[0] = width;
    mViewSize[1] = height;
    
    NotifyGraph();
}

void
GraphCurve5::SetYScaleFactor(BL_FLOAT factor)
{
    mYScaleFactor = factor;
    
    NotifyGraph();
}

void
GraphCurve5::SetAutoAdjustFactor(BL_FLOAT factor)
{
    mAutoAdjustFactor = factor;
    
    NotifyGraph();
}

void
GraphCurve5::SetBounds(const BL_GUI_FLOAT bounds[4])
{
    for (int i = 0; i < 4; i++)
    {
        mBounds[i] = bounds[i];
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetDescription(const char *description, int descrColor[4])
{
#define DESCRIPTION_MAX_SIZE 256
    
    if (mDescription != NULL)
        delete []mDescription;
    
    mDescription = new char[DESCRIPTION_MAX_SIZE];
    memset(mDescription, 0, DESCRIPTION_MAX_SIZE*sizeof(char));
    
    sprintf(mDescription, "%s", description);
    
    for (int i = 0; i < 4; i++)
        mDescrColor[i] = descrColor[i];
    
    NotifyGraph();
}

void
GraphCurve5::ResetNumValues(int numValues)
{
    mNumValues = numValues;
    
    mXValues.Resize(mNumValues);
    mYValues.Resize(mNumValues);
    
    BLUtils::FillAllZero(&mXValues);
    BLUtils::FillAllZero(&mYValues);
    
    NotifyGraph();
}

void
GraphCurve5::ClearValues()
{
    BLUtils::FillAllValue(&mXValues, (BL_GUI_FLOAT)CURVE_VALUE_UNDEFINED);
    BLUtils::FillAllValue(&mYValues, (BL_GUI_FLOAT)CURVE_VALUE_UNDEFINED);
    
    NotifyGraph();
}

void
GraphCurve5::SetYScale(Scale::Type scale, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY)
{
    mYScale = scale;
    mMinY = minY;
    mMaxY = maxY;
    
    NotifyGraph();
}

void
GraphCurve5::GetYScale(Scale::Type *scale, BL_GUI_FLOAT *minY, BL_GUI_FLOAT *maxY)
{
    *scale = mYScale;
    *minY = mMinY;
    *maxY = mMaxY;
}

void
GraphCurve5::FillAllYValues(BL_GUI_FLOAT val)
{
    for (int i = 0; i < mYValues.GetSize(); i++)
    {
        mYValues.Get()[i] = val;
    }
    
    NotifyGraph();
}

void
GraphCurve5::FillAllXValues(BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    for (int i = 0; i < mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(mXValues.GetSize() - 1);
        
        BL_GUI_FLOAT x = minX + t*(maxX - minX);
        
        mXValues.Get()[i] = x;
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetYValues(const WDL_TypedBuf<BL_GUI_FLOAT> *yValues,
                        BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    if (mYValues.GetSize() != yValues->GetSize())
        return;
    
    mYValues = *yValues;
    
    //mXValues.Resize(0);
    mXValues.Resize(mYValues.GetSize());
    for (int i = 0; i < mYValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(mYValues.GetSize() - 1);
        
        BL_GUI_FLOAT x = minX + t*(maxX - minX);
    
        //mXValues.Add(x);
        mXValues.Get()[i] = x;
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValue(BL_GUI_FLOAT t, BL_GUI_FLOAT x, BL_GUI_FLOAT y)
{
    int pos = t*(mNumValues - 1);
    
    if ((pos  < 0) || (pos >= mNumValues))
        return;
    
    mXValues.Get()[pos] = x;
    mYValues.Get()[pos] = y;
    
    NotifyGraph();
}

void
GraphCurve5::PushValue(BL_GUI_FLOAT x, BL_GUI_FLOAT y)
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
        WDL_TypedBuf<BL_GUI_FLOAT> &newXValues = mTmpBuf0;
        newXValues.Resize(mNumValues + 1);
        
        WDL_TypedBuf<BL_GUI_FLOAT> &newYValues = mTmpBuf1;
        newYValues.Resize(mNumValues + 1);
        
        for (int i = 1; i < mNumValues + 1; i++)
        {
            BL_GUI_FLOAT x = mXValues.Get()[i];
            BL_GUI_FLOAT y = mYValues.Get()[i];
            
            //newXValues.Add(x);
            //newYValues.Add(y);

            newXValues.Get()[i] = x;
            newYValues.Get()[i] = y;
        }
        
        mXValues = newXValues;
        mYValues = newYValues;
    }
    
    NotifyGraph();
}

void
GraphCurve5::NormalizeXValues(BL_GUI_FLOAT maxXValue)
{
    for (int i = 0; i < mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(mNumValues - 1);
        
        mXValues.Get()[i] = t*maxXValue;
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetXScale(Scale::Type scale, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    mXScale = scale;
    mMinX = minX;
    mMaxX = maxX;
    
    NotifyGraph();
}

void
GraphCurve5::SetPointValues(const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                            const WDL_TypedBuf<BL_GUI_FLOAT> &yValues)
{
    mXValues = xValues;
    mYValues = yValues;
    
    NotifyGraph();
}

void
GraphCurve5::SetLimitToBounds(bool flag)
{
    mLimitToBounds = flag;
    
    NotifyGraph();
}

void
GraphCurve5::SetWeightTargetColor(int color[4])
{
    mUseWeightTargetColor = true;
    
    SET_COLOR_FROM_INT(mWeightTargetColor,
                       color[0], color[1], color[2], color[3]);
    
    NotifyGraph();
}

void
GraphCurve5::SetColor(int r, int g, int b)
{
    SET_COLOR_FROM_INT(mColor, r, g, b, 255);
    
    NotifyGraph();
}

void
GraphCurve5::SetAlpha(BL_GUI_FLOAT alpha)
{
    mAlpha = alpha;
    
    NotifyGraph();
}

void
GraphCurve5::SetLineWidth(BL_GUI_FLOAT lineWidth)
{
    mLineWidth = lineWidth;
    
    NotifyGraph();
}

void
GraphCurve5::SetBevel(bool bevelFlag)
{
    mBevelFlag = bevelFlag;
    
    NotifyGraph();
}

void
GraphCurve5::SetSmooth(bool flag)
{
    mDoSmooth = flag;
    
    NotifyGraph();
}

void
GraphCurve5::SetFill(bool flag, BL_GUI_FLOAT originY)
{
    mCurveFill = flag;
    mCurveFillOriginY = originY;
    
    NotifyGraph();
}

void
GraphCurve5::SetFillColor(int r, int g, int b)
{
    SET_COLOR_FROM_INT(mFillColor, r, g, b, 255);
    mFillColorSet = true;
    
    NotifyGraph();
}

void
GraphCurve5::SetFillAlpha(BL_GUI_FLOAT alpha)
{
    mFillAlpha = alpha;
    
    NotifyGraph();
}

void
GraphCurve5::SetFillAlphaUp(BL_GUI_FLOAT alpha)
{
    mFillAlphaUpFlag = true;
    mFillAlphaUp = alpha;
    
    NotifyGraph();
}

void
GraphCurve5::SetPointSize(BL_GUI_FLOAT pointSize)
{
    mPointSize = pointSize;
    
    NotifyGraph();}

void
GraphCurve5::SetPointOverlay(bool flag)
{
    mPointOverlay = flag;
    
    NotifyGraph();
}

void
GraphCurve5::SetWeightMultAlpha(bool flag)
{
    mWeightMultAlpha = flag;
    
    NotifyGraph();
}

void
GraphCurve5::SetPointStyle(bool pointFlag,
                           bool pointsAsLinesPolar,
                           bool pointsAsLines)
{
    mPointStyle = pointFlag;
    mPointsAsLinesPolar = pointsAsLinesPolar;
    mPointsAsLines = pointsAsLines;
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesPoint(const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                            const WDL_TypedBuf<BL_GUI_FLOAT> &yValues)
{
    mPointStyle = true;
    
    ClearValues();
    
    int width = mViewSize[0];
    int height = mViewSize[1];
    
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = xValues.Get()[i];
        
        if (i >= yValues.GetSize())
            // Avoids a crash
            continue;
        
        BL_GUI_FLOAT y = yValues.Get()[i];
        
#if !OPTIM_CONVERT_XY
        x = ConvertX(x, width);
        y = ConvertY(y, height);
#endif
        
        mXValues.Get()[i] = x;
        mYValues.Get()[i] = y;
    }
    
#if OPTIM_CONVERT_XY
    ConvertX(&mXValues, width);
    ConvertY(&mYValues, height);
#endif
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesPointEx(const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                              const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                              bool singleScale, bool scaleX, bool centerFlag)
{
    mPointStyle = true;
    
    ClearValues();
    
    int width = mViewSize[0];
    int height = mViewSize[1];
    
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = xValues.Get()[i];
        
        if (i >= yValues.GetSize())
            // Avoids a crash
            continue;
        
        BL_GUI_FLOAT y = yValues.Get()[i];
        
#if !OPTIM_CONVERT_XY
        if (!singleScale)
        {
            x = ConvertX(x, width);
            y = ConvertY(y, height);
        }
        else
        {
            if (scaleX)
            {
                x = ConvertX(x, width);
                y = ConvertY(y, width);
                
                // TODO: test this
                if (centerFlag)
                    x -= (width - height)*0.5;
            }
            else
            {
                x = ConvertX(x, height);
                y = ConvertY(y, height);
                
                if (centerFlag)
                    x += (width - height)*0.5;
            }
        }
#endif
        
        mXValues.Get()[i] = x;
        mYValues.Get()[i] = y;
    }
    
#if OPTIM_CONVERT_XY
    if (!singleScale)
    {
        ConvertX(&mXValues, width);
        ConvertY(&mYValues, height);
    }
    else
    {
        if (scaleX)
        {
            ConvertX(&mXValues, width);
            ConvertY(&mYValues, width);
            
            // TODO: test this
            if (centerFlag)
            {
                int offset = (width - height)*0.5;
                for (int i = 0; i < mXValues.GetSize(); i++)
                {
                    mXValues.Get()[i] -= offset;
                }
            }
        }
        else
        {
            ConvertX(&mXValues, height);
            ConvertY(&mYValues, height);
            
            if (centerFlag)
            {
                int offset = (width - height)*0.5;
                for (int i = 0; i < mXValues.GetSize(); i++)
                {
                    mXValues.Get()[i] += offset;
                }
            }
        }
    }
#endif
    
    NotifyGraph();
}


void
GraphCurve5::SetValuesPointWeight(const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                  const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                                  const WDL_TypedBuf<BL_GUI_FLOAT> &weights)
{
    mPointStyle = true;
    
    ClearValues();
    
    int width = mViewSize[0];
    int height = mViewSize[1];
    
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = xValues.Get()[i];
        BL_GUI_FLOAT y = yValues.Get()[i];
        
#if !OPTIM_CONVERT_XY
        x = ConvertX(x, width);
        y = ConvertY(y, height);
#endif
        
        mXValues.Get()[i] = x;
        mYValues.Get()[i] = y;
    }
    
#if OPTIM_CONVERT_XY
    ConvertX(&mXValues, width);
    ConvertY(&mYValues, height);
#endif
    
    mWeights = weights;
    
    NotifyGraph();
}

void
GraphCurve5::SetColorWeight(const WDL_TypedBuf<BL_GUI_FLOAT> &colorWeights)
{
    mWeights = colorWeights;
    
    NotifyGraph();
}

void
GraphCurve5::SetSingleValueH(bool flag)
{
    mSingleValueH = flag;
    
    NotifyGraph();
}

void
GraphCurve5::SetSingleValueV(bool flag)
{
    mSingleValueV = flag;
    
    NotifyGraph();
}

void
GraphCurve5::SetOptimSameColor(bool flag)
{
    mOptimSameColor = flag;
    
    NotifyGraph();
}

void
GraphCurve5::Reset(BL_GUI_FLOAT val)
{
    FillAllXValues(mMinX, mMaxX);
    FillAllYValues(val);
    
    NotifyGraph();
}

void
GraphCurve5::FillAllValues(BL_GUI_FLOAT val)
{
    FillAllXValues(mMinX, mMaxX);
    FillAllYValues(val);
    
    NotifyGraph();
}

void
GraphCurve5::SetValues(const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    SetYValues(values, mMinX, mMaxX);
    
    NotifyGraph();
}

void
GraphCurve5::SetValues2(const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        BL_GUI_FLOAT y = values->Get()[i];
        
        SetValue(t, y);
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValues3(const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    ClearValues();
    
    if (values->GetSize() == 0)
        return;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        BL_GUI_FLOAT y = values->Get()[i];
        
        SetValue(t, y);
    }
    
    NotifyGraph();
}

// Avoid having undefined values
// values (y) must be in amp units
void
GraphCurve5::SetValues4(const WDL_TypedBuf<BL_GUI_FLOAT> &values,
                        bool applyYScale)
{
    //if (mYValues.GetSize() != values.GetSize())
    //    return;
    
    // Normalize, then adapt to the graph
    int width = mViewSize[0];
    int height = mViewSize[1];
    
    mXValues.Resize(values.GetSize());
    int numXValues = mXValues.GetSize();
    BL_GUI_FLOAT *xValuesData = mXValues.Get();
    
    BL_GUI_FLOAT t = 0.0;
    BL_GUI_FLOAT tincr = 0.0;
    if (mXValues.GetSize() > 1)
        tincr = 1.0/(mXValues.GetSize() - 1);
    for (int i = 0; i < numXValues; i++)
    {
        //BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(mXValues.GetSize() - 1);
        
        BL_GUI_FLOAT x = mScale->ApplyScale(mXScale, t, mMinX, mMaxX);
        
        // Scale for the interface
        x = x * width;
        
        //mXValues.Get()[i] = x;
        xValuesData[i] = x;

        t += tincr;
    }
    
    mYValues.Resize(values.GetSize());
    int numYValues = mYValues.GetSize();
    BL_GUI_FLOAT *yValuesData = mYValues.Get();
    
    BL_GUI_FLOAT *valuesData = values.Get();
    for (int i = 0; i < numYValues; i++)
    {
        BL_GUI_FLOAT y = valuesData[i];

        if (applyYScale)
            y = ConvertY(y, height);
        else
            y = ConvertYNoScale(y, height);
        
        // For Ghost and mini view
        if (mLimitToBounds)
        {
            if (y < (1.0 - mBounds[3])*height)
                y = (1.0 - mBounds[3])*height;
            
            if (y > (1.0 - mBounds[1])*height)
                y = (1.0 - mBounds[1])*height;
        }
        
        // No need, this is done in ConvertY()
        //y = y * height;
        
        //mYValues.Get()[i] = y;
        yValuesData[i] = y;
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValues5(const WDL_TypedBuf<BL_GUI_FLOAT> &values,
                        bool applyXScale, bool applyYScale)
{
    Command &cmd = mTmpBuf9;
    cmd.mType = Command::SET_VALUES5;
    cmd.mValues = values;
    cmd.mApplyXScale = applyXScale;
    cmd.mApplyYScale = applyYScale;

    mLockFreeQueues[0].push(cmd);
}

// Avoid having undefined values
// values (y) must be in amp units
//
// And unroll some code
void
GraphCurve5::SetValues5LF(const WDL_TypedBuf<BL_GUI_FLOAT> &values,
                          bool applyXScale, bool applyYScale)
{
    // Normalize, then adapt to the graph
    int width = mViewSize[0];
    int height = mViewSize[1];

    // X
    mXValues.Resize(values.GetSize());
    int numXValues = mXValues.GetSize();
    BL_GUI_FLOAT *xValuesData = mXValues.Get();

    BL_GUI_FLOAT t = 0.0;
    BL_GUI_FLOAT tincr = 0.0;
    if (mXValues.GetSize() > 1)
        tincr = 1.0/(mXValues.GetSize() - 1);
    for (int i = 0; i < numXValues; i++)
    {
        xValuesData[i] = t;
        t += tincr;
    }

    if (applyXScale)
        mScale->ApplyScaleForEach(mXScale, &mXValues, mMinX, mMaxX);

    BLUtils::MultValues(&mXValues, (BL_GUI_FLOAT)width);
    
    // Y
    mYValues = values;
    int numYValues = mYValues.GetSize();
    BL_GUI_FLOAT *yValuesData = mYValues.Get();
    
    if (applyYScale)
    {
        if (mYScale != Scale::LINEAR)
            mScale->ApplyScaleForEach(mYScale, &mYValues, mMinY, mMaxY);
        else
            mScale->ApplyScaleForEach(Scale::NORMALIZED, &mYValues, mMinY, mMaxY);
    }
    
    BL_FLOAT factor = mAutoAdjustFactor * mYScaleFactor * height;
    BLUtils::MultValues(&mYValues, factor);

    // For Ghost and mini view
    if (mLimitToBounds)
    {
        for (int i = 0; i < numYValues; i++)
        {
            BL_GUI_FLOAT y = yValuesData[i];
        
            if (y < (1.0 - mBounds[3])*height)
                y = (1.0 - mBounds[3])*height;
            
            if (y > (1.0 - mBounds[1])*height)
                y = (1.0 - mBounds[1])*height;

            yValuesData[i] = y;
        }
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesDecimateSimple(const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    int width = mViewSize[0];
    
    BL_GUI_FLOAT prevX = -1.0;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        
        BL_GUI_FLOAT x = t*width;
        BL_GUI_FLOAT y = values->Get()[i];
        
        if (x - prevX < 1.0)
            continue;
        
        prevX = x;
        
        SetValue(t, y);
    }
    
    // Avoid last value at 0 !
    // (would make a traversing line in the display)
    BL_GUI_FLOAT lastValue = values->Get()[values->GetSize() - 1];
    SetValue(1.0, lastValue);
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesDecimate(const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                               bool isWaveSignal)
{
    int width = mViewSize[0];
    
    BL_GUI_FLOAT prevX = -1.0;
    BL_GUI_FLOAT maxY = -1.0;
    
    if (isWaveSignal)
        maxY = (mMinY + mMaxY)/2.0;
    
    BL_GUI_FLOAT thrs = 1.0/CURVE_PIXEL_DENSITY;
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        
        BL_GUI_FLOAT x = t*width;
        BL_GUI_FLOAT y = values->Get()[i];
        
        // Keep the maximum
        // (we prefer keeping the maxima, and not discard them)
        if (!isWaveSignal)
        {
            if (std::fabs(y) > maxY)
                maxY = y;
        }
        else
        {
            if (std::fabs(y) > std::fabs(maxY))
                maxY = y;
        }
        
        if (x - prevX < thrs)
            continue;
        
        prevX = x;
        
        SetValue(t, maxY);
        
        maxY = -1.0;
        
        if (isWaveSignal)
            maxY = (mMinY + mMaxY)/2.0;
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesDecimate2(const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                BL_FLOAT decFactor,
                                bool isWaveSignal)
{
    ClearValues();
    
    if (values->GetSize() == 0)
        return;
    
    // Decimate
    WDL_TypedBuf<BL_GUI_FLOAT> &decimValues = mTmpBuf3;
    if (isWaveSignal)
        BLUtilsDecim::DecimateSamples(&decimValues, *values, decFactor);
    else
        BLUtilsDecim::DecimateValues(&decimValues, *values, decFactor);
    
    for (int i = 0; i < decimValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(decimValues.GetSize() - 1);
        
        BL_GUI_FLOAT y = decimValues.Get()[i];
        
        SetValue(t, y);
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesDecimate3(const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                BL_GUI_FLOAT decFactor,
                                bool isWaveSignal)
{
    ClearValues();
    
    if (values->GetSize() == 0)
        return;
    
    // Decimate
    WDL_TypedBuf<BL_GUI_FLOAT> &decimValues = mTmpBuf4;
    if (isWaveSignal)
        BLUtilsDecim::DecimateSamples2(&decimValues, *values, decFactor);
    else
        BLUtilsDecim::DecimateValues(&decimValues, *values, decFactor);
    
    for (int i = 0; i < decimValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(decimValues.GetSize() - 1);
        
        BL_GUI_FLOAT y = decimValues.Get()[i];
        
        SetValue(t, y);
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesXDbDecimate(const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                  int nativeBufferSize, BL_GUI_FLOAT sampleRate,
                                  BL_GUI_FLOAT decimFactor)
{
    int bufferSize = BLUtilsPlug::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    BL_GUI_FLOAT hzPerBin = sampleRate/bufferSize;
    
    BL_GUI_FLOAT minHzValue;
    BL_GUI_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                     bufferSize, sampleRate);
    
    WDL_TypedBuf<BL_GUI_FLOAT> &logSignal = mTmpBuf5;
    BLUtils::FreqsToDbNorm(&logSignal, *values, hzPerBin,
                           minHzValue, maxHzValue);
    
    WDL_TypedBuf<BL_GUI_FLOAT> &decimSignal = mTmpBuf6;
    BLUtilsDecim::DecimateValues(&decimSignal, logSignal, decimFactor);
    
    for (int i = 0; i < decimSignal.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i) / (decimSignal.GetSize() - 1);
        
        SetValue(t, decimSignal.Get()[i]);
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValuesXDbDecimateDb(const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                    int nativeBufferSize, BL_GUI_FLOAT sampleRate,
                                    BL_GUI_FLOAT decimFactor, BL_GUI_FLOAT minValueDb)
{
    int bufferSize = BLUtilsPlug::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    BL_GUI_FLOAT hzPerBin = sampleRate/bufferSize;
    
    BL_GUI_FLOAT minHzValue;
    BL_GUI_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                     bufferSize, sampleRate);
    
    WDL_TypedBuf<BL_GUI_FLOAT> &logSignal = mTmpBuf7;
    BLUtils::FreqsToDbNorm(&logSignal, *values, hzPerBin,
                           minHzValue, maxHzValue);
    
    WDL_TypedBuf<BL_GUI_FLOAT> &decimSignal = mTmpBuf8;
    BLUtilsDecim::DecimateValuesDb(&decimSignal, logSignal, decimFactor, minValueDb);
    
    for (int i = 0; i < decimSignal.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i) / (decimSignal.GetSize() - 1);
        
        SetValue(t, decimSignal.Get()[i]);
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetValue(BL_GUI_FLOAT t, BL_GUI_FLOAT val)
{
    // Normalize, then adapt to the graph
    int width = mViewSize[0];
    int height = mViewSize[1];
    
    BL_GUI_FLOAT x = t;
    
    if (x < CURVE_VALUE_UNDEFINED) // for float
    {
        x = mScale->ApplyScale(mXScale, x, mMinX, mMaxX);
            
        // X should be already normalize in input
        
        // Scale for the interface
        x = x * width;
    }
    
    BL_GUI_FLOAT y = ConvertY(val, height);
    
    // For Ghost and mini view
    if (mLimitToBounds)
    {
        if (y < (1.0 - mBounds[3])*height)
            y = (1.0 - mBounds[3])*height;
        
        if (y > (1.0 - mBounds[1])*height)
            y = (1.0 - mBounds[1])*height;
    }
    
    SetValue(t, x, y);
    
    NotifyGraph();
}

void
GraphCurve5::SetSingleValueH(BL_GUI_FLOAT val)
{
    SetValue(0.0, val);
    
    NotifyGraph();
}

void
GraphCurve5::SetSingleValueV(BL_GUI_FLOAT val)
{
    SetValue(0.0, val);
    
    NotifyGraph();
}

void
GraphCurve5::PushValue(BL_GUI_FLOAT val)
{
    int height = mViewSize[1];
    
    val = ConvertY(val, height);
    
    BL_GUI_FLOAT dummyX = 1.0;
    PushValue(dummyX, val);
    
    BL_GUI_FLOAT maxXValue = mViewSize[0];
    NormalizeXValues(maxXValue);
    
    NotifyGraph();
}

BL_GUI_FLOAT
GraphCurve5::ConvertX(BL_GUI_FLOAT val, BL_GUI_FLOAT width)
{
    BL_GUI_FLOAT x = val;
    if (x < CURVE_VALUE_UNDEFINED)
    {
        if (mXScale != Scale::LINEAR)
            x = mScale->ApplyScale(mXScale, x, mMinX, mMaxX);
        else
            x = (x - mMinX)/(mMaxX - mMinX);
        
        x = x * width;
    }
    
    return x;
}

BL_GUI_FLOAT
GraphCurve5::ConvertY(BL_GUI_FLOAT val, BL_GUI_FLOAT height)
{
    BL_GUI_FLOAT y = val;
    if (y < CURVE_VALUE_UNDEFINED)
    {
        if (mYScale != Scale::LINEAR)
            y = mScale->ApplyScale(mYScale, y, mMinY, mMaxY);
        else
            y = (y - mMinY)/(mMaxY - mMinY);
        
        y = y * mAutoAdjustFactor * mYScaleFactor * height;
    }
    
    return y;
}

BL_GUI_FLOAT
GraphCurve5::ConvertYNoScale(BL_GUI_FLOAT val, BL_GUI_FLOAT height)
{
    BL_GUI_FLOAT y = val;
    if (y < CURVE_VALUE_UNDEFINED)
    {
        y = y * mAutoAdjustFactor * mYScaleFactor * height;
    }
    
    return y;
}

// Optimized versions
void
GraphCurve5::ConvertX(WDL_TypedBuf<BL_GUI_FLOAT> *vals, BL_GUI_FLOAT width)
{
    BL_GUI_FLOAT xCoeff = width/(mMaxX - mMinX);
    
    for (int i = 0; i < vals->GetSize(); i++)
    {
        BL_GUI_FLOAT x = vals->Get()[i];
        if (x < CURVE_VALUE_UNDEFINED)
        {
            if (mXScale == Scale::DB)
            {
                if (x > 0.0)
                {
                    if (std::fabs(x) < BL_EPS)
                        x = mMinX;
                    else
                        x = BLUtils::AmpToDB(x);
                    
                    x = (x - mMinX)*xCoeff;
                }
            }
            else if ((mXScale == Scale::LOG) ||
                     (mXScale == Scale::LOG10) ||
                     (mXScale == Scale::LOG_FACTOR) ||
                     (mXScale == Scale::MEL))
            {
                // Not optimized
                x = mScale->ApplyScale(mXScale, x, mMinX, mMaxX);
                
            }
            else
            {
                x = (x - mMinX)*xCoeff;
            }
        }
        
        vals->Get()[i] = x;
    }
    
    NotifyGraph();
}

void
GraphCurve5::ConvertY(WDL_TypedBuf<BL_GUI_FLOAT> *vals,
                      BL_GUI_FLOAT height)
{
    BL_GUI_FLOAT yCoeff =
        mAutoAdjustFactor * mYScaleFactor*height/(mMaxY - mMinY);
    
    for (int i = 0; i < vals->GetSize(); i++)
    {
        BL_GUI_FLOAT y = vals->Get()[i];
        if (y < CURVE_VALUE_UNDEFINED)
        {
            if (mYScale == Scale::DB)
            {
                if (y > 0.0)
                {
                    if (std::fabs(y) < BL_EPS)
                        y = mMinY;
                    else
                        y = BLUtils::AmpToDB(y);
                    
                    y = (y - mMinY)*yCoeff;
                }
            }
            else if ((mYScale == Scale::LOG) ||
                     (mYScale == Scale::LOG10) ||
                     (mYScale == Scale::LOG_FACTOR) ||
                     (mYScale == Scale::MEL))
            {
                // Not optimized
                y = mScale->ApplyScale(mYScale, y, mMinY, mMaxY);
            }
            else
            {
                y = (y - mMinY)*yCoeff;
            }
        }
        
        vals->Get()[i] = y;
    }
    
    NotifyGraph();
}

void
GraphCurve5::SetGraph(GraphControl12 *graph)
{
    mGraph = graph;
}

void
GraphCurve5::NotifyGraph()
{
    if (mGraph != NULL)
        // Notify the graph
        mGraph->SetDataChanged();
}

void
GraphCurve5::PushData()
{
    mLockFreeQueues[1].push(mLockFreeQueues[0]);
    mLockFreeQueues[0].clear();
}

void
GraphCurve5::PullData()
{
    mLockFreeQueues[2].push(mLockFreeQueues[1]);
    mLockFreeQueues[1].clear();
}

void
GraphCurve5::ApplyData()
{
    for (int i = 0; i < mLockFreeQueues[2].size(); i++)
    {
        Command &cmd = mTmpBuf10;
        mLockFreeQueues[2].get(i, cmd);

        if (cmd.mType == Command::SET_VALUES5)
            SetValues5LF(cmd.mValues, cmd.mApplyXScale, cmd.mApplyYScale);
        
    }

    mLockFreeQueues[2].clear();
}

#endif
