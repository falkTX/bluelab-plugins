//
//  DebugGraph.cpp
//  Spatializer
//
//  Created by Pan on 06/01/18.
//
//

#include "BLSpectrogram.h"

#include "DebugGraph.h"

#if USE_GRAPH_OGL
#include "GraphControl6.h"

DebugGraph *DebugGraph::mInstance = NULL;

void
DebugGraph::Create(IPlug *plug, IGraphics *graphics, int param,
                   GUIHelper4 *helper,
                   int numCurves, int numPoints)
{
    if (mInstance == NULL)
        mInstance = new DebugGraph();
    
    mInstance->mGraph = helper->CreateGraph6(plug, graphics, param,
                                             GUI_WIDTH - DEBUG_GRAPH_WIDTH, 0,
                                             DEBUG_GRAPH_WIDTH, DEBUG_GRAPH_HEIGHT,
                                             numCurves, numPoints);
}

void
DebugGraph::SetCurveValues(const WDL_TypedBuf<double> &values,
                           int curveNum, double minY, double maxY,
                           double lineWidth, int r, int g, int b,
                           bool curveFill, double curveFillAlpha)
{
    mInstance->SetCurveStyle(curveNum, minY, maxY, lineWidth,
                             r, g, b, curveFill, curveFillAlpha);
    
    mInstance->mGraph->SetCurveValues2(curveNum, &values);
}

void
DebugGraph::PushCurveValue(double value,
                           int curveNum, double minY, double maxY,
                           double lineWidth, int r, int g, int b,
                           bool curveFill, double curveFillAlpha)
{
    mInstance->SetCurveStyle(curveNum, minY, maxY, lineWidth,
                             r, g, b, curveFill, curveFillAlpha);
    
    mInstance->mGraph->PushCurveValue(curveNum, value);
}

void
DebugGraph::SetCurveSingleValue(double value,
                                int curveNum, double minY, double maxY,
                                double lineWidth, int r, int g, int b,
                                bool curveFill, double curveFillAlpha)
{
    mInstance->SetCurveStyle(curveNum, minY, maxY, lineWidth,
                             r, g, b, curveFill, curveFillAlpha);
    
    mInstance->mGraph->SetCurveSingleValueH(curveNum, true);
    mInstance->mGraph->SetCurveValue(curveNum, 0.0, value);
}

// Spectrogram
void
DebugGraph::CreateSpectrogram(int height, int maxCols)
{
    mInstance->mSpectrogram = new BLSpectrogram(height, maxCols);
}

void
DebugGraph::SetSpectrogramParameters(double magnMult, double phaseMult,
                                     bool dispMagns, bool dispPhasesX, bool dispPhasesY,
                                     double zoomX, double zoomY)
{
    mInstance->mSpectrogram->SetMultipliers(magnMult, phaseMult);

    mInstance->mSpectrogram->SetDisplayMagns(dispMagns);
    mInstance->mSpectrogram->SetDisplayPhasesX(dispPhasesX);
    mInstance->mSpectrogram->SetDisplayPhasesY(dispPhasesY);
    
    mInstance->mGraph->SetSpectrogramZoomX(zoomX);
    mInstance->mGraph->SetSpectrogramZoomY(zoomY);
    
}

void
DebugGraph::ShowSpectrogram(bool flag)
{
    
}

void
DebugGraph::AddSpectrogramLine(const WDL_TypedBuf<double> &magns,
                               const WDL_TypedBuf<double> &phases)
{
    
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
                          double minY, double maxY,
                          double lineWidth,
                          int r, int g, int b,
                          bool curveFill, double curveFillAlpha)
{
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

#endif