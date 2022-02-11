//
//  StereoVizVolRender3.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__StereoVizVolRender3__
#define __BL_StereoWidth__StereoVizVolRender3__

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl11.h>

// From StereoVizVolRender
// StereoVizVolRender is used by StereoWidth
// StereoVizVolRender2 may have modifications for StereoViz
// StereoVizVolRender3: for rendering with RayCaster
//

class RayCaster;
class CameraOrientation;

class StereoVizVolRender3 : public GraphCustomControl
{
public:
    StereoVizVolRender3(GraphControl11 *graphControl);
    
    virtual ~StereoVizVolRender3();
    
    virtual void AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                      const WDL_TypedBuf<BL_FLOAT> &yValues,
                                      const WDL_TypedBuf<BL_FLOAT> &colorWeights);
    virtual void Clear();
    
    // Control
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    virtual void OnMouseUp(int x, int y, IMouseMod* pMod);
    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    virtual bool OnMouseDblClick(int x, int y, IMouseMod* pMod);
    virtual bool OnKeyDown(int x, int y, int key, IMouseMod* pMod);
    
    virtual void OnGUIIdle();
    
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
    
    // Threshold
    virtual void SetThreshold(BL_FLOAT threshold);
    
    // Clip outside selection ?
    virtual void SetClipFlag(bool flag);
    
    void SetDisplayRefreshRate(int displayRefreshRate);
    
    // For playing the selection
    long GetNumSlices();
    
    bool SelectionEnabled();
    
    bool GetCenterSliceSelection(vector<bool> *selection,
                                 vector<BL_FLOAT> *xCoords,
                                 long *sliceNum);
    
protected:
    void DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
                       const WDL_TypedBuf<BL_FLOAT> &yValues);
    
    void SelectBorders(int x, int y);
    
    bool InsideSelection(int x, int y);
    
    bool BorderSelected();
    
    // Update selection from StereoVizProcess to RayCaster
    void UpdateSelection();
    
    // Update selection from RayCaster to StereoVizProcess
    void UpdateSelectionRC();

    
    GraphControl11 *mGraph;
    RayCaster *mRayCaster;
    
    // Selection
    bool mMouseIsDown;
    int mPrevDrag[2];
  
    bool mIsSelecting;
    int mSelection[4];
    
    bool mSelectionActive;
    
    bool mPrevMouseInsideSelect;
    bool mBorderSelected[4];
    
    // Used to detect pure mouse up, without drag
    bool mPrevMouseDrag;
    
    // Rotation
    BL_FLOAT mAngle0;
    BL_FLOAT mAngle1;
    
    long int mAddNum;
    
    int mSpeed;
    
    // Cmd key
    bool mPrevCmdPressed;

    CameraOrientation *mStartOrientation;
    CameraOrientation *mEndOrientation;
    CameraOrientation *mPrevUserOrientation;
    
    bool mIsSteppingOrientation;
};

#endif /* defined(__BL_StereoWidth__StereoVizVolRender3__) */
