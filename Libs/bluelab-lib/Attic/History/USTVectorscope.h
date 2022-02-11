//
//  USTVectorscope.h
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#ifndef __UST__USTVectorscope__
#define __UST__USTVectorscope__

#include <vector>
using namespace std;

class GraphControl11;
class USTCircleGraphDrawer;

class USTVectorscope
{
public:
    enum Mode
    {
      POLAR_SAMPLE = 0,
      POLAR_LEVEL,
      LISSAJOUS
    };
    
    USTVectorscope();
    
    virtual ~USTVectorscope();

    void Reset();

    void SetMode(Mode mode);
    
    void SetEnabled(bool flag);
    
    int GetNumCurves();

    int GetNumPoints();

    void SetGraph(GraphControl11 *graph);

    void AddSamples(vector<WDL_TypedBuf<BL_FLOAT> > samples);

protected:

    void SetPointStyle(BL_FLOAT minX, BL_FLOAT maxX,
                       BL_FLOAT minY, BL_FLOAT maxY,
                       BL_FLOAT pointSize,
                       int r, int g, int b,
                       bool curveFill, BL_FLOAT curveFillAlpha,
                       bool pointsAsLines);

    GraphControl11 *mGraph;

    USTCircleGraphDrawer *mCircleDrawer;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
    
    bool mIsEnabled;
    
    Mode mMode;
};

#endif /* defined(__UST__USTVectorscope__) */
