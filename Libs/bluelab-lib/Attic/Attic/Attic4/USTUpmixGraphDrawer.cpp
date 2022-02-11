//
//  USTUpmixGraphDrawer.cpp
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

#include <cmath>

// #bl-iplug2
//#include "nanovg.h"

#include <UST.h>

#include "USTUpmixGraphDrawer.h"

#ifndef M_PI 
#define M_PI 3.141592653589793
#endif

// Grid and circle geometry
#define MIN_ANGLE -0.35 //-0.4
#define MAX_ANGLE 0.35 //0.4

#define ANGLE_OFFSET M_PI/2.0

#define START_CIRCLE_RAD 0.15 //0.25
#define END_CIRCLE_RAD 0.75

#define Y_OFFSET 2.0 //4.0

//
#define NUM_ARCS 5
#define NUM_LINES 7

// Source
#define MIN_SOURCE_RAD 0.1
#define MAX_SOURCE_RAD 0.2

#define ORANGE_COLOR_SCHEME 0
#define BLUE_COLOR_SCHEME 1

#define MALIK_CIRCLE 1


USTUpmixGraphDrawer::USTUpmixGraphDrawer(UST *plug, GraphControl11 *graph)
{
    mGain = 0.0;
    mPan = 0.0;
    mDepth = 0.0;
    mBrillance = 0.0;
    
    // GraphCustomControl
    mWidth = 0;
    mHeight = 0;
    
    mSourceIsSelected = false;
    
    mPlug = plug;
    mGraph = graph;
}

USTUpmixGraphDrawer::~USTUpmixGraphDrawer() {}

void
USTUpmixGraphDrawer::PreDraw(NVGcontext *vg, int width, int height)
{
    mWidth = width;
    mHeight = height;

#if !MALIK_CIRCLE
    BL_FLOAT strokeWidth = 2.0; //3.0;
    int gridColor[4] = { 64, 64, 64, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
#else
    BL_FLOAT strokeWidth = 2.0;
    int gridColor[4] = { 128, 128, 128, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
#endif

    nvgStrokeWidth(vg, strokeWidth);
    
    SWAP_COLOR(gridColor);
    nvgStrokeColor(vg, nvgRGBA(gridColor[0], gridColor[1], gridColor[2], gridColor[3]));

    // Arcs
    for (int i = 0; i < NUM_ARCS; i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(NUM_ARCS - 1);
        
        BL_FLOAT r = (1.0 - t)*START_CIRCLE_RAD + t*END_CIRCLE_RAD + Y_OFFSET;
        
        BL_FLOAT origin[2];
        origin[0] = 0.5;
        origin[1] = START_CIRCLE_RAD - Y_OFFSET;
        
        BL_FLOAT of = origin[1]*height;
        BL_FLOAT a0 = MIN_ANGLE + ANGLE_OFFSET;
        BL_FLOAT a1 = MAX_ANGLE + ANGLE_OFFSET;
#if GRAPH_CONTROL_FLIP_Y
        of = height - of;

        a0 = - a0;
        a1 = - a1;
        
        BL_FLOAT tmp = a0;
        a0 = a1;
        a1 = tmp;
#endif
        
        nvgBeginPath(vg);
        nvgArc(vg,
               origin[0]*width, of,
               r*height,
               a0, a1, NVG_CW);
        
        nvgStroke(vg);
    }
    
    // Lines
    for (int i = 0; i < NUM_LINES; i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(NUM_LINES - 1);
        
        BL_FLOAT angle = (1.0 - t)*MIN_ANGLE + t*MAX_ANGLE;
        
        // NOTE: must multiply by hight here !
        // (to correctly deal with with width/height ratio)
        BL_FLOAT r0 = (START_CIRCLE_RAD + Y_OFFSET)*height;
    
        BL_FLOAT origin[2];
        origin[0] = 0.5*width;
        origin[1] = (START_CIRCLE_RAD - Y_OFFSET)*height;
    
        BL_FLOAT p0[2];
        p0[0] = origin[0] + r0*cos(angle + ANGLE_OFFSET);
        p0[1] = origin[1] + r0*sin(angle + ANGLE_OFFSET);
    
        BL_FLOAT r1 = (END_CIRCLE_RAD + Y_OFFSET)*height;
    
        BL_FLOAT p1[2];
        p1[0] = origin[0] + r1*cos(angle + ANGLE_OFFSET);
        p1[1] = origin[1] + r1*sin(angle + ANGLE_OFFSET);
    
        // Draw
        BL_FLOAT p0f = p0[1];
        BL_FLOAT p1f = p1[1];
#if GRAPH_CONTROL_FLIP_Y
        p0f = height - p0f;
        p1f = height - p1f;
#endif
        
        nvgBeginPath(vg);
        nvgMoveTo(vg, p0[0], p0f);
        nvgLineTo(vg, p1[0], p1f);
        nvgStroke(vg);
    }
    
    DrawSource(vg, width, height);
}

void
USTUpmixGraphDrawer::SetGain(BL_FLOAT gain)
{
    mGain = gain;
    
    //#if !USE_UPMIX_SECTION
    //mGraph->SetDirty(true);
    //#endif
    mGraph->SetDataChanged();
}

void
USTUpmixGraphDrawer::SetPan(BL_FLOAT pan)
{
    mPan = pan;
    
    //#if !USE_UPMIX_SECTION
    //mGraph->SetDirty(true);
    //#endif
    mGraph->SetDataChanged();}

void
USTUpmixGraphDrawer::SetDepth(BL_FLOAT depth)
{
    mDepth = depth;
    
    //#if !USE_UPMIX_SECTION
    //mGraph->SetDirty(true);
    //#endif
    mGraph->SetDataChanged();}

void
USTUpmixGraphDrawer::SetBrillance(BL_FLOAT brillance)
{
    mBrillance = brillance;
}

void
USTUpmixGraphDrawer::DrawSource(NVGcontext *vg, int width, int height)
{
    BL_FLOAT center[2];
    ComputeSourceCenter(center, width, height);
    
    BL_FLOAT radius0 = ComputeRad0(height);
    BL_FLOAT radius1 = ComputeRad1(height);
    
    // Color
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
    int circleColor0[4] = { 64, 64, 255, 200 };
    int circleColor1[4] = { 200, 200, 255, 64 };
#endif
    
#if ORANGE_COLOR_SCHEME
    int circleColor0[4] = { 232, 110, 36, 200 };
    int circleColor1[4] = { 252, 228, 205, 64 };
#endif

#if BLUE_COLOR_SCHEME
    int circleColor0[4] = { 113, 130, 182, 200 };
    int circleColor1[4] = { 193, 229, 237, 64 };
#endif

    
    // Fill
    //
    
    // Inner circle
    SWAP_COLOR(circleColor0);
    nvgFillColor(vg, nvgRGBA(circleColor0[0], circleColor0[1], circleColor0[2], circleColor0[3]));
    
    BL_FLOAT cf = center[1];
#if GRAPH_CONTROL_FLIP_Y
    cf = height - cf;
#endif
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], cf, radius0);
    nvgFill(vg);
    
    // Outer circle
    SWAP_COLOR(circleColor1);
    nvgFillColor(vg, nvgRGBA(circleColor1[0], circleColor1[1], circleColor1[2], circleColor1[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], cf, radius1);
    nvgFill(vg);
    
    // Make a strokes over, because if we only fill, there is aliasing
    // Stroke
    //
    BL_FLOAT strokeWidth = 2.0;
    nvgStrokeWidth(vg, strokeWidth);
    
    // Color
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
    int strokeColor0[4] = { 64, 64, 255, 200 };
    int strokeColor1[4] = { 200, 200, 255, 64 };
#endif
    
#if ORANGE_COLOR_SCHEME
    int strokeColor0[4] = { 232, 110, 36, 200 };
    int strokeColor1[4] = { 252, 228, 205, 64 };
#endif
    
#if BLUE_COLOR_SCHEME
    int strokeColor0[4] = { 113, 130, 182, 200 };
    //int strokeColor1[4] = { 193, 229, 237, 64 };
    
    // Blue
    int strokeColor1[4] = { 255, 255, 255, 255 };
    
    // Orange (Like Malile likes)
    //int strokeColor1[4] = { 234, 101, 0, 255 };
#endif
    
    // Inner circle
    SWAP_COLOR(strokeColor0);
    nvgStrokeColor(vg, nvgRGBA(strokeColor0[0], strokeColor0[1], strokeColor0[2], strokeColor0[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], cf, radius0);
    nvgStroke(vg);
    
    // Outer circle
    SWAP_COLOR(strokeColor1);
    nvgStrokeColor(vg, nvgRGBA(strokeColor1[0], strokeColor1[1], strokeColor1[2], strokeColor1[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], cf, radius1);
    nvgStroke(vg);
}

void
USTUpmixGraphDrawer::ComputeSourceCenter(BL_FLOAT center[2],
                                         int width, int height)
{
    BL_FLOAT normPan = (mPan + 1.0)*0.5;
    normPan = 1.0 - normPan;
    BL_FLOAT angle = (1.0 - normPan)*MIN_ANGLE + normPan*MAX_ANGLE;
    
    BL_FLOAT radius = (1.0 - mDepth)*START_CIRCLE_RAD + mDepth*END_CIRCLE_RAD + Y_OFFSET;
    radius *= height;
    
    BL_FLOAT origin[2];
    origin[0] = 0.5*width;
    origin[1] = (START_CIRCLE_RAD - Y_OFFSET)*height;
    
    center[0] = origin[0] + radius*cos(angle + ANGLE_OFFSET);
    center[1] = origin[1] + radius*sin(angle + ANGLE_OFFSET);
}

BL_FLOAT
USTUpmixGraphDrawer::ComputeRad0(int height)
{
    BL_FLOAT radius0 = MIN_SOURCE_RAD;
    radius0 *= height;
    
    return radius0;
}

BL_FLOAT
USTUpmixGraphDrawer::ComputeRad1(int height)
{
    BL_FLOAT radius1 = (1.0 - mGain)*MIN_SOURCE_RAD + mGain*MAX_SOURCE_RAD;
    radius1 *= height;

    return radius1;
}

void
USTUpmixGraphDrawer::SourceCenterToPanDepth(const BL_FLOAT center[2],
                                            int width, int height,
                                            BL_FLOAT *outPan, BL_FLOAT *outDepth)
{
    BL_FLOAT origin[2];
    origin[0] = 0.5*width;
    origin[1] = (START_CIRCLE_RAD - Y_OFFSET)*height;
    
    // Radius
    BL_FLOAT radius2 = (center[0] - origin[0])*(center[0] - origin[0]) +
                  (center[1] - origin[1])*(center[1] - origin[1]);
    BL_FLOAT radius = 0.0;
    if (radius2 > 0.0)
      radius = std::sqrt(radius2);
    
    radius = radius/height;
    radius -= Y_OFFSET;
    
    BL_FLOAT depth = (radius - START_CIRCLE_RAD)/(END_CIRCLE_RAD - START_CIRCLE_RAD);
    *outDepth = depth;
    
    // Angle
    BL_FLOAT angle = std::atan2(center[1] - origin[1], center[0] - origin[0]);
    angle -= ANGLE_OFFSET;
    
    BL_FLOAT pan = (angle - MIN_ANGLE)/(MAX_ANGLE - MIN_ANGLE);
    pan = 1.0 - pan;
    pan = (pan - 0.5)*2.0;
    *outPan = pan;
}

void
USTUpmixGraphDrawer::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    // Invert
    y = mHeight - y;
    
    BL_FLOAT center[2];
    ComputeSourceCenter(center, mWidth, mHeight);
    
    BL_FLOAT radius1 = ComputeRad1(mHeight);
    BL_FLOAT dist2 = (x - center[0])*(x - center[0]) +
                   (y - center[1])*(y - center[1]);
    
    if (dist2 <= radius1*radius1)
        mSourceIsSelected = true;
}

void
USTUpmixGraphDrawer::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    mSourceIsSelected = false;
}

void
USTUpmixGraphDrawer::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    if (!mSourceIsSelected)
        return;

    if (!pMod->A)
    {
        // Invert
        y = mHeight - y;
    
        BL_FLOAT center[2] = { (BL_FLOAT)x, (BL_FLOAT)y };
        BL_FLOAT newPan;
        BL_FLOAT newDepth;
        SourceCenterToPanDepth(center, mWidth, mHeight,
                               &newPan, &newDepth);
    
#if USE_UPMIX_SECTION
        mPlug->UpdateUpmixPanDepth(newPan, newDepth);
#else
        mPlug->UpdatePanDepthCB(newPan, newDepth);
#endif
    }
    else
    {
        BL_FLOAT drag = -dY;
        drag /= mHeight;
      
#if USE_UPMIX_SECTION
        mPlug->UpdateUpmixGain(drag);
#else
        mPlug->UpdateWidthCB(drag);
#endif
    }
    
    //#if !USE_UPMIX_SECTION
    //mGraph->SetDirty(true);
    //#endif
    mGraph->SetDataChanged();
}
