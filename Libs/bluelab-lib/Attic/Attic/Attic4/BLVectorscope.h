//
//  BLVectorscope.h
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#ifndef __UST__BLVectorscope__
#define __UST__BLVectorscope__

#include <vector>
using namespace std;

#include <GraphControl11.h>

#include "IPlug_include_in_plug_hdr.h"

// Modes
#define NUM_MODES 5

#define POLAR_SAMPLE_MODE_ID    0
#define LISSAJOUS_MODE_ID       1
#define FIREWORKS_MODE_ID       2
#define UPMIX_MODE_ID           3
#define SOURCE_MODE_ID          4

class BLCircleGraphDrawer;
class BLLissajousGraphDrawer;
class BLFireworks;
class BLUpmixGraphDrawer;
class StereoWidthGraphDrawer3;
class SourceComputer;

class BLVectorscopePlug
{
public:
    virtual void VectorscopeUpdatePanCB(BL_FLOAT newPan) = 0;
    virtual void VectorscopeUpdateDPanCB(BL_FLOAT dpan) = 0;
    virtual void VectorscopeUpdateDWidthCB(BL_FLOAT dwidth) = 0;
    virtual void VectorscopeUpdateDepthCB(BL_FLOAT newDepth) = 0;
    virtual void VectorscopeUpdateDDepthCB(BL_FLOAT ddepth) = 0;
};

// USTVectorscope2: several modes
// USTVectorscope3: polar level object
// USTVectorscope4: use fireworks view instead of polar levels
// BLVectorscope: from USTVectorscope4
//
class BLVectorscope : public GraphCustomControl
{
public:
    enum Mode
    {
      POLAR_SAMPLE = 0,
      LISSAJOUS,
      FIREWORKS,
      UPMIX,
      SOURCE
    };
    
    BLVectorscope(BLVectorscopePlug *plug,
                  BL_FLOAT sampleRate);
    
    virtual ~BLVectorscope();

    void Reset(BL_FLOAT sampleRate);
    void Reset();
    
    void SetMode(Mode mode);
    
    void OnMouseDown(int x, int y, IMouseMod* pMod);
    void OnMouseUp(int x, int y, IMouseMod* pMod);
    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    
    int GetNumCurves(int graphNum);
    int GetNumPoints(int graphNum);

    void SetGraphs(GraphControl11 *graph0,
                   GraphControl11 *graph1,
                   GraphControl11 *graph2,
                   GraphControl11 *graph3,
                   GraphControl11 *graph4);

    void AddSamples(vector<WDL_TypedBuf<BL_FLOAT> > samples);

    //
    BLUpmixGraphDrawer *GetUpmixGraphDrawer();
    
protected:
    void SetCurveStyle(GraphControl11 *graph,
                       int curveNum,
                       BL_FLOAT minX, BL_FLOAT maxX,
                       BL_FLOAT minY, BL_FLOAT maxY,
                       bool pointFlag,
                       BL_FLOAT strokeSize,
                       bool bevelFlag,
                       int r, int g, int b,
                       bool curveFill, BL_FLOAT curveFillAlpha,
                       bool pointsAsLines,
                       bool pointOverlay,
                       bool linesPolarFlag);

    //
    Mode mMode;
    
    BLVectorscopePlug *mPlug;
    
    GraphControl11 *mGraphs[NUM_MODES];
    
    BLCircleGraphDrawer *mCircleDrawerPolarSamples;
    BLCircleGraphDrawer *mCircleDrawerFireworks;
    BLLissajousGraphDrawer *mLissajousDrawer;
    BLUpmixGraphDrawer *mUpmixDrawer;
    StereoWidthGraphDrawer3 *mCircleDrawerSource;
    
    BLFireworks *mFireworks;
    SourceComputer *mSourceComputer;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
};

#endif /* defined(__UST__BLVectorscope__) */
