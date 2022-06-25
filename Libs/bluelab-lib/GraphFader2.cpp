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
//  GraphFader.cpp
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include "GraphControl11.h"
#include "GraphFader2.h"

#define THRESHOLD_WEIGHTS     0
#define THRESHOLD_WEIGHTS_EPS 0.1

#define THRESHOLD_WEIGHTS2     0 //1
#define THRESHOLD_WEIGHTS_EPS2 0.5
#define DECIM_FACTOR           8.0


// Try to ignore lower curves
// (because due to alpha and luminance, the points
// will be almost invisible)
#define SUPPRESS_LOW_CURVES 0

GraphFader2::GraphFader2(GraphControl11 *graphControl,
                         int startCurve, int numFadeCurves,
                         BL_FLOAT startAlpha, BL_FLOAT endAlpha,
                         BL_FLOAT startLuminance, BL_FLOAT endLuminance)
{
    mGraph = graphControl;
    
    mStartCurveIndex = startCurve;
    
    mNumCurves = numFadeCurves;
    
    mStartAlpha = startAlpha;
    mEndAlpha = endAlpha;
    
    mStartLuminance = startLuminance;
    mEndLuminance = endLuminance;
    
    mCurveIsPoints = false;
    mPointsAsLines = false;

    for (int i = 0; i < 3; i++)
        mCurveColor[i] = 0;
    
    Reset();
}

void
GraphFader2::Reset()
{
    // x
    mCurvesX.clear();
    mCurvesX.resize(mNumCurves);
    
    // y
    mCurvesY.clear();
    mCurvesY.resize(mNumCurves);
}

void
GraphFader2::SetCurveStyle(BL_FLOAT minY, BL_FLOAT maxY,
                          BL_FLOAT lineWidth,
                          int r, int g, int b,
                          bool curveFill, BL_FLOAT curveFillAlpha)
{
    mCurveIsPoints = false;
    
    mCurveColor[0] = r;
    mCurveColor[1] = g;
    mCurveColor[2] = b;
    
    for (int i = 0; i < mCurvesY.size(); i++)
    {
        int curveNum = mStartCurveIndex + i;
        
        mGraph->SetCurveYScale(curveNum, false, minY, maxY);
        mGraph->SetCurveLineWidth(curveNum, lineWidth);
    
        if ((r >= 0) && (g >= 0) && (b >= 0))
            mGraph->SetCurveColor(curveNum, r, g, b);
    
        if (curveFill)
        {
            mGraph->SetCurveFill(curveNum, true);
            mGraph->SetCurveFillAlpha(curveNum, curveFillAlpha);
        }
    }
}

void
GraphFader2::SetPointStyle(BL_FLOAT minX, BL_FLOAT maxX,
                          BL_FLOAT minY, BL_FLOAT maxY,
                          BL_FLOAT pointSize,
                          int r, int g, int b,
                          bool curveFill, BL_FLOAT curveFillAlpha,
                          bool pointsAsLines)
{
    mCurveIsPoints = true;
    mPointsAsLines = pointsAsLines;
    
    mCurveColor[0] = r;
    mCurveColor[1] = g;
    mCurveColor[2] = b;
    
    for (int i = 0; i < mCurvesY.size(); i++)
    {
        int curveNum = mStartCurveIndex + i;
        
        mGraph->SetCurveXScale(curveNum, false, minX, maxX);
        mGraph->SetCurveYScale(curveNum, false, minY, maxY);
        mGraph->SetCurvePointSize(curveNum, pointSize);
    
        //
        mGraph->SetCurvePointStyle(curveNum, true, pointsAsLines);
        
        if ((r >= 0) && (g >= 0) && (b >= 0))
        {
            mGraph->SetCurveColor(curveNum, r, g, b);
        
            if (curveFill)
            {
                mGraph->SetCurveFill(curveNum, curveFill);
                //mGraph->SetCurveFillAlpha(curveNum, curveFillAlpha);
            }
            
            mGraph->SetCurveFillAlpha(curveNum, curveFillAlpha);
        }
    }
}

GraphFader2::~GraphFader2() {}

void
GraphFader2::AddCurveValues(const WDL_TypedBuf<BL_FLOAT> &xValues,
                            const WDL_TypedBuf<BL_FLOAT> &yValues)
{
    mCurvesY.push_back(yValues);
    if (mCurvesY.size() > mNumCurves)
        mCurvesY.pop_front();
    
    if (xValues.GetSize() > 0 /*!= NULL*/)
        mCurvesX.push_back(xValues);
    if (mCurvesX.size() > mNumCurves)
        mCurvesX.pop_front();
    
    int startI = 0;
    
#if SUPPRESS_LOW_CURVES
    startI = mCurvesY.size()/2;
    //startI = mCurvesY.size() - 1;
#endif
    
    for (int i = startI; i < mCurvesY.size(); i++)
    {
        // Add values
        const WDL_TypedBuf<BL_FLOAT> &curveY = mCurvesY[i];
        if (!mCurveIsPoints)
            mGraph->SetCurveValues2(mStartCurveIndex + i, &curveY);
        else
        {
            if (i < mCurvesX.size())
            {
                const WDL_TypedBuf<BL_FLOAT> &curveX = mCurvesX[i];
                mGraph->SetCurveValuesPoint(mStartCurveIndex + i, curveX, curveY);
            }
        }
        
        // Set alpha
        BL_FLOAT t = 0.0;
        
        if (mCurvesY.size() > 1)
            t = ((BL_FLOAT)i)/(mCurvesY.size() - 1);
        
        // To display the brightest before
        t = 1.0 - t;
        
        // Warning, this is reverse
        BL_FLOAT alpha = t*mEndAlpha + (1.0 - t)*mStartAlpha;
        mGraph->SetCurveAlpha(mStartCurveIndex + i, alpha);
        mGraph->SetCurveFillAlpha(mStartCurveIndex + i, alpha); //
        
        BL_FLOAT luminance = t*mEndLuminance + (1.0 - t)*mStartLuminance;
        int color[3] = { mCurveColor[0], mCurveColor[1], mCurveColor[2] };
        for (int k = 0; k < 3; k++)
        {
            color[k] *= luminance;
            if (color[k] > 255)
                color[k] = 255;
        }
                       
        mGraph->SetCurveColor(mStartCurveIndex + i, color[0], color[1], color[2]);
    }
}

void
GraphFader2::AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                  const WDL_TypedBuf<BL_FLOAT> &yValues,
                                  const WDL_TypedBuf<BL_FLOAT> &colorWeights)
{
    WDL_TypedBuf<BL_FLOAT> xValues0 = xValues;
    WDL_TypedBuf<BL_FLOAT> yValues0 = yValues;
    
    WDL_TypedBuf<BL_FLOAT> colorWeights0 = colorWeights;
    
#if THRESHOLD_WEIGHTS
    xValues0.Resize(0);
    yValues0.Resize(0);
    colorWeights0.Resize(0);
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_FLOAT weight = colorWeights.Get()[i];
        if (weight > THRESHOLD_WEIGHTS_EPS)
        {
            BL_FLOAT x = xValues.Get()[i];
            BL_FLOAT y = yValues.Get()[i];
            
            xValues0.Add(x);
            yValues0.Add(y);
            
            colorWeights0.Add(weight);
        }
    }
#endif
    
    AddCurveValues(xValues0, yValues0);
    
    int curveNum = mStartCurveIndex + mCurvesY.size() - 1;
    mGraph->SetCurveColorWeight(curveNum, colorWeights0);
}

#if 0 // NOT USED
void
GraphFader2::AddCurveValuesWeight2(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                   const WDL_TypedBuf<BL_FLOAT> &yValues,
                                   const WDL_TypedBuf<BL_FLOAT> &colorWeights)
{
    WDL_TypedBuf<BL_FLOAT> xValues0 = xValues;
    WDL_TypedBuf<BL_FLOAT> yValues0 = yValues;
    
    WDL_TypedBuf<BL_FLOAT> colorWeights0 = colorWeights;
    
#if THRESHOLD_WEIGHTS2
    // Low weights located in the center of the circle
    //
    // Keep the eliminated points, and merge them
    // (otherwise it would make a black circle in the circle
    WDL_TypedBuf<BL_FLOAT> xValuesSuppr;
    WDL_TypedBuf<BL_FLOAT> yValuesSuppr;
    WDL_TypedBuf<BL_FLOAT> colorWeightsSuppr;
    
    xValues0.Resize(0);
    yValues0.Resize(0);
    colorWeights0.Resize(0);
    
    // First threshold
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_FLOAT x = xValues.Get()[i];
        BL_FLOAT y = yValues.Get()[i];
        BL_FLOAT weight = colorWeights.Get()[i];
        
        if (weight > THRESHOLD_WEIGHTS_EPS2)
        {
            
            xValues0.Add(x);
            yValues0.Add(y);
            
            colorWeights0.Add(weight);
        }
        else
        {
            xValuesSuppr.Add(x);
            yValuesSuppr.Add(y);
            colorWeightsSuppr.Add(weight);
        }
    }
    
#if 0
    // Then merge the suppressed points
    int numPointsToKeep = ((BL_FLOAT)xValues.GetSize())/DECIM_FACTOR;
    if (numPointsToKeep > xValuesSuppr.GetSize())
        numPointsToKeep = xValuesSuppr.GetSize();
    
    int step = 0;
    if (numPointsToKeep > 0)
        step = xValues.GetSize()/numPointsToKeep;
    
    for (int i = 0; i < numPointsToKeep; i += step)
    {
        BL_FLOAT x = xValues.Get()[i];
        BL_FLOAT y = yValues.Get()[i];
        
        BL_FLOAT weight = 0.0;
        for (int j = 0; j < step; j++)
        {
            BL_FLOAT w = colorWeightsSuppr.Get()[j];
            weight += w;
        }
        
        //if (weight > 1.0)
        //    weight = 1.0;
        
        xValues0.Add(x);
        yValues0.Add(y);
        
        colorWeights0.Add(weight);
    }
#endif
    
#endif
    
    AddCurveValues(xValues0, yValues0);
    
    int curveNum = mStartCurveIndex + mCurvesY.size() - 1;
    mGraph->SetCurveColorWeight(curveNum, colorWeights0);
}
#endif

#endif // IGRAPHICS_NANOVG
