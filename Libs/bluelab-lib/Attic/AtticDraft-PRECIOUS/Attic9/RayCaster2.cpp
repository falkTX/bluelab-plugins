//
//  RayCaster2.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#include <algorithm>
using namespace std;

#include <StlInsertSorted.h>

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

#define TWO_PI 6.28318530717959

#define LOG_2 0.693147180559945

// Necessary if PROJECT_POINT_SIMD is set
//
// glm-0.9.9.7 instead of glm-master
#define USE_NEWEST_GLM 0 //1
#if USE_NEWEST_GLM

// Mac only
#undef __apple_build_version__
#define GLM_FUNC_QUALIFIER
#define GLM_FUNC_DECL

#endif

// AVX: Does not optimize... (even with RC_FLOAT=float)
// Tested SSE4.2: optimize a little (2%)
// Tested SSE3 (with ext): optimize less than SSE4.2
#define PROJECT_POINT_SIMD 0 //1
#if PROJECT_POINT_SIMD

// Change to glm-0.9.9.7 (original: glm-master)
// Change clang "Enable additional vector extension": SSE4.2 (original: Default)

// See: https://glm.g-truc.net/0.9.1/api/a00285.html
// and: https://gamedev.stackexchange.com/questions/132549/how-to-use-glm-simd-using-glm-version-0-9-8-2


// NOTE: original version: glm-master
// NOTE: changed clang "Enable additional vector extension": Default -> AVX
// NOTE: rotating the view doesn't work well with glm-0.9.9.7

//#define GLM_FORCE_AVX2 // interesting
//#define GLM_FORCE_AVX4

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

//#define GLM_FORCE_SSE2
//#define GLM_FORCE_SSE3 1
#define GLM_FORCE_SSE42 1

#include <glm/simd/matrix.h>

#endif // PROJECT_POINT_SIMD


#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <glm/gtx/intersect.hpp>

#include <ColorMap4.h>
#include <ColorMapFactory.h>

#include <Utils.h>
#include <Debug.h>

#include <ImageDisplayColor.h>

#include <UnstableRemoveIf.h>

#include <Axis3D.h>

#include <RCQuadTree.h>
#include <RCQuadTreeVisitor.h>

#include <RCKdTree.h>
#include <RCKdTreeVisitor.h>


#include "RayCaster2.h"


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

#define BLENDING_COEFF 0.75 //0.5 //0.25

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
#define OPTIM_PRE_SORT 1 //0
// Make a bit more small artifacts (sometimes wider incorrect lines)
// And not sure it optimizes (maybe ~0.5/1%)
#define OPTIM_PRE_SORT_DICHO 0 // 1
#define OPTIM_PRE_SORT_SORT_CAM 1

#define DBG_DRAW_SLICES 0 //1 //0 //1

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

// 1 degree! (1e-8 doesn't work)
#define GIMBAL_LOCK_EPS 1.0 //1e-8

// Decrease the render texture resolution if the cell size
// is bigger than 1 pixel
// Optim of 6% (76% -> 70%)
#define OPTIM_ADAPTIVE_TEXTURE_SIZE 1
#if OPTIM_ADAPTIVE_TEXTURE_SIZE
// Pre-compute pixel (i, j) as soon as possible
// (avoid computing it as float, then re-convert to int after
//
// When used with Point fields unions, we gain 1/2% (70% -> 68%)
#define OPTIM_ADAPTIVE_TEXTURE_SIZE2 1 //0
#endif

// Compute point color from colormap at the beginning,
// when the points are added to the object.
// So then colormap won't be computed at each frame for almost every point.
//
// NOTE: optimizes 2% (65% -> 63%)
#define PRECOMPUTE_POINT_COLOR 1

static void
CameraModelProjMat(int winWidth, int winHeight,
                   float angle0, float angle1, float perspAngle,
                   glm::mat4 *model, glm::mat4 *proj)
{
    // Avoid gimbal lock (since RC_FLOAT is float)
    if (angle1 > 90.0 - GIMBAL_LOCK_EPS)
        angle1 = 90.0 - GIMBAL_LOCK_EPS;
        
    glm::vec3 pos(0.0f, 0.0, -2.0f);
    glm::vec3 target(0.0f, 0.37f, 0.0f);
    
    glm::vec3 up(0.0, 1.0f, 0.0f);
    
    glm::vec3 lookVec(target.x - pos.x, target.y - pos.y, target.z - pos.z);
    float radius = glm::length(lookVec);
    
    //angle1 *= 3.14/180.0;
    angle1 *= M_PI/180.0;
    float newZ = cos(angle1)*radius;
    float newY = sin(angle1)*radius;
    
#if USE_NEWEST_GLM
    // In newest versions of glm (required for SIMD),
    // angles are in radians for the functions below
    angle0 *= M_PI/180.0;
    perspAngle *= M_PI/180.0;
#endif
    
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
                const RC_FLOAT p2[3], RC_FLOAT p3[3])
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(winWidth, winHeight,
                       angle0, angle1, camFov,
                       &model, &proj);
    
    glm::vec3 win(p2[0], p2[1], p2[2]);
    
    glm::vec4 viewport(0, 0, winWidth, winHeight);
    glm::vec3 world = glm::unProject(win, model, proj, viewport);
    
    p3[0] = world.x;
    p3[1] = world.y;
    p3[2] = world.z;
}

//

RayCaster2::RayCaster2(int numSlices, int numPointsSlice)
{
    mColorMap = NULL;
    mColorMapFactory = new ColorMapFactory(false, true);
    
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
    
    mPointSize = MIN_POINT_SIZE;
    
    mAlphaCoeff = BLENDING_COEFF;
    
    mThreshold = 0.0;
    
    mClipFlag = false;
    
    // Keep internally
    mViewWidth = 0;
    mViewHeight = 0;
    
    mScrollDirection = RayCaster2::FRONT_BACK;
    
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
    
    if (mAutoQualityFlag)
        AdaptZoomQuality();
    
    // Debug
    mRenderAlgo = RENDER_ALGO_GRID;
    mRenderAlgoParam = 0.0;
    
    // Fill the result with empty data at the beginning
    FillEmptySlices();
    
    // For ViewSignsChanged()
    mPrevViewSigns[0] = -2;
    mPrevViewSigns[1] = -2;
    mPrevViewSigns[2] = -2;
    
#if PROFILE_RENDER
    BlaTimer::Reset(&mTimer0, &mCount0);
#endif
}

RayCaster2::~RayCaster2()
{
    if (mColorMap != NULL)
        delete mColorMap;
    
    delete mColorMapFactory;
    
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
RayCaster2::SetNumSlices(long numSlices)
{
    mNumSlices = numSlices;
    
    FillEmptySlices();
}

long
RayCaster2::GetNumSlices()
{
    return mNumSlices;
}

void
RayCaster2::SetScrollDirection(RayCaster2::ScrollDirection dir)
{
    mScrollDirection = dir;
    
#if 0 // Uncomment ?
    UpdateSlicesZ();
#endif
}

void
RayCaster2::PreDraw(NVGcontext *vg, int width, int height)
{
#if PROFILE_RENDER
    BlaTimer::Start(&mTimer0);
#endif
    
    mViewWidth = width;
    mViewHeight = height;
    
    // Optim: GOOD
    // Do not re-allocate points each time
    // Keep a buffer
    
#if !OPTIM_PRE_SORT
    SlicesToVec(&mTmpPoints, mSlices);
#else
    SlicesToVecPreSort(&mTmpPoints, mSlices);
#endif
    
    vector<Point> &points = mTmpPoints;
    
    //
    RayCast(&points, width, height);
    
    // Draw axis before or after voxels
    // in order to manage occlusions well
    bool preDrawFlags[NUM_AXES];
    ComputeAxisPreDrawFlags(preDrawFlags);
    
    DrawAxes(vg, width, height, 0, preDrawFlags);
    
    //
    nvgSave(vg);
    
#if !RENDER_TEXTURE
    RenderQuads(vg, points);
#else
    
    // Texture adaptive size
    double textureSizeCoeff = -1.0;
#if OPTIM_ADAPTIVE_TEXTURE_SIZE
    //if (mAutoQualityFlag) // Test to compare render quality
    //{
    RC_FLOAT cellSize = ComputeCellSize(width, height);
    textureSizeCoeff = cellSize;
    //}
#endif
    
    RenderTexture(vg, width, height, points, textureSizeCoeff);
#endif
    
    nvgRestore(vg);
    
    DrawAxes(vg, width, height, 1, preDrawFlags);
    
    DrawSelectionVolume(vg, width, height);
    
    DrawPlayBarPos(vg, width, height);
    
    // Selection
    DrawSelection(vg, width, height);
    
#if DBG_DRAW_SLICES
    //DBG_DrawSlicesZ(vg, width, height);
    DBG_DrawSlices(vg, width, height, AXIS_X);
    DBG_DrawSlices(vg, width, height, AXIS_Y);
    DBG_DrawSlices(vg, width, height, AXIS_Z);
    
    //DBG_DrawSliceX(vg, width, height, 0.0);
    //DBG_DrawSliceY(vg, width, height, 0.5);
    //DBG_DrawSliceZ(vg, width, height, 0.0);
#endif
    
#if PROFILE_RENDER
    BlaTimer::StopAndDump(&mTimer0, &mCount0,
                          "profile.txt",
                          "full: %ld ms \n");
#endif
}

void
RayCaster2::ClearSlices()
{
    mSlices.clear();
    
    FillEmptySlices();
}

void
RayCaster2::SetSlices(const vector<vector<Point> > &slices)
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
RayCaster2::AddSlice(const vector<Point> &points, bool skipDisplay)
{
    vector<Point> points0 = points;
    
#if (OPTIM_PRE_SORT)
    int signX;
    int signY;
    int signZ;
    ComputeViewSigns(&signX, &signY, &signZ);
    
    SortVec(&points0, signX, signY, signZ);
#endif
    
    if (skipDisplay)
        return;
    
#if !DISABLE_DISPLAY_REFRESH_RATE
    if (mRefreshNum++ % mDisplayRefreshRate != 0)
        // Don't add the points this time
        return;
#endif
    
#if PRECOMPUTE_POINT_COLOR
    ApplyColorMap(&points0);
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
RayCaster2::SetCameraAngles(RC_FLOAT angle0, RC_FLOAT angle1)
{
    mCamAngle0 = angle0;
    mCamAngle1 = angle1;
}

void
RayCaster2::SetCameraFov(RC_FLOAT camFov)
{
    mCamFov = camFov;
    
    if (mAutoQualityFlag)
        AdaptZoomQuality();
}

void
RayCaster2::ZoomChanged(RC_FLOAT zoomChange)
{
    mCamFov *= 1.0/zoomChange;
    
    if (mCamFov < MIN_CAMERA_FOV)
        mCamFov = MIN_CAMERA_FOV;
    
    if (mCamFov > MAX_CAMERA_FOV)
        mCamFov = MAX_CAMERA_FOV;
    
    if (mAutoQualityFlag)
        AdaptZoomQuality();
}

void
RayCaster2::ProjectPoints(vector<Point> *points, int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height,
                       mCamAngle0, mCamAngle1, mCamFov,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    //
    RC_FLOAT halfWidth = 0.5*width;
    RC_FLOAT halfHeight = 0.5*height;
    
    // Temporary vector
    glm::vec4 v;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        //if (!p.mIsActive)
        if (p.mFlags & RC_POINT_FLAG_THRS_DISCARD)
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
            RC_FLOAT wInv = 1.0/v4.w;
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

#if PROJECT_POINT_SIMD
void
RayCaster2::ProjectPoints(vector<Point> *points, int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height,
                       mCamAngle0, mCamAngle1, mCamFov,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    float m0[16] = { modelProjMat[0][0], modelProjMat[0][1], modelProjMat[0][2], modelProjMat[0][3],
                     modelProjMat[1][0], modelProjMat[1][1], modelProjMat[1][2], modelProjMat[1][3],
                     modelProjMat[2][0], modelProjMat[2][1], modelProjMat[2][2], modelProjMat[2][3],
                     modelProjMat[3][0], modelProjMat[3][1], modelProjMat[3][2], modelProjMat[3][3] };
    
    float v0f[4];
    glm_vec4 &v0 = *((glm_vec4 *)&v0f);
    
    RC_FLOAT halfWidth = width*0.5;
    RC_FLOAT halfHeight = height*0.5;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        // Matrix transform (SIMD)
        
        //
        v0[0] = p.mX;
        v0[1] = p.mY;
        v0[2] = p.mZ;
        v0[3] = 1.0;
        
        //
        glm_vec4 v4 = glm_mat4_mul_vec4((glm_vec4 *)m0, v0);
        float *v4f = (float *)&v4;
        
#define EPS 1e-8
        if ((v4f[3] < -EPS) || (v4f[3] > EPS))
        {
            // Optim
            RC_FLOAT wInv = 1.0/v4f[3];
            v4f[0] *= wInv;
            v4f[1] *= wInv;
            v4f[2] *= wInv;
        }
        
        // Do like OpenGL
        v4f[0] = (v4f[0] + 1.0)*halfWidth;
        v4f[1] = (v4f[1] + 1.0)*halfHeight;
        v4f[2] = (v4f[2] + 1.0f)*0.5;
        
        // Result
        p.mX = v4f[0];
        p.mY = v4f[1];
        p.mZ = v4f[2];
    }
}
#endif

void
RayCaster2::RayCast(vector<Point> *points, int width, int height)
{
#if !OPTIM_AFTER_DISCARD_FARTHEST2
    ThresholdPoints(points);
#endif
    
#if !OPTIM_AFTER_DISCARD_FARTHEST
    ApplyColorMap(points);
#endif
    
    SelectPoints3D(points);
    
    if (mClipFlag)
        ClipPoints(points);
    
    ProjectPoints(points, width, height);
    
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
RayCaster2::DoRayCastGrid(vector<Point> *points, int width, int height)
{    
    mWidth = width;
    mHeight = height;
    
    // Hack
    if (mFirstTimeRender)
    {
        if (mAutoQualityFlag)
            AdaptZoomQuality();
        else
            SetQuality(mQuality);
        
        mFirstTimeRender = false;
    }
    
    RC_FLOAT cellSize = ComputeCellSize(width, height);
    
    //
    int res = GetResolution();
    int resolution[2] = { res, res };
    
    RC_FLOAT maxDim = (width > height) ? width : height;
    RC_FLOAT bbox[2][2] = { { 0, 0 }, { maxDim, maxDim } };
    
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
    RC_FLOAT xRatio = 1.0/((bbox[1][0] - bbox[0][0]));
    RC_FLOAT yRatio = 1.0/((bbox[1][1] - bbox[0][1]));
    
    // New: pre-multiply
    xRatio *= resolution[0];
    yRatio *= resolution[1];
    
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        //if (!p.mIsActive)
        if (p.mFlags & RC_POINT_FLAG_THRS_DISCARD)
            continue;
#endif
        
        RC_FLOAT normX = (p.mX - bbox[0][0])*xRatio;
        RC_FLOAT normY = (p.mY - bbox[0][1])*yRatio;
        
#if !ROUND_POINT_COORDS // Trunk
        int x = (int)(normX/**resolution[0]*/);
        int y = (int)(normY/**resolution[1]*/);
#else
        int x = (int)round(normX/**resolution[0]*/);
        int y = (int)round(normY/**resolution[1]*/);
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
    
    RC_FLOAT x = 0;
    RC_FLOAT y = 0;
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
            
            //
            Point avgPoint;
            
#define INF 1e15
            RC_FLOAT minZ = INF;
            for (int k = 0; k < points0.size(); k++)
            {
                Point &p = points0[k];
                
#if THRESHOLD_OPTIM2
                //if (!p.mIsActive)
                if (p.mFlags & RC_POINT_FLAG_THRS_DISCARD)
                    continue;
#endif

#define EPS_ALPHA 1
                // Ignore if it is transparent
                if (p.mRGBA[3] < EPS_ALPHA)
                    continue;
                
                if (p.mZ < minZ)
                {
                    minZ = p.mZ;
                }
            }
            
            avgPoint.mZ = minZ;
            
            // Blending
            unsigned char rgba[4];
            //BlendDepthSorting(rgba, points0);
            BlendDepthSortingFTB(rgba, points0);
            //BlendWBOIT(rgba, points0);
            
            *((int *)&avgPoint.mRGBA) = *((int *)&rgba);
            
            //avgPoint.mIsSelected = false;
            
#if !OPTIM_ADAPTIVE_TEXTURE_SIZE2
            avgPoint.mX = x;
            avgPoint.mY = y;
#else
            avgPoint.mXi = i;
            avgPoint.mYi = j;
#endif
            
#if !FIXED_POINT_SIZE
            avgPoint.mSize = cellSize;
#else

#if !OPTIM_ADAPTIVE_TEXTURE_SIZE2
            avgPoint.mSize[0] = mPointSize;
            avgPoint.mSize[1] = mPointSize;
#else
            avgPoint.mSizei[0] = 1;
            avgPoint.mSizei[1] = 1;
#endif
            
#endif
            
            points->push_back(avgPoint);
            
            y += cellSize;
        }
        
        y = 0.0;
        x += cellSize;
    }
}

// Avoid black holes btween points when zooming!
// PROBLEM: we have big squares around the boundaries
// (partially solved, but now we have more holes inside the volume)
void
RayCaster2::DoRayCastQuadTree(vector<Point> *points, int width, int height)
{
    mWidth = width;
    mHeight = height;
    
    // Hack
    if (mFirstTimeRender)
    {
        if (mAutoQualityFlag)
            AdaptZoomQuality();
        else
            SetQuality(mQuality);
        
        mFirstTimeRender = false;
    }
    
    RC_FLOAT quality = mQuality;
    if (mAutoQualityFlag)
        quality = mAutoQuality;
    
    //
    int maxDim = (width > height) ? width : height;
    
    int minDim = 128;
    int res = (1.0 - quality)*minDim + quality*maxDim;
     // Adjust max by 0.5 to avoid too big resource consumption
    res = res*0.5;
    
    int resolution[2] = { res, res };
    
    RC_FLOAT bbox[2][2] = { { 0, 0 }, { maxDim, maxDim } };
    //
    
    if ((mQuadTree == NULL) || mQualityChanged)
    {
        if (mQuadTree != NULL)
            delete mQuadTree;
     
        mQualityChanged = false;
        
        mQuadTree = RCQuadTree::BuildFromBottom(bbox, resolution);
    }
    
#if THRESHOLD_OPTIM2
    points->erase(unstable_remove_if(points->begin(), points->end(), Point::IsThrsDiscarded),
                  points->end());
#endif
    
    mQuadTree->Clear();
    RCQuadTree::InsertPoints(mQuadTree, *points);
    
    RCQuadTreeVisitor visitor;
    //visitor.SetMaxSize(10.0/*20.0*/);
    //visitor.SetMaxSize(40.0);
    
    // TODO: setup max size with mRenderAlgoParam ?
    
    // Test to get max size depending on quality..
    RC_FLOAT cellSize = (bbox[1][0] - bbox[0][0])/resolution[0];
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
        RC_FLOAT minZ = INF;
        for (int k = 0; k < points0.size(); k++)
        {
            Point &p = points0[k];
            
#define EPS_ALPHA 1
            // Ignore if it is transparen
            if (p.mRGBA[3] < EPS_ALPHA)
                continue;
                
            if (p.mZ < minZ)
            {
                minZ = p.mZ;
            }
        }
            
        blendPoint.mZ = minZ;
            
        // Blending
        unsigned char rgba[4];
        BlendDepthSorting(rgba, points0);
        
        *((int *)&blendPoint.mRGBA) = *((int *)&rgba);
        
        //blendPoint.mIsSelected = false;
            
        blendPoint.mX = points0[0].mX;
        blendPoint.mY = points0[0].mY;
        
        blendPoint.mSize[0] = points0[0].mSize[0];
        blendPoint.mSize[1] = points0[0].mSize[1];
        
        points->push_back(blendPoint);
    }
}

void
RayCaster2::DoRayCastKdTree(vector<Point> *points, int width, int height)
{
    mWidth = width;
    mHeight = height;
    
    // Hack
    if (mFirstTimeRender)
    {
        if (mAutoQualityFlag)
            AdaptZoomQuality();
        else
            SetQuality(mQuality);
        
        mFirstTimeRender = false;
    }
    
    RC_FLOAT quality = mQuality;
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
    points->erase(unstable_remove_if(points->begin(), points->end(), Point::IsThrsDiscarded),
                  points->end());
#endif
    
    RCKdTree::Clear(mKdTree);
    
    if (mRenderAlgo == RENDER_ALGO_KD_TREE_MEDIAN)
        RCKdTree::InsertPoints(mKdTree, *points, RCKdTree::SPLIT_METHOD_MEDIAN);
    else if (mRenderAlgo == RENDER_ALGO_KD_TREE_MIDDLE)
        RCKdTree::InsertPoints(mKdTree, *points, RCKdTree::SPLIT_METHOD_MIDDLE);
    
    RCKdTreeVisitor visitor;
    
    //RCQuadTreeVisitor::TagBoundaries(mQuadTree);
    RC_FLOAT maxSize = mRenderAlgoParam*width;
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
        RC_FLOAT minZ = INF;
        for (int k = 0; k < points0.size(); k++)
        {
            Point &p = points0[k];
            
#define EPS_ALPHA 1
            // Ignore if it is transparent
            if (p.mRGBA[3] < EPS_ALPHA)
                continue;
            
            if (p.mZ < minZ)
            {
                minZ = p.mZ;
            }
        }
        
        blendPoint.mZ = minZ;
        
        // Blending
        unsigned char rgba[4];
        BlendDepthSorting(rgba, points0);
        
        *((int *)&blendPoint.mRGBA) = *((int *)&rgba);
        
        //blendPoint.mIsSelected = false;
        
        blendPoint.mX = points0[0].mX;
        blendPoint.mY = points0[0].mY;
        
        blendPoint.mSize[0] = points0[0].mSize[0];
        blendPoint.mSize[1] = points0[0].mSize[1];
        
        points->push_back(blendPoint);
    }
}

void
RayCaster2::ComputePackedPointColor(const RayCaster2::Point &p, NVGcolor *color)
{
    // Optim: pre-compute nvg color
    unsigned char color0[4] = { p.mRGBA[0], p.mRGBA[1], p.mRGBA[2], p.mRGBA[3] };
    
    if (color0[3] > 255)
        color0[3] = 255;
    
    SWAP_COLOR(color0);
    
    *color = nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
}

void
RayCaster2::SetColorMap(int colormapId)
{
    const int numColorMaps = 7;
    ColorMapFactory::ColorMap colormaps[numColorMaps] =
    {
        ColorMapFactory::COLORMAP_BLUE,
        ColorMapFactory::COLORMAP_OCEAN,
        ColorMapFactory::COLORMAP_PURPLE,
        ColorMapFactory::COLORMAP_WASP,
        ColorMapFactory::COLORMAP_DAWN_FIXED,
        ColorMapFactory::COLORMAP_RAINBOW,
        ColorMapFactory::COLORMAP_SWEET
    };
    
    if (colormapId >= numColorMaps)
        return;
    
    // Method to create a good colormap
    // - take wasp (export is as ppm)
    // - rescale it to length 128
    // - change hue
    // - pick colors at 1 (for 0.25), 14 (for 0.5) and 40 (for 0.75)
    // color at 0.0 is 0, color at 1.0 is 255
    // - envetually set the same color as wasp for 0.25 (if the new color looks too dirty)
    
    // Keep the current range and contrast
    RC_FLOAT range = 0.0;
    RC_FLOAT contrast = 0.5;
    
    if (mColorMap != NULL)
    {
        range = mColorMap->GetRange();
        contrast = mColorMap->GetContrast();
        
        delete mColorMap;
        mColorMap = NULL;
    }
    
    mColorMap = mColorMapFactory->CreateColorMap(colormaps[colormapId]);
    
    if (mColorMap != NULL)
    {
        // Apply the current range and contrast
        mColorMap->SetRange(range);
        mColorMap->SetContrast(contrast);
    }

    ResetPointColormapSetFlags();
}

void
RayCaster2::SetInvertColormap(bool flag)
{
    mInvertColormap = flag;
    
    ResetPointColormapSetFlags();
}

void
RayCaster2::SetColormapRange(RC_FLOAT range)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetRange(range);
        
        ResetPointColormapSetFlags();
    }
}

void
RayCaster2::SetColormapContrast(RC_FLOAT contrast)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetContrast(contrast);
        
        ResetPointColormapSetFlags();
    }
}

bool
RayCaster2::GetSelection(RC_FLOAT selection[4])
{
    for (int i = 0; i < 4; i++)
        selection[i] = mScreenSelection[i];
    
    ReorderScreenSelection(selection);
    
    if (!mSelectionEnabled || !mVolumeSelectionEnabled)
        return false;
    
    return true;
}

void
RayCaster2::SetSelection(const RC_FLOAT selection[4])
{
    for (int i = 0; i < 4; i++)
        mScreenSelection[i] = selection[i];
    
    // Re-order the selection
    if (mScreenSelection[2] < mScreenSelection[0])
    {
        RC_FLOAT tmp = mScreenSelection[2];
        mScreenSelection[2] = mScreenSelection[0];
        mScreenSelection[0] = tmp;
    }
    
    if (mScreenSelection[3] < mScreenSelection[1])
    {
        RC_FLOAT tmp = mScreenSelection[3];
        mScreenSelection[3] = mScreenSelection[1];
        mScreenSelection[1] = tmp;
    }
    
    mSelectionEnabled = true;
    
    SelectionToVolume();
    
    mVolumeSelectionEnabled = true;
}

void
RayCaster2::EnableSelection()
{
    mSelectionEnabled = true;
}

void
RayCaster2::DisableSelection()
{
    mSelectionEnabled = false;
}

bool
RayCaster2::IsSelectionEnabled()
{
    return mSelectionEnabled;
}

bool
RayCaster2::IsVolumeSelectionEnabled()
{
    return mVolumeSelectionEnabled;
}

void
RayCaster2::ResetSelection()
{
    mSelectionEnabled = false;
    
    mVolumeSelectionEnabled = false;
    
    RC_FLOAT corner0[3] = { -0.5, 0.0, 0.5 };
    RC_FLOAT corner6[3] = { 0.5, 1.0, -0.5 };
    
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
RayCaster2::GetSliceSelection(vector<bool> *selection,
                              vector<RC_FLOAT> *xCoords,
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
        
        //bool selected = p.mIsSelected;
        bool selected = (p.mFlags | RC_POINT_FLAG_SELECTED);
        (*selection)[i] = selected;
        
        RC_FLOAT x = p.mX;
        (*xCoords)[i] = x;
    }
    
    return true;
}

bool
RayCaster2::GetSelectionBoundsLines(int *startLine, int *endLine)
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

long
RayCaster2::GetCurrentSlice()
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
    
    RC_FLOAT t = (mVolumeSelection[0].mZ + 0.5 + mVolumeSelection[4].mZ + 0.5)/2.0;
    
#if FIX_PLAY_SELECTION_REVERSED
    if (mScrollDirection == BACK_FRONT)
#endif
        t = 1.0 - t;
        
    long res = t*numSlices;
    
    return res;
}

void
RayCaster2::SelectionToVolume()
{
#define EPS 1e-15
    
    RC_FLOAT screenSelection[4];
    ScreenToAlignedVolume(mScreenSelection, screenSelection);
    
    // 
    ReorderScreenSelection(screenSelection);
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        RC_FLOAT zMin = 0.5;
        RC_FLOAT zMax = -0.5;
        
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
        RC_FLOAT xMin = -0.5;
        RC_FLOAT xMax = 0.5;
        
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
        RC_FLOAT xMin = -0.5;
        RC_FLOAT xMax = 0.5;
        
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
        RC_FLOAT yMin = 0.0;
        RC_FLOAT yMax = 1.0;
        
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
        RC_FLOAT yMin = 0.0;
        RC_FLOAT yMax = 1.0;
        
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
        RC_FLOAT yMin = 0.0;
        RC_FLOAT yMax = 1.0;
        
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
RayCaster2::VolumeToSelection()
{
#define EPS 1e-15
    
    RC_FLOAT screenSelection[4];
    
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
RayCaster2::TranslateVolumeSelection(RC_FLOAT trans[3])
{
    for (int i = 0; i < 8; i++)
    {
        mVolumeSelection[i].mX += trans[0];
        mVolumeSelection[i].mY += trans[1];
        mVolumeSelection[i].mZ += trans[2];
    }
}

void
RayCaster2::ScreenToAlignedVolume(const RC_FLOAT screenSelectionNorm[4],
                                 RC_FLOAT alignedVolumeSelection[4])
{
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        RC_FLOAT corner[3] = { mVolumeSelection[0].mX, mVolumeSelection[0].mY, mVolumeSelection[0].mZ };
        RC_FLOAT projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        RC_FLOAT p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        RC_FLOAT p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[0];
        alignedVolumeSelection[1] = p3_0[1];
        
        alignedVolumeSelection[2] = p3_1[0];
        alignedVolumeSelection[3] = p3_1[1];
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        RC_FLOAT corner[3] = { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ };
        RC_FLOAT projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        RC_FLOAT p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        RC_FLOAT p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[2];
        alignedVolumeSelection[1] = p3_0[1];
        
        alignedVolumeSelection[2] = p3_1[2];
        alignedVolumeSelection[3] = p3_1[1];
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        RC_FLOAT corner[3] = { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ };
        RC_FLOAT projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        RC_FLOAT p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        RC_FLOAT p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[2];
        alignedVolumeSelection[1] = p3_0[1];
        
        alignedVolumeSelection[2] = p3_1[2];
        alignedVolumeSelection[3] = p3_1[1];
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        RC_FLOAT corner[3] = { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ };
        RC_FLOAT projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        RC_FLOAT p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        RC_FLOAT p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[0];
        alignedVolumeSelection[1] = p3_0[2];
        
        alignedVolumeSelection[2] = p3_1[0];
        alignedVolumeSelection[3] = p3_1[2];
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left (right on the view)
    {
        RC_FLOAT corner[3] = { mVolumeSelection[7].mX, mVolumeSelection[7].mY, mVolumeSelection[7].mZ };
        RC_FLOAT projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        RC_FLOAT p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        RC_FLOAT p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = 1.0 - p3_0[2];
        alignedVolumeSelection[1] = p3_0[0];
        
        alignedVolumeSelection[2] = 1.0 - p3_1[2];
        alignedVolumeSelection[3] = p3_1[0];
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right (left on the view)
    {
        RC_FLOAT corner[3] = { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ };
        RC_FLOAT projCorner[3];
        ProjectPoint(projCorner, corner, mViewWidth, mViewHeight);
        
        RC_FLOAT p2_0[3] = { screenSelectionNorm[0]*mViewWidth, (1.0 - screenSelectionNorm[1])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_0[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_0, p3_0);
        
        RC_FLOAT p2_1[3] = { screenSelectionNorm[2]*mViewWidth, (1.0 - screenSelectionNorm[3])*mViewHeight, projCorner[2] };
        RC_FLOAT p3_1[3];
        CameraUnProject(mViewWidth, mViewHeight, mCamAngle0, mCamAngle1, mCamFov, p2_1, p3_1);
        
        alignedVolumeSelection[0] = p3_0[2];
        alignedVolumeSelection[1] = 1.0 - p3_0[0];
        
        alignedVolumeSelection[2] = p3_1[2];
        alignedVolumeSelection[3] = 1.0 - p3_1[0];
    }
}

// Align selection to colume nearest face
void
RayCaster2::AlignedVolumeToScreen(const RC_FLOAT alignedVolumeSelection[4],
                                  RC_FLOAT screenSelectionNorm[4])
{
    RC_FLOAT projFace[2][3];
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        RC_FLOAT face[2][3] = { { mVolumeSelection[0].mX, mVolumeSelection[0].mY, mVolumeSelection[0].mZ },
                              { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        RC_FLOAT face[2][3] = { { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ },
                              { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        RC_FLOAT face[2][3] = { { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ },
                              { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        RC_FLOAT face[2][3] = { { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ },
                              { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        RC_FLOAT face[2][3] = { { mVolumeSelection[7/*4*/].mX,
                                  mVolumeSelection[7/*4*/].mY,
                                  mVolumeSelection[7/*4*/].mZ },
                                { mVolumeSelection[2/*1*/].mX,
                                  mVolumeSelection[2/*1*/].mY,
                                  mVolumeSelection[2/*1*/].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        RC_FLOAT face[2][3] = { { mVolumeSelection[2/*1*/].mX,
                                  mVolumeSelection[2/*1*/].mY,
                                  mVolumeSelection[2/*1*/].mZ },
                                { mVolumeSelection[7/*4*/].mX,
                                  mVolumeSelection[7/*4*/].mY,
                                  mVolumeSelection[7/*4*/].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    RC_FLOAT screenSelection[4] = { projFace[0][0], projFace[0][1],
                                  projFace[1][0], projFace[1][1] };
    
    screenSelectionNorm[0] = screenSelection[0]/mViewWidth;
    screenSelectionNorm[1] = 1.0 - screenSelection[1]/mViewHeight;
    screenSelectionNorm[2] = screenSelection[2]/mViewWidth;
    screenSelectionNorm[3] = 1.0 - screenSelection[3]/mViewHeight;
}

void
RayCaster2::ReorderScreenSelection(RC_FLOAT screenSelection[4])
{
    // Re-order the selection
    if (screenSelection[2] < screenSelection[0])
    {
        RC_FLOAT tmp = screenSelection[2];
        screenSelection[2] = screenSelection[0];
        screenSelection[0] = tmp;
    }
    
    if (screenSelection[3] < screenSelection[1])
    {
        RC_FLOAT tmp = screenSelection[3];
        screenSelection[3] = screenSelection[1];
        screenSelection[1] = tmp;
    }
}

void
RayCaster2::SetDisplayRefreshRate(int displayRefreshRate)
{
#if !DISABLE_DISPLAY_REFRESH_RATE
    mDisplayRefreshRate = displayRefreshRate;
#endif
}

void
RayCaster2::SetQuality(RC_FLOAT quality)
{
    mQuality = quality;
    
    RC_FLOAT cellSize = ComputeCellSize(mWidth, mHeight);
    //
    mPointSize = cellSize + 1.0;
    
    mQualityChanged = true;
}

void
RayCaster2::SetAutoQuality(RC_FLOAT quality)
{
    mAutoQuality = quality;
    
    RC_FLOAT cellSize = ComputeCellSize(mWidth, mHeight);
    //
    mPointSize = cellSize + 1.0;
    
    mQualityChanged = true;
}

void
RayCaster2::SetQualityT(RC_FLOAT qualityT)
{
    mQualityT = qualityT;
}

void
RayCaster2::SetPointSize(RC_FLOAT size)
{
    mPointSize = (1.0 - size)*MIN_POINT_SIZE + size*MAX_POINT_SIZE;
}

void
RayCaster2::SetAlphaCoeff(RC_FLOAT alphaCoeff)
{
    mAlphaCoeff = alphaCoeff;
}

void
RayCaster2::SetThreshold(RC_FLOAT threshold)
{
    mThreshold = threshold;
}

void
RayCaster2::SetThresholdCenter(RC_FLOAT thresholdCenter)
{
    mThresholdCenter = thresholdCenter;
}

void
RayCaster2::SetClipFlag(bool flag)
{
    mClipFlag = flag;
}

void
RayCaster2::SetPlayBarPos(RC_FLOAT t)
{
    mPlayBarPos = t;
}

void
RayCaster2::SetAxis(int idx, Axis3D *axis)
{
    if (idx >= NUM_AXES)
        return;
    
    if (mAxis[idx] != NULL)
        delete mAxis[idx];
    
    mAxis[idx] = axis;
}

void
RayCaster2::ProjectPoint(RC_FLOAT projP[3], const RC_FLOAT p[3], int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height,
                       mCamAngle0, mCamAngle1, mCamFov,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    // Matrix transform
    glm::vec4 v;
    v.x = p[0];
    v.y = p[1];
    v.z = p[2];
    v.w = 1.0;
    
    glm::vec4 v4 = modelProjMat*v;
    
    RC_FLOAT x = v4.x;
    RC_FLOAT y = v4.y;
    RC_FLOAT z = v4.z;
    RC_FLOAT w = v4.w;
    
#define EPS 1e-8
    if (fabs(w) > EPS)
    {
        // Optim
        RC_FLOAT wInv = 1.0/w;
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
RayCaster2::SetRenderAlgo(RenderAlgo algo)
{
    mRenderAlgo = algo;
    
    // Hack for refreshing
    mQualityChanged = true;
}

void
RayCaster2::SetRenderAlgoParam(RC_FLOAT renderParam)
{
    mRenderAlgoParam = renderParam;
}

void
RayCaster2::SetAutoQuality(bool flag)
{
    mAutoQualityFlag = flag;
    
    if (!mAutoQualityFlag)
        SetQuality(mQuality);
    else
        AdaptZoomQuality();
}

void
RayCaster2::DrawSelection(NVGcontext *vg, int width, int height)
{
    if (!mSelectionEnabled)
        return;
    
    // Avoid display a big square at first selection
    if (!mVolumeSelectionEnabled)
        return;
    
    RC_FLOAT strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1], colors[i][2], colors[i][3]));
        
        // Draw the circle
        nvgBeginPath(vg);
        
        // Draw the line
        nvgMoveTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[1])*height);
        
        nvgLineTo(vg, mScreenSelection[2]*width, (1.0 - mScreenSelection[1])*height);
        nvgLineTo(vg, mScreenSelection[2]*width, (1.0 - mScreenSelection[3])*height);
        nvgLineTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[3])*height);
        nvgLineTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[1])*height);
        
        nvgStroke(vg);
    }
}

void
RayCaster2::DrawSelectionVolume(NVGcontext *vg, int width, int height)
{
    // Avoid displaying the volume when no selection has been done yet
    // (it would display the full cube)
    if (!mVolumeSelectionEnabled)
        return;
    
    RC_FLOAT volume[8][3] =
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
    
    // Projected volume
    RC_FLOAT projectedVolume[8][3];
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
RayCaster2::DrawPlayBarPos(NVGcontext *vg, int width, int height)
{
    if (mPlayBarPos < 0.0)
        return;
    
    RC_FLOAT playBarPosZ = mPlayBarPos - 0.5;
    
    RC_FLOAT square[4][3] =
    {
        { mVolumeSelection[0].mX, mVolumeSelection[0].mY, playBarPosZ },
        { mVolumeSelection[1].mX, mVolumeSelection[1].mY, playBarPosZ },
        { mVolumeSelection[2].mX, mVolumeSelection[2].mY, playBarPosZ },
        { mVolumeSelection[3].mX, mVolumeSelection[3].mY, playBarPosZ },
    };
    
    // Projected volume
    RC_FLOAT projectedSquare[4][3];
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
RayCaster2::SelectPoints3D(vector<Point> *points)
{
    // Unselect all
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
    
        //p.mIsSelected = false;
        
        // See: https://www.eskimo.com/~scs/cclass/int/sx4ab.html
        p.mFlags = p.mFlags & ~RC_POINT_FLAG_SELECTED;
    }
    
    RC_FLOAT minCorner[3] = { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ };
    RC_FLOAT maxCorner[3] = { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ };
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        //if (!p.mIsActive)
        if (p.mFlags & RC_POINT_FLAG_THRS_DISCARD)
            continue;
#endif

        if ((p.mX < minCorner[0]) || (p.mX > maxCorner[0]))
            continue;
        
        if ((p.mY < minCorner[1]) || (p.mY > maxCorner[1]))
            continue;
        
        if ((p.mZ < minCorner[2]) || (p.mZ > maxCorner[2]))
            continue;
        
        //p.mIsSelected = true;
        p.mFlags |= RC_POINT_FLAG_SELECTED;
    }
}

#if !THRESHOLD_OPTIM2
void
RayCaster2::ThresholdPoints(vector<Point> *points)
{
    // NOTE: the push_back() too many resources
    // NOTE: We process around 90000 points
    
    // First, count the result size
    long resultSize = 0;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        RC_FLOAT w = p.mWeight;
        
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
        
        RC_FLOAT w = p.mWeight;
        
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

#if THRESHOLD_OPTIM2
void
RayCaster2::ThresholdPoints(vector<Point> *points)
{
    // Adjust
    RC_FLOAT threshold = 1.0 - mThreshold;
    RC_FLOAT thresholdCenter = 1.0 - mThresholdCenter;
    
    // Compute min and max
    RC_FLOAT minThreshold = thresholdCenter - threshold;
    if (minThreshold < 0.0)
        minThreshold = 0.0;
    
    RC_FLOAT maxThreshold = thresholdCenter + threshold;
    if (maxThreshold > 1.0)
        maxThreshold = 1.0;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        //p.mIsActive = false;
        p.mFlags |= RC_POINT_FLAG_THRS_DISCARD;
        
        RC_FLOAT w = p.mWeight;
        
        // New, with mThreshold and mThresholdCenter
        if (!mInvertColormap)
        {
            if ((w > minThreshold) && (w < maxThreshold))
                //p.mIsActive = true;
                p.mFlags = p.mFlags & ~RC_POINT_FLAG_THRS_DISCARD;
        }
        else
        {
            if ((w < minThreshold) || (w > maxThreshold))
                //p.mIsActive = true;
                p.mFlags = p.mFlags & ~RC_POINT_FLAG_THRS_DISCARD;
        }
    }
}
#endif

void
RayCaster2::ClipPoints(vector<Point> *points)
{
    vector<Point> result;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        //if (!p.mIsActive)
        if (p.mFlags | RC_POINT_FLAG_THRS_DISCARD)
            continue;
#endif

        //if (p.mIsSelected)
        if (p.mFlags | RC_POINT_FLAG_SELECTED)
            result.push_back(p);
    }
    
    *points = result;
}

void
RayCaster2::ApplyColorMap(vector<Point> *points)
{
    ColorMap4::CmColor color;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if THRESHOLD_OPTIM2
        //if (!p.mIsActive)
       if (p.mFlags & RC_POINT_FLAG_THRS_DISCARD)
            continue;
#endif
        
        if (p.mFlags & RC_POINT_FLAG_COLORMAP_SET)
            continue;
        
        RC_FLOAT t = p.mWeight;
        
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
        
        mColorMap->GetColor(t, &color);
        
        *((unsigned int *)&p.mRGBA) = color;
        p.mFlags |= RC_POINT_FLAG_COLORMAP_SET;
    }
}

void
RayCaster2::ApplyColorMap(Point *p)
{
#if THRESHOLD_OPTIM2
    //if (!p->mIsActive)
    if (p->mFlags & RC_POINT_FLAG_THRS_DISCARD)
        return;
#endif
    
    if (p->mFlags & RC_POINT_FLAG_COLORMAP_SET)
        return;
    
    RC_FLOAT t = p->mWeight;
        
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
        
    mColorMap->GetColor(t, (ColorMap4::CmColor *)&p->mRGBA);
    p->mFlags |= RC_POINT_FLAG_COLORMAP_SET;
}

void
RayCaster2::UpdateSlicesZ()
{    
    // Adjust z for all the history
    for (int i = 0; i < mSlices.size(); i++)
    {
        // Compute time step
        RC_FLOAT z = 1.0 - ((RC_FLOAT)i)/mSlices.size();
        
        // Reverse scroll direction if necessary
        if (mScrollDirection == FRONT_BACK)
            z = 1.0 - z;
        
        // Center
        z -= 0.5;
        
        // Get the slice
        vector<RayCaster2::Point> &points = mSlices[i];
        
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

// Manage float step
//
// More verbose implementation to try to optimize
// (but does not optimize a lot)
void
RayCaster2::SlicesToVec(vector<RayCaster2::Point> *points,
                        const deque<vector<RayCaster2::Point> > &slices)
{
    // If mQualityT is 1 => no decim
    // If mQualityT is 0.1 => take 1 slice over 10
    
    RC_FLOAT step = 1.0/mQualityT;
    RC_FLOAT offset = 0.0;
    
#if FIX_JITTER_TIME_QUALITY
    offset = fmod(mSliceNum, step);
    
    offset = -offset;
    while (offset < 0.0)
        offset += step;
#endif
    
    // Compute the number of elements
    long numElements = 0;
    
#if 0 // Reversed display order ?
    RC_FLOAT i0 = offset;
    while((int)i0 < slices.size())
    {
        int i = (int)i0;
        numElements += slices[i].size();
        
        i0 += step;
    }
#else
    RC_FLOAT i0 = slices.size() - 1 - (int)offset;
    while((int)i0 >= 0.0)
    {
        numElements += slices[(int)i0].size();
        
        i0 -= step;
    }
#endif
    
    points->resize(numElements);
    
    long elementId = 0;
#if 0 // Reversed display order ?
    RC_FLOAT i1 = offset;
    while((int)i1 < slices.size())
    {
#else
    RC_FLOAT i1 = slices.size() - 1 - (int)offset;
    while((int)i1 >= 0.0)
    {
#endif
        const vector<RayCaster2::Point> &vec = slices[(int)i1];
        for (int j = 0; j < vec.size(); j++)
        {
            const Point &val = vec[j];
            (*points)[elementId++] = val;
        }
        
#if 0 // Reversed display order ?
        i1 += step;
#else
        i1 -= step;
#endif
    }
}
  
// Manage float step
//
// More verbose implementation to try to optimize
// (but does not optimize a lot)
void
RayCaster2::SlicesToVecPreSort(vector<RayCaster2::Point> *points,
                               deque<vector<RayCaster2::Point> > &slices)
{
//#define EPS 1e-15
    
#if !OPTIM_PRE_SORT_DICHO
    int splitIdxZ = ComputeMostAlignedSliceIdx(mViewWidth, mViewHeight, AXIS_Z);
#else
    int splitIdxZ = ComputeMostAlignedSliceIdxDicho(mViewWidth, mViewHeight, AXIS_Z);
#endif
    
    int signX;
    int signY;
    int signZ;
    
    //int splitIdxZ;
    ComputeViewSigns(&signX, &signY, &signZ); //, &splitIdxZ);
    
    // Now, sort each slice we add
    // Then sort again when displaying, if the view moved enough
    // to change some of the view signs
    bool viewSignsChanged = ViewSignsChanged(signX, signY, signZ);
    
#if OPTIM_PRE_SORT_SORT_CAM
    //viewSignsChanged = true; // For the moment, update each time...
#endif
    
    // Without this, the front view is displayed in the bad order
    if (splitIdxZ == 0)
    //if (splitIdxZ < EPS)
        splitIdxZ = slices.size();
    
    //fprintf(stderr, "---\nidx: %d\n", splitIdxZ);
    //fprintf(stderr, "signs: [%d %d %d]\n", signX, signY, signZ);
    
    // If mQualityT is 1 => no decim
    // If mQualityT is 0.1 => take 1 slice over 10
    RC_FLOAT step = 1.0/mQualityT;
    RC_FLOAT offset = 0.0;
    
#if 0 // Debug, to fix jitter
    //step = 1.0;
    step = 11.11;
#endif
    
    // Avoid jitter: when playing, make scroll existing slices
    // instead of having static slices, and scrolling data
#if FIX_JITTER_TIME_QUALITY
    offset = fmod(mSliceNum, step);
        
    offset = -offset;
    while (offset < 0.0)
        offset += step;
#endif
    
    // Use a list of slices numbers to display
    // Create the list at the beginning
    // and manage it after
    // (instead of re-computing complex and buggy offsets each time)
    
    // Compute the number of elements and the slice numbers
    long numElements = 0;
    vector<int> sliceNumbers;
    RC_FLOAT i0 = offset;
    while((int)i0 < slices.size())
    {
        int i = (int)i0;
        numElements += slices[i].size();
        
        sliceNumbers.push_back(i);
        
        i0 += step;
    }
    
    points->resize(numElements);
    
#if 1
    // Find the closed slice
    // => This avoid jittering when no scrolling,
    // when rotating the view and splitIdxZ is changing
    int closestSlice = 0;
    int minDist = sliceNumbers.size();
    for (int i = 0; i < sliceNumbers.size(); i++)
    {
        int sliceNum = sliceNumbers[i];
        
        /*RC_FLOAT*/ int dist = abs(sliceNum - splitIdxZ);
        if (dist < minDist)
        {
            minDist = dist;
            closestSlice = sliceNum;
        }
    }
    
    //fprintf(stderr, "closest idx: %d (diff: %d)\n",
    //        closestSlice, closestSlice - splitIdxZ);
    
    splitIdxZ = closestSlice;
#endif
    
    //
    //vector<int> DBG_SliceIds;
    //vector<double> DBG_SliceIds1;
    //vector<double> DBG_SliceIds2;
    
    // Fill points
    //
    long elementId = 0;
    
#if 1 // Draw the background (using slices list)
    for (int i = (int)sliceNumbers.size() - 1; i >= 0; i--)
    {
        int sliceNum = sliceNumbers[i];
        if (sliceNum > splitIdxZ)
            continue;
        
        vector<RayCaster2::Point> &vec = slices[sliceNum];
        
        if (viewSignsChanged)
            SortVec(&vec, signX, signY, signZ);
        
        for (int j = 0; j < vec.size(); j++)
        {
            const Point &val = vec[j];
            (*points)[elementId++] = val;
        }
        
        //DBG_SliceIds.push_back(sliceNum);
        //DBG_SliceIds1.push_back(sliceNum);
    }
#endif
    
#if 1 // Draw the foreground (using slices list)
    for (int i = 0; i < sliceNumbers.size(); i++)
    {
        int sliceNum = sliceNumbers[i];
        if (sliceNum <= splitIdxZ)
            continue;
        
        vector<RayCaster2::Point> &vec = slices[sliceNum];
        
        if (viewSignsChanged)
            SortVec(&vec, signX, signY, signZ);
        
        for (int j = 0; j < vec.size(); j++)
        {
            const Point &val = vec[j];
            (*points)[elementId++] = val;
        }
        
        //DBG_SliceIds.push_back(sliceNum);
        //DBG_SliceIds2.push_back(sliceNum);
    }
#endif

    // TEST
    //SortVec(points, signX, signY, signZ);
    
    // DEBUG
    /*sort(DBG_SliceIds.begin(), DBG_SliceIds.end());
    
    WDL_TypedBuf<double> DBG_SliceIdsWd;
    DBG_SliceIdsWd.Resize(DBG_SliceIds.size());
    for (int i = 0; i < DBG_SliceIdsWd.GetSize(); i++)
        DBG_SliceIdsWd.Get()[i] = DBG_SliceIds[i];
    Debug::DumpData("slices.txt", DBG_SliceIdsWd);
     
    WDL_TypedBuf<double> DBG_SliceIdsWd1;
    DBG_SliceIdsWd1.Resize(DBG_SliceIds1.size());
    for (int i = 0; i < DBG_SliceIdsWd1.GetSize(); i++)
        DBG_SliceIdsWd1.Get()[i] = DBG_SliceIds1[i];
    Debug::DumpData("slices1.txt", DBG_SliceIdsWd1);
     
    WDL_TypedBuf<double> DBG_SliceIdsWd2;
    DBG_SliceIdsWd2.Resize(DBG_SliceIds2.size());
    for (int i = 0; i < DBG_SliceIdsWd2.GetSize(); i++)
    DBG_SliceIdsWd2.Get()[i] = DBG_SliceIds2[i];
    Debug::DumpData("slices2.txt", DBG_SliceIdsWd2);
     
    Debug::DumpValue("idx.txt", splitIdxZ);*/
}
    
#if !OPTIM_PRE_SORT_SORT_CAM
// Use view signs
void
RayCaster2::SortVec(vector<Point> *vec, int signX, int signY, int signZ)
{
    // Sort X before or after Y ?
    if (signY >= /**/0)
    {
        sort(vec->begin(), vec->end(), Point::IsGreaterY);
        if (signX >= /**/0)
            stable_sort(vec->begin(), vec->end(), Point::IsGreaterX);
        if (signX < 0)
            stable_sort(vec->begin(), vec->end(), Point::IsSmallerX);
    }
    else if (signY < 0)
    {
        if (signX >= /**/0)
            sort(vec->begin(), vec->end(), Point::IsGreaterX);
        if (signX < 0)
            sort(vec->begin(), vec->end(), Point::IsSmallerX);
            
        stable_sort(vec->begin(), vec->end(), Point::IsGreaterY);
    }
}
#else
// Use distance from the camera
void
RayCaster2::SortVec(vector<Point> *vec, int signX, int signY, int signZ)
{
    // TODO: put it somewhere else in the .h file
    
    // See: https://stackoverflow.com/questions/5733202/how-to-pass-user-data-to-compare-function-of-stdsort
    struct CloseToCam
    {
        CloseToCam(RC_FLOAT camPos[3], RC_FLOAT camAngle0)
        {
            mCamPos[0] = camPos[0];
            mCamPos[1] = camPos[1];
            mCamPos[2] = camPos[2];
            
            mCamAngle0 = camAngle0;
        };
        
        // TODO: optimize this by pre-computing
        bool operator()(const Point &p0, const Point &p1)
        {
#if 1 //0
            RC_FLOAT v0[3] = { p0.mX - mCamPos[0], p0.mY - mCamPos[1], p0.mZ - mCamPos[2] };
            RC_FLOAT d20 = v0[0]*v0[0] + v0[1]*v0[1] + v0[2]*v0[2];
            
            RC_FLOAT v1[3] = { p1.mX - mCamPos[0], p1.mY - mCamPos[1], p1.mZ - mCamPos[2] };
            RC_FLOAT d21 = v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2];
#endif
#if 0 //1
            // TODO: transform inverse camera, instead of transforming all points
            Point pp0;
            RayCaster2::TransformPointModel(p0, &pp0, mCamAngle0);
            
            Point pp1;
            RayCaster2::TransformPointModel(p1, &pp1, mCamAngle0);
            
            RC_FLOAT v0[3] = { pp0.mX - mCamPos[0], pp0.mY - mCamPos[1], pp0.mZ - mCamPos[2] };
            RC_FLOAT d20 = v0[0]*v0[0] + v0[1]*v0[1] + v0[2]*v0[2];
            
            RC_FLOAT v1[3] = { pp1.mX - mCamPos[0], pp1.mY - mCamPos[1], pp1.mZ - mCamPos[2] };
            RC_FLOAT d21 = v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2];
#endif
            
            return d20 < d21;
        }
        
        RC_FLOAT mCamPos[3];
        RC_FLOAT mCamAngle0;
    };
    
    RC_FLOAT camPos[3];
    ComputeCameraPosition(camPos, true);
    
    sort(vec->begin(), vec->end(), CloseToCam(camPos, mCamAngle0));
}
#endif
    
int
RayCaster2::GetResolution()
{
    RC_FLOAT quality = mQuality;
    if (mAutoQualityFlag)
        quality = mAutoQuality;
    
    int resolution = (1.0 - quality)*RESOLUTION_0 + quality*RESOLUTION_1;
    
    return resolution;
}

void
RayCaster2::BlendDepthSorting(unsigned char rgba[4], vector<Point> &points)
{
#if DISCARD_FARTHEST_RAY_POINTS
    DiscardFarthestRayPoints(&points);
    //DiscardFarthestRayPointsAdaptive(&points);
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
    sort(points.begin(), points.end(), Point::IsGreaterZ);
#endif
    
#if ALPHA_BLEND_RAYCASTED_TEXTURE
    RC_FLOAT alpha255 = mAlphaCoeff*255;
#endif
    
    int rgba0[4];
    
#if !FIX_BLEND_DEPTH_BG_ALPHA
    rgba0[0] = 0;
    rgba0[1] = 0;
    rgba0[2] = 0;
    rgba0[3] = 0;
#else
    // Background color
    rgba0[0] = 0;
    rgba0[1] = 0;
    rgba0[2] = 0;
    rgba0[3] = alpha255;
    // Set a background alpha!
    // So we don't have drak single pixels,
    //and we can see axes through the transparent voxels
#endif
    
    for (int k = 0; k < points.size(); k++)
    {
        const Point &p = points[k];
        
#if THRESHOLD_OPTIM2
        //if (!p.mIsActive)
        if (p.mFlags & RC_POINT_FLAG_THRS_DISCARD)
            continue;
#endif

        RC_FLOAT selectCoeff = 1.0;
        //if (!p.mIsSelected)
        if (!(p.mFlags & RC_POINT_FLAG_SELECTED))
            selectCoeff = SELECT_GRAY_COLOR_COEFF;
        
        RC_FLOAT colorCoeff = mAlphaCoeff*selectCoeff;
        
#if BLEND_LESS_HIGH_WEIGHTS
        RC_FLOAT weightAlphaCoeff = 0.0;
        if (p.mWeight > 0.0)
            weightAlphaCoeff = pow(mAlphaCoeff, (RC_FLOAT)1.0/p.mWeight);
        if (weightAlphaCoeff > 1.0)
            weightAlphaCoeff = 1.0;
        
        colorCoeff = weightAlphaCoeff*selectCoeff;
#endif
        
        // Makes a beautiful effect
        if (k == 0)
        {
            rgba0[0] += p.mRGBA[0]*colorCoeff;
            rgba0[1] += p.mRGBA[1]*colorCoeff;
            rgba0[2] += p.mRGBA[2]*colorCoeff;
             
#if !ALPHA_BLEND_RAYCASTED_TEXTURE
            rgba0[3] += mAlphaCoeff*255;
#else
            rgba0[3] += colorCoeff*alpha255;
#endif
        }
        else
        {
            rgba0[0] = (1.0 - colorCoeff)*rgba0[0] + colorCoeff*p.mRGBA[0];
            rgba0[1] = (1.0 - colorCoeff)*rgba0[1] + colorCoeff*p.mRGBA[1];
            rgba0[2] = (1.0 - colorCoeff)*rgba0[2] + colorCoeff*p.mRGBA[2];
            
#if !ALPHA_BLEND_RAYCASTED_TEXTURE
            rgba0[3] = 255;
#else
            rgba0[3] += colorCoeff*alpha255;
#endif
        }
    }
    
    // Clip
    if (!points.empty())
    {
        for (int k = 0; k < 4; k++)
        {
            if (rgba0[k] < 0)
                rgba0[k] = 0;
            if (rgba0[k] > 255)
                rgba0[k] = 255;
            
            rgba[k] = rgba0[k];
        }
    }
}

void
RayCaster2::BlendDepthSortingFTB(unsigned char rgba[4], vector<Point> &points)
{
    // Does not work perfectly...
    //
    // Avoid a bit the color band effect due to early termination
    //
    // NOTE: 0.01 does almost nothing
    // NOTE: 0.1: divide the band width by 2, but we still see them (reduce brightness a bit)
    // NOTE: 0.5: bands are almost not visible anymore,
    // but color brightness is very reduced for parts with few points
#define PONDERATE_COL_WITH_Z 0 //1
#define Z_COEFF 0.1 //0.5 //0.01 //0.1
    
    // Early termination alpha
    //
    // NOTE: with value < 0.95, early term makes some color bands
    // instead of smooth gradients (but it is not very important)
    // NOTE: with 0.9, we gain 12% perfs (74% -> 62%). And small bands (not disturbing).
    // NOTE: with 0.0, we gain 14% perfs (74% -> 60%). And dark color
#define EARLY_TERM_ALPHA 0.9 //0.95 //0.0 //1.0 //0.9 //0.75
  
    // Before this alpha, we don't take the point
    //
    // NOTE: 0.02 => perf gain 11% (77% -> 68%) (rander changed too much)
    // NOTE: 0.05 => perf gain 11% too
    // NOTE: 0.005 => perf gain 6% (77% -> 71%) (render changed a bit)
    // NOTE: 0.001 => perf gain 4% (77% -> 73%) (render not changed)
#define MIN_ALPHA 0.001 //0.0 //0.001 //0.005 //0.02 //0.05
    
#if OPTIM_AFTER_DISCARD_FARTHEST2
    ThresholdPoints(&points);
#endif
        
#if !OPTIM_PRE_SORT
    // Sort from front to back
    sort(points.begin(), points.end(), Point::IsSmallerZ);
#endif
        
    // Init
    RC_FLOAT rgba0[4] = { 0.0, 0.0, 0.0, 0.0 };
    bool earlyTerminated = false;
    
    for (int k = 0; k < points.size(); k++)
    {
        /*const */Point &p = points[k];
            
#if THRESHOLD_OPTIM2
        //if (!p.mIsActive)
        if (p.mFlags & RC_POINT_FLAG_THRS_DISCARD)
            continue;
#endif
            
        RC_FLOAT selectCoeff = 1.0;
        //if (!p.mIsSelected)
        if (!(p.mFlags & RC_POINT_FLAG_SELECTED))
            selectCoeff = SELECT_GRAY_COLOR_COEFF;
            
        RC_FLOAT colorCoeff = mAlphaCoeff*selectCoeff;
            
#if BLEND_LESS_HIGH_WEIGHTS
        RC_FLOAT weightAlphaCoeff = 0.0;
        if (p.mWeight > 0.0)
            weightAlphaCoeff = pow(mAlphaCoeff, (RC_FLOAT)1.0/p.mWeight);
        if (weightAlphaCoeff > 1.0)
            weightAlphaCoeff = 1.0;
            
        colorCoeff = weightAlphaCoeff*selectCoeff;
#endif
        
        if (colorCoeff <= MIN_ALPHA)
            continue;
        
#if PONDERATE_COL_WITH_Z
        colorCoeff *= (1.0 - p.mZ*Z_COEFF);
#endif
        
#if OPTIM_AFTER_DISCARD_FARTHEST
        // Apply colormap after having discarded,
        // so we apply colormap on less points
        ApplyColorMap(&p);
#endif
        
        RC_FLOAT colorCoeffInv255 = colorCoeff*(1.0/255.0);
        
        // Tested another method: does not touch alpha
//#if PONDERATE_COL_WITH_Z
//        colorCoeffInv255 *= (1.0 - p.mZ*Z_COEFF*(1.0/10.0));
//#endif
        
        // Front to back
        //
        // See: https://cgl.ethz.ch/teaching/former/scivis_07/Notes/stuff/StuttgartCourse/VIS-Modules-06-Direct_Volume_Rendering.pdf
        //
        rgba0[0] += (1.0 - rgba0[3])*colorCoeffInv255*p.mRGBA[0];
        rgba0[1] += (1.0 - rgba0[3])*colorCoeffInv255*p.mRGBA[1];
        rgba0[2] += (1.0 - rgba0[3])*colorCoeffInv255*p.mRGBA[2];
        
        rgba0[3] += (1.0 - rgba0[3])*colorCoeff;
        
        // Early termination criterion
        //
        if (rgba0[3] >= EARLY_TERM_ALPHA)
        // Current color is considered as opaque
        {
            earlyTerminated = true;

            break;
        }
    }
    
    if (earlyTerminated)
    // Current color is considered as opaque
    {
        rgba0[3] = 1.0;
    }
    
    // Clip
    if (!points.empty())
    {
        for (int k = 0; k < 4; k++)
        {
            if (rgba0[k] < 0.0)
                rgba0[k] = 0.0;
            if (rgba0[k] > 1.0)
                rgba0[k] = 1.0;
                
            rgba[k] = (int)(rgba0[k]*255.0);
        }
    }
}
    
// Use WBOIT method (a trick), to avoid sorting
// (sorting takes a lot of resource)
void
RayCaster2::BlendWBOIT(int rgba[4], vector<Point> &points0)
{
    rgba[0] = 0;
    rgba[1] = 0;
    rgba[2] = 0;
    rgba[3] = 0;
    
    // Find min Z
    RC_FLOAT minZ = INF;
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
        if (p.mZ < minZ)
            minZ = p.mZ;
    }
    
    // Find max Z
    RC_FLOAT maxZ = -INF;
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
        if (p.mZ > maxZ)
            maxZ = p.mZ;
    }
    
    // See: http://jcgt.org/published/0002/02/09/paper-lowres.pdf
    //
    RC_FLOAT sigmaCiWi[3] = { 0.0, 0.0, 0.0 };
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
     
        RC_FLOAT normZ = 0.0; //1.0;
        if (maxZ - minZ > EPS)
            normZ = (p.mZ - minZ)/(maxZ - minZ);
        normZ = Utils::LogScaleNorm2(normZ, 128.0);
        normZ = 1.0 - normZ;
        
        RC_FLOAT wi = normZ;
        
        sigmaCiWi[0] += p.mRGBA[0]*wi;
        sigmaCiWi[1] += p.mRGBA[1]*wi;
        sigmaCiWi[2] += p.mRGBA[2]*wi;
        
        // Ci have pre-multiplied alpha !
        sigmaCiWi[0] *= mAlphaCoeff;
        sigmaCiWi[1] *= mAlphaCoeff;
        sigmaCiWi[2] *= mAlphaCoeff;
        
        // Select coeff
        RC_FLOAT selectCoeff = 1.0;
        //if (!p.mIsSelected)
        if (!(p.mFlags & RC_POINT_FLAG_SELECTED))
            selectCoeff = SELECT_GRAY_COLOR_COEFF;
        
        sigmaCiWi[0] *= selectCoeff;
        sigmaCiWi[1] *= selectCoeff;
        sigmaCiWi[2] *= selectCoeff;
    }
    
    //
    RC_FLOAT sigmaAlphaiWi = 0.0;
    for (int k = 0; k < points0.size(); k++)
    {
        const Point &p = points0[k];
        
        RC_FLOAT normZ = 0.0;
        if (maxZ - minZ > EPS)
            normZ = (p.mZ - minZ)/(maxZ - minZ);
        normZ = Utils::LogScaleNorm2(normZ, 128.0);
        normZ = 1.0 - normZ;
        
        RC_FLOAT wi = normZ;
        
        sigmaAlphaiWi += mAlphaCoeff*wi;
    }
    
    //
    RC_FLOAT piAlphai = 1.0;
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
RayCaster2::RenderQuads(NVGcontext *vg, const vector<Point> &points)
{
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        NVGcolor color;
        ComputePackedPointColor(p, &color);
        
        nvgStrokeColor(vg, color);
        nvgFillColor(vg, color);
        
        RC_FLOAT x = p.mX;
        RC_FLOAT y = p.mY;
        
        // Quad rendering
        //
        // (maybe reverse order)
        //
#if !FIXED_POINT_SIZE
        RC_FLOAT size = p.mSize*mPointSize;
#else
        RC_FLOAT size = mPointSize;
#endif
        
        /*RC_FLOAT*/ double corners[4][2] =
                                 { { x - size/2.0, y - size/2.0 },
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
    
#if 0 // ORIG
// Render colored texture pixels
//
// The texture size is exactly the view size
//
void
RayCaster2::RenderTexture(NVGcontext *vg, int width, int height,
                          const vector<Point> &points)
{    
    if (mImage == NULL)
    {
        //mImage = new ImageDisplayColor(vg);
        mImage = new ImageDisplayColor(vg, ImageDisplayColor::MODE_NEAREST);
        
        mImage->SetBounds(0.0, 0.0, 1.0, 1.0);
        
        mImage->Show(true);
    }
    
    mImageData.Resize(width*height*4);
    memset(mImageData.Get(), 0, width*height*4*sizeof(unsigned char));
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        RC_FLOAT x = p.mX;
        RC_FLOAT y = p.mY;
        
        RC_FLOAT size[2] = { p.mSize[0], p.mSize[1] };
        
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
                
                *((int *)&mImageData.Get()[(j + k*width)*4]) = *((int *)&p.mRGBA);
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
#endif

#if 1 // NEW
// Render colored texture pixels
//
// OPTIM: If sizeCoeff is greater than 1, we render a smaller texture,
// with nearest interpolation. Then we display this texture
// scaled over the whole view.
// Optimizes 6% (76% -> 70%)
void
RayCaster2::RenderTexture(NVGcontext *vg, int width, int height,
                          const vector<Point> &points,
                          RC_FLOAT sizeCoeff)
{
    bool useCoeff = (sizeCoeff > 0.0);
    RC_FLOAT sizeCoeffInv = 1.0/sizeCoeff;
    
    if (useCoeff)
    {
        width *= sizeCoeffInv;
        height *= sizeCoeffInv;
    }

    if (mImage == NULL)
    {
        mImage = new ImageDisplayColor(vg, ImageDisplayColor::MODE_NEAREST);
            
        mImage->Show(true);
    }
    
    if (!useCoeff)
        mImage->SetBounds(0.0, 0.0, 1.0, 1.0);
    else
    {
        // Use offsets, otherwise we would have sometimes
        // black borders of size of about sizeCoeff
        RC_FLOAT offsetX = sizeCoeff/width;
        RC_FLOAT offsetY = sizeCoeff/height;
        
        mImage->SetBounds(0.0 - offsetX,
                          0.0 - offsetY,
                          // Use 1/width offset to avoid a black column of 1 pixel on the right
                          sizeCoeff + offsetX + 1.0/width,
                          sizeCoeff + offsetY);
    }
    
    mImageData.Resize(width*height*4);
    memset(mImageData.Get(), 0, width*height*4*sizeof(unsigned char));
        
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
      
#if !OPTIM_ADAPTIVE_TEXTURE_SIZE2
        RC_FLOAT x = p.mX;
        RC_FLOAT y = p.mY;
        RC_FLOAT size[2] = { p.mSize[0], p.mSize[1] };
#else
        int x = p.mXi;
        int y = p.mYi;
        int size[2] = { p.mSizei[0], p.mSizei[1] };
#endif
        
        if (useCoeff)
        {
            // If OPTIM_ADAPTIVE_TEXTURE_SIZE2, we already have computed pixel (i, j),
            // and set size to 1
#if !OPTIM_ADAPTIVE_TEXTURE_SIZE2
            x *= sizeCoeffInv;
            y *= sizeCoeffInv;
            size[0] *= sizeCoeffInv;
            size[1] *= sizeCoeffInv;
#endif
        }
        
        int startJ = (int)(x - size[0]/2);
        int endJ = (int)(x + size[0]/2);
        
        int startK = (int)(y - size[1]/2);
        int endK = (int)(y + size[1]/2);

#if OPTIM_ADAPTIVE_TEXTURE_SIZE2
        if (endJ == startJ)
            endJ = startJ + 1;
        if (endK == startK)
            endK = startK + 1;
#endif
        
        for (int j = startJ; j < endJ; j++)
            for (int k = startK; k < endK; k++)
            {
                if (j < 0)
                    continue;
                if (j >= width)
                    continue;
                    
                if (k < 0)
                    continue;
                if (k >= height)
                    continue;
                    
                *((int *)&mImageData.Get()[(j + k*width)*4]) = *((int *)&p.mRGBA);
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
#endif
    
long
RayCaster2::ComputeNumRayPoints(const vector<Point> &points, RC_FLOAT z)
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
RayCaster2::DiscardFarthestRayPoints(vector<Point> *points)
{
    // Do a dichotomic search, to find the correct Z that will keep only MAX_NUM_RAY_POINTS
    // Then dicard points corresponding to that Z.
    
    if (points->size() < MAX_NUM_RAY_POINTS)
        // num points is small enough
        return;
    
    // Compute min and max z
    RC_FLOAT minZ = INF;
    RC_FLOAT maxZ = -INF;
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
        RC_FLOAT currentZ = (minZ + maxZ)*0.5;
        
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
    RC_FLOAT z = (minZ + maxZ)*0.5;
    
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
        static bool ZIsFar(const Point &p, RC_FLOAT zLimit)
        {
            return (p.mZ > zLimit);
        }
    };
    
    points->erase(unstable_remove_if(points->begin(), points->end(), Pred::ZIsFar, z),
                  points->end());
    
#endif
}

void
RayCaster2::DiscardFarthestRayPointsAdaptive(vector<Point> *points)
{
    // DEBUG: IGNORE DISCARD ?
    //
    // If not discard, avoid artifact if we decrease the transparency
    // e.g: set transparency to 30%, look at the bottom
    // => strong black zones at the bottom
    //return;
    
    // Orig: 68%
    // With 32 => 65%
    // With 16 instead of 32, => 63% (but starts to jitter)
    // With 8 => 62%
#define MAX_NUM_RAY_POINTS 32
    
    // Do a dichotomic search, to find the correct Z that will keep only MAX_NUM_RAY_POINTS
    // Then dicard points corresponding to that Z.
        
    if (points->size() < MAX_NUM_RAY_POINTS)
        // num points is small enough
        return;
    
    // Compute the number of iterations
    //
    // Computing the number of iterations adaptively
    // makes possible to make fewer iterations when number of points is small
    RC_FLOAT ni = round(((RC_FLOAT)points->size())/MAX_NUM_RAY_POINTS);
    RC_FLOAT numDichoItersf = log(ni)/LOG_2;
    int numDichoIters = round(numDichoItersf);
    
    //
    //numDichoIters = numDichoIters + 1;
    
    //
    if (numDichoIters == 0)
        return;
    
    // Compute min and max z
    RC_FLOAT minZ = INF;
    RC_FLOAT maxZ = -INF;
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
            
        if (p.mZ < minZ)
            minZ = p.mZ;
        if (p.mZ > maxZ)
            maxZ = p.mZ;
    }
        
    // Dichotomic search of optimal z
    for (int i = 0; i < numDichoIters; i++)
    {
        RC_FLOAT currentZ = (minZ + maxZ)*0.5;
            
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
    RC_FLOAT z = (minZ + maxZ)*0.5;
        
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
        static bool ZIsFar(const Point &p, RC_FLOAT zLimit)
        {
            return (p.mZ > zLimit);
        }
    };
        
    points->erase(unstable_remove_if(points->begin(), points->end(), Pred::ZIsFar, z),
                    points->end());        
#endif
}
    
RC_FLOAT
RayCaster2::ComputeCellSize(int width, int height)
{
    int res = GetResolution();
    int resolution[2] = { res, res };
    
    RC_FLOAT maxDim = (width > height) ? width : height;
    RC_FLOAT bbox[2][2] = { { 0, 0 }, { maxDim, maxDim } };
    
    // Compute the grid cells color
    RC_FLOAT cellSize = (bbox[1][0] - bbox[0][0])/resolution[0];
    
    return cellSize;
}

void
RayCaster2::ComputeAxisPreDrawFlags(bool preDrawFlags[NUM_AXES])
{
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
RayCaster2::DrawAxes(NVGcontext *vg, int width, int height,
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
RayCaster2::DilateImageAux(int width, int height,
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
                        
                        
                        // Take max alpha
                        if (color0[3] > color[3])
                        {
                            *((unsigned int *)color) = *((unsigned int *)color0);
                        }
                    }
                }
            }
        }
    }
}
    
void
RayCaster2::DilateImage(int width, int height,
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
RayCaster2::AdaptZoomQuality()
{
    RC_FLOAT t = (mCamFov - MIN_CAMERA_FOV)/(MAX_CAMERA_FOV - MIN_CAMERA_FOV);
    
    // Change shape
    //t = pow(t, 2.0);
    
    RC_FLOAT minQuality = 0.0;
    RC_FLOAT maxQuality = 0.35;
    
    RC_FLOAT quality = (1.0 - t)*minQuality + t*maxQuality;
    
    SetAutoQuality(quality);
}

void
RayCaster2::ResetPointColormapSetFlags()
{
    for (int i = 0; i < mSlices.size(); i++)
    {
        vector<Point> &slice = mSlices[i];
        for (int j = 0; j < slice.size(); j++)
        {
            Point &p = slice[j];
            
            p.mFlags = p.mFlags & ~RC_POINT_FLAG_COLORMAP_SET;
        }
    }
}

void
RayCaster2::DBG_DrawSlice(NVGcontext *vg, int width, int height,
                          RC_FLOAT slice[4][3], int highlight)
{
    nvgSave(vg);
        
    // Projected slice
    RC_FLOAT projectedSlice[4][3];
    for (int i = 0; i < 4; i++)
    {
        ProjectPoint(projectedSlice[i], slice[i], width, height);
    }
        
    // Draw the projected slice
    nvgStrokeWidth(vg, 1.0);
    
    //
    int alpha = 128;
    
    int color[4] = { 255, 255, 255, alpha };
    if (highlight == 0)
    {
        color[0] = 255;
        color[1] = 0;
        color[2] = 0;
        color[3] = alpha;
            
        //nvgStrokeWidth(vg, 2.0);
        //nvgStrokeWidth(vg, 1.0);
    }
    
    if (highlight == 1)
    {
        color[0] = 0;
        color[1] = 255;
        color[2] = 0;
        color[3] = alpha;
    }
    
    if (highlight == 2)
    {
        color[0] = 0;
        color[1] = 0;
        color[2] = 255;
        color[3] = alpha;
    }
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    //
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedSlice[0][0], projectedSlice[0][1]);
    nvgLineTo(vg, projectedSlice[1][0], projectedSlice[1][1]);
    nvgLineTo(vg, projectedSlice[2][0], projectedSlice[2][1]);
    nvgLineTo(vg, projectedSlice[3][0], projectedSlice[3][1]);
    nvgLineTo(vg, projectedSlice[0][0], projectedSlice[0][1]);
    nvgStroke(vg);
        
    nvgRestore(vg);
}
    
#if 0
//
#define DBG_DRAW_SLICES_ONLY_MIDDLE 1 // 0
void
RayCaster2::DBG_DrawSlicesZ(NVGcontext *vg, int width, int height)
{
#if !OPTIM_PRE_SORT_DICHO
    int alignIdxZ = ComputeMostAlignedSliceIdx(width, height, AXIS_Z);
#else
    int alignIdxZ = ComputeMostAlignedSliceIdxDicho(width, height, AXIS_Z);
#endif
    
    //
    for (int i = 0; i < mSlices.size(); i++)
    {
#if DBG_DRAW_SLICES_ONLY_MIDDLE
        if (i != alignIdxZ)
            continue;
#endif
        
        if (mSlices[i].empty())
            continue;
        
        RC_FLOAT z = mSlices[i][0].mZ;
    
        RC_FLOAT slice[4][3] =
        {
            { -0.5, 0.0, z },
            {  0.5, 0.0, z },
            {  0.5, 1.0, z },
            { -0.5, 1.0, z }
        };
    
        int highlight = (i == alignIdxZ) ? 2 : -1;
        DBG_DrawSlice(vg, width, height, slice, highlight);
    }
}
#endif
    
//
#define DBG_DRAW_SLICES_ONLY_MIDDLE 1 // 0
void
RayCaster2::DBG_DrawSlices(NVGcontext *vg, int width, int height, Axis axis)
{
#if !OPTIM_PRE_SORT_DICHO
    int alignIdx = ComputeMostAlignedSliceIdx(width, height, axis);
#else
    int alignIdx = ComputeMostAlignedSliceIdxDicho(width, height, axis);
#endif
        
    //
    for (int i = 0; i < mSlices.size(); i++)
    {
#if DBG_DRAW_SLICES_ONLY_MIDDLE
        if (i != alignIdx)
            continue;
#endif
            
        if (mSlices[i].empty())
            continue;
        
        RC_FLOAT slice[4][3];
        
        if (axis == AXIS_X)
        {
            RC_FLOAT x = ((RC_FLOAT)i)/(mSlices.size() - 1);
            x = x - 0.5;
            
            RC_FLOAT sliceX[4][3] =
            {
                { x, 0.0, -0.5 },
                { x, 1.0, -0.5 },
                { x, 1.0,  0.5 },
                { x, 0.0,  0.5 }
            };
            
            memcpy(slice, sliceX, 12*sizeof(RC_FLOAT));
        }
        
        if (axis == AXIS_Y)
        {
            RC_FLOAT y = ((RC_FLOAT)i)/(mSlices.size() - 1);
            
            RC_FLOAT sliceY[4][3] =
            {
                { -0.5, y, -0.5 },
                {  0.5, y, -0.5 },
                {  0.5, y,  0.5 },
                { -0.5, y,  0.5 }
            };
            
            memcpy(slice, sliceY, 12*sizeof(RC_FLOAT));
        }
        
        if (axis == AXIS_Z)
        {
            RC_FLOAT z = mSlices[i][0].mZ;
            RC_FLOAT sliceZ[4][3] =
            {
                { -0.5, 0.0, z },
                {  0.5, 0.0, z },
                {  0.5, 1.0, z },
                { -0.5, 1.0, z }
            };
            
            memcpy(slice, sliceZ, 12*sizeof(RC_FLOAT));
        }
        
        int highlight = (i == alignIdx) ? (int)axis : -1;
        DBG_DrawSlice(vg, width, height, slice, highlight);
    }
}

void
RayCaster2::DBG_DrawSliceX(NVGcontext *vg, int width, int height, RC_FLOAT x)
{
    RC_FLOAT slice[4][3] =
    {
        { x, 0.0, -0.5 },
        { x, 1.0, -0.5 },
        { x, 1.0,  0.5 },
        { x, 0.0,  0.5 }
    };
            
    DBG_DrawSlice(vg, width, height, slice, 0);
}

void
RayCaster2::DBG_DrawSliceY(NVGcontext *vg, int width, int height, RC_FLOAT y)
{        
    RC_FLOAT slice[4][3] =
    {
        { -0.5, y, -0.5 },
        {  0.5, y, -0.5 },
        {  0.5, y,  0.5 },
        { -0.5, y,  0.5 }
    };
    
    DBG_DrawSlice(vg, width, height, slice, 1);
}

    
void
RayCaster2::DBG_DrawSliceZ(NVGcontext *vg, int width, int height, RC_FLOAT z)
{
    RC_FLOAT slice[4][3] =
    {
        { -0.5, 0.0, z },
        {  0.5, 0.0, z },
        {  0.5, 1.0, z },
        { -0.5, 1.0, z }
    };
    
    DBG_DrawSlice(vg, width, height, slice, 2);
}

#if 0
int
RayCaster2::ComputeMostAlignedSlice(int width, int height, Axis axis,
                                    int *sign, RC_FLOAT *alignedCoord,
                                    RC_FLOAT *fixedCoord)
{
    // Compute the more flat projected slice
    // by compting the area of the slices
    if (sign != NULL)
        *sign = 0;
    
    int index = -1;
    RC_FLOAT minArea = INF;
    
    for (int i = 0; i < mSlices.size(); i++)
    {
        if (mSlices[i].empty())
            continue;
        
        RC_FLOAT slice[4][3];
        
        RC_FLOAT coord;
        
        // X
        if (axis == AXIS_X)
        {
            RC_FLOAT x = ((RC_FLOAT)i)/(mSlices.size() - 1);
            x = x - 0.5;
            
            if (fixedCoord != NULL)
                x = *fixedCoord;
            
            coord = x;
            
            RC_FLOAT sliceX[4][3] =
            {
                { x, 0.0, -0.5 },
                { x, 1.0, -0.5 },
                { x, 1.0,  0.5 },
                { x, 0.0,  0.5 }
            };
            
            memcpy(slice, sliceX, 12*sizeof(RC_FLOAT));
        }
        
        // Y
        if (axis == AXIS_Y)
        {
            RC_FLOAT y = ((RC_FLOAT)i)/(mSlices.size() - 1);
            
            if (fixedCoord != NULL)
                y = *fixedCoord;
            
            coord = y;
            
            RC_FLOAT sliceY[4][3] =
            {
                { -0.5, y, -0.5 },
                {  0.5, y, -0.5 },
                {  0.5, y,  0.5 },
                { -0.5, y,  0.5 }
            };
            
            memcpy(slice, sliceY, 12*sizeof(RC_FLOAT));
        }
        
        // Z
        if (axis == AXIS_Z)
        {
            RC_FLOAT z = mSlices[i][0].mZ;
            
            if (fixedCoord != NULL)
                z = *fixedCoord;
            
            coord = z;
            
            RC_FLOAT sliceZ[4][3] =
            {
                { -0.5, 0.0, z },
                {  0.5, 0.0, z },
                {  0.5, 1.0, z },
                { -0.5, 1.0, z }
            };
            
            memcpy(slice, sliceZ, 12*sizeof(RC_FLOAT));
        }
        
        // Projected slice
        RC_FLOAT projectedSlice[4][3];
        for (int k = 0; k < 4; k++)
        {
            ProjectPoint(projectedSlice[k], slice[k], width, height);
        }
        
        // Compute the  area
        //
        // See: https://www.mathopenref.com/coordpolygonarea.html
        //
        RC_FLOAT area = 0.0;
        for (int k = 0; k < 4; k++)
        {
            area += projectedSlice[k % 4][0]*projectedSlice[(k + 1) % 4][1] -
                    projectedSlice[k % 4][1]*projectedSlice[(k + 1) % 4][0];
        }
        area *= 0.5;
        
        // area = fabs(area);
        
        // Check min
        if (fabs(area) < minArea)
        {
            minArea = fabs(area);
            
            // Update the sign
            if (sign != NULL)
            {
                // See: https://stackoverflow.com/questions/2150050/finding-signed-angle-between-vectors
                RC_FLOAT a[2] = { projectedSlice[1][0] - projectedSlice[0][0],
                                  projectedSlice[1][1] - projectedSlice[0][1] };
                RC_FLOAT b[2] = { projectedSlice[2][0] - projectedSlice[1][0],
                                  projectedSlice[2][1] - projectedSlice[1][1] };
                
                //RC_FLOAT signedAngle = atan2(b[1], b[0]) - atan2(a[1], a[0]);
                
                RC_FLOAT signF = a[0]*b[1] - a[1]*b[0];
                
                if (signF < 0.0)
                    *sign = -1;
                if (signF > 0.0)
                    *sign = 1;
            }
            
            if (alignedCoord != NULL)
                *alignedCoord = coord;
            
            if (axis == AXIS_Z)
                index = i;
        }
        
        // FIX
        if (fixedCoord != NULL)
            break;
    }
    
    return index;
}
#endif
   
int
RayCaster2::ComputeMostAlignedSliceIdx(int width, int height, Axis axis)
{
    int minIndex = -1;
    RC_FLOAT minArea = INF;
        
    for (int i = 0; i < mSlices.size(); i++)
    {
        if (mSlices[i].empty())
            continue;
            
        RC_FLOAT coord;
            
        // X
        if (axis == AXIS_X)
        {
            RC_FLOAT x = ((RC_FLOAT)i)/(mSlices.size() - 1);
            x = x - 0.5;
            
            coord = x;
        }
            
        // Y
        if (axis == AXIS_Y)
        {
            RC_FLOAT y = ((RC_FLOAT)i)/(mSlices.size() - 1);
            
            coord = y;
        }
            
        // Z
        if (axis == AXIS_Z)
        {
            RC_FLOAT z = mSlices[i][0].mZ;
            
            coord = z;
        }
        
        RC_FLOAT area = ComputeSliceArea(width, height, axis, coord);
            
        // Check min
        if (area < minArea)
        {
            minArea = area;
            
            minIndex = i;
        }
    }
        
    return minIndex;
}
 
#if 0 // TEST to have more precision
RC_FLOAT
RayCaster2::ComputeMostAlignedSliceIdx(int width, int height, Axis axis)
{
#define PRECISION 4 // 1
    
    int minIndex = -1;
    RC_FLOAT minArea = INF;
    
    int numSlices = mSlices.size()*PRECISION;
    
    for (int i = 0; i < numSlices; i++)
    {
        //if (mSlices[i].empty())
        //    continue;
            
        RC_FLOAT coord;
            
        // X
        if (axis == AXIS_X)
        {
            RC_FLOAT x = ((RC_FLOAT)i)/(numSlices - 1);
            x = x - 0.5;
                
            coord = x;
        }
            
        // Y
        if (axis == AXIS_Y)
        {
            RC_FLOAT y = ((RC_FLOAT)i)/(numSlices - 1);
            
            coord = y;
        }
            
        // Z
        if (axis == AXIS_Z)
        {
            RC_FLOAT z = ((RC_FLOAT)i)/(numSlices - 1);
            z = z - 0.5;
                
            coord = z;
        }
            
        RC_FLOAT area = ComputeSliceArea(width, height, axis, coord);
            
        // Check min
        if (area < minArea)
        {
            minArea = area;
                
            minIndex = i;
        }
    }
    
    RC_FLOAT resultIndex = ((RC_FLOAT)minIndex)/PRECISION;
    
    return resultIndex;
}
#endif
    
// NOTE: a bit less accurate than the non-dichotomic method
int
RayCaster2::ComputeMostAlignedSliceIdxDicho(int width, int height, Axis axis)
{
    // We can not use a standard dichotomic method
    // because we have a V-shaped gradient.
    //
    // So we use the area of the middle point, then compute two gradients (simple differences)
    // and keep the indices which make the max gradient.
    
    // Min and max indices
    int indices[3] = { 0, (mSlices.size() - 1)/2, mSlices.size() - 1 };
    while(indices[0] != indices[2])
    {
        RC_FLOAT areas[3];
        for (int i = 0; i < 3; i++)
        {
            RC_FLOAT coord;
        
            // X
            if (axis == AXIS_X)
            {
                RC_FLOAT x = ((RC_FLOAT)indices[i])/(mSlices.size() - 1);
                x = x - 0.5;
                
                coord = x;
            }
            
            // Y
            if (axis == AXIS_Y)
            {
                RC_FLOAT y = ((RC_FLOAT)indices[i])/(mSlices.size() - 1);
                
                coord = y;
            }
            
            // Z
            if (axis == AXIS_Z)
            {
                RC_FLOAT z = mSlices[indices[i]][0].mZ;
                
                coord = z;
            }
            
            areas[i] = ComputeSliceArea(width, height, axis, coord);
        }
        
        if (areas[0] - areas[1] >= areas[2] - areas[1])
            // Max gradient on the left
        {
            indices[0] = indices[1];
        }
        else
            // Max gradient on the right
        {
            indices[2] = indices[1];
        }
        
        // Compute the middle index
        int middleIndex = (indices[0] + indices[2])/2;
        indices[1] = middleIndex;
        
        if ((indices[1] == indices[0]) || (indices[1] == indices[2]))
        {
            // We must stop (otherwise we will get an infinite loop)
            
            // Find the index which gives the min area
            int minIndex = indices[0];
            RC_FLOAT minArea = areas[0];
            if (areas[1] < minArea)
            {
                minArea = areas[1];
                minIndex = indices[1];
            }
            
            if (areas[2] < minArea)
            {
                minArea = areas[2];
                minIndex = indices[2];
            }
            
            indices[1] = minIndex;
            
            break;
        }
    }
    
    int resultIndex = indices[1];
                       
    return resultIndex;
}

    
RC_FLOAT
RayCaster2::ComputeSliceArea(int width, int height, Axis axis,
                             RC_FLOAT coord, int *sign)
{
    // Compute the more flat projected slice
    // by compting the area of the slices
    if (sign != NULL)
        *sign = 0;
            
    RC_FLOAT slice[4][3];
    
    // X
    if (axis == AXIS_X)
    {
        RC_FLOAT sliceX[4][3] =
        {
            { coord, 0.0, -0.5 },
            { coord, 1.0, -0.5 },
            { coord, 1.0,  0.5 },
            { coord, 0.0,  0.5 }
        };
                
        memcpy(slice, sliceX, 12*sizeof(RC_FLOAT));
    }
            
    // Y
    if (axis == AXIS_Y)
    {
        RC_FLOAT sliceY[4][3] =
        {
            { -0.5, coord, -0.5 },
            {  0.5, coord, -0.5 },
            {  0.5, coord,  0.5 },
            { -0.5, coord,  0.5 }
        };
                
        memcpy(slice, sliceY, 12*sizeof(RC_FLOAT));
    }
            
    // Z
    if (axis == AXIS_Z)
    {
        RC_FLOAT sliceZ[4][3] =
        {
            { -0.5, 0.0, coord },
            {  0.5, 0.0, coord },
            {  0.5, 1.0, coord },
            { -0.5, 1.0, coord }
        };
                
        memcpy(slice, sliceZ, 12*sizeof(RC_FLOAT));
    }
            
    // Projected slice
    RC_FLOAT projectedSlice[4][3];
    for (int k = 0; k < 4; k++)
    {
        ProjectPoint(projectedSlice[k], slice[k], width, height);
    }
            
    // Compute the  area
    //
    // See: https://www.mathopenref.com/coordpolygonarea.html
    //
    RC_FLOAT area = 0.0;
    for (int k = 0; k < 4; k++)
    {
        area += projectedSlice[k % 4][0]*projectedSlice[(k + 1) % 4][1] -
        projectedSlice[k % 4][1]*projectedSlice[(k + 1) % 4][0];
    }
    area *= 0.5;
    area = fabs(area);
            
    // Update the sign
    if (sign != NULL)
    {
        // See: https://stackoverflow.com/questions/2150050/finding-signed-angle-between-vectors
        RC_FLOAT a[2] = { projectedSlice[1][0] - projectedSlice[0][0],
                          projectedSlice[1][1] - projectedSlice[0][1] };
        RC_FLOAT b[2] = { projectedSlice[2][0] - projectedSlice[1][0],
                          projectedSlice[2][1] - projectedSlice[1][1] };
                    
        RC_FLOAT signF = a[0]*b[1] - a[1]*b[0];
        
        if (signF < 0.0)
            *sign = -1;
        if (signF > 0.0)
            *sign = 1;
    }
        
    return area;
}
    
#if 0
void
RayCaster2::ComputeViewSigns(int *signX, int *signY, int *signZ,
                             int *splitIdxZ)
{
    RC_FLOAT alignedX;
    RC_FLOAT fixedX = 0.0;
    ComputeMostAlignedSlice(mViewWidth, mViewHeight, AXIS_X,
                            signX, &alignedX, &fixedX);
    
    RC_FLOAT alignedY;
    RC_FLOAT fixedY = 0.5;
    ComputeMostAlignedSlice(mViewWidth, mViewHeight, AXIS_Y,
                            signY, &alignedY, &fixedY);
        
    RC_FLOAT alignedZ;
    RC_FLOAT fixedZ = 0.5;
    int splitIdxZ0 = ComputeMostAlignedSlice(mViewWidth, mViewHeight, AXIS_Z,
                                             signZ, &alignedZ); //, &fixedZ);
    
    if (splitIdxZ != NULL)
        *splitIdxZ = splitIdxZ0;
}
#endif

void
RayCaster2::ComputeViewSigns(int *signX, int *signY, int *signZ)
{
    RC_FLOAT coordX = 0.0;
    /*RC_FLOAT areaX =*/ ComputeSliceArea(mViewWidth, mViewHeight, AXIS_X,
                                          coordX, signX);
    
    RC_FLOAT coordY = 0.5;
    /*RC_FLOAT areaY =*/ ComputeSliceArea(mViewWidth, mViewHeight, AXIS_Y,
                                          coordY, signY);
    
    RC_FLOAT coordZ = 0.5;
    /*RC_FLOAT areaX =*/ ComputeSliceArea(mViewWidth, mViewHeight, AXIS_Z,
                                          coordZ, signZ);
}
    
bool
RayCaster2::ViewSignsChanged(int signX, int signY, int signZ)
{
    bool changed = false;
    if (signX != mPrevViewSigns[0])
        changed = true;
    
    if (signY != mPrevViewSigns[1])
        changed = true;
    
    if (signZ != mPrevViewSigns[2])
        changed = true;
    
    // Update prev values
    mPrevViewSigns[0] = signX;
    mPrevViewSigns[1] = signY;
    mPrevViewSigns[2] = signZ;
    
    return changed;
}
    
void
RayCaster2::FillEmptySlices()
{
    vector<Point> points;
    // Set a single empty point (we can have to set its z, for slice z)
    Point emptyPoint;
    points.push_back(emptyPoint);
    
    while(mSlices.size() < mNumSlices)
    {
        mSlices.push_back(points);
    }
}

void
RayCaster2::ComputeCameraPosition(RC_FLOAT camPos[3], bool invertModel)
{
    glm::vec3 pos(0.0f, 0.0, -2.0f);
    glm::vec3 target(0.0f, 0.37f, 0.0f);
    
    glm::vec3 up(0.0, 1.0f, 0.0f);
    
    glm::vec3 lookVec(target.x - pos.x, target.y - pos.y, target.z - pos.z);
    float radius = glm::length(lookVec);
    
    float angle1 = mCamAngle1;
    //angle1 *= 3.14/180.0;
    angle1 *= M_PI/180.0;
    float newZ = cos(angle1)*radius;
    float newY = sin(angle1)*radius;
    
#if USE_NEWEST_GLM
    // In newest versions of glm (required for SIMD),
    // angles are in radians for the functions below
    angle0 *= M_PI/180.0;
    perspAngle *= M_PI/180.0;
#endif
    
    // Seems to work (no need to touch up vector)
    pos.z = newZ;
    pos.y = newY;
    
    // Result
    camPos[0] = pos.x;
    camPos[1] = pos.y;
    camPos[2] = pos.z;
    
    if (invertModel)
    {
        glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
        
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f)); //
        modelMat = glm::rotate(modelMat, mCamAngle0, glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.33/*0.0f*/, 0.0f));
        
        // Invert matrix
        modelMat = glm::inverse(modelMat);
        
        // Transform
        glm::vec4 pos(camPos[0], camPos[1], camPos[2], 1.0);
        
        glm::vec4 v4 = modelMat*pos;
        
#define EPS 1e-8
        if ((v4.w < -EPS) || (v4.w > EPS))
        {
            // Optim
            RC_FLOAT wInv = 1.0/v4.w;
            v4.x *= wInv;
            v4.y *= wInv;
            v4.z *= wInv;
        }
        
        camPos[0] = v4.x;
        camPos[1] = v4.y;
        camPos[2] = v4.z;
    }
}

void
RayCaster2::TransformPointModel(const Point &p, Point *result, RC_FLOAT camAngle0)
{
    glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
        
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f)); //
    modelMat = glm::rotate(modelMat, camAngle0, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.33/*0.0f*/, 0.0f));
    
    // Transform
    glm::vec4 pos(p.mX, p.mY, p.mZ, 1.0);
    
    glm::vec4 v4 = modelMat*pos;
    
#define EPS 1e-8
    if ((v4.w < -EPS) || (v4.w > EPS))
    {
        // Optim
        RC_FLOAT wInv = 1.0/v4.w;
        v4.x *= wInv;
        v4.y *= wInv;
        v4.z *= wInv;
    }
    
    *result = p;
    
    result->mX = v4.x;
    result->mY = v4.y;
    result->mZ = v4.z;
}
