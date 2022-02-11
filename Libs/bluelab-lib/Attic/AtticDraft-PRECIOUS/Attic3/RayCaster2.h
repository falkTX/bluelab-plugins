//
//  RayCaster2.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__RayCaster2__
#define __BL_StereoWidth__RayCaster2__

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

// Float type
//

// Origin
//#define RC_FLOAT double

// Optimizes well! -15% CPU (100% -> 85%)
#define RC_FLOAT float

// RayCaster2: from RayCaster
// - big code clean

class ColorMap4;
class ImageDisplayColor;
class RCQuadTree;
class RCKdTree;
class ColorMapFactory;
class RayCaster2 : public GraphCustomDrawer,
                  //public PointProjector
                  public PointProjectorFloat
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
            
            *((int *)&mRGBA) = 0;
            
            mFlags = 0;
            
            //mIsSelected = false;
            //mIsActive = true;
        };
        
        union {
            // When point is 3D
            RC_FLOAT mX;
            
            // When point is projected to pixel
            int mXi;
        };
        
        union {
            // When point is 3D
            RC_FLOAT mY;
            
            // When point is projected to pixel
            int mYi;
        };
        
        RC_FLOAT mZ;
        
        union {
            // When point is 3D
            RC_FLOAT mSize[2];
            
            // When point is projected to pixel
            int mSizei[2];
        };
        
        RC_FLOAT mWeight;
        
        unsigned char mRGBA[4];
        
        // Flags
        //
        
#define RC_POINT_FLAG_SELECTED     0x1
        // If discarded by threshold, this flag is set (previously "!mIsActive")
#define RC_POINT_FLAG_THRS_DISCARD 0x2
        // for lazy evaluation
#define RC_POINT_FLAG_COLORMAP_SET 0x4

        // Use flags to keep the structure size to 32 octets
        unsigned short mFlags;

        
        //
        static inline bool IsSmallerX(const Point &p0, const Point &p1)
        {
            return p0.mX < p1.mX;
        }
        
        static inline bool IsGreaterX(const Point &p0, const Point &p1)
        {
            return p0.mX > p1.mX;
        }
        
        static inline bool IsSmallerY(const Point &p0, const Point &p1)
        {
            return p0.mY < p1.mY;
        }
        
        static inline bool IsGreaterY(const Point &p0, const Point &p1)
        {
            return p0.mY > p1.mY;
        }
        
        static inline bool IsSmallerZ(const Point &p0, const Point &p1)
        {
            return p0.mZ < p1.mZ;
        }
        
        // Not used anymore
        static inline bool IsSmallerZP(const Point *p0, const Point *p1)
        {
            return p0->mZ < p1->mZ;
        }
        
        // To sort in the reverse order
        static inline bool IsGreaterZ(const Point &p0, const Point &p1)
        {
            return p0.mZ > p1.mZ;
        }
        
        // Not used anymore
        static inline int IsSmallerZ_C(const void * a, const void * b)
        {
            // Warning: we are managing floats, and retuning ints !
            // (take care of truncation !)
            // So this doesn't work.
            //return ( ((Point *)a)->mZ - ((Point *)b)->mZ );
            
            if (((Point *)a)->mZ > ((Point *)b)->mZ)
                return 1;
            
            if (((Point *)a)->mZ < ((Point *)b)->mZ)
                return -1;
            
            return 0;
        }
        
        //
        static bool IsThrsDiscarded(const Point &p0)
        {
            //return !p0.mIsActive;
            return (p0.mFlags & RC_POINT_FLAG_THRS_DISCARD);
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
    RayCaster2(int numSlices, int numPointsSlice);
    
    virtual ~RayCaster2();
    
    void SetNumSlices(long numSlices);
    
    long GetNumSlices();
    
    void SetScrollDirection(ScrollDirection dir);
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height);
    
    void ClearSlices();
    
    void SetSlices(const vector< vector<Point> > &slices);
    
    void AddSlice(const vector<Point> &points, bool skipDisplay);

    void SetCameraAngles(RC_FLOAT angle0, RC_FLOAT angle1);
    void SetCameraFov(RC_FLOAT camFov);
    
    void ZoomChanged(RC_FLOAT zoomChange);
    
    // Colormap
    void SetColorMap(int colormapId);
    void SetInvertColormap(bool flag);
    
    void SetColormapRange(RC_FLOAT range);
    void SetColormapContrast(RC_FLOAT contrast);
    
    // Selection
    void SetSelection(const RC_FLOAT selection[4]);

    // Return true if the current selection is valid, false otherwise
    bool GetSelection(RC_FLOAT selection[4]);
    
    void EnableSelection();
    
    void DisableSelection();
    
    bool IsSelectionEnabled();
    bool IsVolumeSelectionEnabled();

    
    void ResetSelection();
    
    bool GetSliceSelection(vector<bool> *selection,
                           vector<RC_FLOAT> *xCoords,
                           long sliceNum);
    
    long GetCurrentSlice();
    
    bool GetSelectionBoundsLines(int *startLine, int *endLine);

    
    // Extract screen selection from volume selection
    void VolumeToSelection();
    
    void TranslateVolumeSelection(RC_FLOAT trans[3]);
    
    //
    void SetDisplayRefreshRate(int displayRefreshRate);
    
    void SetQuality(RC_FLOAT quality);
    void SetQualityT(RC_FLOAT quality);
    
    // Normalized value
    void SetPointSize(RC_FLOAT size);
    
    void SetAlphaCoeff(RC_FLOAT alpha);
    
    // Threshold
    
    // Amount
    void SetThreshold(RC_FLOAT threshold);
    void SetThresholdCenter(RC_FLOAT thresholdCenter);
    
    void SetClipFlag(bool flag);
    
    // Play
    void SetPlayBarPos(RC_FLOAT t);
    
    // Axes
    void SetAxis(int idx, Axis3D *axis);
    
    // Axis PointProjector overwrite
    void ProjectPoint(RC_FLOAT projP[3], const RC_FLOAT p[3], int width, int height);
    
    // Debug
    void SetRenderAlgo(RenderAlgo algo);
    void SetRenderAlgoParam(RC_FLOAT renderParam);
    
    //
    void SetAutoQuality(bool flag);
    
protected:
    // Selection
    void DrawSelection(NVGcontext *vg, int width, int height);
   
    void DrawSelectionVolume(NVGcontext *vg, int width, int height);
    
    // Play
    void DrawPlayBarPos(NVGcontext *vg, int width, int height);
    
    void SelectPoints3D(vector<Point> *points);
    
    void ProjectPoints(vector<Point> *slices, int width, int height);
    
    void RayCast(vector<Point> *slices, int width, int height);
    
    void DoRayCastGrid(vector<Point> *slices, int width, int height);
    void DoRayCastQuadTree(vector<Point> *slices, int width, int height);
    void DoRayCastKdTree(vector<Point> *slices, int width, int height);
    
    void ComputePackedPointColor(const RayCaster2::Point &p, NVGcolor *color);
    
    void ThresholdPoints(vector<Point> *points);

    void ClipPoints(vector<Point> *points);
    
    void ApplyColorMap(vector<Point> *points);
    void ApplyColorMap(Point *p);
    
    void UpdateSlicesZ();
    
    void SlicesDequeToVec(vector<RayCaster2::Point> *res,
                          const deque<vector<RayCaster2::Point> > &slicesQue);
    
    int GetResolution();
    
    //
    void SelectionToVolume();

    void ScreenToAlignedVolume(const RC_FLOAT screenSelectionNorm[4],
                               RC_FLOAT alignedVolumeSelection[4]);
    
    void AlignedVolumeToScreen(const RC_FLOAT alignedVolumeSelection[4],
                               RC_FLOAT screenSelectionNorm[4]);
    
    void ReorderScreenSelection(RC_FLOAT screenSelection[4]);
    
    // Different blending methods
    //
    
    // Sort by z and then blend in order
    void BlendDepthSorting(unsigned char rgba[4], vector<Point> &points0);
    
    // Front to back version
    void SortFTB(vector<Point> *points);
    void BlendDepthSortingFTB(unsigned char rgba[4], vector<Point> &points0);
    
    
    // Use the method described here:
    // http://jcgt.org/published/0002/02/09/
    // and here: https://habr.com/en/post/457292/
    void BlendWBOIT(int rgba[4], vector<Point> &points0);
    
    //
    void RenderQuads(NVGcontext *vg, const vector<Point> &points);

    void RenderTexture(NVGcontext *vg, int width, int height,
                       const vector<Point> &points,
                       RC_FLOAT sizeCoeff = -1.0);
    
    // Optim
    long ComputeNumRayPoints(const vector<Point> &points, RC_FLOAT z);
    void DiscardFarthestRayPoints(vector<Point> *points);
    void DiscardFarthestRayPointsAdaptive(vector<Point> *points);
    
    RC_FLOAT ComputeCellSize(int width, int height);

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
    
    void SetAutoQuality(RC_FLOAT quality);

    void ResetPointColormapSetFlags();
    
    // Pre sort points on X or Y
    void PreSortPoints(vector<Point> *points);

    // Compute the index of the slice that is the most aligned to the view
    // (the smallest bounnding box)
    int ComputeAlignedSliceIdx(int width, int height);
    
    // Debug
    void DBG_DrawSlices(NVGcontext *vg, int width ,int height);
    
    //
    typedef struct SelectionPoint
    {
        RC_FLOAT mX;
        RC_FLOAT mY;
        RC_FLOAT mZ;
        
        SelectionPoint()
        {
            mX = 0.0;
            mY = 0.0;
            mZ = 0.0;
        }
        
        SelectionPoint(RC_FLOAT x, RC_FLOAT y, RC_FLOAT z)
        {
            mX = x;
            mY = y;
            mZ = z;
        }
    } SelectionPoint;
    //
    
    deque<vector<Point> > mSlices;
    
    //
    
    RC_FLOAT mCamAngle0;
    RC_FLOAT mCamAngle1;
    
    // Zoom
    RC_FLOAT mCamFov;
    
    //
    ColorMapFactory *mColorMapFactory;
    ColorMap4 *mColorMap;
    bool mInvertColormap;
    
    //
    long mNumSlices;
    
    // Selection
    
    // 2D selection, for screen
    RC_FLOAT mScreenSelection[4];
    
    bool mSelectionEnabled;
    
    // 3D selection, for volume
    // (xmin, xmax), (ymin, ymax), (zmin, zmax)
    SelectionPoint mVolumeSelection[8];
    
    bool mVolumeSelectionEnabled;
    
    //
    int mDisplayRefreshRate;
    long mRefreshNum;
    
    // Dummy image of 1 white pixel
    // for quad rendering
    //
    // NOTE: rendering quads with or without texture: same perfs
    //
    int mWhitePixImg;
    
    RC_FLOAT mQuality;
    RC_FLOAT mQualityT;
    
    RC_FLOAT mPointSize;
    
    RC_FLOAT mAlphaCoeff;
    
    RC_FLOAT mThreshold;
    RC_FLOAT mThresholdCenter;
    
    bool mClipFlag;
    
    // Optim: GOOD (optim x2)
    // Do not re-allocate points each time
    // Keep a buffer
    vector<RayCaster2::Point> mTmpPoints;
    
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
    RC_FLOAT mPlayBarPos;
    
    // X and Y axis
    Axis3D *mAxis[NUM_AXES];
  
    // Debug
    RenderAlgo mRenderAlgo;
    RC_FLOAT mRenderAlgoParam;
    
    bool mAutoQualityFlag;
    RC_FLOAT mAutoQuality;
    
    //vector<RC_FLOAT> mDBG_SlicesZ;
    
#if PROFILE_RENDER
    BlaTimer mTimer0;
    long mCount0;
#endif
};

#endif /* defined(__BL_StereoWidth__RayCaster2__) */
