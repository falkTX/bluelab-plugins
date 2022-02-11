//
//  SMVVolRender3.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__SMVVolRender3__
#define __BL_StereoWidth__SMVVolRender3__

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl11.h>
#include <SoundMetaViewerPluginInterface.h>

// Camera angles
//

// Max: 90 = >but we will see slices too much

// Sides
//#define MAX_ANGLE_0 70.0

// Set to 90 for debugging
#define MAX_CAMERA_ANGLE_0 90.0
#define DEFAULT_ANGLE_0 0.0

// Above
#define MAX_CAMERA_ANGLE_1 90.0 //70.0
#define DEFAULT_ANGLE_1 0.0

// Below
#define MIN_CAMERA_ANGLE_1 -20.0

// For Zoom
#define DEFAULT_CAMERA_FOV 45.0f
#define MIN_CAMERA_FOV 15.0
#define MAX_CAMERA_FOV 52.0

// From StereoVizVolRender
// StereoVizVolRender is used by StereoWidth
// StereoVizVolRender2 may have modifications for StereoViz
// StereoVizVolRender3: for rendering with RayCaster
//
// From StereoVizVolRender3
//

class RayCaster2;
class CameraOrientation;
class Axis3D;
//class SoundMetaViewer;

class SMVVolRender3 : public GraphCustomControl
{
public:
    SMVVolRender3(SoundMetaViewerPluginInterface *plug,
                  GraphControl11 *graphControl,
                  int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~SMVVolRender3();
    
    void Reset(BL_FLOAT sampleRate);
    
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
    
    virtual void OnMouseWheel(int x, int y, IMouseMod* pMod, BL_FLOAT d);
    
    virtual void OnGUIIdle();
    
    // Quality parameters
    virtual void SetQualityXY(BL_FLOAT quality);
    virtual void SetQualityT(BL_FLOAT quality);
    virtual void SetQuality(BL_FLOAT quality);
    
    virtual void SetSpeedT(BL_FLOAT speed);
    
    virtual void SetSpeed(BL_FLOAT speed);
    
    // ColorMap
    virtual void SetColormap(int colormapId);
    virtual void SetInvertColormap(bool flag);
    
    virtual void SetColormapRange(BL_FLOAT range);
    virtual void SetColormapContrast(BL_FLOAT contract);
    
    // Point size (debug)
    // Normalized value
    virtual void SetPointSize(BL_FLOAT size);
    
#if 1
    // Alpha coeff
    virtual void SetAlphaCoeff(BL_FLOAT coeff);
#endif
    
    // Threshold
    //
    
    // Amount
    virtual void SetThreshold(BL_FLOAT threshold);
    // Center
    virtual void SetThresholdCenter(BL_FLOAT thresholdCenter);
    
    // Clip outside selection ?
    virtual void SetClipFlag(bool flag);
    
    void SetDisplayRefreshRate(int displayRefreshRate);
    
    // For playing the selection
    long GetNumSlices();
    
    // Selection 2D
    bool IsSelectionEnabled();
    // Selection 3D
    bool IsVolumeSelectionEnabled();
    
    long GetCurrentSlice();
    
    bool GetSliceSelection(vector<bool> *selection,
                           vector<BL_FLOAT> *xCoords,
                           long sliceNum);
    
    bool GetSelectionBoundsLines(int *startLine, int *endLine);
    
    // Play
    void SetPlayBarPos(BL_FLOAT t);
    
    // Axes
    void SetAxis(int idx, Axis3D *axis);
    
    // Only for Axes
    RayCaster2 *GetRayCaster();
    
    // Debug
    void SetRenderAlgo(int algo);
    void SetRenderAlgoParam(BL_FLOAT renderParam);
    
    //
    void SetAutoQuality(bool flag);
    
    // For camera parameter sent from plug (save state and automation)
    void SetCamAngle0(BL_FLOAT angle);
    void SetCamAngle1(BL_FLOAT angle);
    void SetCamFov(BL_FLOAT angle);
    
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

    // Automatically update quality t
    void AdaptQualityT(BL_FLOAT speed);

    //
    GraphControl11 *mGraph;
    RayCaster2 *mRayCaster;
    
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
    
    //
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
    // Plug
    SoundMetaViewerPluginInterface *mPlug;
};

#endif /* defined(__BL_StereoWidth__SMVVolRender3__) */
