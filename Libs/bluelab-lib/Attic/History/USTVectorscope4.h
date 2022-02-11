//
//  USTVectorscope4.h
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#ifndef __UST__USTVectorscope4__
#define __UST__USTVectorscope4__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include <GraphControl11.h>

#include "IPlug_include_in_plug_hdr.h"

#define POLAR_SAMPLE_MODE_ID    0
#define LISSAJOUS_MODE_ID       1
#define FIREWORKS_MODE_ID       2
#define UPMIX_MODE_ID           3

#define VECTORSCOPE_NUM_GRAPHS 4
#define VECTORSCOPE_NUM_GRAPH_SAMPLES 3

class GraphControl11;
class USTCircleGraphDrawer;
class USTLissajousGraphDrawer;
class USTFireworks;
class USTUpmixGraphDrawer;
class USTPluginInterface;

// USTVectorscope2: several modes
// USTVectorscope3: polar level object
// USTVectorscope4: use fireworks view instead of polar levels
class USTVectorscope4 : public GraphCustomControl
{
public:
    enum Mode
    {
      POLAR_SAMPLE = 0,
      LISSAJOUS,
      FIREWORKS,
      UPMIX
    };
    
    USTVectorscope4(USTPluginInterface *plug, BL_FLOAT sampleRate);
    
    virtual ~USTVectorscope4();

    void OnMouseDown(float x, float y, const IMouseMod &mod);
    void OnMouseUp(float x, float y, const IMouseMod &mod);
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod &mod);
    
    void Reset(BL_FLOAT sampleRate);

    void SetMode(Mode mode);
    
    void SetEnabled(bool flag);
    
    int GetNumCurves(int graphNum);

    int GetNumPoints(int graphNum);

    void SetGraphs(GraphControl11 *graph0,
                   GraphControl11 *graph1,
                   GraphControl11 *graph2,
                   GraphControl11 *graph3);

    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);

    //
    USTUpmixGraphDrawer *GetUpmixGraphDrawer();
    
protected:

    void SetCurveStyle(GraphControl11 *graph,
                       int curveNum,
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

    USTPluginInterface *mPlug;
    
    GraphControl11 *mGraphs[VECTORSCOPE_NUM_GRAPHS];
    
    USTCircleGraphDrawer *mCircleDrawerPolarSamples;
    USTCircleGraphDrawer *mCircleDrawerFireworks;
    USTLissajousGraphDrawer *mLissajousDrawer;
    //
    USTUpmixGraphDrawer *mUpmixDrawer;
    
    // PolarSample, Lissajous, Flame (3)
    WDL_TypedBuf<BL_GUI_FLOAT> mSamples[VECTORSCOPE_NUM_GRAPH_SAMPLES][2];
    
    bool mIsEnabled;
    
    Mode mMode;
    
    USTFireworks *mFireworks;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__UST__USTVectorscope4__) */
