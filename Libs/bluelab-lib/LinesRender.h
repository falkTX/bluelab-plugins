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
//  LineRender.h
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_Waves__LineRender__
#define __BL_Waves__LineRender__

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl12.h>
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


class Axis3D;

class LinesRender : public GraphCustomDrawer,
                    public PointProjector
{
public:
    enum Mode
    {
        LINES_FREQ = 0,
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
        BL_FLOAT mX;
        BL_FLOAT mY;
        BL_FLOAT mZ;
        
        unsigned char mR;
        unsigned char mG;
        unsigned char mB;
        unsigned char mA;
    };
    
    LinesRender();
    
    virtual ~LinesRender();

    void ProjectPoint(BL_FLOAT projP[3], const BL_FLOAT p[3], int width, int height);
    
    void Init();

    void SetNumSlices(long numSlices);
    
    long GetNumSlices();
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height);
    
    void ClearSlices();
    
    //void SetSlices(const vector< vector<Point> > &points);
    
    void AddSlice(const vector<Point> &points);

    
    void SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1);
    
    BL_FLOAT GetCameraFov();
    void SetCameraFov(BL_FLOAT angle);
    
    void ZoomChanged(BL_FLOAT zoomChange);
    
    void ResetZoom();
    
    //
    void SetMode(Mode mode);
    
    void SetDensity(BL_FLOAT density);
    
    BL_FLOAT GetScale();
    void SetScale(BL_FLOAT scale);

    void SetSpeed(BL_FLOAT speed);
    
    void SetScrollDirection(ScrollDirection dir);
    
    //
    void AddAxis(Axis3D *axis);
    void RemoveAxis(Axis3D *axis);
    
    void SetShowAxis(bool flag);
    
protected:
    void ProjectPoints(vector<Point> *slices, int width, int height);
    
    void ProjectSlices(vector<vector<Point> > *points,
                       int width, int height);
    
    void RenderLines(vector<Point> *slices, int width, int height);
    
    void DoRenderLines(vector<Point> *slices, int width, int height);
    
    void UpdateSlicesZ();
    
    static void DequeToVec(vector<LinesRender::Point> *res,
                           const deque<vector<LinesRender::Point> > &que);
    
    //
    void DrawPoints(NVGcontext *vg, const vector<vector<Point> > &points);

    void DrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points);
    
    void DrawGrid(NVGcontext *vg, const vector<vector<Point> > &points);
    
    //
    void DoDrawPoints(NVGcontext *vg, const vector<vector<Point> > &points,
                      unsigned char inColor[4], BL_FLOAT pointSize);
    
    void DoDrawLinesFreq(NVGcontext *vg, const vector<vector<Point> > &points,
                         unsigned char inColor[4], BL_FLOAT lineWidth);
    
    void DoDrawLinesTime(NVGcontext *vg, const vector<vector<Point> > &points,
                         unsigned char inColor[4], BL_FLOAT lineWidth);
    
    void DoDrawGrid(NVGcontext *vg, const vector<vector<Point> > &points,
                    unsigned char inColor[4], BL_FLOAT lineWidth);
    
    
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
    
    bool mShowAxis;
    
    int mSpeed;
    long int mAddNum;
       
    // Optim: GOOD (optim x2)
    // Do not re-allocate points each time
    // Keep a buffer
    vector<LinesRender::Point> mTmpPoints;
    
    // Optim: GOOD (optim x2)
    // Do not re-allocate the grid each time
    vector<vector<vector<Point> > > mTmpGrid;
    
    int mViewWidth;
    int mViewHeight;
    
    vector<Axis3D *> mAxis;
    
#if PROFILE_RENDER
    BlaTimer mTimer0;
    long mCount0;
#endif
};

#endif /* defined(__BL_StereoWidth__RayCaster__) */
