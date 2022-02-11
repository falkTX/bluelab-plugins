//
//  BLUpmixGraphDrawer.cpp
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include <BLVectorscope.h>

#include "BLUpmixGraphDrawer.h"

#ifndef M_PI 
#define M_PI 3.141592653589793
#endif

// Grid and circle geometry
#define MIN_ANGLE -0.35
#define MAX_ANGLE 0.35

#define ANGLE_OFFSET M_PI/2.0

#define START_CIRCLE_RAD 0.15
#define END_CIRCLE_RAD 0.75

#define Y_OFFSET 2.0

//
#define NUM_ARCS 5
#define NUM_LINES 7

// Source
#define MIN_SOURCE_RAD 0.1
#define MAX_SOURCE_RAD 0.2

#define TITLE_POS_X FONT_SIZE*0.5
#define TITLE_POS_Y FONT_SIZE*0.5

// Fix Protools crash (mac)
#define FIX_PROTOOLS_CRASH_AT_INSERT 1

#define FIX_PT_ALT_DRAG_UPMIX 1


BLUpmixGraphDrawer::BLUpmixGraphDrawer(BLVectorscopePlug *plug, GraphControl11 *graph,
                                       const char *title)
{
    mGain = 0.0;
    mPan = 0.0;
    mDepth = 0.0;
    mBrillance = 0.0;
    
    // GraphCustomControl
    mWidth = 0;
    mHeight = 0;
    
    mSourceIsSelected = false;
    mPrevMouseDrag = false;
    
    mPlug = plug;
    mGraph = graph;
    
    // Title
    mTitleSet = false;
    
    if (title != NULL)
    {
        mTitleSet = true;
        
        sprintf(mTitleText, "%s", title);
    }
}

BLUpmixGraphDrawer::~BLUpmixGraphDrawer() {}

void
BLUpmixGraphDrawer::PreDraw(NVGcontext *vg, int width, int height)
{
    mWidth = width;
    mHeight = height;

    //
    BL_FLOAT strokeWidth = 2.0;
    int gridColor[4] = { 128, 128, 128, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };

    nvgStrokeWidth(vg, strokeWidth);
    
    SWAP_COLOR(gridColor);
    nvgStrokeColor(vg, nvgRGBA(gridColor[0], gridColor[1], gridColor[2], gridColor[3]));

    // Arcs
    for (int i = 0; i < NUM_ARCS; i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(NUM_ARCS - 1);
        
        BL_FLOAT r = (1.0 - t)*START_CIRCLE_RAD + t*END_CIRCLE_RAD + Y_OFFSET;
        
        BL_FLOAT origin[2];
        origin[0] = 0.5*width;
        origin[1] = (START_CIRCLE_RAD - Y_OFFSET)*height;
        
        BL_FLOAT origin1Yf = origin[1];
        BL_FLOAT a0 = MIN_ANGLE + ANGLE_OFFSET;
        BL_FLOAT a1 = MAX_ANGLE + ANGLE_OFFSET;
#if GRAPH_CONTROL_FLIP_Y
        origin1Yf = height - origin1Yf;
        
        a0 = - a0;
        a1 = - a1;
        
        BL_FLOAT tmp = a0;
        a0 = a1;
        a1 = tmp;
#endif
        
        nvgBeginPath(vg);
        nvgArc(vg,
               origin[0],
               origin1Yf,
               r*height,
               a0, a1,
               NVG_CW);
        
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
    
        BL_FLOAT p0Yf = p0[1];
        BL_FLOAT p1Yf = p1[1];
#if GRAPH_CONTROL_FLIP_Y
        p0Yf = height - p0Yf;
        p1Yf = height - p1Yf;
#endif
        
        // Draw
        nvgBeginPath(vg);
        nvgMoveTo(vg, p0[0], p0Yf);
        nvgLineTo(vg, p1[0], p1Yf);
        nvgStroke(vg);
    }
    
    DrawSource(vg, width, height);
    
    if (mTitleSet)
    {
        GraphControl11::DrawText(vg,
                                 TITLE_POS_X, height - TITLE_POS_Y,
                                 width, height,
                                 FONT_SIZE, mTitleText, fontColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_TOP);
    }
}

void
BLUpmixGraphDrawer::OnMouseDown(int x, int y, IMouseMod* pMod)
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
    
    mPrevMouseDrag = false;
}

void
BLUpmixGraphDrawer::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    mSourceIsSelected = false;
    
    mPrevMouseDrag = false;
}

void
BLUpmixGraphDrawer::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    // Protools intercepts alt+mouse down, and does not send mouse down
    //
    // (so we miss the first mouse down, we don't select the source, and
    // we can't gro the width).
    // Effect: we must first click, then push alt key
    // (pushing alt key first, then mouse drag doesn't work)
    //
    // FIX: Give a second chance to select the sournce
#if FIX_PT_ALT_DRAG_UPMIX
#if AAX_API
    if (!mPrevMouseDrag)
    {
        if (!mSourceIsSelected)
        {
            OnMouseDown(x, y, pMod);
        }
    }
    
    mPrevMouseDrag = true;
#endif
#endif
    
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
        
        //mPlug->VectorscopeUpdatePanDepthCB(newPan, newDepth);
        mPlug->VectorscopeUpdatePanCB(newPan);
    }
    else
    {
        BL_FLOAT drag = -dY;
        drag /= mHeight;
        
        mPlug->VectorscopeUpdateDWidthCB(drag);
    }
    
#if !FIX_PROTOOLS_CRASH_AT_INSERT
    mGraph->SetDirty(true);
#else
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif
}

void
BLUpmixGraphDrawer::DrawSource(NVGcontext *vg, int width, int height)
{
    BL_FLOAT center[2];
    ComputeSourceCenter(center, width, height);
    
    BL_FLOAT radius0 = ComputeRad0(height);
    BL_FLOAT radius1 = ComputeRad1(height);
    
    // Color
    //int circleColor0[4] = { 113, 130, 182, 200 };
    int circleColor0[4] = { 153, 176, 246, 255/*200*/ };
    int circleColor1[4] = { 193, 229, 237, 64 };
    
    // Fill
    //
    
    // Inner circle
    SWAP_COLOR(circleColor0);
    nvgFillColor(vg, nvgRGBA(circleColor0[0], circleColor0[1], circleColor0[2], circleColor0[3]));
    
    BL_FLOAT centerYf = center[1];
#if GRAPH_CONTROL_FLIP_Y
    centerYf = height - centerYf;
#endif
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], centerYf, radius0);
    nvgFill(vg);
    
    // Outer circle
    SWAP_COLOR(circleColor1);
    nvgFillColor(vg, nvgRGBA(circleColor1[0], circleColor1[1], circleColor1[2], circleColor1[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], centerYf, radius1);
    nvgFill(vg);
    
    // Make a strokes over, because if we only fill, there is aliasing
    // Stroke
    //
    BL_FLOAT strokeWidth = 2.0;
    nvgStrokeWidth(vg, strokeWidth);
    
    // Color
    //int strokeColor0[4] = { 113, 130, 182, 200 };
    int strokeColor0[4] = { 153, 176, 246, 200 };
    
    int strokeColor1[4] = { 255, 255, 255, 255 };
    
    // Inner circle
    SWAP_COLOR(strokeColor0);
    nvgStrokeColor(vg, nvgRGBA(strokeColor0[0], strokeColor0[1], strokeColor0[2], strokeColor0[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], centerYf, radius0);
    nvgStroke(vg);
    
    // Outer circle
    SWAP_COLOR(strokeColor1);
    nvgStrokeColor(vg, nvgRGBA(strokeColor1[0], strokeColor1[1], strokeColor1[2], strokeColor1[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, center[0], centerYf, radius1);
    nvgStroke(vg);
}

void
BLUpmixGraphDrawer::ComputeSourceCenter(BL_FLOAT center[2],
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
BLUpmixGraphDrawer::ComputeRad0(int height)
{
    BL_FLOAT radius0 = MIN_SOURCE_RAD;
    radius0 *= height;
    
    return radius0;
}

BL_FLOAT
BLUpmixGraphDrawer::ComputeRad1(int height)
{
    BL_FLOAT radius1 = (1.0 - mGain)*MIN_SOURCE_RAD + mGain*MAX_SOURCE_RAD;
    radius1 *= height;
    
    return radius1;
}

void
BLUpmixGraphDrawer::SourceCenterToPanDepth(const BL_FLOAT center[2],
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
BLUpmixGraphDrawer::SetGain(BL_FLOAT gain)
{
    mGain = gain;
    
#if !FIX_PROTOOLS_CRASH_AT_INSERT
    mGraph->SetDirty(true);
#else
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif
}

void
BLUpmixGraphDrawer::SetPan(BL_FLOAT pan)
{
    mPan = pan;
    
#if !FIX_PROTOOLS_CRASH_AT_INSERT
    mGraph->SetDirty(true);
#else
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif
}

void
BLUpmixGraphDrawer::SetDepth(BL_FLOAT depth)
{
    mDepth = depth;
    
#if !FIX_PROTOOLS_CRASH_AT_INSERT
    mGraph->SetDirty(true);
#else
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif
}

void
BLUpmixGraphDrawer::SetBrillance(BL_FLOAT brillance)
{
    mBrillance = brillance;
}
