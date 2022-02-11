//
//  USTVectorscope3.h
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#ifndef __UST__USTVectorscope3__
#define __UST__USTVectorscope3__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class USTCircleGraphDrawer;
class USTLissajousGraphDrawer;
class USTPolarLevel;

// USTVectorscope2: several modes
// USTVectorscope3: polar level object

class USTVectorscope3
{
public:
    enum Mode
    {
      POLAR_SAMPLE = 0,
      POLAR_LEVEL,
      LISSAJOUS
    };
    
    USTVectorscope3(BL_FLOAT sampleRate);
    
    virtual ~USTVectorscope3();

    void Reset(BL_FLOAT sampleRate);

    void SetMode(Mode mode);
    
    void SetEnabled(bool flag);
    
    int GetNumCurves(int graphNum);

    int GetNumPoints(int graphNum);

    void SetGraphs(GraphControl11 *graph0,
                   GraphControl11 *graph1,
                   GraphControl11 *graph2);

    void AddSamples(vector<WDL_TypedBuf<BL_FLOAT> > samples);

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

    GraphControl11 *mGraphs[3];
    
    USTCircleGraphDrawer *mCircleDrawer0;
    USTCircleGraphDrawer *mCircleDrawer1;
    USTLissajousGraphDrawer *mLissajousDrawer;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
    
    bool mIsEnabled;
    
    Mode mMode;
    
    USTPolarLevel *mPolarLevel;
};

#endif /* defined(__UST__USTVectorscope3__) */
