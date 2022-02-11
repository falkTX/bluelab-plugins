//
//  GraphFader3.cpp
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#include "GraphControl10.h"
#include "GraphFader3.h"

// Hard code for the moment
#define MIN_POINT_SIZE 8.0 //4.0
#define MAX_POINT_SIZE 32.0 //128.0

GraphFader3::GraphFader3(GraphControl10 *graphControl,
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
GraphFader3::SetCurveStyle(double minY, double maxY,
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
GraphFader3::SetPointStyle(double minX, double maxX,
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

GraphFader3::~GraphFader3() {}

void
GraphFader3::AddCurveValues(const WDL_TypedBuf<double> *xValues,
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
        
#if 0
        // Warning, this is reverse
        double alpha = t*mEndAlpha + (1.0 - t)*mStartAlpha;
        mGraph->SetCurveAlpha(mStartCurveIndex + i, alpha);
        mGraph->SetCurveFillAlpha(mStartCurveIndex + i, alpha); //
        
        double luminance = t*mEndLuminance + (1.0 - t)*mStartLuminance;
#endif
        
        // Set point size
        double pointSize = t*MAX_POINT_SIZE + (1.0 - t)*MIN_POINT_SIZE;
        mGraph->SetCurvePointSize(mStartCurveIndex + i, pointSize);
        
        // TEST
        //t = 1.0 - t;
        
        double luminance = MIN_POINT_SIZE/pointSize;
        //luminance = pow(luminance, 0.125);
        
        luminance = pow(luminance, 0.5);
        
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
        
#define ALPHA 0.05 //0.1
        double alpha = ALPHA;
        mGraph->SetCurveAlpha(mStartCurveIndex + i, alpha);
        mGraph->SetCurveFillAlpha(mStartCurveIndex + i, alpha);
    }
}

// WARNING: the weights are now ignored !

void
GraphFader3::AddCurveValuesWeight(const WDL_TypedBuf<double> *xValues,
                                 const WDL_TypedBuf<double> &yValues,
                                 const WDL_TypedBuf<double> &colorWeights)
{
    AddCurveValues(xValues, yValues);
    
#if 0 // TEST
    int curveNum = mStartCurveIndex + mCurvesY.size() - 1;
    mGraph->SetCurveColorWeight(curveNum, colorWeights);
#endif
}
