//
//  LineRender.cpp
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <algorithm>
using namespace std;

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <glm/gtx/intersect.hpp>

#include <GraphSwapColor.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "LinesRender2.h"

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

#define DENSITY_MIN_NUM_SLICES 16
#define DENSITY_MAX_NUM_SLICES 64

// More detailed, used for freqs (total: 1024 values)
#define DENSITY_MAX_NUM_SLICES_FREQS 256

#define BUFFER_SIZE 2048

#define NUM_TOTAL_SLICES 32

#define MIN_SPEED 1
#define MAX_SPEED 8

//#define MIN_FOV 15.0
//#define MAX_FOV 45.0

#define REVERSE_SCROLL_ORDER 1

// When changing the density, change the number of points
// of the lines too
// (fot the moment, this is not very beautiful)
#define OPTIM_DENSITY_LINES 0

// Optim: 64 instead of 256
// Profile display time: 40 -> 30 ms
// (but triangular shape for peaks)
//#define DENSITY_MAX_NUM_SLICES_FREQS 64

// Optimize horizontal and vertical lines
#define OPTIM_STRAIGHT_LINES 1
// 1e-2 => clips everything
// 1e-4 => 30ms instead of 40
// 1e-5 => optimizes well too
#define OPTIM_STRAIGHT_LINES_EPS 1e-4

// Possibly use db scale for amp axis
#define USE_DB_SCALE 1

// Apply a scale to the y of the points,
// when in db mode, so the display matches
// almost the amp axis
// (but it is not perfect)
#define HACK_DB_SCALE 1

// Jittering frequency lines with low density, just higher than the lowest density
#define FIX_JITTER_LOW_DENSITY 1

static void
CameraModelProjMat(int winWidth, int winHeight, float angle0, float angle1,
                   BL_FLOAT perspAngle,
                   glm::mat4 *model, glm::mat4 *proj)
{
    glm::vec3 pos(0.0f, 0.0, -2.0f);
    glm::vec3 target(0.0f, 0.37f, 0.0f);
    
    glm::vec3 up(0.0, 1.0f, 0.0f);
    
    glm::vec3 lookVec(target.x - pos.x, target.y - pos.y, target.z - pos.z);
    float radius = glm::length(lookVec);
    
    angle1 *= 3.14/180.0;
    float newZ = std::cos(angle1)*radius;
    float newY = std::sin(angle1)*radius;
    
    // Seems to work (no need to touch up vector)
    pos.z = newZ;
    pos.y = newY;
    
    glm::mat4 viewMat = glm::lookAt(pos, target, up);
    
    glm::mat4 perspMat = glm::perspective((float)perspAngle/*45.0f*//*60.0f*/,
                                          ((float)winWidth)/winHeight,
                                          NEAR_PLANE, FAR_PLANE);
    
    glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
    
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f)); //
    modelMat = glm::rotate(modelMat, angle0, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::translate(modelMat,
                              glm::vec3(0.0f, 0.5+0.33/*0.33*//*0.0f*/, 0.0f));
    
    *model = viewMat * modelMat;
    *proj = perspMat;
}

//

LinesRender2::LinesRender2()
{
    mCamAngle0 = 0.0;
    mCamAngle1 = 0.0;
    
    mCamFov = MAX_FOV;
    
    mNumSlices = NUM_TOTAL_SLICES;
    
    mWhitePixImg = -1;
        
    mMode = LinesRender2::POINTS;
    mDensityNumSlices = DENSITY_MIN_NUM_SLICES;
    mScale = 0.5;
    
    mScrollDirection = LinesRender2::BACK_FRONT;
    
    mAddNum = 0;
    mNumLinesAdded = 0;
    mSpeed = MIN_SPEED;
    
    // Keep internally
    mViewWidth = 0;
    mViewHeight = 0;
    
    mDensePointsFlag = true;
    
    mDisplayAllSlices = false;
    
    mShowAxes = false;
    
    mShowAdditionalLines = false;
    mAdditionalLinesWidth = 1.0;

    mShowAdditionalPoints = false;
    mAdditionalPointsWidth = 1.0;
    
    mMustRecomputeProj = true;
    
    mDBScale = false;
    mMinDB = -60.0;
    
    // Color
    mColor0[0] = 128;
    mColor0[1] = 128;
    mColor0[2] = 255;
    mColor0[3] = 255;

    mColor1[0] = 255;
    mColor1[1] = 255;
    mColor1[2] = 255;
    mColor1[3] = 255;

    mDbgForceDensityNumSlices = false;

    mNeedRedraw = true;

    mUseLegacyLock = false;
    
    Init();
}

LinesRender2::~LinesRender2() {}

void
LinesRender2::SetUseLegacyLock(bool flag)
{
    mUseLegacyLock = flag;
}

void
LinesRender2::ProjectPoint(BL_FLOAT projP[3],
                           const BL_FLOAT p[3], int width, int height)
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
    
    BL_FLOAT x = v4.x;
    BL_FLOAT y = v4.y;
    BL_FLOAT z = v4.z;
    BL_FLOAT w = v4.w;
    
#define EPS 1e-8
    if (std::fabs(w) > EPS)
    {
        // Optim
        BL_FLOAT wInv = 1.0/w;
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
LinesRender2::Init()
{
    // Fill with zero values
    // To have a flat grid at the beginning
    vector<Point> &points = mTmpBuf2;
    if (!mDensePointsFlag)
        points.clear();
    
    if (mDensePointsFlag)
    {
        points.resize(BUFFER_SIZE/2);
        for (int i = 0; i < points.size(); i++)
        {
            Point &p = points[i];
            
            if (points.size() <= 1)
                p.mX = ((BL_FLOAT)i)/points.size() - 0.5;
            else
                p.mX = ((BL_FLOAT)i)/(points.size() - 1) - 0.5;
            
            p.mY = 0.0;
        }
    }
    
    for (int i = 0; i < mNumSlices; i++)
        mSlices.push_back(points);
    
#if 0
    UpdateSlicesZ();
#endif
    
    mMustRecomputeProj = true;

    mNeedRedraw = true;
}

void
LinesRender2::SetNumSlices(long numSlices)
{
    if (!mDisplayAllSlices)
        mNumSlices = numSlices;
    
    mMustRecomputeProj = true;
    
    if (mDbgForceDensityNumSlices)
    {
        mDensityNumSlices = mNumSlices;
    }

    mNeedRedraw = true;
}

long
LinesRender2::GetNumSlices()
{
    return mNumSlices;
}

void
LinesRender2::SetDensePointsFlag(bool flag)
{
    mDensePointsFlag = flag;
    
    mMustRecomputeProj = true;

    mNeedRedraw = true;
}

void
LinesRender2::PreDraw(NVGcontext *vg, int width, int height)
{
    // NOTE: does not work, makes the display blink
    // (sometimes the graph need to redraw itself,
    // whereas LineResner2 has not changed)
    
    //if (mMustRecomputeProj)
    //    return;
    
    mViewWidth = width;
    mViewHeight = height;
    
    vector<vector<Point> > &points = mTmpBuf3;
    points = mPrevProjPoints;

    // Make a copy of the sensitive variables, to be able to release
    // the mutex early
    bl_queue<vector<Point> > &slices = mTmpBuf4;
    slices = mSlices;
    
    if (mMustRecomputeProj)
    {
        if (!mDisplayAllSlices)
            ProjectSlices(&points, slices, mViewWidth, mViewHeight);
        else
            ProjectSlicesNoDecim(&points, slices, mViewWidth, mViewHeight);
        
        mPrevProjPoints = points;
        
        mMustRecomputeProj = false;
    }
    
    nvgSave(vg);
    
    if (mMode == POINTS)
        DrawPoints(vg, points);
    else if (mMode == LINES_TIME)
        DrawLinesTime(vg, points);
    else if (mMode == LINES_FREQ)
        DrawLinesFreq(vg, points);
    else if (mMode == GRID)
        DrawGrid(vg, points);
    
    nvgRestore(vg);

    DrawAdditionalLines(vg, width, height);
    DrawAdditionalPoints(vg, width, height);
    
    if (mShowAxes)
    {
        for (int i = 0; i < mAxis.size(); i++)
        {
            Axis3D *axis = mAxis[i];
            axis->Draw(vg, width, height);
        }
    }

    mNeedRedraw = false;
}

void
LinesRender2::DrawPoints(NVGcontext *vg, const vector<vector<Point> > &points)
{
#if 0 // Simple
    unsigned char color[4] = { 200, 200, 255, 255 };
    DoDrawPoints(vg, points, color, 2.0);
#endif
    
#if 1 // USED for BL-Waves
    //unsigned char color0[4] = { 128, 128, 255, 255 };
    //DoDrawPoints(vg, points, color0, 3.0);
    DoDrawPoints(vg, points, mColor0, 3.0);
    
    //unsigned char color2[4] = { 255, 255, 255, 255 };
    //DoDrawPoints(vg, points, color2, 2.0/*1.0*/);
    DoDrawPoints(vg, points, mColor1, 2.0/*1.0*/);
#endif
    
#if 0 // For debug SASviewer / tracking
    // Draw points by color
    // (for SASViewer)
    //DoDrawPoints(vg, points, 6.0); // origin
    DoDrawPoints(vg, points, -1.0);
#endif
}

void
LinesRender2::DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                           unsigned char inColor[4], BL_FLOAT inPointSize)
{
    unsigned char color0[4] = { inColor[0], inColor[1], inColor[2], inColor[3] };
    SWAP_COLOR(color0);
    
    NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
    
    nvgStrokeColor(vg, color);
    nvgFillColor(vg, color);
    
    for (int i = 0; i < points.size(); i++)
    {
        vector<Point> &points0 = mTmpBuf5;
        points0 = points[i];
        
        for (int j = 0; j < points0.size(); j++)
        {
            const Point &p = points0[j];
    
            BL_FLOAT pointSize = inPointSize;
            if (inPointSize < 0.0)
            // Global point size not defined
            {
                pointSize = p.mSize;
            }
            
            BL_FLOAT x = p.mX;
            BL_FLOAT y = p.mY;
            
            BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
            yf = mViewHeight - yf;
#endif
            
            // Quad rendering
            float corners[4][2] =
            { { (float)(x - pointSize/2.0), (float)(yf - pointSize/2.0) },
              { (float)(x + pointSize/2.0), (float)(yf - pointSize/2.0) },
              { (float)(x + pointSize/2.0), (float)(yf + pointSize/2.0) },
              { (float)(x - pointSize/2.0), (float)(yf + pointSize/2.0) } };
            
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
}

void
LinesRender2::DrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points)
{
#if 0 // Simple
    unsigned char color[4] = { 200, 200, 255, 255 };
    v(vg, points, color, 2.0);
#endif
    
#if 0 // Fat
    unsigned char color0[4] = { 128, 128, 255, 255 };
    DoDrawLinesFreq(vg, points, color0, 5.0);
    
    unsigned char color1[4] = { 200, 200, 255, 255 };
    DoDrawLinesFreq(vg, points, color1, 2.0);
    
    unsigned char color2[4] = { 255, 255, 255, 255 };
    DoDrawLinesFreq(vg, points, color2, 1.0);
#endif
    
    //unsigned char color0[4] = { 128, 128, 255, 255 };
    //DoDrawLinesFreq(vg, points, color0, 3.0);
    DoDrawLinesFreq(vg, points, mColor0, 3.0);
    
    //unsigned char color2[4] = { 255, 255, 255, 255 };
    //DoDrawLinesFreq(vg, points, color2, 1.0);
    DoDrawLinesFreq(vg, points, mColor1, 1.0);
}

void
LinesRender2::DoDrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points,
                              unsigned char inColor[4], BL_FLOAT lineWidth)
{
    if (points.empty())
        return;
    
    unsigned char color0[4] = { inColor[0], inColor[1], inColor[2], inColor[3] };
    
    SWAP_COLOR(color0);
    
    NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
    
    nvgStrokeColor(vg, color);
    nvgFillColor(vg, color);
    
    nvgStrokeWidth(vg, lineWidth);
    
    for (int i = 0; i < points.size(); i++)
    {
#if FIX_JITTER_LOW_DENSITY
        // Do not draw the first line (it has not been projected previously)
        if (mMode == LINES_FREQ)
        {
            if (i == 0)
                continue;
        }
#endif
        
        const vector<Point> &points0 = points[i];
        
        nvgBeginPath(vg);
        
        for (int j = 0; j < points0.size(); j++)
        {
            const Point &p = points0[j];
            
#if OPTIM_STRAIGHT_LINES
            if ((j > 0) && (j < points0.size() - 1))
            {
                if ((mMode == LINES_FREQ) && p.mSkipDisplayX)
                    continue;
                
                if ((mMode == GRID) &&
                    p.mSkipDisplayX && p.mSkipDisplayZ)
                    continue;
            }
#endif
            
            BL_FLOAT x = p.mX;
            BL_FLOAT y = p.mY;
            
            BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
            yf = mViewHeight - y;
#endif
            if (j == 0)
                nvgMoveTo(vg, x, yf);
            else
                nvgLineTo(vg, x, yf);
        }
        
        nvgStroke(vg);
    }
}

void
LinesRender2::DrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points)
{
#if 0 // Simple
    unsigned char color[4] = { 200, 200, 255, 255 };
    DoDrawLinesTime(vg, points, color, 2.0);
#endif
    
    //unsigned char color0[4] = { 128, 128, 255, 255 };
    //DoDrawLinesTime(vg, points, color0, 3.0);
    DoDrawLinesTime(vg, points, mColor0, 3.0);
    
    //unsigned char color2[4] = { 255, 255, 255, 255 };
    //DoDrawLinesTime(vg, points, color2, 1.0);
    DoDrawLinesTime(vg, points, mColor1, 1.0);
}
                           
// Exactly the same as DoDrawLinesFreq ?
void
LinesRender2::DoDrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points,
                              unsigned char inColor[4], BL_FLOAT lineWidth)
{
    unsigned char color0[4] = { inColor[0], inColor[1], inColor[2], inColor[3] };
    
    SWAP_COLOR(color0);
    
    NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
    
    nvgStrokeColor(vg, color);
    nvgFillColor(vg, color);
    
    nvgStrokeWidth(vg, lineWidth);
    
    if (points.empty())
        return;
    
    vector<Point> points0 = points[0];
    for (int j = 0; j < points0.size(); j++)
    {
        nvgBeginPath(vg);

        for (int i = 0; i < points.size(); i++)
        {
            const Point &p = points[i][j];
            
#if OPTIM_STRAIGHT_LINES
            if ((i > 0) && (i < points.size() - 1))
            {
                if ((mMode == LINES_TIME) && p.mSkipDisplayZ)
                    continue;
                
                if ((mMode == GRID) &&
                    p.mSkipDisplayX && p.mSkipDisplayZ)
                    continue;
            }
#endif
            
            BL_FLOAT x = p.mX;
            BL_FLOAT y = p.mY;
            
            BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
            yf = mViewHeight - y;
#endif
            
            if (i == 0)
                nvgMoveTo(vg, x, yf);
            else
                nvgLineTo(vg, x, yf);
        }
        
        nvgStroke(vg);
    }
}

void
LinesRender2::DoDrawLineSimple(NVGcontext *vg, const vector<Point> &points,
                               unsigned char inColor[4], BL_FLOAT lineWidth)
{
    if (points.empty())
        return;
    
    unsigned char color0[4] = { inColor[0], inColor[1], inColor[2], inColor[3] };
    
    SWAP_COLOR(color0);
    
    NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
    
    nvgStrokeColor(vg, color);
    nvgFillColor(vg, color);
    
    nvgStrokeWidth(vg, lineWidth);
        
    nvgBeginPath(vg);    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
                        
        BL_FLOAT x = p.mX;
        BL_FLOAT y = p.mY;
            
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = mViewHeight - y;
#endif
        if (i == 0)
            nvgMoveTo(vg, x, yf);
        else
            nvgLineTo(vg, x, yf);
    }
    
    nvgStroke(vg);
}

void
LinesRender2::DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                           BL_FLOAT inPointSize)
{
    for (int i = 0; i < points.size(); i++)
    {
        vector<Point> &points0 = mTmpBuf6;
        points0 = points[i];
        
        for (int j = 0; j < points0.size(); j++)
        {
            const Point &p = points0[j];
            
            BL_FLOAT pointSize = inPointSize;
            if (inPointSize < 0.0)
            {
                pointSize = p.mSize;
            }
    
            // Color
            unsigned char color0[4] = { p.mR, p.mG, p.mB, p.mA };
            SWAP_COLOR(color0);
            
            NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
            
            nvgStrokeColor(vg, color);
            nvgFillColor(vg, color);
            
            // Coords
            BL_FLOAT x = p.mX;
            BL_FLOAT y = p.mY;
            
            BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
            yf = mViewHeight - yf;
#endif
            
            // Quad rendering
            float corners[4][2] =
            { { (float)(x - pointSize/2.0), (float)(yf - pointSize/2.0) },
              { (float)(x + pointSize/2.0), (float)(yf - pointSize/2.0) },
              { (float)(x + pointSize/2.0), (float)(yf + pointSize/2.0) },
              { (float)(x - pointSize/2.0), (float)(yf + pointSize/2.0) } };
            
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
}

void
LinesRender2::DoDrawPointsSimple(NVGcontext *vg, const vector<Point> &points,
                                 BL_FLOAT inPointSize)
{
    vector<Point> &points0 = mTmpBuf18;
    points0 = points;
        
    for (int j = 0; j < points0.size(); j++)
    {
        const Point &p = points0[j];
        
        BL_FLOAT pointSize = inPointSize;
        if (inPointSize < 0.0)
        {
            pointSize = p.mSize;
        }
    
        // Color
        unsigned char color0[4] = { p.mR, p.mG, p.mB, p.mA };
        //unsigned char color0[4] = { 255, 0, 255, 255 };
        SWAP_COLOR(color0);
        
        NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
        
        nvgStrokeColor(vg, color);
        nvgFillColor(vg, color);
        
        // Coords
        BL_FLOAT x = p.mX;
        BL_FLOAT y = p.mY;
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = mViewHeight - yf;
#endif
            
        // Quad rendering
        float corners[4][2] =
            { { (float)(x - pointSize/2.0), (float)(yf - pointSize/2.0) },
              { (float)(x + pointSize/2.0), (float)(yf - pointSize/2.0) },
              { (float)(x + pointSize/2.0), (float)(yf + pointSize/2.0) },
              { (float)(x - pointSize/2.0), (float)(yf + pointSize/2.0) } };
        
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

void
LinesRender2::DrawGrid(NVGcontext *vg, const vector<vector<Point> > &points)
{
    //unsigned char color0[4] = { 128, 128, 255, 255 };
    //DoDrawGrid(vg, points, color0, 3.0);
    DoDrawGrid(vg, points, mColor0, 3.0);
    
    //unsigned char color2[4] = { 255, 255, 255, 255 };
    //DoDrawGrid(vg, points, color2, 1.0);
    DoDrawGrid(vg, points, mColor1, 1.0);
}

void
LinesRender2::DoDrawGrid(NVGcontext *vg, const vector<vector<Point> > &points,
                        unsigned char inColor[4], BL_FLOAT lineWidth)
{
    DoDrawLinesFreq(vg, points, inColor, lineWidth);
    DoDrawLinesTime(vg, points, inColor, lineWidth);
}

void
LinesRender2::ClearSlices()
{
    mSlices.unfreeze();
    mSlices.clear();
    
    Init();
    
    mShowAdditionalLines = false;
    mShowAdditionalPoints = false;
    
    mMustRecomputeProj = true;

    mNeedRedraw = true;
}

bool
LinesRender2::MustAddSlice()
{
    if (!mDisplayAllSlices)
    {
        // Warning: mAddNum is incremented, but we are in an accessor
        bool skipDisplay = (mAddNum++ % mSpeed != 0);
        if (skipDisplay)
            return false;
    }

    return true;
}

void
LinesRender2::AddSlice(const vector<Point> &points)
{
    if (mUseLegacyLock)
    {
        AddSliceLF(points);

        return;
    }
    
#if 0
    if (!mDisplayAllSlices)
    {
        bool skipDisplay = (mAddNum++ % mSpeed != 0);
        if (skipDisplay)
            return;
    }
#endif
    
    Slice &slice = mTmpBuf15;
    slice.mPoints = points;
    
    mLockFreeQueues[0].push(slice);
}

void
LinesRender2::PushData()
{
    // Must dirty this CustomDrawer if some data is pushed
    // Because GraphControl12::IsDirty() is called before PullAllData(),
    // which is called in GraphControl12::Draw()
    if (!mLockFreeQueues[0].empty())
        mNeedRedraw = true;
    
    mLockFreeQueues[1].push(mLockFreeQueues[0]);
    mLockFreeQueues[0].clear();
}

void
LinesRender2::PullData()
{   
    mLockFreeQueues[2].push(mLockFreeQueues[1]);
    mLockFreeQueues[1].clear();
}

void
LinesRender2::ApplyData()
{
    for (int i = 0; i < mLockFreeQueues[2].size(); i++)
    {
        Slice &slice = mTmpBuf16;
        mLockFreeQueues[2].get(i, slice);

        AddSliceLF(slice.mPoints);
    }

    mLockFreeQueues[2].clear();
}


void
LinesRender2::AddSliceLF(const vector<Point> &points)
{
    vector<Point> &points0 = mTmpBuf7;
    points0 = points;

    if (mSlices.size() != mNumSlices)
    {
        mSlices.push_back(points0);
        while (mSlices.size() > mNumSlices)
            mSlices.pop_front();
    }
    else
    {
        mSlices.freeze();
        mSlices.push_pop(points0);
    }
    
    mNumLinesAdded++;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1)
{
    mCamAngle0 = angle0;
    mCamAngle1 = angle1;
    
    mMustRecomputeProj = true;
}

BL_FLOAT
LinesRender2::GetCameraFov()
{
    return mCamFov;
}

void
LinesRender2::SetCameraFov(BL_FLOAT angle)
{
    mCamFov = angle;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::ZoomChanged(BL_FLOAT zoomChange)
{
    mCamFov *= 1.0/zoomChange;
  
    if (mCamFov < MIN_FOV)
        mCamFov = MIN_FOV;
    
    if (mCamFov > MAX_FOV)
        mCamFov = MAX_FOV;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::ResetZoom()
{
    mCamFov = MAX_FOV;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::ProjectPoints(vector<Point> *points, int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height, mCamAngle0, mCamAngle1,
                       mCamFov,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    // Optim
    glm::vec4 v;
    glm::vec4 v4;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
#if OPTIM_STRAIGHT_LINES
        // Do not project the point if we will ignore it during display
        if ((mMode == LINES_FREQ) && p.mSkipDisplayX)
            continue;
        
        if ((mMode == LINES_TIME) && p.mSkipDisplayZ)
            continue;
            
        if ((mMode == GRID) &&
             p.mSkipDisplayX && p.mSkipDisplayZ)
            continue;
#endif
        
        // Matrix transform
        v.x = p.mX;
        v.y = p.mY;
        v.z = p.mZ;
        v.w = 1.0;
        
        v4 = modelProjMat*v;
        
        BL_FLOAT x = v4.x;
        BL_FLOAT y = v4.y;
        BL_FLOAT z = v4.z;
        BL_FLOAT w = v4.w;
        
#define EPS 1e-8
        if (std::fabs(w) > EPS)
        {
            // Optim
            BL_FLOAT wInv = 1.0/w;
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

void
LinesRender2::ProjectSlices(vector<vector<Point> > *points,
                            const bl_queue<vector<Point> > &slices,
                            int width, int height)
{
    // Decimate the right way:
    // - iterate over the target data
    // - use an offset to keep synchronized
    
    // Compute decimation parameters
    int densityNumSlicesI = mDensityNumSlices;
    int densityNumSlicesJ = mDensityNumSlices;
    
    // Will reduce number of points per line
    // (but is not very beautiful)
    if (mMode == LINES_FREQ)
        densityNumSlicesJ = DENSITY_MAX_NUM_SLICES_FREQS;
    
    if (mMode == LINES_TIME)
        densityNumSlicesI = DENSITY_MAX_NUM_SLICES;
    
    // Keep computed Z, to use them later
    points->resize(densityNumSlicesI);
    WDL_TypedBuf<BL_FLOAT> &pointsZ = mTmpBuf1;
    pointsZ.Resize((int)points->size());

    // Coeff
    BL_FLOAT iCoeff = ((BL_FLOAT)mNumSlices)/densityNumSlicesI;
    
    BL_FLOAT iOffset = 0.0;
    if (iCoeff > 1.0)
        iOffset = std::fmod((BL_FLOAT)mNumLinesAdded, iCoeff);

#if FIX_JITTER_LOW_DENSITY
    // FIX: fixes well jumps/sparkles with low density just greater than the minimum
    // This fixes everything except the first line
    if (mMode == LINES_FREQ)
        iOffset = -iOffset;
#endif
    
    // Decimate / augment on i (slices)
    for (int i = 0; i < points->size(); i++)
    {
#if FIX_JITTER_LOW_DENSITY
        // Suppress the first line, and later will rescale a
        // bit over z to fill the gap
        if (mMode == LINES_FREQ)
        {
            if (i == 0)
                continue;
        }
#endif
        
        BL_FLOAT targetIIdx = bl_round(i*iCoeff + iOffset);
        if (targetIIdx < 0.0)
            targetIIdx = 0.0;
        
        if (targetIIdx > slices.size() - 1)
            targetIIdx = slices.size() - 1;
        
        if (mDensePointsFlag)
        {
            // Decimate / augment on j (lines)
            vector<Point> &newPoints = mTmpBuf10;
            newPoints.resize(densityNumSlicesJ);
            
            //BL_FLOAT jCoeff =
            // ((BL_FLOAT)slices[targetIIdx].size())/densityNumSlicesJ;
            BL_FLOAT jCoeff =
                ((BL_FLOAT)slices[targetIIdx].size())/(densityNumSlicesJ - 1);
            for (int j = 0; j < newPoints.size(); j++)
            {
                int targetJIdx = bl_round(j*jCoeff);
                if (targetJIdx > (int)slices[targetIIdx].size() - 1)
                    targetJIdx = (int)slices[targetIIdx].size() - 1;
                
                const Point &p = slices[targetIIdx][targetJIdx];
                    
                newPoints[j] = p;
                newPoints[j].mSkipDisplayX = false;
                newPoints[j].mSkipDisplayZ = false;
            }
            
            (*points)[i] = newPoints;
        }
        else
            (*points)[i] = slices[targetIIdx];
        
        // Compute time step
        BL_FLOAT z = 0.0;
        if (densityNumSlicesI > 1)
        {
            if (mScrollDirection == BACK_FRONT)
                z = 1.0 - ((BL_FLOAT)i)/(points->size() - 1);
            else
                z = ((BL_FLOAT)i)/(points->size() - 1);
        }

#if FIX_JITTER_LOW_DENSITY
        // Scale z a bit, to compensate for the suppression of the first line
        if (mMode == LINES_FREQ)
        {
            BL_FLOAT coeff = ((BL_FLOAT)(points->size() + 1.0))/points->size();
            z *= coeff;

            if (mScrollDirection == FRONT_BACK)
                z -= 1.0/points->size();
        }
#endif
        
        // Center
        z -= 0.5;
        
        pointsZ.Get()[i] = z;
    }
    
    // Process result points
    for (int i = 0; i < points->size(); i++)
    {
        BL_FLOAT z = pointsZ.Get()[i];
        for (int j = 0; j < (*points)[i].size(); j++)
        {
            Point &p = (*points)[i][j];
            p.mZ = z;
        }
        
        // Scale on Y
        for (int j = 0; j < (*points)[i].size(); j++)
        {
            Point &p = (*points)[i][j];
            
#if USE_DB_SCALE
            if (mDBScale)
            {
                p.mY = BLUtils::AmpToDBNorm(p.mY, (BL_FLOAT)1e-15, mMinDB);
                
                // If we want to adjust more the db scale to the axis,
                // first try to examine this value
#if HACK_DB_SCALE
                p.mY *= 0.5;
#endif
            }
#endif
            
            p.mY *= mScale;
        }
    }
    
    // Final processes
    //
#if OPTIM_STRAIGHT_LINES
    if ((mMode == LINES_FREQ) ||
        (mMode == GRID))
    {
        OptimStraightLineX(points);
    }
    
    if (mMode == LINES_TIME)
    {
        OptimStraightLineZ(points);
    }
    
    // Project everything at one time
    for (int i = 0; i < points->size(); i++)
    {
        ProjectPoints(&(*points)[i], width, height);
    }
#endif
}

void
LinesRender2::ProjectSlicesNoDecim(vector<vector<Point> > *points,
                                   const bl_queue<vector<Point> > &slices,
                                   int width, int height)
{
    points->resize(slices.size());
    for (int i = 0; i < slices.size(); i++)
    {
        // Rescale on j
        vector<Point> &newPoints = mTmpBuf11;
        newPoints.resize(slices[i].size());
        for (int j = 0; j < slices[i].size(); j++)
        {
                
            const Point &p = slices[i][j];
            newPoints[j] = p;
            
            (*points)[i] = newPoints;
        }
        
        // Update z
        //
        
        // Compute time step
        BL_FLOAT z;
        if (slices.size() <= 1)
            z = 1.0 - ((BL_FLOAT)i)/slices.size();
        else
            z = 1.0 - ((BL_FLOAT)i)/(slices.size() - 1);
        
        // Center
        z -= 0.5;
        
        for (int j = 0; j < (*points)[i].size(); j++)
        {
            Point &p = (*points)[i][j];
            p.mZ = z;
        }
        
        // Scale on Y
        for (int j = 0; j < (*points)[i].size(); j++)
        {
            Point &p = (*points)[i][j];
            
#if USE_DB_SCALE
            if (mDBScale)
            {
                p.mY = BLUtils::AmpToDBNorm(p.mY, (BL_FLOAT)1e-15, mMinDB);
                
#if HACK_DB_SCALE
                p.mY *= 0.5;
#endif
            }
#endif

            p.mY *= mScale;
        }
        
        ProjectPoints(&(*points)[i], width, height);
    }
}

void
LinesRender2::SetMode(LinesRender2::Mode mode)
{
    mMode = mode;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::SetDensity(BL_FLOAT density)
{
    mDensityNumSlices = (1.0 - density)*DENSITY_MIN_NUM_SLICES +
                                density*DENSITY_MAX_NUM_SLICES;
    
    mMustRecomputeProj = true;
    
    if (mDbgForceDensityNumSlices)
    {
        mDensityNumSlices = mNumSlices;
    }
}

void
LinesRender2::DBG_ForceDensityNumSlices()
{
    mDbgForceDensityNumSlices = true;
    
    mDensityNumSlices = mNumSlices;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::SetScale(BL_FLOAT scale)
{
    mScale = scale;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::SetSpeed(BL_FLOAT speed)
{
    // Speed is in fact a step
    speed = 1.0 - speed;
    
    mSpeed = (1.0 - speed)*MIN_SPEED + speed*MAX_SPEED;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::SetScrollDirection(LinesRender2::ScrollDirection dir)
{
    mScrollDirection = dir;
}

int
LinesRender2::GetSpeed()
{
    return mSpeed;
}

void
LinesRender2::AddAxis(Axis3D *axis)
{
    mAxis.push_back(axis);
}

void
LinesRender2::RemoveAxis(Axis3D *axis)
{
    vector<Axis3D *> newAxes;
    for (int i = 0; i < mAxis.size(); i++)
    {
        Axis3D *a = mAxis[i];
        if (a != axis)
        {
            newAxes.push_back(a);
        }
    }
    
    mAxis = newAxes;
}

void
LinesRender2::SetShowAxes(bool flag)
{
    mShowAxes = flag;
}

void
LinesRender2::SetDBScale(bool flag, BL_FLOAT minDB)
{
    mDBScale = flag;
    mMinDB = minDB;
    
    mMustRecomputeProj = true;
}

void
LinesRender2::SetAdditionalLines(const vector<Line> &lines, BL_FLOAT lineWidth)
{
    mAdditionalLines = lines;
    
    mAdditionalLinesWidth = lineWidth;
}

void
LinesRender2::ClearAdditionalLines()
{
    mAdditionalLines.clear();
}

void
LinesRender2::ShowAdditionalLines(bool flag)
{
    mShowAdditionalLines = flag;
}

void
LinesRender2::DrawAdditionalLines(NVGcontext *vg, int width, int height)
{
    if (!mShowAdditionalLines)
        return;
    
    if (mAdditionalLines.empty())
        return;

    vector<Line> &lines = mTmpBuf12;
    lines = mAdditionalLines;
    ProjectAdditionalLines(&lines, width, height); // Was commented
    //ProjectAdditionalLines2(&lines, width, height); // Buggy

    for (int i = 0; i < lines.size(); i++)
    {
        Line &line = lines[i];
                
        DoDrawLineSimple(vg, line.mPoints, line.mColor, mAdditionalLinesWidth);
    }
}

void
LinesRender2::ProjectAdditionalLines(vector<Line> *lines, int width, int height)
{
    *lines = mAdditionalLines;
    
    for (int i = 0; i < lines->size(); i++)
    {
        Line &line = (*lines)[i];
        
        for (int j = 0; j < line.mPoints.size(); j++)
        {
            LinesRender2::Point &p = line.mPoints[j];
            
            p.mZ = 1.0 - p.mZ;
            
            // Center
            p.mZ -= 0.5;
        
#if USE_DB_SCALE
            if (mDBScale)
            {
                p.mY = BLUtils::AmpToDBNorm(p.mY, (BL_FLOAT)1e-15, mMinDB);
                
#if HACK_DB_SCALE
                p.mY *= 0.5;
#endif
            }
#endif
            p.mY *= mScale;
        }
        
        ProjectPoints(&line.mPoints, width, height);
    }
}

void
LinesRender2::ProjectAdditionalLines2(vector<Line> *lines, int width, int height)
{
    BL_FLOAT step = 1.0;
    if (mDensityNumSlices < mNumSlices)
        step = ((BL_FLOAT)mNumSlices)/mDensityNumSlices;
    step = bl_round(step);
    if (step < 1.0)
        step = 1.0;
    
    for (int i = 0; i < lines->size(); i++)
    {
        Line &line = (*lines)[i];
        
        Line &newLine = mTmpLine;
        for (int k = 0; k < 4; k++)
            newLine.mColor[k] = line.mColor[k];

        int pointOffset = line.mPoints.empty() ? 0 : 1;
        newLine.mPoints.resize(line.mPoints.size()/step + pointOffset);
        int pointIdx = 0;
        
        for (int j = 0; j < line.mPoints.size(); j += (int)step)
        {
            LinesRender2::Point p = line.mPoints[j];

            // TEST
            //p.mSkipDisplayX = false;
            //p.mSkipDisplayZ = false;
                
#if 1 // For SASViewer, TRACKING mode
            p.mZ = 1.0 - p.mZ;
#endif
            
            // Center
            p.mZ -= 0.5;
            
#if USE_DB_SCALE
            if (mDBScale)
            {
                p.mY = BLUtils::AmpToDBNorm(p.mY, (BL_FLOAT)1e-15, mMinDB);
                
#if HACK_DB_SCALE
                p.mY *= 0.5;
#endif
            }
#endif
            
            p.mY *= mScale;
            
            newLine.mPoints[pointIdx++] = p;
        }
        
        // Fix the extremity of the lines at low density
        if (step >= 2.0)
        {
            if (!line.mPoints.empty())
            {
                LinesRender2::Point p = line.mPoints[line.mPoints.size() - 1];
            
                // Center
                p.mZ -= 0.5;
                
#if USE_DB_SCALE
                if (mDBScale)
                {
                    p.mY = BLUtils::AmpToDBNorm(p.mY, (BL_FLOAT)1e-15, mMinDB);
                    
#if HACK_DB_SCALE
                    p.mY *= 0.5;
#endif
                }
#endif
                
                p.mY *= mScale;
            
                newLine.mPoints[pointIdx++] = p;
            }
        }

#if 1 // FIX: this fixes bad point at the end (this made draw "star" shapes)
        // Just to be sure...
        // To eliminate potential invalid points at the end
        newLine.mPoints.resize(pointIdx);
#endif
        
#if 1 //0 //1 // ORIG
        line = newLine;
        ProjectPoints(&line.mPoints, width, height);
#endif
        
#if 0 //1 //0 //1 // TEST
        ProjectPoints(&newLine.mPoints, width, height);
        (*lines)[i] = newLine;;
#endif
        
    }
}

void
LinesRender2::SetAdditionalPoints(const vector<Line> &lines, BL_FLOAT lineWidth)
{
    mAdditionalPoints = lines;
    
    mAdditionalPointsWidth = lineWidth;
}

void
LinesRender2::ClearAdditionalPoints()
{
    mAdditionalPoints.clear();
}

void
LinesRender2::ShowAdditionalPoints(bool flag)
{
    mShowAdditionalPoints = flag;
}

void
LinesRender2::DrawAdditionalPoints(NVGcontext *vg, int width, int height)
{
    if (!mShowAdditionalPoints)
        return;
    
    if (mAdditionalPoints.empty())
        return;

    vector<Line> &lines = mTmpBuf17;
    lines = mAdditionalPoints;
    ProjectAdditionalPoints(&lines, width, height);

    for (int i = 0; i < lines.size(); i++)
    {
        Line &line = lines[i];

        DoDrawPointsSimple(vg, line.mPoints, mAdditionalPointsWidth);
    }
}

void
LinesRender2::ProjectAdditionalPoints(vector<Line> *lines, int width, int height)
{
    *lines = mAdditionalPoints;
    
    for (int i = 0; i < lines->size(); i++)
    {
        Line &line = (*lines)[i];
        
        for (int j = 0; j < line.mPoints.size(); j++)
        {
            LinesRender2::Point &p = line.mPoints[j];
            
            p.mZ = 1.0 - p.mZ;
            
            // Center
            p.mZ -= 0.5;
        
#if USE_DB_SCALE
            if (mDBScale)
            {
                p.mY = BLUtils::AmpToDBNorm(p.mY, (BL_FLOAT)1e-15, mMinDB);
                
#if HACK_DB_SCALE
                p.mY *= 0.5;
#endif
            }
#endif
            p.mY *= mScale;
        }
        
        ProjectPoints(&line.mPoints, width, height);
    }
}

void
LinesRender2::SetDirty()
{
    mMustRecomputeProj = true;
}

void
LinesRender2::SetColors(unsigned char color0[4], unsigned char color1[4])
{
    for (int i = 0; i < 4; i++)
        mColor0[i] = color0[i];
    
    for (int i = 0; i < 4; i++)
        mColor1[i] = color1[i];
}

// Suppress points that are on a sgtraight line
// (criterion is on y)
void
LinesRender2::DecimateStraightLine(vector<Point> *points)
{    
    // Decimate only when we have "horizontal" lines
    if (mMode != LINES_FREQ)
        return;
    
    int numPoints = 0;
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
    
        bool skip = false;
        if ((i > 0) && (i < points->size() - 1))
        {
            if ((std::fabs((*points)[i - 1].mY - (*points)[i].mY) <
                 OPTIM_STRAIGHT_LINES_EPS) &&
                (std::fabs((*points)[i].mY - (*points)[i + 1].mY) <
                 OPTIM_STRAIGHT_LINES_EPS))
                // We are on a straight line
            {
                skip = true;
            }
        }
        
        if (!skip)
            numPoints++;
    }

    vector<Point> &newPoints = mTmpBuf14;
    newPoints.resize(numPoints);
    int pointIdx = 0;
    
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
    
        bool skip = false;
        if ((i > 0) && (i < points->size() - 1))
        {
            if ((std::fabs((*points)[i - 1].mY - (*points)[i].mY) <
                 OPTIM_STRAIGHT_LINES_EPS) &&
                (std::fabs((*points)[i].mY - (*points)[i + 1].mY) <
                 OPTIM_STRAIGHT_LINES_EPS))
                // We are on a straight line
            {
                skip = true;
            }
        }
        
        if (!skip)
            newPoints[pointIdx++] = p;
    }
    
    *points = newPoints;
}

void
LinesRender2::OptimStraightLineX(vector<vector<Point> > *points)
{
    for (int i = 0; i < points->size(); i++)
    {
        vector<Point> &line = (*points)[i];
        
        for (int j = 0; j < line.size(); j++)
        {
            Point &p = line[j];
            
            bool skip = false;
            if ((j > 0) && (j < line.size() - 1))
            {
                if ((std::fabs(line[j - 1].mY - line[j].mY) <
                     OPTIM_STRAIGHT_LINES_EPS) &&
                    (std::fabs(line[j].mY - line[j + 1].mY) <
                     OPTIM_STRAIGHT_LINES_EPS))
                    // We are on a straight line
                {
                    skip = true;
                }
            }
            
            p.mSkipDisplayX = skip;
        }
    }
}

void
LinesRender2::OptimStraightLineZ(vector<vector<Point> > *points)
{
    if (points->empty())
        return;
    
    for (int j = 0; j < (*points)[0].size(); j++)
    {
        for (int i = 0; i < points->size(); i++)
        {
            Point &p = (*points)[i][j];
            
            bool skip = false;
            if ((i > 0) && (i < points->size() - 1))
            {
                if ((std::fabs((*points)[i - 1][j].mY - (*points)[i][j].mY) <
                     OPTIM_STRAIGHT_LINES_EPS) &&
                    (std::fabs((*points)[i][j].mY - (*points)[i + 1][j].mY) <
                     OPTIM_STRAIGHT_LINES_EPS))
                    // We are on a straight line
                {
                    skip = true;
                }
            }
            
            p.mSkipDisplayZ = skip;
        }
    }
}

#endif // IGRAPHICS_NANOVG
