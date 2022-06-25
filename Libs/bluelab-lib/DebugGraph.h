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

#if 0
#define DEBUG_GRAPH_WIDTH 400
#define DEBUG_GRAPH_HEIGHT 400
#endif

#if 1 // For PitchShift tests
#define DEBUG_GRAPH_WIDTH 512
#define DEBUG_GRAPH_HEIGHT 512
#endif

#include "resource.h"

#if USE_GRAPH_OGL

#include "GUIHelper6.h"

class BLSpectrogram4;
class GraphControl8;

class DebugGraph
{
public:
    static void Create(IPlug *plug, IGraphics *graphics, int param,
                       GUIHelper6 *helper,
                       int numCurves, int numPoints);
    
    static GraphControl8 *GetGraph();
    
    static void SetCurveValues(const WDL_TypedBuf<BL_FLOAT> &values,
                               int curveNum, BL_FLOAT minY, BL_FLOAT maxY,
                               BL_FLOAT lineWidth, int r, int g, int b,
                               bool curveFill = false, BL_FLOAT curveFillAlpha = 1.0);
    
    static void PushCurveValue(BL_FLOAT value,
                               int curveNum, BL_FLOAT minY, BL_FLOAT maxY,
                               BL_FLOAT lineWidth, int r, int g, int b,
                               bool curveFill = false, BL_FLOAT curveFillAlpha = 1.0);
    
    
    static void SetCurveSingleValue(BL_FLOAT values,
                                    int curveNum, BL_FLOAT minY, BL_FLOAT maxY,
                                    BL_FLOAT lineWidth, int r, int g, int b,
                                    bool curveFill = false, BL_FLOAT curveFillAlpha = 1.0);
    
    static void SetPointValues(const WDL_TypedBuf<BL_FLOAT> &xValues,
                               const WDL_TypedBuf<BL_FLOAT> &yValues,
                               int curveNum,
                               BL_FLOAT minX, BL_FLOAT maxX,
                               BL_FLOAT minY, BL_FLOAT maxY,
                               BL_FLOAT pointSize,
                               int r, int g, int b,
                               bool fillFlag, BL_FLOAT fillAlpha);
    
    static void SetPointValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                     const WDL_TypedBuf<BL_FLOAT> &yValues,
                                     const WDL_TypedBuf<BL_FLOAT> &weights,
                                     int curveNum,
                                     BL_FLOAT minX, BL_FLOAT maxX,
                                     BL_FLOAT minY, BL_FLOAT maxY,
                                     BL_FLOAT pointSize,
                                     int r, int g, int b,
                                     bool fillFlag, BL_FLOAT fillAlpha);
    
    static void CreateSpectrogram(int height, int maxCols = -1);
    
    static void SetSpectrogramParameters(BL_FLOAT magnMult, BL_FLOAT phaseMult,
                                         bool yLogScale,
                                         bool dispMagns, bool dispPhasesX, bool dispPhasesY,
                                         BL_FLOAT zoomX, BL_FLOAT zoomY);
    
    static void SetSpectrogramDisplay(bool dispMagns, bool dispPhasesX, bool dispPhasesY);
    
    static void SetSpectrogramZoom(BL_FLOAT zoomX, BL_FLOAT zoomY);
    
    static void ShowSpectrogram(bool flag);
    
    static void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   const WDL_TypedBuf<BL_FLOAT> &phases);
    
    static void UpdateSpectrogram(bool updateData = true);
    
protected:
    DebugGraph();
    
    virtual ~DebugGraph();
    
    void SetCurveStyle(int curveNum,
                       BL_FLOAT minY, BL_FLOAT maxY,
                       BL_FLOAT lineWidth,
                       int r, int g, int b,
                       bool curveFill, BL_FLOAT curveFillAlpha);
    
    void SetPointStyle(int curveNum,
                       BL_FLOAT minX, BL_FLOAT maxX,
                       BL_FLOAT minY, BL_FLOAT maxY,
                       BL_FLOAT pointSize,
                       int r, int g, int b,
                       bool curveFill, BL_FLOAT curveFillAlpha);

    
    GraphControl8 *mGraph;
    
    BLSpectrogram4 *mSpectrogram;
    
private:
    static DebugGraph *mInstance;
};

#endif

#endif /* defined(__Spatializer__DebugGraph__) */
