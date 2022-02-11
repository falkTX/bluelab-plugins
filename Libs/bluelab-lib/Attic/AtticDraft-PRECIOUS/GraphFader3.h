//
//  GraphFader3.h
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__GraphFader3__
#define __BL_StereoWidth__GraphFader3__

#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl10;
class GraphFader3
{
public:
    GraphFader3(GraphControl10 *graphControl,
               int startCurve, int numFadeCurves,
               double startAlpha, double endAlpha,
               double startLuminance, double endLuminance);
    
    virtual ~GraphFader3();
    
    void Reset();
    
    void SetCurveStyle(double minY, double maxY,
                       double lineWidth,
                       int r, int g, int b,
                       bool curveFill, double curveFillAlpha);
    
    void SetPointStyle(double minX, double maxX,
                       double minY, double maxY,
                       double pointSize,
                       int r, int g, int b,
                       bool curveFill, double curveFillAlpha,
                       bool pointsAsLines);
    
    // xValues can be NULL if we don't use curve point style
    void AddCurveValues(const WDL_TypedBuf<double> *xValues,
                        const WDL_TypedBuf<double> &yValues);
    
    void AddCurveValuesWeight(const WDL_TypedBuf<double> *xValues,
                              const WDL_TypedBuf<double> &yValues,
                              const WDL_TypedBuf<double> &colorWeights);
    
protected:
    GraphControl10 *mGraph;
    int mNumCurves;
    
    deque<WDL_TypedBuf<double> > mCurvesX;
    deque<WDL_TypedBuf<double> > mCurvesY;
    
    int mStartCurveIndex;
    
    double mStartAlpha;
    double mEndAlpha;
    
    double mStartLuminance;
    double mEndLuminance;
    
    int mCurveColor[3];
    
    bool mCurveIsPoints;
    
    bool mPointsAsLines;
};

#endif /* defined(__BL_StereoWidth__GraphFader3__) */
