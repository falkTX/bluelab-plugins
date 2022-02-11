//
//  GraphFader.h
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__GraphFader2__
#define __BL_StereoWidth__GraphFader2__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class GraphFader2
{
public:
    GraphFader2(GraphControl11 *graphControl,
               int startCurve, int numFadeCurves,
               BL_FLOAT startAlpha, BL_FLOAT endAlpha,
               BL_FLOAT startLuminance, BL_FLOAT endLuminance);
    
    virtual ~GraphFader2();
    
    void Reset();
    
    void SetCurveStyle(BL_FLOAT minY, BL_FLOAT maxY,
                       BL_FLOAT lineWidth,
                       int r, int g, int b,
                       bool curveFill, BL_FLOAT curveFillAlpha);
    
    void SetPointStyle(BL_FLOAT minX, BL_FLOAT maxX,
                       BL_FLOAT minY, BL_FLOAT maxY,
                       BL_FLOAT pointSize,
                       int r, int g, int b,
                       bool curveFill, BL_FLOAT curveFillAlpha,
                       bool pointsAsLines);
    
    // xValues can be NULL if we don't use curve point style
    void AddCurveValues(const WDL_TypedBuf<BL_FLOAT> &xValues,
                        const WDL_TypedBuf<BL_FLOAT> &yValues);
    
    void AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                              const WDL_TypedBuf<BL_FLOAT> &yValues,
                              const WDL_TypedBuf<BL_FLOAT> &colorWeights);
    
#if 0 // NOT USED
    // Decimate intelligently
    void AddCurveValuesWeight2(const WDL_TypedBuf<BL_FLOAT> &xValues,
                               const WDL_TypedBuf<BL_FLOAT> &yValues,
                               const WDL_TypedBuf<BL_FLOAT> &colorWeights);
#endif
    
protected:
    GraphControl11 *mGraph;
    int mNumCurves;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mCurvesX;
    deque<WDL_TypedBuf<BL_FLOAT> > mCurvesY;
    
    int mStartCurveIndex;
    
    BL_FLOAT mStartAlpha;
    BL_FLOAT mEndAlpha;
    
    BL_FLOAT mStartLuminance;
    BL_FLOAT mEndLuminance;
    
    int mCurveColor[3];
    
    bool mCurveIsPoints;
    
    bool mPointsAsLines;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__GraphFader2__) */
