//
//  RayCaster.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#include <algorithm>
using namespace std;

// Does not optimize...
#define PROJECT_POINT_SIMD 0 //1
#if PROJECT_POINT_SIMD

// Change to glm-0.9.9.7 (original: glm-master)
// Change clang "Enable additional vector extension": AVX (original: Default)

// SIMD
#define USE_NEWEST_GLM 1 //0 //1
#if USE_NEWEST_GLM

// Mac only
#undef __apple_build_version__
#define GLM_FUNC_QUALIFIER
#define GLM_FUNC_DECL

#endif

// See: https://glm.g-truc.net/0.9.1/api/a00285.html
// and: https://gamedev.stackexchange.com/questions/132549/how-to-use-glm-simd-using-glm-version-0-9-8-2


// NOTE: original version: glm-master
// NOTE: changed clang "Enable additional vector extension": Default -> AVX
// NOTE: rotating the view doesn't work well with glm-0.9.9.7

//#define GLM_FORCE_AVX2 // interesting

//#define GLM_COMPILER 0

//#define GLM_FORCE_COMPILER_UNKNOWN
//#define GLM_COMPILER GLM_COMPILER_CLANG

//#define GLM_COMPILER_CLANG 1

//#define GLM_FORCE_SIMD_AVX2
//#define GLM_FORCE_AVX4

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_SSE2

#define GLM_FORCE_SIMD_AVX2 // TEST

#include <glm/simd/matrix.h>

#endif // PROJECT_POINT_SIMD

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <glm/gtx/intersect.hpp>

#include <ColorMap4.h>

#include <Utils.h>
#include <Debug.h>

#include <ImageDisplayColor.h>

#include <UnstableRemoveIf.h>

#include <Axis3D.h>

#include <RCQuadTree.h>
#include <RCQuadTreeVisitor.h>

#include <RCKdTree.h>
#include <RCKdTreeVisitor.h>


#include "RayCaster.h"

#define DISABLE_DISPLAY_REFRESH_RATE 1

#define TRANSPARENT_COLORMAP 1

#define RESOLUTION_0 32
#define RESOLUTION_1 512

// Prev: when unlselected only darkened the color
//#define SELECT_GRAY_COLOR_COEFF 0.5
// New: no unselect makes more transparent (cooler feature !)
#define SELECT_GRAY_COLOR_COEFF 0.1
// TEST to be less transparent (not so good)
//#define SELECT_GRAY_COLOR_COEFF 0.15

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// Select points in 2D or in  3D ?
#define SELECT_POINTS_3D 1

// 1: sort
// 2: take min Z
// 3: blend points
#define SORT_POINTS_METHOD1 0
#define SORT_POINTS_METHOD2 0
#define SORT_POINTS_METHOD3 1

#if SORT_POINTS_METHOD3
//#define REVERT_DISPLAY_ORDER 1
//#define BLENDING_COEFF 0.25 // ORIG
#define BLENDING_COEFF 0.75 //0.5 //0.25
#endif

// For first pixel: put the full color, or blend with black ?
#define BLEND_ALSO_FIRST_COLOR 1

// Not sure it changes anything...
#define ROUND_POINT_COORDS 1

// Avoid changing point size when changin quality
#define FIXED_POINT_SIZE 1

#define MIN_POINT_SIZE 1.0
#define MAX_POINT_SIZE 24.0

// Quick sort depending on the view direction,
// and considering we have a cube of voxels
// (instead of sorting each time over each ray)
//
// NOTE: does not work
// (for different slices, the y are not sorted well, because
// we sort slice by slice)
//
// => May be improved later...
#define OPTIM_PRE_SORT 0 //1

// VERY GOOD!
// Render on a texture instead of on many quads
// RESULTS:
// - with standard config, goes from 90% CPU to 80% CPU
// - now we can use higher xy quality with good perfs
#define RENDER_TEXTURE 1

// VERY GOOD!
// ThresholdPoints() now seems to cost almost nothing
// (before, it was a bottleneck)
// (that was due to duplicating very big buffers of data)
#define THRESHOLD_OPTIM2 1

// VERY GOOD!
// => gain around 10%
//
// Keep only a resaonable number of points over each ray
// (if we have too many points, farthest point don't change a lot the result color)
// (and it will optimize depth sorting for blending)
//
// NOTE: when MAX_NUM_RAY_POINTS is low, NUM_DICHO_ITERATIONS must be high enough
#define DISCARD_FARTHEST_RAY_POINTS 1
#define MAX_NUM_RAY_POINTS 32 //4 //32 //5 //100 //10 //100
#define NUM_DICHO_ITERATIONS 3 //8 //3 //8 //4 //3 //8 //4 //3

// Apply as many thing as possible after having discarded farthest ray points

// GOOD !
// Apply colormap after discard farthest
#define OPTIM_AFTER_DISCARD_FARTHEST 1

// BAD (worse preformances, better to threshold before)
// Apply threshold after discard farthest
#define OPTIM_AFTER_DISCARD_FARTHEST2 0 // 1

// NOTE: does not realy optimize
#define OPTIM_REMOVE_IF 1

#define FIX_JITTER_TIME_QUALITY 1

// FIX: To be able to play samples in volume selection
// NOTE: also defined in SMVProcess4
#define FIX_PLAY_SELECTION 1

// Selection play over time axis was reversed
#define FIX_PLAY_SELECTION_REVERSED 1

// FIX: at startup, when playing, the computed center slice num varied
// (so the sound was played kind of "slow")
#define FIX_SLOW_PLAY_AT_STARTUP 1

// When increasing transparency, set more transparent the voxels with lower weights
// (so we can find zone with high weights inside a volume)
#define BLEND_LESS_HIGH_WEIGHTS 1

// Sometimes it crashed when selecting in 2D
#define FIX_SELECT_CRASH 1

// For pre and post rendering of x axis
// Arbitrary value, dending on the current camera pararameters
// May be changed if we modify camera parameters
#define AXIS_X_CAM_ANGLE0_LIMIT 70.0
#define AXIS_X_CAM_ANGLE1_LIMIT 70.0

#define AXIS_Y_CAM_ANGLE1_LIMIT 70.0

#define AXIS_Z_CAM_ANGLE0_LIMIT 20.0
#define AXIS_Z_CAM_ANGLE1_LIMIT 70.0

// VERY GOOD
// New alpha blending
// Add alpha value to the rendered pixels
// This way, we can see partially axes through the raycast texture (very good)
// And we also have a better blending, when setting more transparent,
// the high density zones stay more visible
#define ALPHA_BLEND_RAYCASTED_TEXTURE 1

// VERY GOOD!
// FIX: avoid that the pixels from ray with only one points are too dark,
// and still enable to see axes behind semi-transparent pixels.
// (this was because after blend depth sorting computation,
// we blend the texture over the axes - this is an additional blending)
#define FIX_BLEND_DEPTH_BG_ALPHA 1

// Decrease the quality when zooming in ?
// => now possible to avoid black gaps automatically !
#define ADAPTIVE_QUALITY_ZOOM 1

// REM: if changing it, maybe look to change PROJECT_POINT_SIMD
#define PROJECT_POINT_ORIG 0
#define PROJECT_POINT_OPTIM 1

static void
CameraModelProjMat(int winWidth, int winHeight, float angle0, float angle1,
                   float perspAngle,
                   glm::mat4 *model, glm::mat4 *proj)
{
    glm::vec3 pos(0.0f, 0.0, -2.0f);
    glm::vec3 target(0.0f, 0.37f, 0.0f);
    
    glm::vec3 up(0.0, 1.0f, 0.0f);
    
    glm::vec3 lookVec(target.x - pos.x, target.y - pos.y, target.z - pos.z);
    float radius = glm::length(lookVec);
    
    angle1 *= 3.14/180.0;
    float newZ = cos(angle1)*radius;
    float newY = sin(angle1)*radius;
    
    // Seems to work (no need to touch up vector)
    pos.z = newZ;
    pos.y = newY;
    
    glm::mat4 viewMat = glm::lookAt(pos, target, up);
    
    glm::mat4 perspMat = glm::perspective(perspAngle/*45.0f*//*60.0f*/,
                                          ((float)winWidth)/winHeight, NEAR_PLANE, FAR_PLANE);
    
    glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
    
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f)); //
    modelMat = glm::rotate(modelMat, angle0, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.33/*0.0f*/, 0.0f));
    
    *model = viewMat * modelMat;
    *proj = perspMat;
}

static void
CameraUnProject(int winWidth, int winHeight, float angle0, float angle1,
                float camFov,
                const double p2[3], double p3[3])
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(winWidth, winHeight, angle0, angle1,
                       camFov,
                       &model, &proj);
    
    glm::vec3 win(p2[0], p2[1], p2[2]);
    
    glm::vec4 viewport(0, 0, winWidth, winHeight);
    glm::vec3 world = glm::unProject(win, model, proj, viewport);
    
    p3[0] = world.x;
    p3[1] = world.y;
    p3[2] = world.z;
}

// See: https://gamedev.stackexchange.com/questions/142918/intersecting-triangle-and-boundingbox-in-3d-environment
//

//////////////////////////////////////////////////////
//    use separating axis theorem to test overlap
//    between triangle and box
//    need to test for overlap in these directions:
//    1) the {x,y,z}-directions (actually, since we use
//       the AABB of the triangle
//       we do not even need to test these)
//    2) normal of the triangle
//    3) crossproduct(edge from tri, {x,y,z}-directin)
//       this gives 3x3=9 more tests
#define OUTSIDE   -1
#define INTERSECT  0
#define INSIDE     1
static int
TriangleVsAABB2(const glm::vec3 &bmin,
                const glm::vec3 &bmax,
                const glm::vec3 &boxcenter,
                const glm::vec3 &boxhalfsize,
                const glm::vec3 &trivert0,
                const glm::vec3 &trivert1,
                const glm::vec3 &trivert2,
                const glm::vec3 &normal)
{
    
    // if triangle is entirely inside the bounding box, then exit
    
    if ((trivert0.x >= bmin.x && trivert0.x <= bmax.x) &&
        (trivert0.y >= bmin.y && trivert0.y <= bmax.y) &&
        (trivert0.z >= bmin.z && trivert0.z <= bmax.z))
        if ((trivert1.x >= bmin.x && trivert1.x <= bmax.x) &&
            (trivert1.y >= bmin.y && trivert1.y <= bmax.y) &&
            (trivert1.z >= bmin.z && trivert1.z <= bmax.z))
            if ((trivert2.x >= bmin.x && trivert2.x <= bmax.x) &&
                (trivert2.y >= bmin.y && trivert2.y <= bmax.y) &&
                (trivert2.z >= bmin.z && trivert2.z <= bmax.z))
                return INSIDE;
    
    // if we get here triangle maybe intersecting the cube or could be totally out
    
    float d,min,max;
    float p0, p1, p2;
    glm::vec3 fe,vmin,vmax;
    glm::vec3 v0,v1,v2;
    glm::vec3 e0, e1, e2;
    
    // center at 0,0,0
    
    v0 = trivert0 - boxcenter;
    v1 = trivert1 - boxcenter;
    v2 = trivert2 - boxcenter;
    
    // compute triangle edges
    
    e0 = v1 - v0;
    e1 = v2 - v1;
    e2 = v0 - v2;
    
    //  test the 9 tests for axis overlapping
    
    // axis test x01
    
    fe = glm::abs(e0);
    
    p0 = e0.z * v0.y - e0.y * v0.z;
    p2 = e0.z * v2.y - e0.y * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2)
    {
        min = p0;
        max = p2;
    }
    
    d = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test y02
    
    p0 = -e0.z * v0.x + e0.x * v0.z;
    p2 = -e0.z * v2.x + e0.x * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2) { min = p0; max = p2; }
    
    d = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test z12
    
    p1 = e0.y * v1.x - e0.x * v1.y;
    p2 = e0.y * v2.x - e0.x * v2.y;
    
    min = p1;
    max = p2;
    
    if (p2<p1)
    {
        min = p2;
        max = p1;
    }
    
    d = fe.y * boxhalfsize.x + fe.x * boxhalfsize.y;
    
    if (min>d || max<-d) return OUTSIDE;
    
    fe = glm::abs(e1);
    
    // axis test x01
    
    p0 = e1.z * v0.y - e1.y * v0.z;
    p2 = e1.z * v2.y - e1.y * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2)
    {
        min = p0;
        max = p2;
    }
    
    d = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test y02
    
    p0 = -e1.z * v0.x + e1.x * v0.z;
    p2 = -e1.z * v2.x + e1.x * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2)
    {
        min = p0;
        max = p2;
    }
    
    d = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test z0
    
    p0 = e1.y * v0.x - e1.x * v0.y;
    p1 = e1.y * v1.x - e1.x * v1.y;
    
    min = p1;
    max = p0;
    
    if (p0<p1)
    {
        min = p0;
        max = p1;
    }
    
    d = fe.y* boxhalfsize.x + fe.x * boxhalfsize.y;
    
    if (min>d || max<-d) return OUTSIDE;
    
    fe = glm::abs(e2);
    
    // axis test x02
    
    p0 = e2.z * v0.y - e2.y * v0.z;
    p1 = e2.z * v1.y - e2.y * v1.z;
    
    min = p1;
    max = p0;
    
    if (p0<p1)
    {
        min = p0;
        max = p1;
    }
    
    d = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test y01
    
    p0 = -e2.z * v0.x + e2.x * v0.z;
    p1 = -e2.z * v1.x + e2.x * v1.z;
    
    min = p1;
    max = p0;
    
    if (p0<p1)
    {
        min = p0;
        max = p1;
    }
    
    d = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test z12
    
    p1 = e2.y * v1.x - e2.x * v1.y;
    p2 = e2.y * v2.x - e2.x * v2.y;
    
    min = p1;
    max = p2;
    
    if (p2<p1)
    {
        min = p2;
        max = p1;
    }
    
    d = fe.y * boxhalfsize.x + fe.x * boxhalfsize.y;
    
    if (min>d || max<-d) return 0;
    
    //  first test overlap in the {x,y,z}-directions
    //  find min, max of the triangle each direction,
    //  and test for overlap in
    //  that direction -- this is equivalent to testing
    //  a minimal AABB around
    //  the triangle against the AABB
    
    // test in X-direction
    
    min = v0.x;
    max = v0.x;
    
    if (v1.x<min) min = v1.x;
    if (v1.x>max) max = v1.x;
    if (v2.x<min) min = v2.x;
    if (v2.x>max) max = v2.x;
    
    if (min>boxhalfsize.x || max<-boxhalfsize.x) return OUTSIDE;
    
    // test in Y-direction
    
    min = v0.y;
    max = v0.y;
    
    if (v1.y<min) min = v1.y;
    if (v1.y>max) max = v1.y;
    if (v2.y<min) min = v2.y;
    if (v2.y>max) max = v2.y;
    
    if (min>boxhalfsize.y || max<-boxhalfsize.y) return OUTSIDE;
    
    // test in Z-direction
    
    min = v0.z;
    max = v0.z;
    
    if (v1.z<min) min = v1.z;
    if (v1.z>max) max = v1.z;
    if (v2.z<min) min = v2.z;
    if (v2.z>max) max = v2.z;
    
    if (min>boxhalfsize.z || max<-boxhalfsize.z) return OUTSIDE;
    
    //  test if the box intersects the plane of the triangle
    
    // test x
    
    if (normal.x>0.0f)
    {
        vmin.x = -boxhalfsize.x;
        vmax.x =  boxhalfsize.x;
    }
    else
    {
        vmin.x =  boxhalfsize.x;
        vmax.x = -boxhalfsize.x;
    }
    
    // test y
    
    if (normal.y>0.0f)
    {
        vmin.y = -boxhalfsize.y;
        vmax.y =  boxhalfsize.y;
    }
    else
    {
        vmin.y =  boxhalfsize.y;
        vmax.y = -boxhalfsize.y;
    }
    
    // test z
    
    if (normal.z>0.0f)
    {
        vmin.z = -boxhalfsize.z;
        vmax.z =  boxhalfsize.z;
    }
    else
    {
        vmin.z =  boxhalfsize.z;
        vmax.z = -boxhalfsize.z;
    }
    
    // test max and min
    
    d = -(normal.x * v0.x + normal.y * v0.y + normal.z * v0.z);
    
    if (normal.x * vmin.x + normal.y * vmin.y + normal.z * vmin.z + d >  0.0f) return OUTSIDE;
    
    if (normal.x * vmax.x + normal.y * vmax.y + normal.z * vmax.z + d >= 0.0f) return INTERSECT;
    
    // box and triangle don't overlap
    
    return OUTSIDE;
}

//

RayCaster::RayCaster(int numSlices, int numPointsSlice)
{
    mColorMap = NULL;
    
    SetColorMap(0);
    
    mInvertColormap = false;
    
    mNumSlices = numSlices;
    
    // Selection
    ResetSelection();
    
    mWhitePixImg = -1;
    
    mDisplayRefreshRate = 1;
    mRefreshNum = 0;
    
    mQuality = 1.0;
    mAutoQuality = 1.0;
    
    mQualityT = 1.0;
    
    mPointSize = MIN_POINT_SIZE/*1.0*/;
    //mPointSize = -1;
    
    //mAlphaScale = 1.0;
    mAlphaCoeff = BLENDING_COEFF;
    
    mThreshold = 0.0;
    
    mClipFlag = false;
    
    // Keep internally
    mViewWidth = 0;
    mViewHeight = 0;
    
    mScrollDirection = RayCaster::FRONT_BACK;
    
    mImage = NULL;
    
#if FIX_JITTER_TIME_QUALITY
    mSliceNum = 0;
#endif
    
    mWidth = 1;
    mHeight = 1;
    
    mFirstTimeRender = true;
    
    mPlayBarPos = -1.0;
    
    for (int i = 0; i < NUM_AXES; i++)
        mAxis[i] = NULL;
    
    mQuadTree = NULL;
    mKdTree = NULL;
    mQualityChanged = true;
    
    mCamAngle0 = 0.0;
    mCamAngle1 = 0.0;
    
    mCamFov = DEFAULT_CAMERA_FOV;
    
    mAutoQualityFlag = true;
    
#if ADAPTIVE_QUALITY_ZOOM
    if (mAutoQualityFlag)
        AdaptZoomQuality();
#endif
    
    // Debug
    mRenderAlgo = RENDER_ALGO_GRID;
    mRenderAlgoParam = 0.0;
    
#if PROFILE_RENDER
    BlaTimer::Reset(&mTimer0, &mCount0);
#endif
}

RayCaster::~RayCaster()
{
    if (mColorMap != NULL)
        delete mColorMap;
    
    // Must use the ResetGL() mechanism.
    // As it is with simple delete, it crashes when quitting
    //if (mImage != NULL)
    //    delete mImage;
    
    if (mQuadTree != NULL)
        delete mQuadTree;
    
    if (mKdTree != NULL)
        delete mKdTree;
}

void
RayCaster::SetNumSlices(long numSlices)
{
    mNumSlices = numSlices;
}

long
RayCaster::GetNumSlices()
{
    return mNumSlices;
}

void
RayCaster::SetScrollDirection(RayCaster::ScrollDirection dir)
{
    mScrollDirection = dir;
    
#if 0 // Uncomment ?
    UpdateSlicesZ();
#endif
}

void
RayCaster::PreDraw(NVGcontext *vg, int width, int height)
{
#if PROFILE_RENDER
    BlaTimer::Start(&mTimer0);
#endif
    
    mViewWidth = width;
    mViewHeight = height;
    
    // Optim: GOOD
    // Do not re-allocate points each time
    // Keep a buffer
    
    //vector<Point> points;
    SlicesDequeToVec(&mTmpPoints/*&points*/, mSlices);
    vector<Point> &points = mTmpPoints;
    
    //long numPointsStart = points.size();
    
    RayCast(&points, width, height);
    
    //long numPointsEnd = points.size();
    
    // Draw axis before or after voxels
    // in order to manage occlusions well
    //
    
    bool preDrawFlags[NUM_AXES];
    ComputeAxisPreDrawFlags(preDrawFlags);
    
    DrawAxes(vg, width, height, 0, preDrawFlags);
    
    //
    nvgSave(vg);
    
#if !RENDER_TEXTURE
    RenderQuads(vg, points);
#else
    RenderTexture(vg, width, height, points);
#endif
    
    nvgRestore(vg);
    
    DrawAxes(vg, width, height, 1, preDrawFlags);
    
    DrawSelectionVolume(vg, width, height);
    
    DrawPlayBarPos(vg, width, height);
    
    // TODO: extract the drawing of the volume rendering in a separate function
    // (to have a clean PreDraw() )
    // Selection
    DrawSelection(vg, width, height);
        
#if PROFILE_RENDER
    BlaTimer::StopAndDump(&mTimer0, &mCount0,
                          "profile.txt",
                          "full: %ld ms \n");
#endif
}

void
RayCaster::ClearSlices()
{
    mSlices.clear();
}

void
RayCaster::SetSlices(const vector<vector<Point> > &slices)
{
    mSlices.clear();
    
    for (int i = 0; i < slices.size(); i++)
        mSlices.push_back(slices[i]);
    
    // Could be improved
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();
    
    UpdateSlicesZ();
}

void
RayCaster::AddSlice(const vector<Point> &points, bool skipDisplay)
{
    vector<Point> points0 = points;
    
#if OPTIM_PRE_SORT
    //sort(points0.begin(), points0.end(), Point::IsSmallerX);
    sort(points0.begin(), points0.end(), Point::IsSmallerY);
#endif
    
    if (skipDisplay)
        return;
    
#if !DISABLE_DISPLAY_REFRESH_RATE
    if (mRefreshNum++ % mDisplayRefreshRate != 0)
        // Don't add the points this time
        return;
#endif
    
    mSlices.push_back(points0);
    
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();
    
    UpdateSlicesZ();
    
#if FIX_JITTER_TIME_QUALITY
    mSliceNum++;
#endif
}

void
RayCaster::SetCameraAngles(double angle0, double angle1)
{
    mCamAngle0 = angle0;
    mCamAngle1 = angle1;
}

void
RayCaster::SetCameraFov(double camFov)
{
    mCamFov = camFov;
    
#if ADAPTIVE_QUALITY_ZOOM
    if (mAutoQualityFlag)
        AdaptZoomQuality();
#endif
}

void
RayCaster::ZoomChanged(double zoomChange)
{
    mCamFov *= 1.0/zoomChange;
    
    if (mCamFov < MIN_CAMERA_FOV)
        mCamFov = MIN_CAMERA_FOV;
    
    if (mCamFov > MAX_CAMERA_FOV)
        mCamFov = MAX_CAMERA_FOV;
    
#if ADAPTIVE_QUALITY_ZOOM
    if (mAutoQualityFlag)
        AdaptZoomQuality();
#endif
}

#if PROJECT_POINT_ORIG
void
RayCaster::ProjectPoints(vector<Point> *points, int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height, mCamAngle0, mCamAngle1,
                       mCamFov,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif

        // Matrix transform
        glm::vec4 v;
        v.x = p.mX;
        v.y = p.mY;
        v.z = p.mZ;
        v.w = 1.0;
        
        glm::vec4 v4 = modelProjMat*v;
        
        double x = v4.x;
        double y = v4.y;
        double z = v4.z;
        double w = v4.w;
        
#define EPS 1e-8
        if (fabs(w) > EPS)
        {
            // Optim
            double wInv = 1.0/w;
            x *= wInv;
            y *= wInv;
            z *= wInv;
        }
        
        // Do like OpenGL
        x = (x + 1.0)*0.5*width;
        y = (y + 1.0)*0.5*height;
        z = (z + 1.0f)*0.5;
        
        // Result
        p.mX = x;
        p.mY = y;
        p.mZ = z;
    }
}
#endif

// Avoid some temporary variables
#if PROJECT_POINT_OPTIM
void
RayCaster::ProjectPoints(vector<Point> *points, int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height, mCamAngle0, mCamAngle1,
                       mCamFov,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    //
    double halfWidth = 0.5*width;
    double halfHeight = 0.5*height;
    
    // Temporary vector
    glm::vec4 v;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif
        
        // Matrix transform
        v.x = p.mX;
        v.y = p.mY;
        v.z = p.mZ;
        v.w = 1.0;
        
        glm::vec4 v4 = modelProjMat*v;
        
#define EPS 1e-8
        if ((v4.w < -EPS) || (v4.w > EPS))
        {
            // Optim
            double wInv = 1.0/v4.w;
            v4.x *= wInv;
            v4.y *= wInv;
            v4.z *= wInv;
        }
        
        // Do like OpenGL
        v4.x = (v4.x + 1.0)*halfWidth;
        v4.y = (v4.y + 1.0)*halfHeight;
        v4.z = (v4.z + 1.0f)*0.5;
        
        // Result
        p.mX = v4.x;
        p.mY = v4.y;
        p.mZ = v4.z;
    }
}
#endif

#if PROJECT_POINT_SIMD
void
RayCaster::ProjectPoints(vector<Point> *points, int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height, mCamAngle0, mCamAngle1,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    float m0[16] = { modelProjMat[0][0], modelProjMat[0][1], modelProjMat[0][2], modelProjMat[0][3],
                     modelProjMat[1][0], modelProjMat[1][1], modelProjMat[1][2], modelProjMat[1][3],
                     modelProjMat[2][0], modelProjMat[2][1], modelProjMat[2][2], modelProjMat[2][3],
                     modelProjMat[3][0], modelProjMat[3][1], modelProjMat[3][2], modelProjMat[3][3] };
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        // Matrix transform (SIMD)
        
        //
        float v[4] = { p.mX, p.mY, p.mZ, 1.0 };
        glm_vec4 &v0 = *((glm_vec4 *)&v);
       
        //
        glm_vec4 v4 = glm_mat4_mul_vec4((glm_vec4 *)m0, v0);
        
        //
        double x = ((float *)&v4)[0];
        double y = ((float *)&v4)[1];
        double z = ((float *)&v4)[2];
        double w = ((float *)&v4)[3];
        
#define EPS 1e-8
        if (fabs(w) > EPS)
        {
            // Optim
            double wInv = 1.0/w;
            x *= wInv;
            y *= wInv;
            z *= wInv;
        }
        
        // Do like OpenGL
        x = (x + 1.0)*0.5*width;
        y = (y + 1.0)*0.5*height;
        z = (z + 1.0f)*0.5;
        
        // Result
        p.mX = x;
        p.mY = y;
        p.mZ = z;
    }
}
#endif

void
RayCaster::RayCast(vector<Point> *points, int width, int height)
{
    SetPointIds(points);

#if !OPTIM_AFTER_DISCARD_FARTHEST2
    ThresholdPoints(points);
#endif
    
#if !OPTIM_AFTER_DISCARD_FARTHEST
    ApplyColorMap(points);
#endif
    
#if SELECT_POINTS_3D
    SelectPoints3D(points);
#endif
    
    if (mClipFlag)
        ClipPoints(points);
    
    ProjectPoints(points, width, height);

    // NOTE: - when no selection, consume few additional performances
    //       - when (large) selection, consume as much as ProjectPoints()
#if !SELECT_POINTS_3D
    SelectPoints2D(points, width, height);
#endif
    
    PropagateSelection(*points);

    if ((mRenderAlgo == RENDER_ALGO_GRID) ||
        (mRenderAlgo == RENDER_ALGO_GRID_DILATE))
        DoRayCastGrid(points, width, height);
    else if (mRenderAlgo == RENDER_ALGO_QUAD_TREE)
        DoRayCastQuadTree(points, width, height);
    else if ((mRenderAlgo == RENDER_ALGO_KD_TREE_MEDIAN) ||
             (mRenderAlgo == RENDER_ALGO_KD_TREE_MIDDLE))
        // Interesting! (for avoiding black gaps between slices
        // But consumes resources.
        // And boundaries are not well managed maybe
        DoRayCastKdTree(points, width, height);
}

void
RayCaster::DoRayCastGrid(vector<Point> *points, int width, int height)
{
    mWidth = width;
    mHeight = height;
    
    // Hack
    if (mFirstTimeRender)
    {
#if !ADAPTIVE_QUALITY_ZOOM
        SetQuality(mQuality);
#else
        if (mAutoQualityFlag)
            AdaptZoomQuality();
        else
            SetQuality(mQuality);
#endif
        
        mFirstTimeRender = false;
    }
    
    double cellSize = ComputeCellSize(width, height);
    
    //
    int res = GetResolution();
    int resolution[2] = { res, res };
    
    double maxDim = (width > height) ? width : height;
    double bbox[2][2] = { { 0, 0 }, { maxDim, maxDim } };
    
    // Create the grid
    if (mTmpGrid.size() != resolution[0])
    {
        mTmpGrid.resize(resolution[0]);
        for (int i = 0; i < resolution[0]; i++)
        {
            mTmpGrid[i].resize(resolution[1]);
        }
    }
    
    // Empty the grid
    for (int i = 0; i < resolution[0]; i++)
    {
        for (int j = 0; j < resolution[1]; j++)
        {
            mTmpGrid[i][j].clear();
        }
    }
    
    vector<vector<vector<Point> > > &grid = mTmpGrid;
    
    // Fill the grid
    
    // Optim
    double xRatio = 1.0/((bbox[1][0] - bbox[0][0]));
    double yRatio = 1.0/((bbox[1][1] - bbox[0][1]));
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif
        
        double normX = (p.mX - bbox[0][0])*xRatio;
        double normY = (p.mY - bbox[0][1])*yRatio;
        
#if !ROUND_POINT_COORDS // Trunk
        int x = (int)(normX*resolution[0]);
        int y = (int)(normY*resolution[1]);
#else
        int x = (int)round(normX*resolution[0]);
        int y = (int)round(normY*resolution[1]);
#endif
        
        // Test to avoid crash when projected points are outside the screen
        if ((x >= 0) && (x < resolution[0]) &&
            (y >= 0) && (y < resolution[1]))
        {
            // Add the point
            grid[x][y].push_back(p);
        }
    }
    
    // Clear the result
    points->clear();
    
    // DEBUG
    //int dbgMaxNumCellPoints = 0;
    
    double x = 0;
    double y = 0;
    for (int i = 0; i < grid.size(); i++)
    {
        vector<vector<Point> > &line = grid[i];
        
        for (int j = 0; j < line.size(); j++)
        {
            vector<Point> &points0 = line[j];
            
            if (points0.empty())
            {
                y += cellSize;
                
                continue;
            }
            
#if SORT_POINTS_METHOD1 // Sort, then process the list (takes more resources)
      //
            // Get a single point from the point list for this cell
            sort(points0.begin(), points0.end(), Point::IsSmallerZ);
            
            for (int k = 0; k < points0.size(); k++)
            //for (int k = points0.size() - 1; k >= 0; k--)
            {
                Point &p = points0[k];
            
#if THRESHOLD_OPTIM2
                if (!p.mIsActive)
                    continue;
#endif

#define EPS_ALPHA 1
                // Except if it is transparent
                if (p.mA < EPS_ALPHA)
                    continue;
                
                //p.mX = i*cellSize;
                //p.mY = j*cellSize;
            
                // OPTIM (not very efficient)
                p.mX = x;
                p.mY = y;
                
                p.mSize = cellSize;
            
                points->push_back(p);
                
                // Simply get the nearest point (for the moment)
                break;
            }
#endif
            
            // OPTIM: GOOD (30% better)
            // Find the point with minimum Z
            // (instead of sorting)
            
#if SORT_POINTS_METHOD2 // Do not sort, take min z !
            
#define INF 1e15
            Point *minPoint = NULL;
            double minZ = INF;
            
            for (int k = 0; k < points0.size(); k++)
            {
                Point &p = points0[k];
                
#if THRESHOLD_OPTIM2
                if (!p.mIsActive)
                    continue;
#endif

#define EPS_ALPHA 1
                // Ignore if it is transparent
                if (p.mA < EPS_ALPHA)
                    continue;
                
                if (p.mZ < minZ)
                {
                    minPoint = &p;
                    minZ = p.mZ;
                }
            }
            
            // If point found, add it to the list
            if (minPoint != NULL)
            {
                minPoint->mX = x;
                minPoint->mY = y;
                
                minPoint->mSize = cellSize;
                
                points->push_back(*minPoint);
            }
#endif

#if SORT_POINTS_METHOD3
            Point avgPoint;
            
#if 0
            WDL_TypedBuf<double> z;
            z.Resize(points0.size());
            for (int i = 0; i < points0.size(); i++)
            {
                Point &p = points0[i];
                
                double z0 = p.mZ;
                
                z.Get()[i] = z0;
            }
            Debug::DumpData("z.txt", z);
#endif
            
#define INF 1e15
            double minZ = INF;
            for (int k = 0; k < points0.size(); k++)
            {
                Point &p = points0[k];
                
#if THRESHOLD_OPTIM2
                if (!p.mIsActive)
                    continue;
#endif

#define EPS_ALPHA 1
                // Ignore if it is transparent
                if (p.mA < EPS_ALPHA)
                    continue;
                
                if (p.mZ < minZ)
                {
                    minZ = p.mZ;
                }
            }
            
            avgPoint.mZ = minZ;
            
            // Blending
            int rgba[4];
            BlendDepthSorting(rgba, points0);
            //BlendWBOIT(rgba, points0);
            
            // DEBUG
            //if (points0.size() > dbgMaxNumCellPoints)
            //    dbgMaxNumCellPoints = points0.size();
            
            avgPoint.mR = rgba[0];
            avgPoint.mG = rgba[1];
            avgPoint.mB = rgba[2];
            avgPoint.mA = rgba[3];
            
            avgPoint.mIsSelected = false;
            
            avgPoint.mX = x;
            avgPoint.mY = y;
            
#if !FIXED_POINT_SIZE
            avgPoint.mSize = cellSize;
#else
            //avgPoint.mSize = 1.0;
            avgPoint.mSize[0] = mPointSize;
            avgPoint.mSize[1] = mPointSize;
#endif
            
            points->push_back(avgPoint);
#endif
            
            y += cellSize;
        }
        
        y = 0.0;
        x += cellSize;
    }
    
    //fprintf(stderr, "max cell num points: %d\n", dbgMaxNumCellPoints);
}

// Avoid black holes btween points when zooming!
// PROBLEM: we have big squares around the boundaries
// (partially solved, but now we have more holes inside the volume)
void
RayCaster::DoRayCastQuadTree(vector<Point> *points, int width, int height)
{
    mWidth = width;
    mHeight = height;
    
    // Hack
    if (mFirstTimeRender)
    {
#if !ADAPTIVE_QUALITY_ZOOM
        SetQuality(mQuality);
#else
        if (mAutoQualityFlag)
            AdaptZoomQuality();
        else
            SetQuality(mQuality);
#endif
        
        mFirstTimeRender = false;
    }
    
    double quality = mQuality;
    if (mAutoQualityFlag)
        quality = mAutoQuality;
    
    //
    int maxDim = (width > height) ? width : height;
    
    int minDim = 128;
    int res = (1.0 - quality)*minDim + quality*maxDim;
     // Adjust max by 0.5 to avoid too big resource consumption
    res = res*0.5;
    
    int resolution[2] = { res, res };
    
    double bbox[2][2] = { { 0, 0 }, { maxDim, maxDim } };
    //
    
    if ((mQuadTree == NULL) || mQualityChanged)
    {
        if (mQuadTree != NULL)
            delete mQuadTree;
     
        mQualityChanged = false;
        
        mQuadTree = RCQuadTree::BuildFromBottom(bbox, resolution);
    }
    
#if THRESHOLD_OPTIM2
    points->erase(unstable_remove_if(points->begin(), points->end(), Point::IsNotActive),
                  points->end());
#endif
    
    mQuadTree->Clear();
    RCQuadTree::InsertPoints(mQuadTree, *points);
    
    RCQuadTreeVisitor visitor;
    //visitor.SetMaxSize(10.0/*20.0*/);
    //visitor.SetMaxSize(40.0);
    
    // TODO: setup max size with mRenderAlgoParam ?
    
    // Test to get max size depending on quality..
    double cellSize = (bbox[1][0] - bbox[0][0])/resolution[0];
    visitor.SetMaxSize(6.0*cellSize);
    
    RCQuadTreeVisitor::TagBoundaries(mQuadTree);
    
    RCQuadTree::Visit(mQuadTree, visitor);
    
    vector<vector<Point> > pixelPoints;
    RCQuadTree::GetPoints(mQuadTree, &pixelPoints);
    
    // Clear the result
    points->clear();
    
    
    for (int i = 0; i < pixelPoints.size(); i++)
    {
        vector<Point> &points0 = pixelPoints[i];
            
        if (points0.empty())
            continue;
        
        // Result of blending a line of points over z
        Point blendPoint;
        
#define INF 1e15
        double minZ = INF;
        for (int k = 0; k < points0.size(); k++)
        {
            Point &p = points0[k];
            
#define EPS_ALPHA 1
            // Ignore if it is transparent
            if (p.mA < EPS_ALPHA)
                continue;
                
            if (p.mZ < minZ)
            {
                minZ = p.mZ;
            }
        }
            
        blendPoint.mZ = minZ;
            
        // Blending
        int rgba[4];
        BlendDepthSorting(rgba, points0);
        
        blendPoint.mR = rgba[0];
        blendPoint.mG = rgba[1];
        blendPoint.mB = rgba[2];
        blendPoint.mA = rgba[3];
            
        blendPoint.mIsSelected = false;
            
        blendPoint.mX = points0[0].mX;
        blendPoint.mY = points0[0].mY;
        
        blendPoint.mSize[0] = points0[0].mSize[0];
        blendPoint.mSize[1] = points0[0].mSize[1];
        
        points->push_back(blendPoint);
    }
}

void
RayCaster::DoRayCastKdTree(vector<Point> *points, int width, int height)
{
    mWidth = width;
    mHeight = height;
    
    // Hack
    if (mFirstTimeRender)
    {
#if !ADAPTIVE_QUALITY_ZOOM
        SetQuality(mQuality);
#else
        if (mAutoQualityFlag)
            AdaptZoomQuality();
        else
            SetQuality(mQuality);
#endif
        
        mFirstTimeRender = false;
    }
    
    double quality = mQuality;
    if (mAutoQualityFlag)
        quality = mAutoQuality;
    
    //
    int minDepth = 7;
    int maxDepth = 15; //13; //9;
    int depth = (1.0 - quality)*minDepth + quality*maxDepth;
    
    //
    if ((mKdTree == NULL) || mQualityChanged)
    {
        if (mKdTree != NULL)
            delete mKdTree;
        
        mQualityChanged = false;
        
        mKdTree = RCKdTree::Build(depth);
    }
    
#if THRESHOLD_OPTIM2
    points->erase(unstable_remove_if(points->begin(), points->end(), Point::IsNotActive),
                  points->end());
#endif
    
    RCKdTree::Clear(mKdTree);
    
    if (mRenderAlgo == RENDER_ALGO_KD_TREE_MEDIAN)
        RCKdTree::InsertPoints(mKdTree, *points, RCKdTree::SPLIT_METHOD_MEDIAN);
    else if (mRenderAlgo == RENDER_ALGO_KD_TREE_MIDDLE)
        RCKdTree::InsertPoints(mKdTree, *points, RCKdTree::SPLIT_METHOD_MIDDLE);
    
    RCKdTreeVisitor visitor;
    
    //RCQuadTreeVisitor::TagBoundaries(mQuadTree);
    double maxSize = mRenderAlgoParam*width;
    visitor.SetMaxSize(maxSize);
    
    RCKdTree::Visit(mKdTree, visitor);
    
    vector<vector<Point> > pixelPoints;
    RCKdTree::GetPoints(mKdTree, &pixelPoints);
    
    // Clear the result
    points->clear();
    
    
    for (int i = 0; i < pixelPoints.size(); i++)
    {
        vector<Point> &points0 = pixelPoints[i];
        
        if (points0.empty())
            continue;
        
        // Result of blending a line of points over z
        Point blendPoint;
        
#define INF 1e15
        double minZ = INF;
        for (int k = 0; k < points0.size(); k++)
        {
            Point &p = points0[k];
            
#define EPS_ALPHA 1
            // Ignore if it is transparent
            if (p.mA < EPS_ALPHA)
                continue;
            
            if (p.mZ < minZ)
            {
                minZ = p.mZ;
            }
        }
        
        blendPoint.mZ = minZ;
        
        // Blending
        int rgba[4];
        BlendDepthSorting(rgba, points0);
        
        blendPoint.mR = rgba[0];
        blendPoint.mG = rgba[1];
        blendPoint.mB = rgba[2];
        blendPoint.mA = rgba[3];
        
        blendPoint.mIsSelected = false;
        
        blendPoint.mX = points0[0].mX;
        blendPoint.mY = points0[0].mY;
        
        blendPoint.mSize[0] = points0[0].mSize[0];
        blendPoint.mSize[1] = points0[0].mSize[1];
        
        points->push_back(blendPoint);
    }
}

void
RayCaster::ComputePackedPointColor(const RayCaster::Point &p, NVGcolor *color)
{
    // Optim: pre-compute nvg color
    unsigned char color0[4] = { p.mR, p.mG, p.mB, p.mA };
    
#if !SORT_POINTS_METHOD3
    if (mSelectionEnabled)
    {
        // If not selected, gray out
        if (!p.mIsSelected)
        {
            for (int i = 0; i < 3; i++)
                color0[i] *= SELECT_GRAY_COLOR_COEFF;
        }
    }
#endif
    
#if 0 // Global alpha scale
    color0[3] *= mAlphaScale;
#endif
    
    // TEST
    //color0[3] *= 0.5;
    
    if (color0[3] > 255)
        color0[3] = 255;
    
    SWAP_COLOR(color0);
    
    *color = nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
}

void
RayCaster::SetColorMap(int colormapId)
{
    switch(colormapId)
    {
        case 0:
        {
            if (mColorMap != NULL)
                delete mColorMap;
    
            // Blue and dark pink
            mColorMap = new ColorMap4(false);
            
#if TRANSPARENT_COLORMAP
            mColorMap->AddColor(0, 0, 0, 0/*255*/, 0.0);
#else
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
#endif
            
            mColorMap->AddColor(68, 22, 68, 255, 0.25);
            mColorMap->AddColor(32, 122, 190, 255, 0.5);
            mColorMap->AddColor(172, 212, 250, 255, 0.75);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case 1:
        {
            if (mColorMap != NULL)
                delete mColorMap;
            
            // Dawn (thermal...)
            mColorMap = new ColorMap4(false);
            mColorMap->AddColor(4, 35, 53, 255, 0.0);
            
            mColorMap->AddColor(90, 61, 154, 255, 0.25);
            mColorMap->AddColor(185, 97, 125, 255, 0.5);
            
            mColorMap->AddColor(245, 136, 71, 255, 0.75);
            
            mColorMap->AddColor(233, 246, 88, 255, 1.0);
        }
        break;
            
        case 2:
        {
            if (mColorMap != NULL)
                delete mColorMap;
            
            // Multicolor ("jet")
            mColorMap = new ColorMap4(false);
            mColorMap->AddColor(0, 0, 128, 255, 0.0);
            mColorMap->AddColor(0, 0, 246, 255, 0.1);
            mColorMap->AddColor(0, 77, 255, 255, 0.2);
            mColorMap->AddColor(0, 177, 255, 255, 0.3);
            mColorMap->AddColor(38, 255, 209, 255, 0.4);
            mColorMap->AddColor(125, 255, 122, 255, 0.5);
            mColorMap->AddColor(206, 255, 40, 255, 0.6);
            mColorMap->AddColor(255, 200, 0, 255, 0.7);
            mColorMap->AddColor(255, 100, 0, 255, 0.8);
            mColorMap->AddColor(241, 8, 0, 255, 0.9);
            mColorMap->AddColor(132, 0, 0, 255, 1.0);
        }
        break;
    
        case 3:
        {
            // Cyan and Pink ("Sweet")
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(0, 150, 150, 255, 0.2);//
            mColorMap->AddColor(0, 255, 255, 255, 0.5/*0.2*/);
            mColorMap->AddColor(255, 0, 255, 255, 0.8);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        default:
            break;
    }
    
    if (mColorMap != NULL)
        mColorMap->Generate();
    
#if 0
    // Sky (ice)
    mColorMap = new ColorMap4(false);
    mColorMap->AddColor(4, 6, 19, 255, 0.0);
    
    mColorMap->AddColor(58, 61, 126, 255, 0.25);
    mColorMap->AddColor(67, 126, 184, 255, 0.5);
    
    mColorMap->AddColor(73, 173, 208, 255, 0.75);
    
    mColorMap->AddColor(232, 251, 252, 255, 1.0);
#endif
}

void
RayCaster::SetInvertColormap(bool flag)
{
    mInvertColormap = flag;
}

void
RayCaster::SetColormapRange(double range)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetRange(range);
    }
}

void
RayCaster::SetColormapContrast(double contrast)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetContrast(contrast);
    }
}

bool
RayCaster::GetSelection(double selection[4])
{
    for (int i = 0; i < 4; i++)
        selection[i] = mScreenSelection[i];
    
    ReorderScreenSelection(selection);
    
    if (!mSelectionEnabled || !mVolumeSelectionEnabled)
        return false;
    
    return true;
}

void
RayCaster::SetSelection(const double selection[4])
{
    for (int i = 0; i < 4; i++)
        mScreenSelection[i] = selection[i];
    
    // Re-order the selection
    if (mScreenSelection[2] < mScreenSelection[0])
    {
        double tmp = mScreenSelection[2];
        mScreenSelection[2] = mScreenSelection[0];
        mScreenSelection[0] = tmp;
    }
    
    if (mScreenSelection[3] < mScreenSelection[1])
    {
        double tmp = mScreenSelection[3];
        mScreenSelection[3] = mScreenSelection[1];
        mScreenSelection[1] = tmp;
    }
    
    mSelectionEnabled = true;
    
    SelectionToVolume();
    
    mVolumeSelectionEnabled = true;
}

void
RayCaster::EnableSelection()
{
    mSelectionEnabled = true;
}

void
RayCaster::DisableSelection()
{
    mSelectionEnabled = false;
}

bool
RayCaster::IsSelectionEnabled()
{
    return mSelectionEnabled;
}

bool
RayCaster::IsVolumeSelectionEnabled()
{
    return mVolumeSelectionEnabled;
}

void
RayCaster::ResetSelection()
{
    mSelectionEnabled = false;
    
    mVolumeSelectionEnabled = false;
    
    double corner0[3] = { -0.5, 0.0, 0.5 };
    double corner6[3] = { 0.5, 1.0, -0.5 };
    
    mVolumeSelection[0] = SelectionPoint(corner0[0], corner0[1], corner0[2]);
    mVolumeSelection[1] = SelectionPoint(corner6[0], corner0[1], corner0[2]);
    mVolumeSelection[2] = SelectionPoint(corner6[0], corner6[1], corner0[2]);
    mVolumeSelection[3] = SelectionPoint(corner0[0], corner6[1], corner0[2]);
    
    mVolumeSelection[4] = SelectionPoint(corner0[0], corner0[1], corner6[2]);
    mVolumeSelection[5] = SelectionPoint(corner6[0], corner0[1], corner6[2]);
    mVolumeSelection[6] = SelectionPoint(corner6[0], corner6[1], corner6[2]);
    mVolumeSelection[7] = SelectionPoint(corner0[0], corner6[1], corner6[2]);
}

bool
RayCaster::GetSliceSelection(vector<bool> *selection,
                             vector<double> *xCoords,
                             long sliceNum)
{
    // Cannot simply take the extremity selected points
    // (it is unstable and selected slice num jitters)
    
    if ((sliceNum < 0) || (sliceNum >= mSlices.size()))
        // Just in case
        return false;
    
    //const vector<Point> &slice = mSlices[centerSlice];
    vector<Point> slice = mSlices[sliceNum];
    
#if FIX_PLAY_SELECTION
    // Must re-threshold and re-select,
    // because when raycasting, threshold and selection are only
    // done is a copy of mSlices
    ThresholdPoints(&slice);
    
    SelectPoints3D(&slice);
#endif
    
    // Fill selection flags and x coords
    selection->resize(slice.size());
    xCoords->resize(slice.size());
    
    for (int i = 0; i < slice.size(); i++)
    {
        const Point &p = slice[i];
        
        bool selected = p.mIsSelected;
        (*selection)[i] = selected;
        
        double x = p.mX;
        (*xCoords)[i] = x;
    }
    
    return true;
}

bool
RayCaster::GetSelectionBoundsLines(int *startLine, int *endLine)
{
    if (!mVolumeSelectionEnabled)
        return false;
    
    int start = (mVolumeSelection[0].mZ + 0.5)*mNumSlices;
    int end = (mVolumeSelection[4].mZ + 0.5)*mNumSlices;
    
    if (start > end)
    {
        int tmp = end;
        end = start;
        start = tmp;
    }
    
    // Check selection totally out of bounds
    if ((start < 0) && (end < 0))
        return false;
    if ((start > mSlices.size() - 1) && (end > mSlices.size() - 1))
        return false;
    
    if (start < 0)
        start = 0;
    if (start > mSlices.size() - 1)
        start = mSlices.size() - 1;
    
    if (end < 0)
        end = 0;
    if (end > mSlices.size() - 1)
        end = mSlices.size() - 1;
    
    *startLine = start;
    *endLine = end;
    
    return true;
}

#if 0 // OLD
bool
RayCaster::SelectionIntersectSlice2D(const double selection2D[2][2],
                                     const double projSlice2D[4][2])
{
    glm::vec3 bmin(selection2D[0][0], selection2D[0][1], 0.0);
    glm::vec3 bmax(selection2D[1][0], selection2D[1][1], 0.0);
    glm::vec3 boxcenter((bmin + bmax)/2.0f);
    glm::vec3 boxhalfsize((bmax - bmin)/2.0f);
    
    for (int i = 0; i < 2; i++)
    {
        // Test with both normals
        glm::vec3 normal(0.0, 0.0, 1.0);
        if (i == 1)
            normal *= -1.0;
        
        glm::vec3 trivert0(projSlice2D[0][0], projSlice2D[0][1], 0.0);
        glm::vec3 trivert1(projSlice2D[1][0], projSlice2D[1][1], 0.0);
        glm::vec3 trivert2(projSlice2D[2][0], projSlice2D[2][1], 0.0);
        glm::vec3 trivert3(projSlice2D[3][0], projSlice2D[3][1], 0.0);
        
        // First triangle
        int intersect = TriangleVsAABB2(bmin, bmax,
                                        boxcenter, boxhalfsize,
                                        trivert0, trivert1, trivert2,
                                        normal);
        
        if (intersect != OUTSIDE)
            return true;
        
        // Second triangle
        intersect = TriangleVsAABB2(bmin, bmax,
                                    boxcenter, boxhalfsize,
                                    trivert0, trivert2, trivert3,
                                    normal);
        
        if (intersect != OUTSIDE)
            return true;
    }
    
    return false;
}
#endif

#if 0 // OLD
// For old system of selection
long
RayCaster::GetCenterSlice()
{
    // Project each slice, and make slice (2 triangles) intersection with
    // the selection axis aligned bounding box (in 2d)
    
    long minSlice = mSlices.size() - 1;
    long maxSlice = 0;
    
    double selection[4] = { mScreenSelection[0]*mViewWidth + 4.0, (1.0 - mScreenSelection[3])*mViewHeight + 4.0,
                            mScreenSelection[2]*mViewWidth + 1.0, (1.0 - mScreenSelection[1])*mViewHeight + 1.0 };
    
    double selection2D[2][2] = { { selection[0], selection[1] },
                                 { selection[2], selection[3] } };
        
        
    
    for (int i = 0; i < mSlices.size(); i++)
    {
        // Compute time step
        double z = 1.0 - ((double)i)/mSlices.size();
        
        // Center
        z -= 0.5;
        
        // Slice in world space
        double slice[4][3] = { { -0.5, -0.5, z },
                               { 0.5, -0.5, z },
                               { 0.5, 0.5, z },
                               { -0.5, 0.5, z } };
        
        // Project the slice
        double projSlice[4][3];
        for (int k = 0; k < 4; k++)
        {
            ProjectPoint(projSlice[k], slice[k], mViewWidth, mViewHeight);
        }
        
        double projSlice2D[4][2];
        for (int k = 0; k < 4; k++)
        {
            projSlice2D[k][0] = projSlice[k][0];
            projSlice2D[k][1] = projSlice[k][1];
        }
        
        bool intersect = SelectionIntersectSlice2D(selection2D, projSlice2D);
        if (intersect)
        {
            if (i < minSlice)
                minSlice = i;
                
            if (i > maxSlice)
                maxSlice = i;
        }
    }
    
    if (minSlice > maxSlice)
        // Failed
        return -1;
    
    long res = (minSlice + maxSlice)/2;
    
    // DEBUG
    //DBG_SelectSlice(res);
    
    return res;
}
#endif

long
RayCaster::GetCurrentSlice()
{
#if !FIX_SLOW_PLAY_AT_STARTUP
    long numSlices = mSlices.size() - 1;
#else
    // No selection => take the first slice!
    // (so when start playing, we will start play directly at startup)
    if (!mVolumeSelectionEnabled)
        return mSlices.size() - 1;
    
    // Max num slices!
    // So when pushing progressively the slices at startup,
    // we always take the midle of the volume
    // (and not a moving position)
    long numSlices = mNumSlices;
#endif
    
    double t = (mVolumeSelection[0].mZ + 0.5 + mVolumeSelection[4].mZ + 0.5)/2.0;
    
#if FIX_PLAY_SELECTION_REVERSED
    if (mScrollDirection == BACK_FRONT)
#endif
        t = 1.0 - t;
        
    long res = t*numSlices;
    
    return res;
}

void
RayCaster::SelectionToVolume()
{
#define EPS 1e-15
    
    double screenSelection[4];
    ScreenToAlignedVolume(mScreenSelection, screenSelection);
    
    // TEST
    ReorderScreenSelection(screenSelection);
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double zMin = 0.5;
        double zMax = -0.5;
        
        if (mVolumeSelectionEnabled)
        {
            zMin = mVolumeSelection[0].mZ;
            zMax = mVolumeSelection[4].mZ;
        }
        
        mVolumeSelection[0] = SelectionPoint(screenSelection[0], screenSelection[1], zMin);
        mVolumeSelection[1] = SelectionPoint(screenSelection[2], screenSelection[1], zMin);
        mVolumeSelection[2] = SelectionPoint(screenSelection[2], screenSelection[3], zMin);
        mVolumeSelection[3] = SelectionPoint(screenSelection[0], screenSelection[3], zMin);
        
        mVolumeSelection[4] = SelectionPoint(screenSelection[0], screenSelection[1], zMax);
        mVolumeSelection[5] = SelectionPoint(screenSelection[2], screenSelection[1], zMax);
        mVolumeSelection[6] = SelectionPoint(screenSelection[2], screenSelection[3], zMax);
        mVolumeSelection[7] = SelectionPoint(screenSelection[0], screenSelection[3], zMax);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double xMin = -0.5;
        double xMax = 0.5;
        
        if (mVolumeSelectionEnabled)
        {
            xMin = mVolumeSelection[0].mX;
            xMax = mVolumeSelection[1].mX;
        }
        
        mVolumeSelection[0] = SelectionPoint(xMin, screenSelection[1], screenSelection[2]);
        mVolumeSelection[1] = SelectionPoint(xMax, screenSelection[1], screenSelection[2]);
        mVolumeSelection[2] = SelectionPoint(xMax, screenSelection[3], screenSelection[2]);
        mVolumeSelection[3] = SelectionPoint(xMin, screenSelection[3], screenSelection[2]);
        
        mVolumeSelection[4] = SelectionPoint(xMin, screenSelection[1], screenSelection[0]);
        mVolumeSelection[5] = SelectionPoint(xMax, screenSelection[1], screenSelection[0]);
        mVolumeSelection[6] = SelectionPoint(xMax, screenSelection[3], screenSelection[0]);
        mVolumeSelection[7] = SelectionPoint(xMin, screenSelection[3], screenSelection[0]);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double xMin = -0.5;
        double xMax = 0.5;
        
        if (mVolumeSelectionEnabled)
        {
            xMin = mVolumeSelection[0].mX;
            xMax = mVolumeSelection[1].mX;
        }
        
        mVolumeSelection[0] = SelectionPoint(xMin, screenSelection[1], screenSelection[2]); // TEST
        mVolumeSelection[1] = SelectionPoint(xMax, screenSelection[1], screenSelection[2]);
        mVolumeSelection[2] = SelectionPoint(xMax, screenSelection[3], screenSelection[2]);
        mVolumeSelection[3] = SelectionPoint(xMin, screenSelection[3], screenSelection[2]);
        
        mVolumeSelection[4] = SelectionPoint(xMin, screenSelection[1], screenSelection[0]);
        mVolumeSelection[5] = SelectionPoint(xMax, screenSelection[1], screenSelection[0]);
        mVolumeSelection[6] = SelectionPoint(xMax, screenSelection[3], screenSelection[0]);
        mVolumeSelection[7] = SelectionPoint(xMin, screenSelection[3], screenSelection[0]);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double yMin = 0.0;
        double yMax = 1.0;
        
        if (mVolumeSelectionEnabled)
        {
            yMin = mVolumeSelection[0].mY;
            yMax = mVolumeSelection[3].mY;
        }
        
        mVolumeSelection[0] = SelectionPoint(screenSelection[0], yMin, screenSelection[3]);
        mVolumeSelection[1] = SelectionPoint(screenSelection[2], yMin, screenSelection[3]);
        mVolumeSelection[2] = SelectionPoint(screenSelection[2], yMax, screenSelection[3]);
        mVolumeSelection[3] = SelectionPoint(screenSelection[0], yMax, screenSelection[3]);
        
        mVolumeSelection[4] = SelectionPoint(screenSelection[0], yMin, screenSelection[1]);
        mVolumeSelection[5] = SelectionPoint(screenSelection[2], yMin, screenSelection[1]);
        mVolumeSelection[6] = SelectionPoint(screenSelection[2], yMax, screenSelection[1]);
        mVolumeSelection[7] = SelectionPoint(screenSelection[0], yMax, screenSelection[1]);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        double yMin = 0.0;
        double yMax = 1.0;
        
        if (mVolumeSelectionEnabled)
        {
            yMin = mVolumeSelection[0].mY;
            yMax = mVolumeSelection[3].mY;
        }
        
        mVolumeSelection[0] = SelectionPoint(1.0 - screenSelection[3], yMin, screenSelection[2]);
        mVolumeSelection[1] = SelectionPoint(1.0 - screenSelection[1], yMin, screenSelection[2]);
        mVolumeSelection[2] = SelectionPoint(1.0 - screenSelection[1], yMax, screenSelection[2]);
        mVolumeSelection[3] = SelectionPoint(1.0 - screenSelection[3], yMax, screenSelection[2]);
        
        mVolumeSelection[4] = SelectionPoint(1.0 - screenSelection[3], yMin, screenSelection[0]);
        mVolumeSelection[5] = SelectionPoint(1.0 - screenSelection[1], yMin, screenSelection[0]);
        mVolumeSelection[6] = SelectionPoint(1.0 - screenSelection[1], yMax, screenSelection[0]);
        mVolumeSelection[7] = SelectionPoint(1.0 - screenSelection[3], yMax, screenSelection[0]);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        double yMin = 0.0;
        double yMax = 1.0;
        
        if (mVolumeSelectionEnabled)
        {
            yMin = mVolumeSelection[0].mY;
            yMax = mVolumeSelection[3].mY;
        }
        
        mVolumeSelection[0] = SelectionPoint(screenSelection[1], yMin, 1.0 - screenSelection[0]);
        mVolumeSelection[1] = SelectionPoint(screenSelection[3], yMin, 1.0 - screenSelection[0]);
        mVolumeSelection[2] = SelectionPoint(screenSelection[3], yMax, 1.0 - screenSelection[0]);
        mVolumeSelection[3] = SelectionPoint(screenSelection[1], yMax, 1.0 - screenSelection[0]);
        
        mVolumeSelection[4] = SelectionPoint(screenSelection[1], yMin, 1.0 - screenSelection[2]);
        mVolumeSelection[5] = SelectionPoint(screenSelection[3], yMin, 1.0 - screenSelection[2]);
        mVolumeSelection[6] = SelectionPoint(screenSelection[3], yMax, 1.0 - screenSelection[2]);
        mVolumeSelection[7] = SelectionPoint(screenSelection[1], yMax, 1.0 - screenSelection[2]);
    }
}

void
RayCaster::VolumeToSelection()
{
#define EPS 1e-15
    
    double screenSelection[4];
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        screenSelection[0] = mVolumeSelection[0].mX;
        screenSelection[1] = mVolumeSelection[0].mY;
        
        screenSelection[2] = mVolumeSelection[2].mX;
        screenSelection[3] = mVolumeSelection[2].mY;
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        screenSelection[0] = mVolumeSelection[4].mZ;
        screenSelection[1] = mVolumeSelection[4].mY;
        
        screenSelection[2] = mVolumeSelection[3].mZ;
        screenSelection[3] = mVolumeSelection[3].mY;
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        screenSelection[0] = mVolumeSelection[1].mZ;
        screenSelection[1] = mVolumeSelection[1].mY;
        
        screenSelection[2] = mVolumeSelection[6].mZ;
        screenSelection[3] = mVolumeSelection[6].mY;
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        screenSelection[0] = mVolumeSelection[3].mX;
        screenSelection[1] = mVolumeSelection[3].mZ;
        
        screenSelection[2] = mVolumeSelection[6].mX;
        screenSelection[3] = mVolumeSelection[6].mZ;
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        screenSelection[0] = 1.0 - mVolumeSelection[7].mZ;
        screenSelection[1] = mVolumeSelection[7].mX;
        
        screenSelection[2] = 1.0 - mVolumeSelection[2].mZ;
        screenSelection[3] = mVolumeSelection[2].mX;
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        screenSelection[0] = mVolumeSelection[2].mZ;
        screenSelection[1] = 1.0 - mVolumeSelection[2].mX;
        
        screenSelection[2] = mVolumeSelection[7].mZ;
        screenSelection[3] = 1.0 - mVolumeSelection[7].mX;
    }
    
    ReorderScreenSelection(screenSelection);
    
    AlignedVolumeToScreen(screenSelection, mScreenSelection);
}

void
RayCaster::TranslateVolumeSelection(double trans[3])
{
    for (int i = 0; i < 8; i++)
    {
        mVolumeSelection[i].mX += trans[0];
        mVolumeSelection[i].mY += trans[1];
        mVolumeSelection[i].mZ += trans[2];
    }
}


#if 0
void
RayCaster::ScreenToAlignedVolume(const double screenSelectionNorm[4],
                                 double alignedVolumeSelection[4])
{
    double projFace[2][3];
    ProjectFace(projFace);
    
    double screenSelection[4] = { screenSelectionNorm[0],
                                  1.0 - screenSelectionNorm[1],
                                  screenSelectionNorm[2],
                                  1.0 - screenSelectionNorm[3] };
    
    screenSelection[0] *= mViewWidth;
    screenSelection[1] *= mViewHeight;
    screenSelection[2] *= mViewWidth;
    screenSelection[3] *= mViewHeight;
    
    alignedVolumeSelection[0] = (screenSelection[0] - projFace[0][0])/(projFace[1][0] - projFace[0][0]);
    alignedVolumeSelection[1] = (screenSelection[1] - projFace[0][1])/(projFace[1][1] - projFace[0][1]);
    
    alignedVolumeSelection[2] = (screenSelection[2] - projFace[0][0])/(projFace[1][0] - projFace[0][0]);
    alignedVolumeSelection[3] = (screenSelection[3] - projFace[0][1])/(projFace[1][1] - projFace[0][1]);
}
#endif

void
RayCaster::ScreenToAlignedVolume(const double screenSelectionNorm[4],
                                 double alignedVolumeSelection[4])
{
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double corner[3] = { mVolumeSelection[0].mX, mVolumeSelection[0].mY, mVolumeSelection[0].mZ };
        double projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        double p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        double p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        double p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        double p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[0];
        alignedVolumeSelection[1] = p3_0[1];
        
        alignedVolumeSelection[2] = p3_1[0];
        alignedVolumeSelection[3] = p3_1[1];
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double corner[3] = { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ };
        double projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        double p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        double p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        double p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        double p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[2];
        alignedVolumeSelection[1] = p3_0[1];
        
        alignedVolumeSelection[2] = p3_1[2];
        alignedVolumeSelection[3] = p3_1[1];
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double corner[3] = { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ };
        double projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        double p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        double p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        double p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        double p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[2];
        alignedVolumeSelection[1] = p3_0[1];
        
        alignedVolumeSelection[2] = p3_1[2];
        alignedVolumeSelection[3] = p3_1[1];
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double corner[3] = { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ };
        double projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        double p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        double p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        double p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        double p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[0];
        alignedVolumeSelection[1] = p3_0[2];
        
        alignedVolumeSelection[2] = p3_1[0];
        alignedVolumeSelection[3] = p3_1[2];
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left (right on the view)
    {
        double corner[3] = { mVolumeSelection[7].mX, mVolumeSelection[7].mY, mVolumeSelection[7].mZ };
        double projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        double p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        double p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        double p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        double p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = 1.0 - p3_0[2];
        alignedVolumeSelection[1] = p3_0[0];
        
        alignedVolumeSelection[2] = 1.0 - p3_1[2];
        alignedVolumeSelection[3] = p3_1[0];
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right (left on the view)
    {
        double corner[3] = { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ };
        double projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        double p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        double p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        double p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        double p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[2];
        alignedVolumeSelection[1] = 1.0 - p3_0[0];
        
        alignedVolumeSelection[2] = p3_1[2];
        alignedVolumeSelection[3] = 1.0 - p3_1[0];
    }
}


#if 0
void
RayCaster::AlignedVolumeToScreen(const double alignedVolumeSelection[4],
                                 double screenSelectionNorm[4])
{
    double projFace[2][3];
    ProjectFace(projFace);
    
    double screenSelection[4];
    
    screenSelection[0] = alignedVolumeSelection[0]*(projFace[1][0] - projFace[0][0]) + projFace[0][0];
    screenSelection[1] = alignedVolumeSelection[1]*(projFace[1][1] - projFace[0][1]) + projFace[0][1];
    
    screenSelection[2] = alignedVolumeSelection[2]*(projFace[1][0] - projFace[0][0]) + projFace[0][0];
    screenSelection[3] = alignedVolumeSelection[3]*(projFace[1][1] - projFace[0][1]) + projFace[0][1];
    
    screenSelectionNorm[0] = screenSelection[0]/mViewWidth;
    screenSelectionNorm[1] = 1.0 - screenSelection[1]/mViewHeight;
    screenSelectionNorm[2] = screenSelection[2]/mViewWidth;
    screenSelectionNorm[3] = 1.0 - screenSelection[3]/mViewHeight;
}
#endif

// Align selection to colume nearest face
void
RayCaster::AlignedVolumeToScreen(const double alignedVolumeSelection[4],
                                 double screenSelectionNorm[4])
{
    double projFace[2][3];
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double face[2][3] = { { mVolumeSelection[0].mX, mVolumeSelection[0].mY, mVolumeSelection[0].mZ },
                              { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double face[2][3] = { { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ },
                              { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double face[2][3] = { { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ },
                              { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double face[2][3] = { { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ },
                              { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        double face[2][3] = { { mVolumeSelection[7/*4*/].mX, mVolumeSelection[7/*4*/].mY, mVolumeSelection[7/*4*/].mZ },
                              { mVolumeSelection[2/*1*/].mX, mVolumeSelection[2/*1*/].mY, mVolumeSelection[2/*1*/].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        double face[2][3] = { { mVolumeSelection[2/*1*/].mX, mVolumeSelection[2/*1*/].mY, mVolumeSelection[2/*1*/].mZ },
                              { mVolumeSelection[7/*4*/].mX, mVolumeSelection[7/*4*/].mY, mVolumeSelection[7/*4*/].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    double screenSelection[4] = { projFace[0][0], projFace[0][1],
                                  projFace[1][0], projFace[1][1] };
    
    screenSelectionNorm[0] = screenSelection[0]/mViewWidth;
    screenSelectionNorm[1] = 1.0 - screenSelection[1]/mViewHeight;
    screenSelectionNorm[2] = screenSelection[2]/mViewWidth;
    screenSelectionNorm[3] = 1.0 - screenSelection[3]/mViewHeight;
}

#if 0
void
RayCaster::ProjectFace(double projFace[2][3])
{
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double face[2][3] = { { 0.0, 0.0, 0.5 },
                              { 1.0, 1.0, 0.5 } };
            
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double face[2][3] = { { -0.5, 0.0, -0.5 },
                              { -0.5, 1.0, 0.5 } };

        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double face[2][3] = { { 0.5, 0.0, 0.5 },
                              { 0.5, 1.0, -0.5 } };

        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double face[2][3] = { { -0.5, 1.0, 0.5 },
                              { 0.5, 1.0, -0.5 } };
            
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        double face[2][3] = { { -0.5, 1.0, -0.5 },
                              { 0.5, 1.0, 0.5 } };

        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        double face[2][3] = { { 0.5, 1.0, 0.5 },
                              { 0.5, 1.0, -0.5 } };

        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
}
#endif

#if 0
void
RayCaster::ProjectFace(double projFace[2][3])
{
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double face[2][3] = { { mVolumeSelection[0].mX, mVolumeSelection[0].mY, mVolumeSelection[0].mZ },
                              { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double face[2][3] = { { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ },
                              { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double face[2][3] = { { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ },
                              { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double face[2][3] = { { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ },
                              { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        double face[2][3] = { { mVolumeSelection[7].mX, mVolumeSelection[7].mY, mVolumeSelection[7].mZ },
                              { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        double face[2][3] = { { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ },
                              { mVolumeSelection[7].mX, mVolumeSelection[7].mY, mVolumeSelection[7].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
}
#endif

// TEST
#if 0
void
RayCaster::ProjectFace(double projFace[2][3])
{
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double face[2][3] = { { 0.0, 0.0, mVolumeSelection[0].mZ },
                              { 1.0, 1.0, mVolumeSelection[0].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double face[2][3] = { { mVolumeSelection[4].mX, 0.0, -0.5 },
                              { mVolumeSelection[4].mX, 1.0, 0.5 } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double face[2][3] = { { mVolumeSelection[1].mX, 0.0, 0.5 },
                              { mVolumeSelection[1].mX, 1.0, -0.5 } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double face[2][3] = { { -0.5, mVolumeSelection[3].mY, 0.5 },
                              { 0.5, mVolumeSelection[3].mY, -0.5 } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        double face[2][3] = { { -0.5, mVolumeSelection[3].mY, -0.5 },
                              { 0.5, mVolumeSelection[3].mY, 0.5 } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        double face[2][3] = { { 0.5, mVolumeSelection[3].mY, 0.5 },
                              { 0.5, mVolumeSelection[3].mY, -0.5 } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
}
#endif

void
RayCaster::ReorderScreenSelection(double screenSelection[4])
{
    // Re-order the selection
    if (screenSelection[2] < screenSelection[0])
    {
        double tmp = screenSelection[2];
        screenSelection[2] = screenSelection[0];
        screenSelection[0] = tmp;
    }
    
    if (screenSelection[3] < screenSelection[1])
    {
        double tmp = screenSelection[3];
        screenSelection[3] = screenSelection[1];
        screenSelection[1] = tmp;
    }
}

void
RayCaster::DBG_SelectSlice(long sliceNum)
{
    if ((sliceNum < 0) || (sliceNum >= mSlices.size()))
        return;
    
    vector<Point> &slice = mSlices[sliceNum];
    for (int i = 0; i < slice.size(); i++)
    {
        Point &p = slice[i];
        p.mIsSelected = true;
    }
}

void
RayCaster::SetDisplayRefreshRate(int displayRefreshRate)
{
#if !DISABLE_DISPLAY_REFRESH_RATE
    mDisplayRefreshRate = displayRefreshRate;
#endif
}

void
RayCaster::SetQuality(double quality)
{
    mQuality = quality;
    
    double cellSize = ComputeCellSize(mWidth, mHeight);
    //
    mPointSize = cellSize + 1.0;
    
    mQualityChanged = true;
}

void
RayCaster::SetAutoQuality(double quality)
{
    mAutoQuality = quality;
    
    double cellSize = ComputeCellSize(mWidth, mHeight);
    //
    mPointSize = cellSize + 1.0;
    
    mQualityChanged = true;
}

void
RayCaster::SetQualityT(double qualityT)
{
    mQualityT = qualityT;
}

void
RayCaster::SetPointSize(double size)
{
    mPointSize = (1.0 - size)*MIN_POINT_SIZE + size*MAX_POINT_SIZE;
}

#if 0
void
RayCaster::SetAlphaScale(double scale)
{
    mAlphaScale = scale;
}
#endif

void
RayCaster::SetAlphaCoeff(double alphaCoeff)
{
    mAlphaCoeff = alphaCoeff;
}

void
RayCaster::SetThreshold(double threshold)
{
    mThreshold = threshold;
}

void
RayCaster::SetThresholdCenter(double thresholdCenter)
{
    mThresholdCenter = thresholdCenter;
}

void
RayCaster::SetClipFlag(bool flag)
{
    mClipFlag = flag;
}

void
RayCaster::SetPlayBarPos(double t)
{
    mPlayBarPos = t;
}

void
RayCaster::SetAxis(int idx, Axis3D *axis)
{
    if (idx >= NUM_AXES)
        return;
    
    if (mAxis[idx] != NULL)
        delete mAxis[idx];
    
    mAxis[idx] = axis;
}

void
RayCaster::ProjectPoint(double projP[3], const double p[3], int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height, mCamAngle0, mCamAngle1,
                       mCamFov,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    // Matrix transform
    glm::vec4 v;
    v.x = p[0];
    v.y = p[1];
    v.z = p[2];
    v.w = 1.0;
    
    glm::vec4 v4 = modelProjMat*v;
    
    double x = v4.x;
    double y = v4.y;
    double z = v4.z;
    double w = v4.w;
    
#define EPS 1e-8
    if (fabs(w) > EPS)
    {
        // Optim
        double wInv = 1.0/w;
        x *= wInv;
        y *= wInv;
        z *= wInv;
    }
    
    // Do like OpenGL
    x = (x + 1.0)*0.5*width;
    y = (y + 1.0)*0.5*height;
    z = (z + 1.0)*0.5;
    
    // Result
    projP[0] = x;
    projP[1] = y;
    projP[2] = z;
}

void
RayCaster::SetRenderAlgo(RenderAlgo algo)
{
    mRenderAlgo = algo;
    
    // Hack for refreshing
    mQualityChanged = true;
}

void
RayCaster::SetRenderAlgoParam(double renderParam)
{
    mRenderAlgoParam = renderParam;
}

void
RayCaster::SetAutoQuality(bool flag)
{
    mAutoQualityFlag = flag;
    
    if (!mAutoQualityFlag)
        SetQuality(mQuality);
    else
        AdaptZoomQuality();
}

void
RayCaster::DrawSelection(NVGcontext *vg, int width, int height)
{
    if (!mSelectionEnabled)
        return;
    
    // Avoid display a big square at first selection
    if (!mVolumeSelectionEnabled)
        return;
    
    double strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1], colors[i][2], colors[i][3]));
        
        // Draw the circle
        nvgBeginPath(vg);
        
#if 0
        // Draw the line
        nvgMoveTo(vg, mScreenSelection[0], height - mScreenSelection[1]);
        
        nvgLineTo(vg, mScreenSelection[2], height - mScreenSelection[1]);
        nvgLineTo(vg, mScreenSelection[2], height - mScreenSelection[3]);
        nvgLineTo(vg, mScreenSelection[0], height - mScreenSelection[3]);
        nvgLineTo(vg, mScreenSelection[0], height - mScreenSelection[1]);
#endif
        
#if 1
        // Draw the line
        nvgMoveTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[1])*height);
        
        nvgLineTo(vg, mScreenSelection[2]*width, (1.0 - mScreenSelection[1])*height);
        nvgLineTo(vg, mScreenSelection[2]*width, (1.0 - mScreenSelection[3])*height);
        nvgLineTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[3])*height);
        nvgLineTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[1])*height);
#endif
        
        nvgStroke(vg);
    }
}

void
RayCaster::DrawSelectionVolume(NVGcontext *vg, int width, int height)
{
    //if (!mSelectionEnabled)
    //    return;
    
    // Avoid displaying the volume when no selection has been done yet
    // (it would display the full cube)
    if (!mVolumeSelectionEnabled)
        return;
    
#if 0
    // Full Volume
    double volume[8][3] =
    {   { -0.5, 0.0, -0.5 },
        { 0.5, 0.0, -0.5 },
        { 0.5, 1.0, -0.5 },
        { -0.5, 1.0, -0.5 },
        
        { -0.5, 0.0, 0.5 },
        { 0.5, 0.0, 0.5 },
        { 0.5, 1.0, 0.5 },
        { -0.5, 1.0, 0.5 }
    };
#endif
    
#if 1
    double volume[8][3] =
    {
        { mVolumeSelection[0].mX, mVolumeSelection[0].mY, mVolumeSelection[0].mZ },
        { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ },
        { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ },
        { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ },
        
        { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ },
        { mVolumeSelection[5].mX, mVolumeSelection[5].mY, mVolumeSelection[5].mZ },
        { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ },
        { mVolumeSelection[7].mX, mVolumeSelection[7].mY, mVolumeSelection[7].mZ },
    };
#endif
    
    // Projected volume
    double projectedVolume[8][3];
    for (int i = 0; i < 8; i++)
    {
        ProjectPoint(projectedVolume[i], volume[i], width, height);        
    }
    
    // Draw the projected volume
    nvgStrokeWidth(vg, 1.0);
    
    int color[4] = { 255, 255, 255, 255 };
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    // Draw front face
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[0][0], projectedVolume[0][1]);
    nvgLineTo(vg, projectedVolume[1][0], projectedVolume[1][1]);
    nvgLineTo(vg, projectedVolume[2][0], projectedVolume[2][1]);
    nvgLineTo(vg, projectedVolume[3][0], projectedVolume[3][1]);
    nvgLineTo(vg, projectedVolume[0][0], projectedVolume[0][1]);
    nvgStroke(vg);
    
    // Draw back face
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[4][0], projectedVolume[4][1]);
    nvgLineTo(vg, projectedVolume[5][0], projectedVolume[5][1]);
    nvgLineTo(vg, projectedVolume[6][0], projectedVolume[6][1]);
    nvgLineTo(vg, projectedVolume[7][0], projectedVolume[7][1]);
    nvgLineTo(vg, projectedVolume[4][0], projectedVolume[4][1]);
    nvgStroke(vg);
    
    // Draw intermediate lines
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[0][0], projectedVolume[0][1]);
    nvgLineTo(vg, projectedVolume[4][0], projectedVolume[4][1]);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[1][0], projectedVolume[1][1]);
    nvgLineTo(vg, projectedVolume[5][0], projectedVolume[5][1]);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[3][0], projectedVolume[3][1]);
    nvgLineTo(vg, projectedVolume[7][0], projectedVolume[7][1]);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[2][0], projectedVolume[2][1]);
    nvgLineTo(vg, projectedVolume[6][0], projectedVolume[6][1]);
    nvgStroke(vg);
}

void
RayCaster::DrawPlayBarPos(NVGcontext *vg, int width, int height)
{
    if (mPlayBarPos < 0.0)
        return;
    
    double playBarPosZ = mPlayBarPos - 0.5;
    
    double square[4][3] =
    {
        { mVolumeSelection[0].mX, mVolumeSelection[0].mY, playBarPosZ },
        { mVolumeSelection[1].mX, mVolumeSelection[1].mY, playBarPosZ },
        { mVolumeSelection[2].mX, mVolumeSelection[2].mY, playBarPosZ },
        { mVolumeSelection[3].mX, mVolumeSelection[3].mY, playBarPosZ },
    };
    
    // Projected volume
    double projectedSquare[4][3];
    for (int i = 0; i < 4; i++)
    {
        ProjectPoint(projectedSquare[i], square[i], width, height);
    }
    
    // Draw the projected volume
    nvgStrokeWidth(vg, 1.0);
    
    int color[4] = { 255, 255, 255, 255 };
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    // Draw face
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedSquare[0][0], projectedSquare[0][1]);
    nvgLineTo(vg, projectedSquare[1][0], projectedSquare[1][1]);
    nvgLineTo(vg, projectedSquare[2][0], projectedSquare[2][1]);
    nvgLineTo(vg, projectedSquare[3][0], projectedSquare[3][1]);
    nvgLineTo(vg, projectedSquare[0][0], projectedSquare[0][1]);
    nvgStroke(vg);
}

void
RayCaster::SelectPoints2D(vector<Point> *projPoints, int width, int height)
{
    // Unselect all
    for (int i = 0; i < projPoints->size(); i++)
    {
        Point &p = (*projPoints)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif
        
        // Comment if debug center slice
        //DBG_SelectSlice();
        //
        p.mIsSelected = false;
    }
    
    if (!mSelectionEnabled)
        return;
    
    // Add offsets to keep the selected points exactly inside the drawn borders
    double selection[4] = { mScreenSelection[0]*width + 4.0, (1.0 - mScreenSelection[3])*height + 4.0,
                            mScreenSelection[2]*width + 1.0, (1.0 - mScreenSelection[1])*height + 1.0 };
    
    // Select the points inside the selection rectangle
    for (int i = 0; i < projPoints->size(); i++)
    {
        Point &p = (*projPoints)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif

        if ((p.mX > selection[0]) && (p.mX < selection[2]) &&
            (p.mY > selection[1]) && (p.mY < selection[3]))
        {
            p.mIsSelected = true;
        }
    }
}

void
RayCaster::SelectPoints3D(vector<Point> *points)
{
    // Unselect all
    //for (int i = 0; i < points->size(); i++)
    //{
    //    Point &p = (*points)[i];
    //    p.mIsSelected = false;
    //}
    
    // Commented this to fix "when voxel zones selected, it scrolled when new data arrived"
    //if (!mSelectionEnabled)
    //    return;
    
    // Unselect all
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
//#if THRESHOLD_OPTIM2
//        if (!p.mIsActive)
//            continue;
//#endif

        p.mIsSelected = false;
    }
    
    double minCorner[3] = { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ };
    double maxCorner[3] = { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ };
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif

        if ((p.mX < minCorner[0]) || (p.mX > maxCorner[0]))
            continue;
        
        if ((p.mY < minCorner[1]) || (p.mY > maxCorner[1]))
            continue;
        
        if ((p.mZ < minCorner[2]) || (p.mZ > maxCorner[2]))
            continue;
        
        p.mIsSelected = true;
    }
}

void
RayCaster::SetPointIds(vector<Point> *points)
{
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        p.mId = i;
    }
}

#if 0 // OLD: Not optimized at all
// Propagate selection to the points in mSlices
void
RayCaster::PropagateSelection(const vector<Point> &points)
{
    // Do nothing if we don't care about selection
    if (!mSelectionEnabled)
        return;
    
    // Apply selection of the selected points to the queue
    long pointId = 0;
    
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < mSlices.size(); i++)
#else
    for (int i = mSlices.size() - 1; i >= 0; i--)
#endif
    {
        vector<RayCaster::Point> &slice = mSlices[i];
        for (int j = 0; j < slice.size(); j++)
        {
            // NOTE: Not optimized at all (triple loop)...
            int id = -1;
            for (int k = 0; k < points.size(); k++)
            {
                const Point &p = points[k];
                if (p.mId == pointId)
                {
                    id = k;
                    break;
                }
            }
            
            if (id == -1)
                continue;
            
            const Point &p = points[id];
            bool selected = p.mIsSelected;
            
            Point &sliceP = slice[j];
                
            sliceP.mIsSelected = selected;
            
            pointId++;
        }
    }
}
#endif

// Propagate selection to the points in mSlices
// (OPTIMIZED)
void
RayCaster::PropagateSelection(const vector<Point> &points)
{
    // Do nothing if we don't care about selection
    if (!mSelectionEnabled)
        return;
    
    if (points.empty())
        return;
    
    // Apply selection of the selected points to the queue
    long pointIdIn = 0;
    long pointId = points[pointIdIn].mId;
    long pointIdSlice = 0;
    
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < mSlices.size(); i++)
#else
    for (int i = mSlices.size() - 1; i >= 0; i--)
#endif
    {
        vector<RayCaster::Point> &slice = mSlices[i];
        for (int j = 0; j < slice.size(); j++)
        {
            if (pointId == pointIdSlice)
            {
                const Point &p = points[pointIdIn];
      
                bool selected = p.mIsSelected;
                Point &sliceP = slice[j];
                
#if THRESHOLD_OPTIM2
                if (!p.mIsActive)
                {
                    sliceP.mIsSelected = false;
                    
                    continue;
                }
#endif
            
                sliceP.mIsSelected = selected;
                
                pointIdIn++;
                
#if FIX_SELECT_CRASH
                // Security(1/2) to avoid a crash (not sure it is correct...)
                if (pointIdIn >= points.size())
                    break;
#endif
                
                pointId = points[pointIdIn].mId;
            }
        
            pointIdSlice++;
        }
        
#if FIX_SELECT_CRASH
        // Security(2/2) to avoid a crash (not sure it is correct...)
        if (pointIdIn >= points.size())
            break;
#endif
    }
}

#if 0 // ORIG
void
RayCaster::ThresholdPoints(vector<Point> *points)
{
    vector<Point> result;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        double w = p.mWeight;
        
        if (!mInvertColormap)
        {
            if (w > mThreshold)
                result.push_back(p);
        }
        else
        {
            if (w < 1.0 - mThreshold)
                result.push_back(p);
        }
    }
    
    *points = result;
}
#endif

#if !THRESHOLD_OPTIM2
#if 1 // OPTIM
void
RayCaster::ThresholdPoints(vector<Point> *points)
{
    // NOTE: the push_back() too many resources
    // NOTE: We process around 90000 points
    
    // First, count the result size
    long resultSize = 0;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        double w = p.mWeight;
        
        if (!mInvertColormap)
        {
            if (w > mThreshold)
            {
                resultSize++;
            }
        }
        else
        {
            if (w < 1.0 - mThreshold)
            {
                resultSize++;
            }
        }
    }
    
    // Then allowate the result
    vector<Point> result;
    result.resize(resultSize);
    
    long pointId = 0;
    for (int i = 0; i < points->size(); i++)
    {
        // Just in case
        if (pointId >= points->size())
            break;
        
        Point &p = (*points)[i];
        
        double w = p.mWeight;
        
        if (!mInvertColormap)
        {
            if (w > mThreshold)
            {
                result[pointId++] = p;
            }
        }
        else
        {
            if (w < 1.0 - mThreshold)
            {
                result[pointId++] = p;
            }
        }
    }
    
    *points = result;
}
#endif
#endif

#if THRESHOLD_OPTIM2
void
RayCaster::ThresholdPoints(vector<Point> *points)
{
    // Adjust
    double threshold = 1.0 - mThreshold;
    double thresholdCenter = 1.0 - mThresholdCenter;
    
    // Compute min and max
    double minThreshold = thresholdCenter - threshold;
    if (minThreshold < 0.0)
        minThreshold = 0.0;
    
    double maxThreshold = thresholdCenter + threshold;
    if (maxThreshold > 1.0)
        maxThreshold = 1.0;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        p.mIsActive = false;
        
        double w = p.mWeight;
        
#if 0 // ORIGIN, with only mThreshold
        if (!mInvertColormap)
        {
            if (w > mThreshold)
                p.mIsActive = true;
        }
        else
        {
            if (w < 1.0 - mThreshold)
                p.mIsActive = true;
        }
#endif
        
#if 1 // new, with mThreshold and mThresholdCenter
        if (!mInvertColormap)
        {
            if ((w > minThreshold) && (w < maxThreshold))
                p.mIsActive = true;
        }
        else
        {
            if ((w < minThreshold) || (w > maxThreshold))
                p.mIsActive = true;
        }
#endif
    }
}
#endif

void
RayCaster::ClipPoints(vector<Point> *points)
{
    vector<Point> result;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif

        if (p.mIsSelected)
            result.push_back(p);
    }
    
    *points = result;
}

void
RayCaster::ApplyColorMap(vector<Point> *points)
{
    ColorMap4::CmColor color;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif
        
        double t = p.mWeight;
        
#if 1
        if (mInvertColormap)
        {
            t = 1.0 - t;
            
            // Saturate less with pow2
            //t = pow(t, 6.0);
            
            // Optim: GOOD (x4)
            t = t*t*t;
            t = t*t;
        }
        
        if (!mInvertColormap)
        {
            // Saturate less with pow2
            //t = pow(t, 2.0);
            
            // Optim: GOOD (x4)
            t = t*t;
        }
#endif
        
        mColorMap->GetColor(t, &color);
        
        unsigned char *col8 = ((unsigned char *)&color);
        p.mR = col8[0];
        p.mG = col8[1];
        p.mB = col8[2];
        p.mA = col8[3];
    }
}

void
RayCaster::UpdateSlicesZ()
{
    // Adjust z for all the history
    for (int i = 0; i < mSlices.size(); i++)
    {
        // Compute time step
        double z = 1.0 - ((double)i)/mSlices.size();
        
        // Reverse scroll direction if necessary
        if (mScrollDirection == FRONT_BACK)
            z = 1.0 - z;
        
        // Center
        z -= 0.5;
        
        // Get the slice
        vector<RayCaster::Point> &points = mSlices[i];
        
        // Set the same time step for all the points of the slice
        //for (int j = 0; j < points.size(); j++)
        long pointsSize = points.size();
        for (int j = 0; j < pointsSize; j++)
        {
            points[j].mZ = z;
        }
        
#if RENDER_SLICES_TEXTURE
        mGPUSlices[i].SetZ(z);
#endif
    }
}

#if 0 // Shorter implementation
void
RayCaster::DequeToVec(vector<RayCaster::Point> *res,
                      const deque<vector<RayCaster::Point> > &que)
{
    res->clear();
    
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < que.size(); i++)
#else
    for (int i = que.size() - 1; i >= 0; i--)
#endif
    {
        res->insert(res->end(), que[i].begin(), que[i].end());
    }
}
#endif

#if 0 // Manage int step (origin)
//
// More verbose implementation to try to optimize
// (but does not optimize a lot)
void
RayCaster::SlicesDequeToVec(vector<RayCaster::Point> *res,
                            const deque<vector<RayCaster::Point> > &slicesQue,
                            double invDecimFactor)
{
    //int step = 1;
    int step = 1.0/invDecimFactor;
    
    long numElements = 0;
    for (int i = 0; i < slicesQue.size(); i += step)
        numElements += slicesQue[i].size();
    
    res->resize(numElements);
    
    long elementId = 0;
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < slicesQue.size(); i += step)
#else
    for (int i = slicesQue.size() - 1; i >= 0; i -= step)
#endif
    {
        const vector<RayCaster::Point> &vec = slicesQue[i];
        for (int j = 0; j < vec.size(); j++)
        {
            const Point &val = vec[j];
            (*res)[elementId++] = val;
        }
    }
}
#endif

#if 1 // Manage float step
//
// More verbose implementation to try to optimize
// (but does not optimize a lot)
void
RayCaster::SlicesDequeToVec(vector<RayCaster::Point> *res,
                            const deque<vector<RayCaster::Point> > &slicesQue)
{
    // If mQualityT is 1 => no decim
    // If mQualityT is 0.1 => take 1 slice over 10
    
    //int step = 1;
    double step = 1.0/mQualityT;
    double offset = 0.0;
#if FIX_JITTER_TIME_QUALITY
    //offset = mSliceNum % (int)step;
    offset = fmod(mSliceNum, step);
    
    offset = -offset;
    while (offset < 0.0)
        offset += step;
#endif
    
    long numElements = 0;
    double i0 = offset;
    while((int)i0 < slicesQue.size())
    {
        int i = (int)i0;
        numElements += slicesQue[i].size();
        
        i0 += step;
    }
    res->resize(numElements);
    
    // Debug
    //int debugNumSlices = 0;
    
    long elementId = 0;
#if !REVERT_DISPLAY_ORDER
    double i1 = offset;
    while((int)i1 < slicesQue.size())
    {
#else
    double i1 = slicesQue.size() - 1 - (int)offset;
    while((int)i1 >= 0.0)
    {
#endif
        {
            const vector<RayCaster::Point> &vec = slicesQue[(int)i1];
            for (int j = 0; j < vec.size(); j++)
            {
                const Point &val = vec[j];
                (*res)[elementId++] = val;
            }
            
            // Debug
            //debugNumSlices++;
        }
        
#if !REVERT_DISPLAY_ORDER
        i1 += step;
#else
        i1 -= step;
#endif
    }
        
    //fprintf(stderr, "num slices: %d\n", debugNumSlices);
}
#endif
    
int
RayCaster::GetResolution()
{
    double quality = mQuality;
    if (mAutoQualityFlag)
        quality = mAutoQuality;
    
    int resolution = (1.0 - quality)*RESOLUTION_0 + quality*RESOLUTION_1;
    
    return resolution;
}

void
RayCaster::BlendDepthSorting(int rgba[4], vector<Point> &points)
{
#if DISCARD_FARTHEST_RAY_POINTS
    DiscardFartestRayPoints(&points);
#endif

#if OPTIM_AFTER_DISCARD_FARTHEST2
    ThresholdPoints(&points);
#endif
    
#if OPTIM_AFTER_DISCARD_FARTHEST
    // Apply colormap after having discarded,
    // so we apply colormap on less points
    // OPTIM: 5% gain !
    ApplyColorMap(&points);
#endif
    
#if !OPTIM_PRE_SORT
    // With this, display in the correct order
    sort(points.begin(), points.end(), Point::IsSmallerZ);
    reverse(points.begin(), points.end());
#endif
    
#if ALPHA_BLEND_RAYCASTED_TEXTURE
    double alpha255 = mAlphaCoeff*255;
#endif
    
#if !FIX_BLEND_DEPTH_BG_ALPHA
    rgba[0] = 0;
    rgba[1] = 0;
    rgba[2] = 0;
    rgba[3] = 0;
#else
    // Background color
    rgba[0] = 0;
    rgba[1] = 0;
    rgba[2] = 0;
    rgba[3] = alpha255;
    // Set a background alpha!
    // So we don't have drak single pixels,
    //and we can see axes through the transparent voxels
#endif
    
    for (int k = 0; k < points.size(); k++)
    {
        const Point &p = points[k];
        
#if THRESHOLD_OPTIM2
        if (!p.mIsActive)
            continue;
#endif

        double selectCoeff = 1.0;
        if (!p.mIsSelected)
            selectCoeff = SELECT_GRAY_COLOR_COEFF;
        
        double colorCoeff = mAlphaCoeff*selectCoeff;
        
#if BLEND_LESS_HIGH_WEIGHTS
        double weightAlphaCoeff = 0.0;
        if (p.mWeight > 0.0)
            weightAlphaCoeff = pow(mAlphaCoeff, 1.0/p.mWeight);
        if (weightAlphaCoeff > 1.0)
            weightAlphaCoeff = 1.0;
        
        colorCoeff = weightAlphaCoeff*selectCoeff;
#endif
        
        // Makes a beautiful effect
        if (k == 0)
        {
#if !BLEND_ALSO_FIRST_COLOR
            rgba[0] = p.mR*selectCoeff;
            rgba[1] = p.mG*selectCoeff;
            rgba[2] = p.mB*selectCoeff;
            rgba[3] = 255;
#else
            rgba[0] += p.mR*colorCoeff;
            rgba[1] += p.mG*colorCoeff;
            rgba[2] += p.mB*colorCoeff; //
            
#if !ALPHA_BLEND_RAYCASTED_TEXTURE
            rgba[3] += mAlphaCoeff*255;
#else
            rgba[3] += colorCoeff*alpha255; // ORIG
            
            // TEST 1: a bit better
            //rgba[3] = colorCoeff*255;
            
            // TEST2: quite similar to test 1 (should be a bit better)
            //rgba[3] = alpha255;
            
            // VERY GOOD !
            // TEST 3: take alpha of the background (as if background pixel was previusly rendered),
            // plus the alpha of the first point (this is the same value)
            //rgba[3] = colorCoeff*alpha255 + colorCoeff*alpha255;
            
            // TEST: for pixels which are summation of only one point,
            // the alpha value is too small
            // (they are two dark when blending the texture over axes)
            //rgba[3] = 0;
            
            // TEST: axes are hidden, even when mAlphaCoeff is 0
            //rgba[3] = 255;
#endif
#endif
        }
        else
        {
            rgba[0] = (1.0 - colorCoeff)*rgba[0] + colorCoeff*p.mR;
            rgba[1] = (1.0 - colorCoeff)*rgba[1] + colorCoeff*p.mG;
            rgba[2] = (1.0 - colorCoeff)*rgba[2] + colorCoeff*p.mB;
            
#if !ALPHA_BLEND_RAYCASTED_TEXTURE
            rgba[3] = 255;
#else
            rgba[3] += colorCoeff*alpha255;
#endif
        }
    }
    
    // Clip
    if (!points.empty())
    {
        for (int k = 0; k < 4; k++)
        {
            if (rgba[k] < 0)
                rgba[k] = 0;
            if (rgba[k] > 255)
                rgba[k] = 255;
        }
    }
}

static double
ComputeW(double alpha, double z)
{
    double a = 3.0*1e3*pow((1.0 - z), 3.0);
    if (a < 1e-2)
        a = 1e-2;
    
    double w = alpha*a;
    
    return w;
}

// Use WBOIT method (a trick), to avoid sorting
// (sorting takes a lot of resource)
void
RayCaster::BlendWBOIT(int rgba[4], vector<Point> &points0)
{
    rgba[0] = 0;
    rgba[1] = 0;
    rgba[2] = 0;
    rgba[3] = 0;
    
    // Find min Z
    double minZ = INF;
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
        if (p.mZ < minZ)
            minZ = p.mZ;
    }
    
    // Find max Z
    double maxZ = -INF;
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
        if (p.mZ > maxZ)
            maxZ = p.mZ;
    }
    
    // See: http://jcgt.org/published/0002/02/09/paper-lowres.pdf
    //
    double sigmaCiWi[3] = { 0.0, 0.0, 0.0 };
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
     
        double normZ = 0.0; //1.0;
        if (maxZ - minZ > EPS)
            normZ = (p.mZ - minZ)/(maxZ - minZ);
        normZ = Utils::LogScaleNorm2(normZ, 128.0);
        normZ = 1.0 - normZ;
        
        double wi = normZ;
        //double wi = ComputeW(mAlphaCoeff, p.mZ);
        
        sigmaCiWi[0] += p.mR*wi;
        sigmaCiWi[1] += p.mG*wi;
        sigmaCiWi[2] += p.mB*wi;
        
        // Ci have pre-multiplied alpha !
        sigmaCiWi[0] *= mAlphaCoeff;
        sigmaCiWi[1] *= mAlphaCoeff;
        sigmaCiWi[2] *= mAlphaCoeff;
        
        // Select coeff
        double selectCoeff = 1.0;
        if (!p.mIsSelected)
            selectCoeff = SELECT_GRAY_COLOR_COEFF;
        
        sigmaCiWi[0] *= selectCoeff;
        sigmaCiWi[1] *= selectCoeff;
        sigmaCiWi[2] *= selectCoeff;
    }
    
#if 0 //1 // DEBUG
    if (points0.size() >= 16)
    {
        WDL_TypedBuf<double> z;
        z.Resize(points0.size());
        WDL_TypedBuf<double> nz;
        nz.Resize(points0.size());
        for (int k = 0; k < points0.size(); k++)
        {
            const Point &p = points0[k];
        
            z.Get()[k] = p.mZ;
        
            double normZ = 0.0;
            if (maxZ - minZ > EPS)
                normZ = (p.mZ - minZ)/(maxZ - minZ);
            nz.Get()[k] = normZ;
        }
        Debug::DumpData("z.txt", z);
        Debug::DumpData("nz.txt", nz);
    }
#endif
    
    //
    double sigmaAlphaiWi = 0.0;
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
        
        double normZ = 0.0; //1.0;
        if (maxZ - minZ > EPS)
            normZ = (p.mZ - minZ)/(maxZ - minZ);
        normZ = Utils::LogScaleNorm2(normZ, 128.0);
        normZ = 1.0 - normZ;
        
        double wi = normZ;
        //double wi = ComputeW(mAlphaCoeff, p.mZ);
        
        sigmaAlphaiWi += mAlphaCoeff*wi;
    }
    
    //
    double piAlphai = 1.0;
    for (int k = 0; k < points0.size(); k++)
    {
        piAlphai *= (1.0 - mAlphaCoeff);
    }
    
    for (int k = 0; k < 3; k++)
    {
        rgba[k] = (sigmaCiWi[k]/sigmaAlphaiWi)*(1.0 - piAlphai); // + C0*piAlphai
    }
    
    rgba[3] = 255;
    
    // Just in case
    if (!points0.empty())
    {
        for (int k = 0; k < 4; k++)
        {
            if (rgba[k] < 0)
                rgba[k] = 0;
            if (rgba[k] > 255)
                rgba[k] = 255;
        }
    }
}

// Render many colored quads
void
RayCaster::RenderQuads(NVGcontext *vg, const vector<Point> &points)
{
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        NVGcolor color;
        ComputePackedPointColor(p, &color);
        
        nvgStrokeColor(vg, color);
        nvgFillColor(vg, color);
        
        double x = p.mX;
        double y = p.mY;
        
        // Quad rendering
        //
        // (maybe reverse order)
        //
#if !FIXED_POINT_SIZE
        double size = p.mSize*mPointSize;
#else
        double size = mPointSize;
#endif
        
        double corners[4][2] = { { x - size/2.0, y - size/2.0 },
                                 { x + size/2.0, y - size/2.0 },
                                 { x + size/2.0, y + size/2.0 },
                                 { x - size/2.0, y + size/2.0 } };
        
        if (mWhitePixImg < 0)
        {
            unsigned char white[4] = { 255, 255, 255, 255 };
            mWhitePixImg = nvgCreateImageRGBA(vg,
                                              1, 1,
                                              NVG_IMAGE_NEAREST,
                                              white);
        }
        
        nvgQuad(vg, corners, mWhitePixImg);
    }
}

// Render colored texture pixels
void
RayCaster::RenderTexture(NVGcontext *vg, int width, int height,
                         const vector<Point> &points)
{
    if (mImage == NULL)
    {
        mImage = new ImageDisplayColor(vg);
        mImage->SetBounds(0.0, 0.0, 1.0, 1.0);
        mImage->Show(true);
    }
    
    mImageData.Resize(width*height*4);
    memset(mImageData.Get(), 0, width*height*4*sizeof(unsigned char));
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        NVGcolor color;
        ComputePackedPointColor(p, &color);
        
        double x = p.mX;
        double y = p.mY;
        
        //double size = mPointSize;
        double size[2] = { p.mSize[0], p.mSize[1] };
        
        unsigned char rgba[4] = { color.r*255, color.g*255,
                                  color.b*255, color.a*255 };
        
        for (int j = (int)(x - size[0]/2); j < (int)(x + size[0]/2); j++)
            for (int k = (int)(y - size[1]/2); k < (int)(y + size[1]/2); k++)
            {
                if (j < 0)
                    continue;
                if (j >= width)
                    continue;
                
                if (k < 0)
                    continue;
                if (k >= height)
                    continue;
                
                mImageData.Get()[(j + k*width)*4] = rgba[0];
                mImageData.Get()[(j + k*width)*4 + 1] = rgba[1];
                mImageData.Get()[(j + k*width)*4 + 2] = rgba[2];
                mImageData.Get()[(j + k*width)*4 + 3] = rgba[3];
            }
    }
    
    if (mRenderAlgo == RENDER_ALGO_GRID_DILATE)
    {
        DilateImage(width, height, &mImageData);
    }
    
    //
    
    mImage->SetImage(width, height, mImageData);
    
    mImage->DoUpdateImage();
    
    // Finally draw the image
    mImage->DrawImage(width, height);
}

long
RayCaster::ComputeNumRayPoints(const vector<Point> &points, double z)
{
    long numPoints = 0;
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        if (p.mZ < z)
            numPoints++;
    }
    
    return numPoints;
}

void
RayCaster::DiscardFartestRayPoints(vector<Point> *points)
{
    // Do a dichotomic search, to find the correct Z that will keep only MAX_NUM_RAY_POINTS
    // Then dicard points corresponding to that Z.
    
    if (points->size() < MAX_NUM_RAY_POINTS)
        // num points is small enough
        return;
    
    // Compute min and max z
    double minZ = INF;
    double maxZ = -INF;
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
        
        if (p.mZ < minZ)
            minZ = p.mZ;
        if (p.mZ > maxZ)
            maxZ = p.mZ;
    }
    
    // Dichotomic search of optimal z
    for (int i = 0; i < NUM_DICHO_ITERATIONS; i++)
    {
        double currentZ = (minZ + maxZ)*0.5;
        
        long numPoints = ComputeNumRayPoints(*points, currentZ);
        if (numPoints == MAX_NUM_RAY_POINTS)
            break;
        
        if (numPoints > MAX_NUM_RAY_POINTS)
        {
            maxZ = currentZ;
        }
        else
        {
            if (numPoints < MAX_NUM_RAY_POINTS)
            {
                minZ = currentZ;
            }
        }
    }
    
    // Suppress points with to great z
    double z = (minZ + maxZ)*0.5;
    
#if !OPTIM_REMOVE_IF
    vector<Point> result;
    
    // OPTIM: avoid many push_back()
    long numPoints = ComputeNumRayPoints(*points, z);
    result.resize(numPoints);
    int count = 0;
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
        if (p.mZ < z)
            result[count++] = p;
    }
    
    *points = result;
#else
    struct Pred
    {
        static bool ZIsFar(const Point &p, double zLimit)
        {
            return (p.mZ > zLimit);
        }
    };
    
    points->erase(unstable_remove_if(points->begin(), points->end(), Pred::ZIsFar, z),
                  points->end());
    
#endif
}
    
double
RayCaster::ComputeCellSize(int width, int height)
{
    int res = GetResolution();
    int resolution[2] = { res, res };
    
    double maxDim = (width > height) ? width : height;
    double bbox[2][2] = { { 0, 0 }, { maxDim, maxDim } };
    
    // Compute the grid cells color
    double cellSize = (bbox[1][0] - bbox[0][0])/resolution[0];
    
    return cellSize;
}

void
RayCaster::ComputeAxisPreDrawFlags(bool preDrawFlags[NUM_AXES])
{
#if 0
    fprintf(stderr, "angle0: %g\n", mCamAngle0);
    fprintf(stderr, "angle1: %g\n", mCamAngle1);
#endif
    
    // NOTE: a bit hackish, but this works !
    
    // X
    bool preDrawXAxis = (mCamAngle0 < -AXIS_X_CAM_ANGLE0_LIMIT);
    if (mCamAngle0 > AXIS_X_CAM_ANGLE0_LIMIT)
        preDrawXAxis = true;
    if (mCamAngle1 > AXIS_X_CAM_ANGLE1_LIMIT)
        preDrawXAxis = true;
    
    // Y
    bool preDrawYAxis = (mCamAngle0 < 0.0);
    if (mCamAngle1 > AXIS_Y_CAM_ANGLE1_LIMIT)
        preDrawYAxis = true;
    
    // Z
    bool preDrawZAxis = (mCamAngle0 < AXIS_Z_CAM_ANGLE0_LIMIT);
    if (mCamAngle1 > AXIS_Z_CAM_ANGLE1_LIMIT);
    preDrawZAxis = true;
    if ((mCamAngle0 > AXIS_Z_CAM_ANGLE0_LIMIT) && (mCamAngle1 < 0.0))
        preDrawZAxis =  false;
    
    preDrawFlags[0] = preDrawXAxis;
    preDrawFlags[1] = preDrawYAxis;
    preDrawFlags[2] = preDrawZAxis;
}
    
void
RayCaster::DrawAxes(NVGcontext *vg, int width, int height,
                    int stepNum, bool preDrawFlags[NUM_AXES])
{
    // Pre-draw
    for (int i = 0; i < NUM_AXES; i++)
    {
        if ((stepNum == 0) && preDrawFlags[i])
        {
            // Draw
            if (mAxis[i] != NULL)
                mAxis[i]->Draw(vg, width, height);
        }
    }
    
    // Post-draw
    for (int i = 0; i < NUM_AXES; i++)
    {
        if ((stepNum == 1) && !preDrawFlags[i])
        {
            // Draw
            if (mAxis[i] != NULL)
                mAxis[i]->Draw(vg, width, height);
        }
    }
}

void
RayCaster::DilateImageAux(int width, int height,
                          WDL_TypedBuf<unsigned char> *imageData)
{
    WDL_TypedBuf<unsigned char> imageDataCopy = *imageData;
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            unsigned char *color = &imageData->Get()[(i + j*width)*4];
            
            if (*((unsigned int *)color) == 0)
                // Not set color (black)
            {
                for (int j0 = j - 1; j0 < j + 1; j0++)
                {
                    for (int i0 = i - 1; i0 < i + 1; i0++)
                    {
                        if ((i0 == 0) && (j0 == 0))
                            // Center
                            continue;
                        
                        // Check bounds
                        if (i0 < 0)
                            continue;
                        if (i0 >= width)
                            continue;
                        
                        if (j0 < 0)
                            continue;
                        if (j0 >= height)
                            continue;
                        
                        // Current color
                        unsigned char *color0 = &imageDataCopy.Get()[(i0 + j0*width)*4];
                        
#if 0 // Take the first not black pixel
                        if (*((unsigned int *)color0) != 0)
                            // First set color
                        {
                            *((unsigned int *)color) = *((unsigned int*)color0);
                            
                            break;
                        }
#endif
                        
#if 1 // Take max alpha
                        if (color0[3] > color[3])
                        {
                            *((unsigned int *)color) = *((unsigned int *)color0);
                        }
#endif
                    }
                }
            }
        }
    }
}
    
void
RayCaster::DilateImage(int width, int height,
                       WDL_TypedBuf<unsigned char> *imageData)
{
    int maxNumIterations = 8;
    int numIterations = mRenderAlgoParam*maxNumIterations;
    
    for (int i = 0; i < numIterations; i++)
    {
        DilateImageAux(width, height, imageData);
    }
}
    
void
RayCaster::AdaptZoomQuality()
{
    double t = (mCamFov - MIN_CAMERA_FOV)/(MAX_CAMERA_FOV - MIN_CAMERA_FOV);
    
    // Change shape
    //t = pow(t, 2.0);
    
    double minQuality = 0.0;
    double maxQuality = 0.35;
    
    double quality = (1.0 - t)*minQuality + t*maxQuality;
    
    SetAutoQuality(quality);
}
