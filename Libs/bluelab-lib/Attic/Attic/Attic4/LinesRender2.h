//
//  LineRender2.h
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_Waves__LineRender2__
#define __BL_Waves__LineRender2__

#include <vector>
#include <deque>
using namespace std;

// #bl-iplug2
//#include "nanovg.h"

#include <GraphControl11.h>

#include <Axis3D.h>

#define PROFILE_RENDER 0

#if PROFILE_RENDER
#include <BlaTimer.h>
#endif

// Modified the max fov to have the axis
// in the view when exactly in front
#define MIN_FOV 15.0

// Don't forget to set in the plugin if it is a saved parameter
//#define MAX_FOV 45.0
#define MAX_FOV 52.0

//#define MAX_FOV 90.0 // For debugging

// Use mutex with more granularity
// (to avoid locking the audio thread for a too long time)
//#define USE_OWN_MUTEX 1

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
        }
        
        virtual ~Point() {}
        
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
    };
    
    LinesRender2();
    
    virtual ~LinesRender2();
    
    // Inherited
    //virtual bool HasOwnMutex();
    
    void ProjectPoint(BL_FLOAT projP[3], const BL_FLOAT p[3], int width, int height);
    
    void Init();

    
    void SetNumSlices(long numSlices);
    
    long GetNumSlices();
    
    // NEW
    void SetDensePointsFlag(bool flag);
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height);
    
    void ClearSlices();
    
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
    
    void SetShowAxis(bool flag);
    
    void SetDBScale(bool flag, BL_FLOAT minDB);

    
    // Additional, unstrutured lines
    //
    // NOTE: was for PartialTracker debug
    void SetAdditionalLines(const vector<vector<LinesRender2::Point> > &lines,
                            unsigned char color[4], BL_FLOAT lineWidth);
    
    void ClearAdditionalLines();
    
    void ShowAdditionalLines(bool flag);
    
    void DrawAdditionalLines(NVGcontext *vg, int width, int height);

    void ProjectAdditionalLines(vector<vector<LinesRender2::Point> > *lines,
                                int width, int height);
    
    // Optimized version
    void ProjectAdditionalLines2(vector<vector<LinesRender2::Point> > *lines,
                                 int width, int height);
    
    // To force recomputing projection when gui size changes for example
    // and while sound is not playing
    void SetDirty();
    
    void SetColors(unsigned char color0[4], unsigned char color1[4]);
    
    //
    void DBG_SetDisplayAllSlices(bool flag);
    
protected:
    void ProjectPoints(vector<Point> *slices, int width, int height);
    
    void ProjectSlices(vector<vector<Point> > *points,
                       const deque<vector<Point> > &slices,
                       int width, int height);
    
    // For debugging: display all
    void ProjectSlicesNoDecim(vector<vector<Point> > *points,
                              const deque<vector<Point> > &slices,
                              int width, int height);

    
    void RenderLines(vector<Point> *slices, int width, int height);
    
    void DoRenderLines(vector<Point> *slices, int width, int height);
    
    // Disabled because it seemed unused
#if 0
    void UpdateSlicesZ();
#endif
    
    static void DequeToVec(vector<LinesRender2::Point> *res,
                           const deque<vector<LinesRender2::Point> > &que);
    
    
    void DrawPoints(NVGcontext *vg, const vector<vector<Point> > &points);

    void DrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DrawGrid(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                      unsigned char inColor[4], BL_FLOAT pointSize = -1.0);
    
    void DoDrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points,
                         unsigned char inColor[4], BL_FLOAT lineWidth);
    
    void DoDrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points,
                         unsigned char inColor[4], BL_FLOAT lineWidth);
    
    void DoDrawGrid(NVGcontext *vg, const vector<vector<Point> > &points,
                    unsigned char inColor[4], BL_FLOAT lineWidth);
    
    // NEW
    // Draw using each point color
    void DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                      BL_FLOAT pointSize);
    
    // Optim
    void DecimateStraightLine(vector<Point> *points);
    // Optim2
    void OptimStraightLineX(vector<vector<Point> > *points);
    void OptimStraightLineZ(vector<vector<Point> > *points);
    
    
    deque<vector<Point> > mSlices;
    
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
    bool mShowAxis;
    
    // Log scale
    bool mDBScale;
    BL_FLOAT mMinDB;
    
    // Additional lines
    vector<vector<LinesRender2::Point> > mAdditionalLines;
    bool mShowAdditionalLines;
    unsigned char mAdditionalLinesColor[4];
    BL_FLOAT mAdditionalLinesWidth;
    
    // NOTE: this doesn't optimize
    // Optim
    // Recompute the projection only if something changed
    vector<vector<Point> > mPrevProjPoints;
    bool mMustRecomputeProj;
    
    // Colors
    unsigned char mColor0[4];
    unsigned char mColor1[4];
    
#if PROFILE_RENDER
    BlaTimer mTimer0;
    long mCount0;
#endif
};

#endif /* defined(__BL_StereoWidth__RayCaster__) */
