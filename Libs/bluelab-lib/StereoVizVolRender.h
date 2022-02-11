//
//  StereoVizVolRender.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__StereoVizVolRender__
#define __BL_StereoWidth__StereoVizVolRender__

#ifdef IGRAPHICS_NANOVG

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl11.h>
#include <SparseVolRender.h>

class StereoVizVolRender : public GraphCustomControl
{
public:
    StereoVizVolRender(GraphControl11 *graphControl);
    
    virtual ~StereoVizVolRender();
    
    virtual void AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                      const WDL_TypedBuf<BL_FLOAT> &yValues,
                                      const WDL_TypedBuf<BL_FLOAT> &colorWeights);
    
    // Control
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    virtual void OnMouseUp(float x, float y, const IMouseMod &mod) override;
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &mod) override;

protected:
    void DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
                       const WDL_TypedBuf<BL_FLOAT> &yValues);
    
    GraphControl11 *mGraph;
    SparseVolRender *mVolRender;
    
    deque<vector<SparseVolRender::Point> > mPoints;
    
    bool mMouseIsDown;
    int mPrevDrag[2];
    
    BL_FLOAT mAngle0;
    BL_FLOAT mAngle1;
    
    long int mAddNum;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__StereoVizVolRender__) */
