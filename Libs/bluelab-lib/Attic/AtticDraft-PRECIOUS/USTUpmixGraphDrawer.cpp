//
//  USTUpmixGraphDrawer.cpp
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

#include "nanovg.h"

#include <UST.h>

#include "USTUpmixGraphDrawer.h"

#ifndef M_PI 
#define M_PI 3.141592653589793
#endif

#define MIN_ANGLE -0.4
#define MAX_ANGLE 0.4

#define ANGLE_OFFSET M_PI/2.0

#define START_CIRCLE_RAD 1.5
#define END_CIRCLE_RAD 1.75

#define Y_OFFSET -2.75

#define NUM_ARCS 5

#define NUM_LINES 7

// Source
#define MIN_SOURCE_RAD 0.1
#define MAX_SOURCE_RAD 0.2

#define ORANGE_COLOR_SCHEME 1


USTUpmixGraphDrawer::USTUpmixGraphDrawer(UST *plug, GraphControl10 *graph)
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
    
    double strokeWidth = 2.0; //3.0;
    
    int gridColor[4] = { 64, 64, 64, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
    
    nvgStrokeWidth(vg, strokeWidth);
    
    SWAP_COLOR(gridColor);
    nvgStrokeColor(vg, nvgRGBA(gridColor[0], gridColor[1], gridColor[2], gridColor[3]));

    // Arcs
    for (int i = 0; i < NUM_ARCS; i++)
    {
        double t = ((double)i)/(NUM_ARCS - 1);
        
        double r = (1.0 - t)*START_CIRCLE_RAD + t*END_CIRCLE_RAD;
        
        double center[2];
        center[0] = 0.5;
        center[1] = r + Y_OFFSET;
        
        nvgBeginPath(vg);
        nvgArc(vg, center[0]*width, center[1]*height, r*height,
               MIN_ANGLE + ANGLE_OFFSET, MAX_ANGLE + ANGLE_OFFSET, NVG_CW);
        
        nvgStroke(vg);
    }
    
    // Lines
    for (int i = 0; i < NUM_LINES; i++)
    {
        double t = ((double)i)/(NUM_LINES - 1);
        
        double angle = (1.0 - t)*MIN_ANGLE + t*MAX_ANGLE;
        
        // NOTE: must multiply by hight here !
        // (to correctly deal with with width/height ratio)
        double r0 = START_CIRCLE_RAD*height;
    
        double center0[2];
        center0[0] = 0.5*width;
        center0[1] = r0 + Y_OFFSET*height;
    
        double p0[2];
        p0[0] = center0[0] + r0*cos(angle + ANGLE_OFFSET);
        p0[1] = center0[1] + r0*sin(angle + ANGLE_OFFSET);
    
        double r1 = END_CIRCLE_RAD*height;
    
        double center1[2];
        center1[0] = 0.5*width;
        center1[1] = r1 + Y_OFFSET*height;
    
        double p1[2];
        p1[0] = center1[0] + r1*cos(angle + ANGLE_OFFSET);
        p1[1] = center1[1] + r1*sin(angle + ANGLE_OFFSET);
    
        // Draw
        nvgBeginPath(vg);
        nvgMoveTo(vg, p0[0], p0[1]);
        nvgLineTo(vg, p1[0], p1[1]);
        nvgStroke(vg);
    }
    
    DrawSource(vg, width, height);
    
    // Draw the texts
//#define TEXT_OFFSET 6.0
    
//    SWAP_COLOR(fontColor); // ?
    
    // Left
//    GraphControl10::DrawText(vg,
//                             (1.0 - COS_PI4)*RADIUS + OFFSET_X - TEXT_OFFSET,
//                             COS_PI4*RADIUS + TEXT_OFFSET,
//                             FONT_SIZE, "L", fontColor,
//                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE /*NVG_ALIGN_BOTTOM*/);
}

void
USTUpmixGraphDrawer::SetGain(double gain)
{
    mGain = gain;
}

void
USTUpmixGraphDrawer::SetPan(double pan)
{
    mPan = pan;
}

void
USTUpmixGraphDrawer::SetDepth(double depth)
{
    mDepth = depth;
}

void
USTUpmixGraphDrawer::SetBrillance(double brillance)
{
    mBrillance = brillance;
}

void
USTUpmixGraphDrawer::DrawSource(NVGcontext *vg, int width, int height)
{
    double center[2];
    ComputeSourceCenter(center, width, height);
    
    double rad0 = ComputeRad0(height);
    double rad1 = ComputeRad1(height);
    
    // Color
#if !ORANGE_COLOR_SCHEME
    int circleColor0[4] = { 64, 64, 255, 200 };
    int circleColor1[4] = { 200, 200, 255, 64 };
#else
    int circleColor0[4] = { 232, 110, 36, 200 };
    int circleColor1[4] = { 252, 228, 205, 64 };
#endif
    
    // Fill
    //
    
    // Inner circle
    SWAP_COLOR(circleColor0);
    nvgFillColor(vg, nvgRGBA(circleColor0[0], circleColor0[1], circleColor0[2], circleColor0[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], center[1], rad0);
    nvgFill(vg);
    
    // Outer circle
    SWAP_COLOR(circleColor1);
    nvgFillColor(vg, nvgRGBA(circleColor1[0], circleColor1[1], circleColor1[2], circleColor1[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], center[1], rad1);
    nvgFill(vg);
    
    // Make a strokes over, because if we only fill, there is aliasing
    // Stroke
    //
    double strokeWidth = 2.0;
    nvgStrokeWidth(vg, strokeWidth);
    
    // Color
#if !ORANGE_COLOR_SCHEME
    int strokeColor0[4] = { 64, 64, 255, 200 };
    int strokeColor1[4] = { 200, 200, 255, 64 };
#else
    int strokeColor0[4] = { 232, 110, 36, 200 };
    int strokeColor1[4] = { 252, 228, 205, 64 };
#endif
    
    // Inner circle
    SWAP_COLOR(strokeColor0);
    nvgStrokeColor(vg, nvgRGBA(strokeColor0[0], strokeColor0[1], strokeColor0[2], strokeColor0[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], center[1], rad0);
    nvgStroke(vg);
    
    // Outer circle
    SWAP_COLOR(strokeColor1);
    nvgStrokeColor(vg, nvgRGBA(strokeColor1[0], strokeColor1[1], strokeColor1[2], strokeColor1[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], center[1], rad1);
    nvgStroke(vg);
}

void
USTUpmixGraphDrawer::ComputeSourceCenter(double center[2],
                                         int width, int height)
{
    double normPan = (mPan + 1.0)*0.5;
    normPan = 1.0 - normPan;
    double angle = (1.0 - normPan)*MIN_ANGLE + normPan*MAX_ANGLE;
    
    double normRad = (1.0 - mDepth)*START_CIRCLE_RAD + mDepth*END_CIRCLE_RAD;
    double rad = normRad*height;
    
    double origin[2];
    origin[0] = 0.5*width;
    origin[1] = rad + Y_OFFSET*height;
    
    center[0] = origin[0] + rad*cos(angle + ANGLE_OFFSET);
    center[1] = origin[1] + rad*sin(angle + ANGLE_OFFSET);
}

double
USTUpmixGraphDrawer::ComputeRad0(int height)
{
    double rad0 = MIN_SOURCE_RAD;
    rad0 *= height;
    
    return rad0;
}

double
USTUpmixGraphDrawer::ComputeRad1(int height)
{
    double rad1 = (1.0 - mGain)*MIN_SOURCE_RAD + mGain*MAX_SOURCE_RAD;
    rad1 *= height;
    
    return rad1;
}

#if 0
void
USTUpmixGraphDrawer::SourceCenterToPanDepth(const double center[2],
                                            int width, int height,
                                            double *outPan, double *outDepth)
{
    double origin[2] = { 0.5*width, height + Y_OFFSET*height };
    //double origin[2];
    //origin[0] = 0.5*width;
    //origin[1] = 0.0;
    
    double rad2 = (center[0] - origin[0])*(center[0] - origin[0]) +
                  (center[1] - origin[1])*(center[1] - origin[1]);
    double rad = 0.0;
    if (rad2 > 0.0)
        rad = sqrt(rad2);
    
    // TEST
    //rad -= Y_OFFSET*height;
    
    fprintf(stderr, "rad: %g\n", rad);
    
    double normRad = rad/height;
    normRad = (normRad - START_CIRCLE_RAD)/(END_CIRCLE_RAD - START_CIRCLE_RAD);
    *outDepth = normRad;
    
    fprintf(stderr, "depth: %g\n", *outDepth);
    
    double angle = atan2(center[1], center[0]);
    //double angle = atan2(center[1] - origin[1], center[0] - origin[0]);
    double pan = (angle - MIN_ANGLE)/(MAX_ANGLE - MIN_ANGLE);
    pan = 1.0 - pan;
    pan = (pan - 0.5)*2.0;
    *outPan = pan;
    
    // DEBUG
    *outPan = 0.0;
}
#endif

void
USTUpmixGraphDrawer::SourceCenterToPanDepth(const double center[2],
                                            int width, int height,
                                            double *outPan, double *outDepth)
{
    //double origin[2] = { 0.5*width, height + Y_OFFSET*height };
    //double origin[2];
    //origin[0] = 0.5*width;
    //origin[1] = 0.0;
    
TODO: invert transformation
    
    center[0] -= 0.5*width;
    center[1] -=
    
    double rad2 = (center[0] - origin[0])*(center[0] - origin[0]) +
    (center[1] - origin[1])*(center[1] - origin[1]);
    double rad = 0.0;
    if (rad2 > 0.0)
        rad = sqrt(rad2);
    
    // TEST
    //rad -= Y_OFFSET*height;
    
    fprintf(stderr, "rad: %g\n", rad);
    
    double normRad = rad/height;
    normRad = (normRad - START_CIRCLE_RAD)/(END_CIRCLE_RAD - START_CIRCLE_RAD);
    *outDepth = normRad;
    
    fprintf(stderr, "depth: %g\n", *outDepth);
    
    double angle = atan2(center[1], center[0]);
    angle -= ANGLE_OFFSET;
    
    //double angle = atan2(center[1] - origin[1], center[0] - origin[0]);
    double pan = (angle - MIN_ANGLE)/(MAX_ANGLE - MIN_ANGLE);
    pan = 1.0 - pan;
    pan = (pan - 0.5)*2.0;
    *outPan = pan;
    
    // DEBUG
    *outPan = 0.0;
}

void
USTUpmixGraphDrawer::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    // Invert
    y = mHeight - y;
    
    double center[2];
    ComputeSourceCenter(center, mWidth, mHeight);
    
    double rad1 = ComputeRad1(mHeight);
    double dist2 = (x - center[0])*(x - center[0]) +
                   (y - center[1])*(y - center[1]);
    
    if (dist2 <= rad1*rad1)
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

    // Invert
    y = mHeight - y;
    
    double center[2] = { x, y };
    double newPan;
    double newDepth;
    SourceCenterToPanDepth(center, mWidth, mHeight,
                           &newPan, &newDepth);
    
    mPlug->UpdateUpmixPanDepth(newPan, newDepth);
    
    mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}
