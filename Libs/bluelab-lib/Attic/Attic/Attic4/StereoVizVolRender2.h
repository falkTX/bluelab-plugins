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

#include <GraphControl11.h>
#include <SparseVolRender.h>

// From StereoVizVolRender
// StereoVizVolRender is used by StereoWidth
// StereoVizVolRender2 may have modifications for StereoViz
//
class StereoVizVolRender2 : public GraphCustomControl
{
public:
    StereoVizVolRender2(GraphControl11 *graphControl);
    
    virtual ~StereoVizVolRender2();
    
    virtual void AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                      const WDL_TypedBuf<BL_FLOAT> &yValues,
                                      const WDL_TypedBuf<BL_FLOAT> &colorWeights);
    
    // Control
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    virtual void OnMouseUp(int x, int y, IMouseMod* pMod);
    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    virtual bool OnMouseDblClick(int x, int y, IMouseMod* pMod);
    
    // Quality parameters
    virtual void SetQualityXY(BL_FLOAT quality);
    virtual void SetQualityT(BL_FLOAT quality);
    virtual void SetQuality(BL_FLOAT quality);
    
    virtual void SetSpeed(BL_FLOAT speed);
    
    // ColorMap
    virtual void SetColormap(int colormapId);
    virtual void SetInvertColormap(bool flag);
    
    virtual void SetColormapRange(BL_FLOAT range);
    virtual void SetColormapContrast(BL_FLOAT contract);
    
    // Point size (debug)
    virtual void SetPointSizeCoeff(BL_FLOAT coeff);
    
    // Alpha coeff (debug)
    virtual void SetAlphaCoeff(BL_FLOAT coeff);
    
    // Points selection
    void GetPointsSelection(vector<bool> *pointFlags);
    
    void SetDisplayRefreshRate(int displayRefreshRate);
    
protected:
    void DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
                       const WDL_TypedBuf<BL_FLOAT> &yValues);
    
    GraphControl11 *mGraph;
    SparseVolRender *mVolRender;
    
    bool mMouseIsDown;
    int mPrevDrag[2];
  
    bool mIsSelecting;
    int mSelection[4];
    bool mSelectionActive;
    
    BL_FLOAT mAngle0;
    BL_FLOAT mAngle1;
    
    long int mAddNum;
    
    // Quality
    //int mNumPointsSlice;
    int mSpeed;
};

#endif /* defined(__BL_StereoWidth__StereoVizVolRender2__) */
