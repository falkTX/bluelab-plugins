//
//  DebugGraph.cpp
//  Spatializer
//
//  Created by Pan on 06/01/18.
//
//

#include "BLSpectrogram4.h"

#include "DebugGraph.h"

#if USE_GRAPH_OGL
#include "GraphControl8.h"

DebugGraph *DebugGraph::mInstance = NULL;

void
DebugGraph::Create(IPlug *plug, IGraphics *graphics, int param,
                   GUIHelper6 *helper,
                   int numCurves, int numPoints)
{
    if (mInstance == NULL)
        mInstance = new DebugGraph();
    
    mInstance->mGraph = helper->CreateGraph8(plug, graphics, param,
                                             GUI_WIDTH - DEBUG_GRAPH_WIDTH, 0,
                                             DEBUG_GRAPH_WIDTH, DEBUG_GRAPH_HEIGHT,
                                             numCurves, numPoints);
}

GraphControl8 *
DebugGraph::GetGraph()
{
    return mInstance->mGraph;
}

void
DebugGraph::SetCurveValues(const WDL_TypedBuf<BL_FLOAT> &values,
                           int curveNum, BL_FLOAT minY, BL_FLOAT maxY,
                           BL_FLOAT lineWidth, int r, int g, int b,
                           bool curveFill, BL_FLOAT curveFillAlpha)
{
    if (mInstance == NULL)
        return;
    
    mInstance->SetCurveStyle(curveNum, minY, maxY, lineWidth,
                             r, g, b, curveFill, curveFillAlpha);
    
    mInstance->mGraph->SetCurveValues2(curveNum, &values);
}

void
DebugGraph::PushCurveValue(BL_FLOAT value,
                           int curveNum, BL_FLOAT minY, BL_FLOAT maxY,
                           BL_FLOAT lineWidth, int r, int g, int b,
                           bool curveFill, BL_FLOAT curveFillAlpha)
{
    if (mInstance == NULL)
        return;
    
    mInstance->SetCurveStyle(curveNum, minY, maxY, lineWidth,
                             r, g, b, curveFill, curveFillAlpha);
    
    mInstance->mGraph->PushCurveValue(curveNum, value);
}

void
DebugGraph::SetCurveSingleValue(BL_FLOAT value,
                                int curveNum, BL_FLOAT minY, BL_FLOAT maxY,
                                BL_FLOAT lineWidth, int r, int g, int b,
                                bool curveFill, BL_FLOAT curveFillAlpha)
{
    if (mInstance == NULL)
        return;
    
    mInstance->SetCurveStyle(curveNum, minY, maxY, lineWidth,
                             r, g, b, curveFill, curveFillAlpha);
    
    mInstance->mGraph->SetCurveSingleValueH(curveNum, true);
    mInstance->mGraph->SetCurveValue(curveNum, 0.0, value);
}

void
DebugGraph::SetPointValues(const WDL_TypedBuf<BL_FLOAT> &xValues,
                           const WDL_TypedBuf<BL_FLOAT> &yValues,
                           int curveNum,
                           BL_FLOAT minX, BL_FLOAT maxX,
                           BL_FLOAT minY, BL_FLOAT maxY,
                           BL_FLOAT pointSize,
                           int r, int g, int b,
                           bool fillFlag, BL_FLOAT fillAlpha)
{
    if (mInstance == NULL)
        return;
    
    mInstance->SetPointStyle(curveNum, minX, maxX, minY, maxY, pointSize,
                             r, g, b, fillFlag, fillAlpha);
    
    mInstance->mGraph->SetCurveValuesPoint(curveNum, xValues, yValues);
}

void
DebugGraph::SetPointValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                 const WDL_TypedBuf<BL_FLOAT> &yValues,
                                 const WDL_TypedBuf<BL_FLOAT> &weights,
                                 int curveNum,
                                 BL_FLOAT minX, BL_FLOAT maxX,
                                 BL_FLOAT minY, BL_FLOAT maxY,
                                 BL_FLOAT pointSize,
                                 int r, int g, int b,
                                 bool fillFlag, BL_FLOAT fillAlpha)
{
    if (mInstance == NULL)
        return;
    
    mInstance->SetPointStyle(curveNum, minX, maxX, minY, maxY, pointSize,
                             r, g, b, fillFlag, fillAlpha);
    
    mInstance->mGraph->SetCurveValuesPointWeight(curveNum,
                                                 xValues,
                                                 yValues,
                                                 weights);
}

// Spectrogram
void
DebugGraph::CreateSpectrogram(int height, int maxCols)
{
    if (mInstance == NULL)
        return;
    
    mInstance->mSpectrogram = new BLSpectrogram4(mSampleRate, height, maxCols);
    
    // DEBUG
    mInstance->mSpectrogram->SetRange(0.97);
    mInstance->mSpectrogram->SetContrast(0.43);
    
    mInstance->mGraph->SetSpectrogram(mInstance->mSpectrogram,
                                      0.0, 0.0, 1.0, 1.0);
}

void
DebugGraph::SetSpectrogramParameters(BL_FLOAT magnMult, BL_FLOAT phaseMult,
                                     bool yLogScale,
                                     bool dispMagns, bool dispPhasesX, bool dispPhasesY,
                                     BL_FLOAT zoomX, BL_FLOAT zoomY)
{
    if (mInstance->mSpectrogram == NULL)
        return;
    
    // TODO: remove this (and above)
   // mInstance->mSpectrogram->SetMultipliers(magnMult, phaseMult);

    mInstance->mSpectrogram->SetYLogScale(yLogScale);
    
    mInstance->mSpectrogram->SetDisplayMagns(dispMagns);
    mInstance->mSpectrogram->SetDisplayPhasesX(dispPhasesX);
    mInstance->mSpectrogram->SetDisplayPhasesY(dispPhasesY);
    
    mInstance->mGraph->SetSpectrogramZoomX(zoomX);
    mInstance->mGraph->SetSpectrogramZoomY(zoomY);
}

void
DebugGraph::SetSpectrogramDisplay(bool dispMagns, bool dispPhasesX, bool dispPhasesY)
{
    if (mInstance == NULL)
        return;
    
    mInstance->mSpectrogram->SetDisplayMagns(dispMagns);
    mInstance->mSpectrogram->SetDisplayPhasesX(dispPhasesX);
    mInstance->mSpectrogram->SetDisplayPhasesY(dispPhasesY);
}

void
DebugGraph::SetSpectrogramZoom(BL_FLOAT zoomX, BL_FLOAT zoomY)
{
    if (mInstance == NULL)
        return;
    
    mInstance->mGraph->SetSpectrogramZoomX(zoomX);
    mInstance->mGraph->SetSpectrogramZoomY(zoomY);
}

void
DebugGraph::ShowSpectrogram(bool flag)
{
    if (mInstance == NULL)
        return;
    
    mInstance->mGraph->ShowSpectrogram(flag);
}

void
DebugGraph::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                               const WDL_TypedBuf<BL_FLOAT> &phases)
{
    if (mInstance == NULL)
        return;
    
    if (mInstance->mSpectrogram == NULL)
        return;
    
    mInstance->mSpectrogram->AddLine(magns, phases);
}

void
DebugGraph::UpdateSpectrogram(bool updateData)
{
    if (mInstance == NULL)
        return;
    
    mInstance->mGraph->UpdateSpectrogram(updateData);
}

//
DebugGraph::DebugGraph()
{
    mGraph = NULL;
    mSpectrogram = NULL;
}

DebugGraph::~DebugGraph() {}

void
DebugGraph::SetCurveStyle(int curveNum,
                          BL_FLOAT minY, BL_FLOAT maxY,
                          BL_FLOAT lineWidth,
                          int r, int g, int b,
                          bool curveFill, BL_FLOAT curveFillAlpha)
{
    if (mInstance == NULL)
        return;
    
    mInstance->mGraph->SetCurveYScale(curveNum, false, minY, maxY);
    mInstance->mGraph->SetCurveLineWidth(curveNum, lineWidth);
    
    if ((r >= 0) && (g >= 0) && (b >= 0))
        mInstance->mGraph->SetCurveColor(curveNum, r, g, b);
    
    if (curveFill)
    {
        mInstance->mGraph->SetCurveFill(curveNum, true);
        mInstance->mGraph->SetCurveFillAlpha(curveNum, curveFillAlpha);
    }
}

void
DebugGraph::SetPointStyle(int curveNum,
                          BL_FLOAT minX, BL_FLOAT maxX,
                          BL_FLOAT minY, BL_FLOAT maxY,
                          BL_FLOAT pointSize,
                          int r, int g, int b,
                          bool curveFill, BL_FLOAT curveFillAlpha)
{
    if (mInstance == NULL)
        return;
    
    mInstance->mGraph->SetCurveXScale(curveNum, false, minY, maxY);
    mInstance->mGraph->SetCurveYScale(curveNum, false, minY, maxY);
    mInstance->mGraph->SetCurvePointSize(curveNum, pointSize);
    
    if ((r >= 0) && (g >= 0) && (b >= 0))
    {
        mInstance->mGraph->SetCurveColor(curveNum, r, g, b);
        
        if (curveFill)
        {
            mInstance->mGraph->SetCurveFill(curveNum, curveFill);
            mInstance->mGraph->SetCurveFillAlpha(curveNum, curveFillAlpha);
        }
    }
}


#endif
