//
//  DebugGraph.h
//  Spatializer
//
//  Created by Pan on 06/01/18.
//
//

#ifndef __Spatializer__DebugGraph__
#define __Spatializer__DebugGraph__

// NOTE (!!!) : If we have severa instances of the plugin in the project,
// the DebugGraph won't display anything

#define DEBUG_GRAPH_WIDTH 400
#define DEBUG_GRAPH_HEIGHT 400

#include "resource.h"

#if USE_GRAPH_OGL

#include "GUIHelper4.h"

class BLSpectrogram;

class DebugGraph
{
public:
    static void Create(IPlug *plug, IGraphics *graphics, int param,
                       GUIHelper4 *helper,
                       int numCurves, int numPoints);
    
    static void SetCurveValues(const WDL_TypedBuf<double> &values,
                               int curveNum, double minY, double maxY,
                               double lineWidth, int r, int g, int b,
                               bool curveFill = false, double curveFillAlpha = 1.0);
    
    static void PushCurveValue(double value,
                               int curveNum, double minY, double maxY,
                               double lineWidth, int r, int g, int b,
                               bool curveFill = false, double curveFillAlpha = 1.0);
    
    
    static void SetCurveSingleValue(double values,
                                    int curveNum, double minY, double maxY,
                                    double lineWidth, int r, int g, int b,
                                    bool curveFill = false, double curveFillAlpha = 1.0);
    
    static void CreateSpectrogram(int height, int maxCols = -1);
    
    static void SetSpectrogramParameters(double magnMult, double phaseMult,
                                         bool dispMagns, bool dispPhasesX, bool dispPhasesY,
                                         double zoomX, double zoomY);
    
    static void ShowSpectrogram(bool flag);
    
    static void AddSpectrogramLine(const WDL_TypedBuf<double> &magns,
                                   const WDL_TypedBuf<double> &phases);
    
protected:
    DebugGraph();
    
    virtual ~DebugGraph();
    
    void SetCurveStyle(int curveNum,
                       double minY, double maxY,
                       double lineWidth,
                       int r, int g, int b,
                       bool curveFill, double curveFillAlpha);
    
    GraphControl6 *mGraph;
    
    BLSpectrogram *mSpectrogram;
    
private:
    static DebugGraph *mInstance;
};

#endif

#endif /* defined(__Spatializer__DebugGraph__) */
