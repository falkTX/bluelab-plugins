//
//  GraphFader3.cpp
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#include <cmath>

#include "GraphControl11.h"
#include "GraphFader3.h"

// Hard code for the moment
#define MIN_POINT_SIZE 8.0 //4.0
#define MAX_POINT_SIZE 32.0 //128.0

#define THRESHOLD_WEIGHTS 0

GraphFader3::GraphFader3(GraphControl11 *graphControl,
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
    
    Reset();
}

void
GraphFader3::Reset()
{
    // x
    mCurvesX.clear();
    mCurvesX.resize(mNumCurves);
    
    // y
    mCurvesY.clear();
    mCurvesY.resize(mNumCurves);
}

void
GraphFader3::SetCurveStyle(BL_FLOAT minY, BL_FLOAT maxY,
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
GraphFader3::SetPointStyle(BL_FLOAT minX, BL_FLOAT maxX,
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

GraphFader3::~GraphFader3() {}

void
GraphFader3::AddCurveValues(const WDL_TypedBuf<BL_FLOAT> &xValues,
                            const WDL_TypedBuf<BL_FLOAT> &yValues)
{
    mCurvesY.push_back(yValues);
    if (mCurvesY.size() > mNumCurves)
        mCurvesY.pop_front();
    
    if (xValues.GetSize() > 0)
        mCurvesX.push_back(xValues);
    if (mCurvesX.size() > mNumCurves)
        mCurvesX.pop_front();
    
    for (int i = 0; i < mCurvesY.size(); i++)
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
        
#if 0
        // Warning, this is reverse
        BL_FLOAT alpha = t*mEndAlpha + (1.0 - t)*mStartAlpha;
        mGraph->SetCurveAlpha(mStartCurveIndex + i, alpha);
        mGraph->SetCurveFillAlpha(mStartCurveIndex + i, alpha); //
        
        BL_FLOAT luminance = t*mEndLuminance + (1.0 - t)*mStartLuminance;
#endif
        
        // Set point size
        BL_FLOAT pointSize = t*MAX_POINT_SIZE + (1.0 - t)*MIN_POINT_SIZE;
        mGraph->SetCurvePointSize(mStartCurveIndex + i, pointSize);
        
        BL_FLOAT luminance = MIN_POINT_SIZE/pointSize;
        //luminance = std::pow(luminance, 0.125);
        
        luminance = std::pow(luminance, (BL_FLOAT)0.5);
        
        // To enable saturation
        luminance *= 2.0;
        
        int color[3] = { mCurveColor[0], mCurveColor[1], mCurveColor[2] };
        for (int k = 0; k < 3; k++)
        {
            color[k] *= luminance;
            if (color[k] > 255)
                color[k] = 255;
        }

        mGraph->SetCurveColor(mStartCurveIndex + i, color[0], color[1], color[2]);
        
#define ALPHA 0.05 //0.05 //0.1
        BL_FLOAT alpha = ALPHA;
        mGraph->SetCurveAlpha(mStartCurveIndex + i, alpha);
        mGraph->SetCurveFillAlpha(mStartCurveIndex + i, alpha);}
	
}

// WARNING: the weights are now ignored !

void
GraphFader3::AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                  const WDL_TypedBuf<BL_FLOAT> &yValues,
                                  const WDL_TypedBuf<BL_FLOAT> &colorWeights)
{
    WDL_TypedBuf<BL_FLOAT> xValues0 = xValues;
    WDL_TypedBuf<BL_FLOAT> yValues0 = yValues;
    
#if THRESHOLD_WEIGHTS
#define EPS 2.0
    xValues0.Resize(0);
    yValues0.Resize(0);
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_FLOAT weight = colorWeights.Get()[i];
        if (weight > EPS)
        {
            BL_FLOAT x = xValues.Get()[i];
            BL_FLOAT y = yValues.Get()[i];
            
            xValues0.Add(x);
            yValues0.Add(y);
        }
    }
#endif
    
    AddCurveValues(xValues0, yValues0);
    
    // ORIGIN: 1
#if 0 // TEST => rendering more cloudy with 0 (good), but colors more homogeneous
    int curveNum = mStartCurveIndex + mCurvesY.size() - 1;
    mGraph->SetCurveColorWeight(curveNum, colorWeights);
#endif
}
