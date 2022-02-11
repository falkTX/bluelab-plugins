//
//  RayCaster.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__RayCaster__
#define __BL_StereoWidth__RayCaster__

#include <vector>
#include <deque>
using namespace std;

#include "nanovg.h"

#include <GraphControl10.h>

#define PROFILE_RENDER 0

#if PROFILE_RENDER
#include <BlaTimer.h>
#endif

#include <Axis3D.h>

// For Zoom
#define DEFAULT_CAMERA_FOV 45.0f
#define MIN_CAMERA_FOV 15.0
#define MAX_CAMERA_FOV 52.0

#define NUM_AXES 3

class ColorMap4;
class ImageDisplayColor;
class RCQuadTree;
class RCKdTree;

class RayCaster : public GraphCustomDrawer,
                  public PointProjector
{
public:
    class Point
    {
    public:
        Point()
        {
            mX = 0.0;
            mY = 0.0;
            mZ = 0.0;
            
            mSize[0] = 0.0;
            mSize[1] = 0.0;
            
            mWeight = 0.0;
            
            mR = 0;
            mG = 0;
            mB = 0;
            mA = 0;
            
            mIsSelected = false;
            
            mId = 0;
            
            mIsActive = true;
        };
        
        double mX;
        double mY;
        double mZ;
        
        double mSize[2];
        
        double mWeight;
        
        unsigned char mR;
        unsigned char mG;
        unsigned char mB;
        unsigned char mA;
        
        bool mIsSelected;
        
        long mId;
        
        // If discarded by threshold, mIsActive is turned to false
        bool mIsActive;
        
        //
        static bool IsSmallerX(const Point &p0, const Point &p1)
        {
            return p0.mX < p1.mX;
        }
        
        static bool IsSmallerY(const Point &p0, const Point &p1)
        {
            return p0.mY < p1.mY;
        }
        
        static bool IsSmallerZ(const Point &p0, const Point &p1)
        {
            return p0.mZ < p1.mZ;
        }
        
        //
        static bool IsNotActive(const Point &p0)
        {
            return !p0.mIsActive;
        }
    };
    
    enum ScrollDirection
    {
        BACK_FRONT = 0,
        FRONT_BACK
    };
    
    enum RenderAlgo
    {
        RENDER_ALGO_GRID = 0,
        RENDER_ALGO_GRID_DILATE,
        RENDER_ALGO_QUAD_TREE,
        RENDER_ALGO_KD_TREE_MEDIAN,
        RENDER_ALGO_KD_TREE_MIDDLE
    };
    
    //
    RayCaster(int numSlices, int numPointsSlice);
    
    virtual ~RayCaster();
    
    void SetNumSlices(long numSlices);
    
    long GetNumSlices();
    
    void SetScrollDirection(ScrollDirection dir);
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height);
    
    void ClearSlices();
    
    void SetSlices(const vector< vector<Point> > &slices);
    
    void AddSlice(const vector<Point> &points, bool skipDisplay);

    void SetCameraAngles(double angle0, double angle1);
    void SetCameraFov(double camFov);
    
    void ZoomChanged(double zoomChange);
    
    // Colormap
    void SetColorMap(int colormapId);
    void SetInvertColormap(bool flag);
    
    void SetColormapRange(double range);
    void SetColormapContrast(double contrast);
    
    // Selection
    void SetSelection(const double selection[4]);

    // Return true if the current selection is valid, false otherwise
    bool GetSelection(double selection[4]);
    
    void EnableSelection();
    
    void DisableSelection();
    
    bool IsSelectionEnabled();
    bool IsVolumeSelectionEnabled();

    
    void ResetSelection();
    
    bool GetSliceSelection(vector<bool> *selection,
                           vector<double> *xCoords,
                           long sliceNum);
    
    long GetCurrentSlice();
    
    bool GetSelectionBoundsLines(int *startLine, int *endLine);

    
    // Extract screen selection from volume selection
    void VolumeToSelection();
    
    void TranslateVolumeSelection(double trans[3]);
    
    //
    void SetDisplayRefreshRate(int displayRefreshRate);
    
    void SetQuality(double quality);
    void SetQualityT(double quality);
    
    // Normalized value
    void SetPointSize(double size);
    
//#if 0 // not used for the moment
    void SetAlphaScale(double scale);
//#endif
    void SetAlphaCoeff(double alpha);
    
    // Threshold
    
    // Amount
    void SetThreshold(double threshold);
    void SetThresholdCenter(double thresholdCenter);
    
    void SetClipFlag(bool flag);
    
    // Play
    void SetPlayBarPos(double t);
    
    // Axes
    void SetAxis(int idx, Axis3D *axis);
    
    // Axis PointProjector overwrite
    void ProjectPoint(double projP[3], const double p[3], int width, int height);
    
    // Debug
    void SetRenderAlgo(RenderAlgo algo);
    void SetRenderAlgoParam(double renderParam);
    
    //
    void SetAutoQuality(bool flag);
    
protected:
    // Selection
    void DrawSelection(NVGcontext *vg, int width, int height);
   
    void DrawSelectionVolume(NVGcontext *vg, int width, int height);
    
    // Play
    void DrawPlayBarPos(NVGcontext *vg, int width, int height);
    
    void SelectPoints2D(vector<Point> *projPoints, int width, int height);
    
    void SelectPoints3D(vector<Point> *points);
    
    void SetPointIds(vector<Point> *points);

    void PropagateSelection(const vector<Point> &projPoints);
    
    void ProjectPoints(vector<Point> *slices, int width, int height);
    
    void RayCast(vector<Point> *slices, int width, int height);
    
    void DoRayCastGrid(vector<Point> *slices, int width, int height);
    void DoRayCastQuadTree(vector<Point> *slices, int width, int height);
    void DoRayCastKdTree(vector<Point> *slices, int width, int height);
    
    void DecimateScreenSpace(deque<vector<Point> > *slices, int width, int height);
    
    void ComputePackedPointColor(const RayCaster::Point &p, NVGcolor *color);
    
    void ThresholdPoints(vector<Point> *points);

    void ClipPoints(vector<Point> *points);
    
    void ApplyColorMap(vector<Point> *points);
    
    void UpdateSlicesZ();
    
    void SlicesDequeToVec(vector<RayCaster::Point> *res,
                          const deque<vector<RayCaster::Point> > &slicesQue);
    
    int GetResolution();
    
    //long GetCenterSlice();
    
    //
    void SelectionToVolume();

    void ScreenToAlignedVolume(const double screenSelectionNorm[4],
                               double alignedVolumeSelection[4]);
    
    void AlignedVolumeToScreen(const double alignedVolumeSelection[4],
                               double screenSelectionNorm[4]);
    
    //void ProjectFace(double projFace[2][3]);
    
    void ReorderScreenSelection(double screenSelection[4]);
    
    // DEBUG
    void DBG_SelectSlice(long sliceNum);
    
#if 0 // OLD
    bool SelectionIntersectSlice2D(const double selection2D[2][2],
                                   const double projSlice2D[4][2]);
#endif
    
    // Different blending methods
    //
    
    // Sort by z and clenad in order
    void BlendDepthSorting(int rgba[4], vector<Point> &points0);
    
    // Use the method described here:
    // http://jcgt.org/published/0002/02/09/
    // and here: https://habr.com/en/post/457292/
    void BlendWBOIT(int rgba[4], vector<Point> &points0);
    
    void RenderQuads(NVGcontext *vg, const vector<Point> &points);

    void RenderTexture(NVGcontext *vg, int width, int height,
                       const vector<Point> &points);
    
    // Optim
    long ComputeNumRayPoints(const vector<Point> &points, double z);
    void DiscardFartestRayPoints(vector<Point> *points);

    double ComputeCellSize(int width, int height);

    // Axes
    void ComputeAxisPreDrawFlags(bool preDrawFlags[NUM_AXES]);

    void DrawAxes(NVGcontext *vg, int width, int height,
                  int stepNum, bool predDrawFlags[NUM_AXES]);
    
    // Image dilation of texture
    void DilateImageAux(int width, int height,
                        WDL_TypedBuf<unsigned char> *imageData);
    
    void DilateImage(int width, int height,
                     WDL_TypedBuf<unsigned char> *imageData);
    
    //
    void AdaptZoomQuality();
    
    void SetAutoQuality(double quality);

    
    //
    typedef struct SelectionPoint
    {
        double mX;
        double mY;
        double mZ;
        
        SelectionPoint()
        {
            mX = 0.0;
            mY = 0.0;
            mZ = 0.0;
        }
        
        SelectionPoint(double x, double y, double z)
        {
            mX = x;
            mY = y;
            mZ = z;
        }
    } SelectionPoint;
    //
    
    deque<vector<Point> > mSlices;
    
    //
    
    double mCamAngle0;
    double mCamAngle1;
    
    ColorMap4 *mColorMap;
    
    bool mInvertColormap;
    
    long mNumSlices;
    
    // Selection
    
    // 2D selection, for screen
    double mScreenSelection[4];
    
    bool mSelectionEnabled;
    
    // 3D selection, for volume
    // (xmin, xmax), (ymin, ymax), (zmin, zmax)
    SelectionPoint mVolumeSelection[8];
    
    bool mVolumeSelectionEnabled;
    
    vector<bool> mPointsSelection;
    
    //
    int mDisplayRefreshRate;
    long mRefreshNum;
    
    // Dummy image of 1 white pixel
    // for quad rendering
    //
    // NOTE: rendering quads with or without texture: same perfs
    //
    int mWhitePixImg;
    
    double mQuality;
    double mQualityT;
    
    double mPointSize;
    
    //double mAlphaScale;
    double mAlphaCoeff;
    
    double mThreshold;
    double mThresholdCenter;
    
    bool mClipFlag;
    
    // Optim: GOOD (optim x2)
    // Do not re-allocate points each time
    // Keep a buffer
    vector<RayCaster::Point> mTmpPoints;
    
    // For grid rendering
    //
    // Optim: GOOD (optim x2)
    // Do not re-allocate the grid each time
    vector<vector<vector<Point> > > mTmpGrid;
    
    int mViewWidth;
    int mViewHeight;
    
    // For QuadTree rendering
    RCQuadTree *mQuadTree;
    
    // For KdTree rendering
    RCKdTree *mKdTree;
    
    bool mQualityChanged;
    
    
    // New
    ScrollDirection mScrollDirection;
    
    // Used only when RENDER_TEXTURE is activated
    ImageDisplayColor *mImage;
    WDL_TypedBuf<unsigned char> mImageData;
    
    long mSliceNum;
    
    int mWidth;
    int mHeight;
    
    bool mFirstTimeRender;
    
    // Play
    double mPlayBarPos;
    
    // Zoom
    double mCamFov;
    
    // X and Y axis
    Axis3D *mAxis[NUM_AXES];
  
    // Debug
    RenderAlgo mRenderAlgo;
    double mRenderAlgoParam;
    
    bool mAutoQualityFlag;
    double mAutoQuality;
    
#if PROFILE_RENDER
    BlaTimer mTimer0;
    long mCount0;
#endif
};

#endif /* defined(__BL_StereoWidth__RayCaster__) */
