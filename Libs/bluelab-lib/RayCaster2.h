//
//  RayCaster2.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__RayCaster2__
#define __BL_StereoWidth__RayCaster2__

#ifdef IGRAPHICS_NANOVG

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl12.h>

#define PROFILE_RENDER 0

#if PROFILE_RENDER
#include <BlaTimer.h>
#endif

#include <Axis3D.h>

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
    // 3D point
    class Point
    {
    public:
        Point()
        {
            mX = 0.0;
            mY = 0.0;
            mZ = 0.0;
            
            mWeight = 0.0;
            
            *((int *)&mRGBA) = 0;
            
            mFlags = 0;
            
            mId = -1;
        };
        
    public:
        RC_FLOAT mX;
        RC_FLOAT mY;
        RC_FLOAT mZ;
        
        RC_FLOAT mWeight;
        
        union {
            // Colormap is applied after the grid pass
            unsigned char mRGBA[4];
            
            // This is used to sort the points depending on distance to camera
            // This is done just after extracting slices
            // (and before colormap)
            // Used only for OPTIM_PRE_SORT_SORT_CAM
            RC_FLOAT mDistToCam2;
        };
        
        // Flags
        //
#define RC_POINT_FLAG_SELECTED            0x1
        // If discarded by threshold
#define RC_POINT_FLAG_DISCARDED_THRS      0x2
        // If discarded by clip
#define RC_POINT_FLAG_DISCARDED_CLIP      0x4
        // For FIX_OPTIM_PRESORT_ARTIFACT
#define RC_POINT_FLAG_NEED_SCREEN_SORTING 0x8
        
        // Use flags to keep the structure size to 32 octets
        unsigned short mFlags;
        
        // Keep the point id, in the order it was added to the slice
        // Useful because we sort the points for rendering, so we mix the points,
        // the order is not kepts. (during: OPTIM_PRE_SORT).
        //
        // We need to retreive the order of the points when selecting or thresholding,
        // to play the corresponding points only.
        int mId;
        
        // For 32 bytes aligment
        //
        // NOTE: better perfs with alignment (with no Point pointers)
        //int dummy[2]; // before mId
        int dummy[1];
        
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
        
        //
        static inline bool IsSmallerDistToCam(const Point &p0, const Point &p1)
        {
            return p0.mDistToCam2 < p1.mDistToCam2;
        }
    };
    
    // Projected 2D point, with z buffer
    class Point2D
    {
    public:
        Point2D()
        {
            //mXi = 0;
            //mYi = 0;
            mX = 0.0;
            mY = 0.0;
            
            mZ = 0.0;
            
            mSizei[0] = 0;
            mSizei[1] = 0;
            
            *((int *)&mRGBA) = 0;
            
            mLastPoint = false;
        }
        
        static inline bool IsSmallerZ(const Point2D &p0, const Point2D &p1)
        {
            return p0.mZ < p1.mZ;
        }
        
    public:
        //int mXi;
        //int mYi;
        RC_FLOAT mX;
        RC_FLOAT mY;
        
        RC_FLOAT mZ;
        
        int mSizei[2];
        
        unsigned char mRGBA[4];
        
        // Identifier to detect the last point in a list
        bool mLastPoint;
      
        // For 32 bytes aligment
        //
        // NOTE: better perfs with alignment (with no Point pointers)
        int dummy[1];
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
    void PreDraw(NVGcontext *vg, int width, int height) override;
    
    void ClearSlices();
    
    void SetSlices(const vector< vector<Point> > &slices);
    
    void AddSlice(const vector<Point> &points, bool skipDisplay);

    void SetCameraAngles(RC_FLOAT angle0, RC_FLOAT angle1);
    void SetCameraFov(RC_FLOAT camFov);
    RC_FLOAT GetCameraFov();
    
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
    void ProjectPoint(RC_FLOAT projP[3], const RC_FLOAT p[3],
                      int width, int height) override;
    
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
    
    void SelectPoint3D(Point *p);
    void SelectPoints3D(vector<Point> *points);
    
    void ProjectPoints(vector<Point> *slices, int width, int height);
    void ProjectPoints(const vector<Point> &points,
                       vector<Point2D> *points2D,
                       int width, int height);
    
    void RayCast(const vector<Point> &points,
                 vector<Point2D> *result2DPoints,
                 int width, int height);
    
    // Ray casting methods
    //
    void DoRayCastGrid(const vector<Point> &points,
                       const vector<Point2D> &projPoints,
                       vector<Point2D> *resultPoints,
                       int width, int height);
    
    void DoRayCastQuadTree(const vector<Point> &points,
                           const vector<Point2D> &projPoints,
                           vector<Point2D> *resultPoints,
                           int width, int height);
    
    void DoRayCastKdTree(const vector<Point> &points,
                         const vector<Point2D> &projPoints,
                         vector<Point2D> *resultPoints,
                         int width, int height);
    
    
    void ComputePackedPointColor(const RayCaster2::Point &p, NVGcolor *color);
    
    void ThresholdPoints(vector<Point> *points);
    void ClipPoints(vector<Point> *points);
    void ApplyColorMap(vector<Point> *points);
    void PreMultAlpha(vector<Point> *points);
    
    void UpdateSlicesZ();
    
    void SlicesToVec(vector<RayCaster2::Point> *res,
                     const deque<vector<RayCaster2::Point> > &slices);
    
    void SlicesToVecPreSort(vector<RayCaster2::Point> *res,
                            deque<vector<RayCaster2::Point> > &slices);
    void SortVec(vector<Point> *vec, int signX, int signY, int signZ);
    void SortVecMiddles(vector<Point> *vec,
                        RC_FLOAT middleX, RC_FLOAT middleY, RC_FLOAT middleZ);

    
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
    void BlendDepthSortingBTF(unsigned char rgba[4], vector<Point> &points0);
    
    // Front to back version
    void BlendDepthSortingFTB(unsigned char rgba[4], vector<Point2D> &points0,
                              bool forceScreenSort = false);
    
    
    // Use the method described here:
    // http://jcgt.org/published/0002/02/09/
    // and here: https://habr.com/en/post/457292/
    void BlendWBOIT(int rgba[4], vector<Point> &points0);
    
    //
    void RenderQuads(NVGcontext *vg, const vector<Point2D> &points);

    void RenderTexture(NVGcontext *vg, int width, int height,
                       const vector<Point2D> &points,
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

    
    // Set invertModel flag to multiply the camera position by the
    // inverse model matrix
    void ComputeCameraPosition(RC_FLOAT camPos[3], bool invertModel);
    
    // Returns the index of the slice that is the most aligned to the view
    // if we use the Z direction.
    // sign is the computed sign of the area
    // alignedCoord is the computed coordinate of the most aligned coord
    // We can process only 1 y value, if we set fixedCoord
    enum Axis
    {
        AXIS_X = 0,
        AXIS_Y,
        AXIS_Z
    };

    // Returns the index of the slice that is the most aligned to the view
    int ComputeMostAlignedSliceIdx(int width, int height, Axis axis,
                                   int *areaSign = NULL, RC_FLOAT *alignedCoord = NULL);
    //RC_FLOAT ComputeMostAlignedSliceIdx(int width, int height, Axis axis);
    int ComputeMostAlignedSliceIdxDicho(int width, int height, Axis axis);
    
    RC_FLOAT ComputeSliceArea(int width, int height, Axis axis,
                              RC_FLOAT coord, int *sign = NULL);
    
    void ComputeViewSigns(int *signX, int *signY, int *signZ);
    bool ViewSignsChanged(int signX, int signY, int signZ);
    
    //
    void ColorMapChanged();
    void AlphaChanged();
    void SelectionChanged();
    void ClipChanged();
    void ThresholdChanged();

    
    // DEBUG
    //
    
    // Highlight is the axis number if any
    void DBG_DrawSlice(NVGcontext *vg, int width, int height,
                       RC_FLOAT slice[4][3], int highlight = -1);
    void DBG_DrawSlices(NVGcontext *vg, int width, int height, Axis axis);
    
    void DBG_DrawSliceX(NVGcontext *vg, int width, int height, RC_FLOAT x);
    void DBG_DrawSliceY(NVGcontext *vg, int width, int height, RC_FLOAT y);
    void DBG_DrawSliceZ(NVGcontext *vg, int width, int height, RC_FLOAT z);
    
    void DBG_DumpPoints(const char *name, const vector<Point> &points);
    
    // Fill the slices with empty data
    // (useful at the bginning, to avoid jittering)
    void FillEmptySlices();
    
    void SetPointIds(vector<Point> *points);

    
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
    vector<Point> mTmp3DPoints;
    
    vector<Point2D> mTmp2DPointsGrid;
    vector<Point2D> mTmp2DPointsProj;
    
    // For grid rendering
    //
    // Optim: GOOD (optim x2)
    // Do not re-allocate the grid each time
    struct GridCell
    {
        GridCell() { mNeedScreenSortFlag = false; };
        
        static void ResetAll(vector<vector<GridCell> > &grid)
        {
            for (int i = 0; i < grid.size(); i++)
                for (int j = 0; j < grid[i].size(); j++)
                {
                    grid[i][j].mPoints.clear();
                    grid[i][j].mNeedScreenSortFlag = false;
                }
        }
        
        vector<Point2D> mPoints;
        
        bool mNeedScreenSortFlag;
    };
    
    // Tmp grid, to avoid allocate it each time
    vector<vector<GridCell> > mTmpGrid;
    
    
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
    
    // To check if view sign just changed
    // (and then re-compute the sorting)
    int mPrevViewSigns[3];
    
#if PROFILE_RENDER
    BlaTimer mTimer0;
    long mCount0;
#endif
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__RayCaster2__) */
