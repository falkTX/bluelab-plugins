//
//  GraphFader.cpp
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#include "GraphControl8.h"
#include "GraphFader.h"

GraphFader::GraphFader(GraphControl8 *graphControl,
                       int startCurve, int numFadeCurves,
                       double startAlpha, double endAlpha,
                       double startLuminance, double endLuminance)
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
GraphFader::Reset()
{
    // x
    mCurvesX.clear();
    mCurvesX.resize(mNumCurves);
    
    // y
    mCurvesY.clear();
    mCurvesY.resize(mNumCurves);
}

void
GraphFader::SetCurveStyle(double minY, double maxY,
                          double lineWidth,
                          int r, int g, int b,
                          bool curveFill, double curveFillAlpha)
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
GraphFader::SetPointStyle(double minX, double maxX,
                          double minY, double maxY,
                          double pointSize,
                          int r, int g, int b,
                          bool curveFill, double curveFillAlpha,
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

GraphFader::~GraphFader() {}

void
GraphFader::AddCurveValues(const WDL_TypedBuf<double> *xValues,
                           const WDL_TypedBuf<double> &yValues)
{
    mCurvesY.push_back(yValues);
    if (mCurvesY.size() > mNumCurves)
        mCurvesY.pop_front();
    
    if (xValues != NULL)
        mCurvesX.push_back(*xValues);
    if (mCurvesX.size() > mNumCurves)
        mCurvesX.pop_front();
    
    for (int i = 0; i < mCurvesY.size(); i++)
    {
        // Add values
        const WDL_TypedBuf<double> &curveY = mCurvesY[i];
        if (!mCurveIsPoints)
            mGraph->SetCurveValues2(mStartCurveIndex + i, &curveY);
        else
        {
            const WDL_TypedBuf<double> &curveX = mCurvesX[i];
            mGraph->SetCurveValuesPoint(mStartCurveIndex + i, curveX, curveY);
        }
        
            // Set alpha
        double t = 0.0;
        
        if (mCurvesY.size() > 1)
            t = ((double)i)/(mCurvesY.size() - 1);
        
        // To display the brightest before
        t = 1.0 - t;
        
        // Warning, this is reverse
        double alpha = t*mEndAlpha + (1.0 - t)*mStartAlpha;
        mGraph->SetCurveAlpha(mStartCurveIndex + i, alpha);
        mGraph->SetCurveFillAlpha(mStartCurveIndex + i, alpha); //
        
        double luminance = t*mEndLuminance + (1.0 - t)*mStartLuminance;
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
GraphFader::AddCurveValuesWeight(const WDL_TypedBuf<double> *xValues,
                                 const WDL_TypedBuf<double> &yValues,
                                 const WDL_TypedBuf<double> &colorWeights)
{
    AddCurveValues(xValues, yValues);
    
    int curveNum = mStartCurveIndex + mCurvesY.size() - 1;
    mGraph->SetCurveColorWeight(curveNum, colorWeights);
}
