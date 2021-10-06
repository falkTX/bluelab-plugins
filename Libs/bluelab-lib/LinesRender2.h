//
//  LineRender2.h
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_Waves__LineRender2__
#define __BL_Waves__LineRender2__

#ifdef IGRAPHICS_NANOVG

#include <vector>
#include <deque>
using namespace std;

#include <bl_queue.h>

#include <GraphControl12.h>
#include <Axis3D.h>

// Modified the max fov to have the axis
// in the view when exactly in front
#define MIN_FOV 15.0

// Don't forget to set in the plugin if it is a saved parameter
//#define MAX_FOV 45.0
#define MAX_FOV 52.0

// From LinesRender
// - added points color
//
class LinesRender2 : public GraphCustomDrawer,
                     public PointProjector
{
public:
    enum Mode
    {
        LINES_FREQ,
        LINES_TIME,
        POINTS,
        GRID
    };
    
    enum ScrollDirection
    {
        BACK_FRONT = 0,
        FRONT_BACK
    };
    
    class Point
    {
    public:
        Point()
        {
            mSkipDisplayX = false;
            mSkipDisplayZ = false;

            //
            mX = 0.0;
            mY = 0.0;
            mZ = 0.0;
        
            mSize = 1.0;
        
            mR = 0;
            mG = 0;
            mB = 0;
            mA = 0;
        
            mId = 0;

            mLinkedId = -1;
        }

        Point(const Point &other)
        {
            mSkipDisplayX = other.mSkipDisplayX;
            mSkipDisplayZ = other.mSkipDisplayZ;

            //
            mX = other.mX;
            mY = other.mY;
            mZ = other.mZ;
        
            mSize = other.mSize;
        
            mR = other.mR;
            mG = other.mG;
            mB = other.mB;
            mA = other.mA;
        
            mId = other.mId;

            mLinkedId = other.mLinkedId;
        }
        
        virtual ~Point() {}

        // For SASViewer
        static bool IsZLEqZero(const Point &p) { return p.mZ <= 0.0; }

        static bool IdLess(const Point &p1, const Point &p2)
        { return (p1.mId < p2.mId); }
        
        //
        BL_FLOAT mX;
        BL_FLOAT mY;
        BL_FLOAT mZ;
        
        BL_FLOAT mSize;
        
        unsigned char mR;
        unsigned char mG;
        unsigned char mB;
        unsigned char mA;
        
        // For displaying partial tracking as lines
        int mId;
        
        // For straight lines optimization
        bool mSkipDisplayX;
        bool mSkipDisplayZ;

        // For SASViewer
        int mLinkedId;
    };
    
    struct Line
    {
        void ComputeIds()
        { mId = -1;
          if (!mPoints.empty()) mId = mPoints[0].mId; }
        // For SASViewer
        static bool IsPointsEmpty(const Line &line) { return line.mPoints.empty(); }

        static bool IdLess(const Line &l1, const Line &l2)
        { return (l1.mId < l2.mId); }
        
        vector<LinesRender2::Point> mPoints;
        unsigned char mColor[4];

        // For SASViewer
        int mId;
        int mLinkedId;
    };
    
    LinesRender2();
    
    virtual ~LinesRender2();

    void SetUseLegacyLock(bool flag);

    void OnUIClose() override;
    
    void ProjectPoint(BL_FLOAT projP[3], const BL_FLOAT p[3],
                      int width, int height) override;
    
    void Init();

    void SetNumSlices(long numSlices);
    
    long GetNumSlices();
    
    // NEW
    void SetDensePointsFlag(bool flag);
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height) override;

    // LockFreeObj
    void PushData() override;
    void PullData() override;
    void ApplyData() override;
    
    void ClearSlices();
    bool MustAddSlice();
    void AddSlice(const vector<Point> &points);
    
    
    void SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1);
    
    BL_FLOAT GetCameraFov();
    void SetCameraFov(BL_FLOAT angle);
    
    void ZoomChanged(BL_FLOAT zoomChange);
    
    void ResetZoom();
    
    //
    void SetMode(Mode mode);
    
    void SetDensity(BL_FLOAT density);
    
    void SetScale(BL_FLOAT scale);

    void SetSpeed(BL_FLOAT speed);
    int GetSpeed();
    
    void SetScrollDirection(ScrollDirection dir);
    
    //
    void AddAxis(Axis3D *axis);
    void RemoveAxis(Axis3D *axis);
    
    void SetShowAxes(bool flag);
    
    void SetDBScale(bool flag, BL_FLOAT minDB);

    
    // Additional, unstrutured lines
    //
    // NOTE: was for PartialTracker debug
    void SetAdditionalLines(const vector<Line> &lines, BL_FLOAT lineWidth);
    void ClearAdditionalLines();
    void DrawAdditionalLines(NVGcontext *vg, int width, int height);
    void ProjectAdditionalLines(vector<Line> *lines, int width, int height);
    void ShowAdditionalLines(bool flag);
    
    // Optimized version
    void ProjectAdditionalLines2(vector<Line> *lines, int width, int height);

    void SetAdditionalPoints(const vector<Line> &lines, BL_FLOAT lineWidth);
    void ClearAdditionalPoints();
    void DrawAdditionalPoints(NVGcontext *vg, int width, int height);
    void ProjectAdditionalPoints(vector<Line> *lines, int width, int height);
    void ShowAdditionalPoints(bool flag);
    
    // To force recomputing projection when gui size changes for example
    // and while sound is not playing
    void SetDirty();
    
    void SetColors(unsigned char color0[4], unsigned char color1[4]);
    
    void DBG_ForceDensityNumSlices();
    void DBG_SetDisplayAllSlices();
    void DBG_FixDensityNumSlices();
    
protected:
    void AddSliceLF(const vector<Point> &points);
    
    void ProjectPoints(vector<Point> *slices, int width, int height);
    
    // GOOD method, do it the best way
    void ProjectSlices(vector<vector<Point> > *points,
                       const bl_queue<vector<Point> > &slices,
                       int width, int height);
    
    // For debugging: display all
    void ProjectSlicesNoDecim(vector<vector<Point> > *points,
                              const bl_queue<vector<Point> > &slices,
                              int width, int height);

    
    void RenderLines(vector<Point> *slices, int width, int height);
    
    void DoRenderLines(vector<Point> *slices, int width, int height);
    
    void DrawPoints(NVGcontext *vg, const vector<vector<Point> > &points);

    void DrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DrawGrid(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                      unsigned char inColor[4], BL_FLOAT pointSize = -1.0);

    void DoDrawPointsSimple(NVGcontext *vg, const vector<Point> &points,
                            BL_FLOAT pointSize = -1.0);
    
    void DoDrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points,
                         unsigned char inColor[4], BL_FLOAT lineWidth);
    
    void DoDrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points,
                         unsigned char inColor[4], BL_FLOAT lineWidth);

    void DoDrawLineSimple(NVGcontext *vg, const vector<Point> &points,
                          unsigned char inColor[4], BL_FLOAT lineWidth);
        
    void DoDrawGrid(NVGcontext *vg, const vector<vector<Point> > &points,
                    unsigned char inColor[4], BL_FLOAT lineWidth);
    
    // Draw using each point color
    void DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                      BL_FLOAT pointSize);
    
    // Optim
    void DecimateStraightLine(vector<Point> *points);
    // Optim2
    void OptimStraightLineX(vector<vector<Point> > *points);
    void OptimStraightLineZ(vector<vector<Point> > *points);
    
    
    //
    //deque<vector<Point> > mSlices;
    bl_queue<vector<Point> > mSlices;
    
    //
    
    BL_FLOAT mCamAngle0;
    BL_FLOAT mCamAngle1;
    
    BL_FLOAT mCamFov;
    
    long mNumSlices;
    
    // Dummy image of 1 white pixel
    // for quad rendering
    //
    // NOTE: rendering quads with or without texture: same perfs
    //
    int mWhitePixImg;
    
    Mode mMode;
    
    int mDensityNumSlices;
    
    BL_FLOAT mScale;
    
    ScrollDirection mScrollDirection;
    
    int mSpeed;
    long int mAddNum;
    // Num lines really added
    long int mNumLinesAdded;
    
    // Optim: GOOD (optim x2)
    // Do not re-allocate points each time
    // Keep a buffer
    vector<LinesRender2::Point> mTmpPoints;
    
    // Optim: GOOD (optim x2)
    // Do not re-allocate the grid each time
    vector<vector<vector<Point> > > mTmpGrid;
    
    int mViewWidth;
    int mViewHeight;
    
    // Set to false if we fill with sparts points
    bool mDensePointsFlag;
    
    bool mDisplayAllSlices;
    
    vector<Axis3D *> mAxis;
    bool mShowAxes;
    
    // Log scale
    bool mDBScale;
    BL_FLOAT mMinDB;
    
    // Additional lines
    vector<Line> mAdditionalLines;
    bool mShowAdditionalLines;
    BL_FLOAT mAdditionalLinesWidth;

    // Additional points
    vector<Line> mAdditionalPoints;
    bool mShowAdditionalPoints;
    BL_FLOAT mAdditionalPointsWidth;
    
    // NOTE: this doesn't optimize
    // Optim
    // Recompute the projection only if something changed
    vector<vector<Point> > mPrevProjPoints;
    bool mMustRecomputeProj;
    
    // Colors
    unsigned char mColor0[4];
    unsigned char mColor1[4];
    
    // Debug
    bool mDbgForceDensityNumSlices;

    // Lock free
    struct Slice
    {
        vector<Point> mPoints;
    };
    
    LockFreeQueue2<Slice> mLockFreeQueues[LOCK_FREE_NUM_BUFFERS];

    bool mNeedRedraw;

    bool mUseLegacyLock;

    NVGcontext *mVg;
    
private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    vector<Point> mTmpBuf2;
    vector<vector<Point> > mTmpBuf3;
    bl_queue<vector<Point> > mTmpBuf4;
    vector<Point> mTmpBuf5;
    vector<Point> mTmpBuf6;
    vector<Point> mTmpBuf7;
    vector<Point> mTmpBuf8;
    vector<Point> mTmpBuf9;
    vector<Point> mTmpBuf10;
    vector<Point> mTmpBuf11;
    vector<Line> mTmpBuf12;
    vector<vector<Point> > mTmpBuf13;
    Line mTmpLine;
    vector<Point> mTmpBuf14;
    Slice mTmpBuf15;
    Slice mTmpBuf16;
    vector<Line> mTmpBuf17;
    vector<Point> mTmpBuf18;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__RayCaster__) */
