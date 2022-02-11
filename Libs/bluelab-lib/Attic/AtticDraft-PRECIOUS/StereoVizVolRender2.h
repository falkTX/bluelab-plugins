//
//  StereoVizVolRender2.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__StereoVizVolRender2__
#define __BL_StereoWidth__StereoVizVolRender2__

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl10.h>
#include <SparseVolRender.h>

// From StereoVizVolRender
// StereoVizVolRender is used by StereoWidth
// StereoVizVolRender2 may have modifications for StereoViz
//
class StereoVizVolRender2 : public GraphCustomControl
{
public:
    StereoVizVolRender2(GraphControl10 *graphControl);
    
    virtual ~StereoVizVolRender2();
    
    virtual void AddCurveValuesWeight(const WDL_TypedBuf<double> &xValues,
                                      const WDL_TypedBuf<double> &yValues,
                                      const WDL_TypedBuf<double> &colorWeights);
    
    // Control
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    virtual void OnMouseUp(int x, int y, IMouseMod* pMod);
    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    virtual bool OnMouseDblClick(int x, int y, IMouseMod* pMod);
    
    // Quality parameters
    virtual void SetQualityXY(double quality);
    virtual void SetQualityT(double quality);
    virtual void SetSpeed(double speed);
    
    virtual void SetColormap(int colormapId);
    virtual void SetInvertColormap(bool flag);
    
protected:
    void DistToDbScale(const WDL_TypedBuf<double> &xValues,
                       const WDL_TypedBuf<double> &yValues);
    
    GraphControl10 *mGraph;
    SparseVolRender *mVolRender;
    
    bool mMouseIsDown;
    int mPrevDrag[2];
    
    double mAngle0;
    double mAngle1;
    
    long int mAddNum;
    
    // Quality
    //int mNumPointsSlice;
    int mSpeed;
};

#endif /* defined(__BL_StereoWidth__StereoVizVolRender2__) */
