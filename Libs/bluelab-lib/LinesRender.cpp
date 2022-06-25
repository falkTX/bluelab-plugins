/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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

#include <Axis3D.h>
#include <BLUtils.h>

#include "LinesRender.h"


#define RESOLUTION_0 32
#define RESOLUTION_1 512

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
    
    glm::mat4 perspMat = glm::perspective((float)perspAngle, ((float)winWidth)/winHeight, NEAR_PLANE, FAR_PLANE);
    
    glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
    
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f)); //
    modelMat = glm::rotate(modelMat, angle0, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.5+0.33/*0.0f*/, 0.0f));
    
    *model = viewMat * modelMat;
    *proj = perspMat;
}

//

LinesRender::LinesRender()
{
    mCamAngle0 = 0.0;
    mCamAngle1 = 0.0;
    
    mCamFov = MAX_FOV;
    
    mNumSlices = NUM_TOTAL_SLICES;
    
    mWhitePixImg = -1;
        
    mMode = LinesRender::POINTS;
    mDensityNumSlices = DENSITY_MIN_NUM_SLICES;
    mScale = 0.5;
    mScrollDirection = LinesRender::BACK_FRONT;
    
    mShowAxis = false;
    
    mAddNum = 0;
    mSpeed = MIN_SPEED;
    
    // Keep internally
    mViewWidth = 0;
    mViewHeight = 0;
    
    Init();
    
#if PROFILE_RENDER
    BlaTimer::Reset(&mTimer0, &mCount0);
#endif
}

LinesRender::~LinesRender() {}

void
LinesRender::Init()
{
    // Fill with zero values
    // To have a flat grid at the beginning
    vector<Point> points;
    points.resize(BUFFER_SIZE/2);
    for (int i = 0; i < points.size(); i++)
    {
        Point &p = points[i];
        p.mX = ((BL_FLOAT)i)/points.size() - 0.5;
        p.mY = 0.0;
    }
    
    for (int i = 0; i < mNumSlices; i++)
        mSlices.push_back(points);
    
    UpdateSlicesZ();
}

void
LinesRender::SetNumSlices(long numSlices)
{
    mNumSlices = numSlices;
}

long
LinesRender::GetNumSlices()
{
    return mNumSlices;
}

void
LinesRender::PreDraw(NVGcontext *vg, int width, int height)
{
#if PROFILE_RENDER
    BlaTimer::Start(&mTimer0);
#endif
    
    mViewWidth = width;
    mViewHeight = height;
    
    vector<vector<Point> > points;
    ProjectSlices(&points, mViewWidth, mViewHeight);
    
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

    if (mShowAxis)
    {
        for (int i = 0; i < mAxis.size(); i++)
        {
            Axis3D *axis = mAxis[i];
            axis->Draw(vg, width, height);
        }
    }
    
#if PROFILE_RENDER
    BlaTimer::StopAndDump(&mTimer0, &mCount0,
                          "profile.txt",
                          "full: %ld ms \n");
#endif
}

void
LinesRender::DrawPoints(NVGcontext *vg, const vector<vector<Point> > &points)
{
#if 0 // Simple
    unsigned char color[4] = { 200, 200, 255, 255 };
    DoDrawPoints(vg, points, color, 2.0);
#endif
    
    unsigned char color0[4] = { 128, 128, 255, 255 };
    DoDrawPoints(vg, points, color0, 3.0);
    
    unsigned char color2[4] = { 255, 255, 255, 255 };
    DoDrawPoints(vg, points, color2, 2.0/*1.0*/);
}

void
LinesRender::DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                          unsigned char inColor[4], BL_FLOAT pointSize)
{
    unsigned char color0[4] = { inColor[0], inColor[1], inColor[2], inColor[3/*2*/] };
    SWAP_COLOR(color0);
    
    NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
    
    nvgStrokeColor(vg, color);
    nvgFillColor(vg, color);
    
    for (int i = 0; i < points.size(); i++)
    {
        vector<Point> points0 = points[i];
        
        for (int j = 0; j < points0.size(); j++)
        {
            const Point &p = points0[j];
        
            BL_FLOAT x = p.mX;
            BL_FLOAT y = p.mY;
            
            BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
            yf = mViewHeight - yf;
#endif
            
            // Quad rendering
            float/*BL_FLOAT*/ corners[4][2] =
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
LinesRender::DrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points)
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
    
    // Medium
    unsigned char color0[4] = { 128, 128, 255, 255 };
    DoDrawLinesFreq(vg, points, color0, 3.0);
    
    unsigned char color2[4] = { 255, 255, 255, 255 };
    DoDrawLinesFreq(vg, points, color2, 1.0);
}

void
LinesRender::DoDrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points,
                             unsigned char inColor[4], BL_FLOAT lineWidth)
{
    if (points.empty())
        return;
    
    unsigned char color0[4] = { inColor[0], inColor[1], inColor[2], inColor[3/*2*/] };
    
    SWAP_COLOR(color0);
    
    NVGcolor color =  nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
    
    nvgStrokeColor(vg, color);
    nvgFillColor(vg, color);
    
    nvgStrokeWidth(vg, lineWidth);
    
    for (int i = 0; i < points.size(); i++)
    {
        vector<Point> points0 = points[i];
        
        int densityStepJ = points0.size()/mDensityNumSlices;
        if (densityStepJ == 0)
            densityStepJ = 1;
        
        nvgBeginPath(vg);
        
        for (int j = 0; j < points0.size(); j++)
        {
            const Point &p = points0[j];
            
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
LinesRender::DrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points)
{
#if 0 // Simple
    unsigned char color[4] = { 200, 200, 255, 255 };
    DoDrawLinesTime(vg, points, color, 2.0);
#endif
    
    unsigned char color0[4] = { 128, 128, 255, 255 };
    DoDrawLinesTime(vg, points, color0, 3.0);
    
    unsigned char color2[4] = { 255, 255, 255, 255 };
    DoDrawLinesTime(vg, points, color2, 1.0);
}
                           

void
LinesRender::DoDrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points,
                             unsigned char inColor[4], BL_FLOAT lineWidth)
{
    unsigned char color0[4] = { inColor[0], inColor[1], inColor[2], inColor[3/*2*/] };
    
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
LinesRender::DrawGrid(NVGcontext *vg, const vector<vector<Point> > &points)
{
    unsigned char color0[4] = { 128, 128, 255, 255 };
    DoDrawGrid(vg, points, color0, 3.0);
    
    unsigned char color2[4] = { 255, 255, 255, 255 };
    DoDrawGrid(vg, points, color2, 1.0);
}

void
LinesRender::DoDrawGrid(NVGcontext *vg, const vector<vector<Point> > &points,
                        unsigned char inColor[4], BL_FLOAT lineWidth)
{
    DoDrawLinesFreq(vg, points, inColor, lineWidth);
    DoDrawLinesTime(vg, points, inColor, lineWidth);
}

void
LinesRender::ClearSlices()
{
    mSlices.clear();
}

/*
void
LinesRender::SetSlices(const vector<vector<Point> > &slices)
{
    mSlices.clear();
    
    for (int i = 0; i < slices.size(); i++)
        mSlices.push_back(slices[i]);
    
    // Could be improved
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();
    
    UpdateSlicesZ();
}
*/

void
LinesRender::AddSlice(const vector<Point> &points)
{
    bool skipDisplay = (mAddNum++ % mSpeed != 0);
    if (skipDisplay)
        return;
    
    vector<Point> points0 = points;
    
    mSlices.push_back(points0);
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();
    
    UpdateSlicesZ();
}

void
LinesRender::SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1)
{
    mCamAngle0 = angle0;
    mCamAngle1 = angle1;
}

BL_FLOAT
LinesRender::GetCameraFov()
{
    return mCamFov;
}

void
LinesRender::SetCameraFov(BL_FLOAT angle)
{
    mCamFov = angle;
}

void
LinesRender::ZoomChanged(BL_FLOAT zoomChange)
{
    mCamFov *= 1.0/zoomChange;
  
    if (mCamFov < MIN_FOV)
        mCamFov = MIN_FOV;
    
    if (mCamFov > MAX_FOV)
        mCamFov = MAX_FOV;
}

void
LinesRender::ResetZoom()
{
    mCamFov = MAX_FOV;
}

void
LinesRender::ProjectPoints(vector<Point> *points, int width, int height)
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
        
        // Matrix transform
        glm::vec4 v;
        v.x = p.mX;
        v.y = p.mY;
        v.z = p.mZ;
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
        z = (z + 1.0f)*0.5;
        
        // Result
        p.mX = x;
        p.mY = y;
        p.mZ = z;
    }
}

void
LinesRender::ProjectSlices(vector<vector<Point> > *points,
                           int width, int height)
{
    int densityNumSlicesI = mDensityNumSlices;
    int densityNumSlicesJ = mDensityNumSlices;
    
    if (mMode == LINES_FREQ)
        densityNumSlicesJ = DENSITY_MAX_NUM_SLICES_FREQS;
    
    if (mMode == LINES_TIME)
        densityNumSlicesI = DENSITY_MAX_NUM_SLICES;
    
    points->resize(densityNumSlicesI);
    for (int i = 0; i < densityNumSlicesI; i++)
    {
        int i0 = 0;
        if (densityNumSlicesI > 1)
        /*BL_FLOAT*/ i0 = (((BL_FLOAT)i)/(densityNumSlicesI - 1))*(mSlices.size() - 1);
    
        // Rescale on j
        vector<Point> newPoints;
        newPoints.resize(densityNumSlicesJ);
        for (int j = 0; j < densityNumSlicesJ; j++)
        {
            int j0 = 0;
            if (densityNumSlicesJ > 1)
                j0 = (((BL_FLOAT)j)/(densityNumSlicesJ - 1))*(mSlices[i0].size() - 1);
            
            Point &p = mSlices[i0][j0];
            
            newPoints[j] = p;
        }
        
        (*points)[i] = newPoints;
        
        // Update z
        //
        
        // Compute time step
        BL_FLOAT z = 0.0;
        if (densityNumSlicesI > 1)
        {
            if (mScrollDirection == BACK_FRONT)
                z = 1.0 - ((BL_FLOAT)i)/(densityNumSlicesI - 1);
            else
                z = ((BL_FLOAT)i)/(densityNumSlicesI - 1);
        }
        
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
            p.mY *= mScale;
        }
        
        ProjectPoints(&(*points)[i], width, height);
    }
}

void
LinesRender::SetMode(LinesRender::Mode mode)
{
    mMode = mode;
}

void
LinesRender::SetDensity(BL_FLOAT density)
{
    mDensityNumSlices = (1.0 - density)*DENSITY_MIN_NUM_SLICES +
                                density*DENSITY_MAX_NUM_SLICES;
}

BL_FLOAT
LinesRender::GetScale()
{
    return mScale;
}

void
LinesRender::SetScale(BL_FLOAT scale)
{
    mScale = scale;
}

void
LinesRender::SetSpeed(BL_FLOAT speed)
{
    // Speed is in fact a step
    speed = 1.0 - speed;
    
    mSpeed = (1.0 - speed)*MIN_SPEED + speed*MAX_SPEED;
}

void
LinesRender::SetScrollDirection(LinesRender::ScrollDirection dir)
{
    mScrollDirection = dir;
    
    // Unused
    UpdateSlicesZ();
}

void
LinesRender::AddAxis(Axis3D *axis)
{
    mAxis.push_back(axis);
}

void
LinesRender::RemoveAxis(Axis3D *axis)
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
LinesRender::SetShowAxis(bool flag)
{
    mShowAxis = flag;
}

// Unused
void
LinesRender::UpdateSlicesZ()
{
    if (mSlices.empty())
        return;
    
    // Adjust z for all the history
    for (int i = 0; i < mSlices.size(); i++)
    {
        // Compute time step
        BL_FLOAT z = 0.0;
        if (mScrollDirection == BACK_FRONT)
        {
            z = 1.0 - ((BL_FLOAT)i)/(mSlices.size() - 1);
        }
        else
        {
            z = ((BL_FLOAT)i)/(mSlices.size() - 1);
        }
        
        // Center
        z -= 0.5;
        
        // Get the slice
        vector<LinesRender::Point> &points = mSlices[i];
        
        // Set the same time step for all the points of the slice
        for (int j = 0; j < points.size(); j++)
        {
            points[j].mZ = z;
        }
    }
}

// UNUSED

// More verbose implementation to try to optimize
// (but does not optimize a lot)
void
LinesRender::DequeToVec(vector<LinesRender::Point> *res,
                        const deque<vector<LinesRender::Point> > &que)
{
    long numElements = 0;
    for (int i = 0; i < que.size(); i++)
        numElements += que[i].size();
    
    res->resize(numElements);
    
    long elementId = 0;
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < que.size(); i++)
#else
    for (int i = que.size() - 1; i >= 0; i--)
#endif
    {
        const vector<LinesRender::Point> &vec = que[i];
        for (int j = 0; j < vec.size(); j++)
        {
            const Point &val = vec[j];
            (*res)[elementId++] = val;
        }
    }
}

void
LinesRender::ProjectPoint(BL_FLOAT projP[3], const BL_FLOAT p[3], int width, int height)
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

#endif // IGRAPHICS_NANOVG
