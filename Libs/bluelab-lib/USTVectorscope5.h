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
//  USTVectorscope5.h
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#ifndef __UST__USTVectorscope5__
#define __UST__USTVectorscope5__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include <GraphControl12.h>
#include <GraphCurve5.h>

#include "IPlug_include_in_plug_hdr.h"

#define POLAR_SAMPLE_MODE_ID    0
#define LISSAJOUS_MODE_ID       1
#define FIREWORKS_MODE_ID       2
#define UPMIX_MODE_ID           3

#define VECTORSCOPE_NUM_GRAPHS 4

class USTCircleGraphDrawer;
class USTLissajousGraphDrawer;
class USTFireworks;
class USTUpmixGraphDrawer;
class USTPluginInterface;

// USTVectorscope2: several modes
// USTVectorscope3: polar level object
// USTVectorscope4: use fireworks view instead of polar levels
// USTVectorscope4: optimize: bufferize the polar samples after the computation
// (instead of bufferizing raw samples)
//
class USTVectorscope5 : public GraphCustomControl
{
public:
    enum Mode
    {
      POLAR_SAMPLE = 0,
      LISSAJOUS,
      FIREWORKS,
      UPMIX
    };
    
    USTVectorscope5(USTPluginInterface *plug, BL_FLOAT sampleRate);
    
    virtual ~USTVectorscope5();

    void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    void OnMouseUp(float x, float y, const IMouseMod &mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod &mod) override;
    
    void Reset(BL_FLOAT sampleRate);

    void SetMode(Mode mode);
    
    void SetEnabled(bool flag);
    
    //int GetNumCurves(int graphNum);
    //int GetNumPoints(int graphNum);

    void SetGraphs(GraphControl12 *graph0,
                   GraphControl12 *graph1,
                   GraphControl12 *graph2,
                   GraphControl12 *graph3);

    void SetGraphsNull();
    
    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);

    //
    USTUpmixGraphDrawer *GetUpmixGraphDrawer();
    
protected:

    void SetCurveStyle(GraphCurve5 *curve,
                       BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX,
                       BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                       bool pointFlag,
                       BL_GUI_FLOAT strokeSize,
                       bool bevelFlag,
                       int r, int g, int b,
                       bool curveFill, BL_GUI_FLOAT curveFillAlpha,
                       bool pointsAsLines,
                       bool pointOverlay,
                       bool linesPolarFlag);

    void CreateCurves();

    //
    USTPluginInterface *mPlug;
    
    GraphControl12 *mGraphs[VECTORSCOPE_NUM_GRAPHS];

    // Polar samples
    GraphCurve5 *mGraph0Curve;
    // Lissajous
    GraphCurve5 *mGraph1Curve;
    // Fireworks
    GraphCurve5 *mGraph2Curves[2];
    // No curve for "upmix" graph
    
    USTCircleGraphDrawer *mCircleDrawerPolarSamples;
    USTCircleGraphDrawer *mCircleDrawerFireworks;
    USTLissajousGraphDrawer *mLissajousDrawer;
    //
    USTUpmixGraphDrawer *mUpmixDrawer;
    
    // PolarSample, Lissajous, Flame
    WDL_TypedBuf<BL_GUI_FLOAT> mPolarSampleSamples[2];
    WDL_TypedBuf<BL_GUI_FLOAT> mLissajousSamples[2];
    WDL_TypedBuf<BL_GUI_FLOAT> mFireworksInSamples[2];
    
    bool mIsEnabled;
    
    Mode mMode;
    
    USTFireworks *mFireworks;
    
private:
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuffersInsertPoints;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mTmpPolarSamples[2];
    WDL_TypedBuf<BL_GUI_FLOAT> mTmpPolarSamplesResult[2];
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__UST__USTVectorscope5__) */
