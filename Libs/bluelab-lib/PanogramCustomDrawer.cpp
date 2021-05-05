//
//  PanogramCustomDrawer.cpp
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>
#include <GraphSwapColor.h>

#include "PanogramCustomDrawer.h"

// Clip the play bar to the selection
#define CLIP_PLAY_BAR 1

// The play bar goes a bit ouside the selection on the right
#define FIX_PLAY_BAR_OUTSIDE_RIGHT 1


PanogramCustomDrawer::PanogramCustomDrawer(Plugin *plug,
                                           BL_FLOAT x0, BL_FLOAT y0,
                                           BL_FLOAT x1, BL_FLOAT y1,
                                           State *state)
{
    mState = state;
    if (mState == NULL)
    {
        mState = new State();
        
        Reset();
    }
    
    mPlug = plug;

    // Warning: y is reversed ??
    mBounds[0] = x0;
    mBounds[1] = y0;
    mBounds[2] = x1;
    mBounds[3] = y1;
}

PanogramCustomDrawer::State *
PanogramCustomDrawer::GetState()
{
    return mState;
}

void
PanogramCustomDrawer::Reset()
{
    mState->mBarActive = false;
    mState->mBarPos = 0.0;
    
    mState->mPlayBarActive = false;
    mState->mPlayBarPos = 0.0;
    
    mState->mSelectionActive = false;
    
    for (int i = 0; i < 4; i++)
        mState->mSelection[i] = 0.0;
}

void
PanogramCustomDrawer::PostDraw(NVGcontext *vg, int width, int height)
{
    DrawBar(vg, width, height);
    
    DrawSelection(vg, width, height);
    
    DrawPlayBar(vg, width, height);
}

void
PanogramCustomDrawer::ClearBar()
{
    mState->mBarActive = false;
}

void
PanogramCustomDrawer::ClearSelection()
{
    mState->mSelectionActive = false;
}

void
PanogramCustomDrawer::SetBarPos(BL_FLOAT pos)
{
    mState->mBarPos = pos;
    
    // Set play bar to the same position, to start
    mState->mPlayBarPos = pos;
    
    mState->mBarActive = true;
    mState->mSelectionActive = false;
}

void
PanogramCustomDrawer::SetSelectionActive(bool flag)
{
    mState->mSelectionActive = flag;
}

BL_FLOAT
PanogramCustomDrawer::GetBarPos()
{
    return mState->mBarPos;
}

void
PanogramCustomDrawer::SetBarActive(bool flag)
{
    mState->mBarActive = flag;
}

bool
PanogramCustomDrawer::IsBarActive()
{
    return mState->mBarActive;
}

void
PanogramCustomDrawer::SetSelection(BL_FLOAT x0, BL_FLOAT y0,
                                BL_FLOAT x1, BL_FLOAT y1)
{
    // Bound only y
    // For x, we may want to select sound outside the view
    // and play it
    
    if (y0 < mBounds[1])
        y0 = mBounds[1];
    if (y0 > mBounds[3]) // Selection can be reversed
        y0 = mBounds[3];
    if (y1 < mBounds[1]) // Selection can be reversed
        y1 = mBounds[1];
    if (y1 > mBounds[3])
        y1 = mBounds[3];
    
    mState->mSelection[0] = x0;
    mState->mSelection[1] = y0;
    mState->mSelection[2] = x1;
    mState->mSelection[3] = y1;
}

void
PanogramCustomDrawer::GetSelection(BL_FLOAT *x0, BL_FLOAT *y0,
                                   BL_FLOAT *x1, BL_FLOAT *y1)
{
    *x0 = mState->mSelection[0];
    *y0 = mState->mSelection[1];
    
    *x1 = mState->mSelection[2];
    *y1 = mState->mSelection[3];
}

bool
PanogramCustomDrawer::IsSelectionActive()
{
    return mState->mSelectionActive;
}

BL_FLOAT
PanogramCustomDrawer::GetPlayBarPos()
{
    return mState->mPlayBarPos;
}

void
PanogramCustomDrawer::SetPlayBarPos(BL_FLOAT pos, bool activate)
{
    mState->mPlayBarPos = pos;
    
    if (activate)
        mState->mPlayBarActive = true;
}

bool
PanogramCustomDrawer::IsPlayBarActive()
{
    return mState->mPlayBarActive;
}

void
PanogramCustomDrawer::SetPlayBarActive(bool flag)
{
    mState->mPlayBarActive = flag;
}

void
PanogramCustomDrawer::SetSelPlayBarPos(BL_FLOAT pos)
{
    mState->mPlayBarPos = mState->mSelection[0] +
                            pos*(mState->mSelection[2] - mState->mSelection[0]);
    
    mState->mPlayBarActive = true;
}

void
PanogramCustomDrawer::DrawBar(NVGcontext *vg, int width, int height)
{
    if (!mState->mBarActive)
        return;
    
    BL_FLOAT strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1],
                                   colors[i][2], colors[i][3]));
        
        // Draw the circle
        nvgBeginPath(vg);
        
        // Draw the line
        BL_FLOAT x = mState->mBarPos*width;
        
        BL_GUI_FLOAT b1f = (1.0 - mBounds[1])*height;
        BL_GUI_FLOAT b3f = (1.0 - mBounds[3])*height;
#if GRAPH_CONTROL_FLIP_Y
        b1f = height - b1f;
        b3f = height - b3f;
#endif
        
        nvgMoveTo(vg, x, b1f);
        nvgLineTo(vg, x, b3f);
        
        nvgStroke(vg);
    }
}

void
PanogramCustomDrawer::DrawSelection(NVGcontext *vg, int width, int height)
{
    if (!mState->mSelectionActive)
        return;
    
    BL_FLOAT strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1],
                                   colors[i][2], colors[i][3]));
        
        // Draw the circle
        nvgBeginPath(vg);
        
        BL_GUI_FLOAT s1f = (1.0 - mState->mSelection[1])*height;
        BL_GUI_FLOAT s3f = (1.0 - mState->mSelection[3])*height;
#if GRAPH_CONTROL_FLIP_Y
        s1f = height - s1f;
        s3f = height - s3f;
#endif
        
        // Draw the line
        nvgMoveTo(vg, mState->mSelection[0]*width, s1f);
        
        nvgLineTo(vg, mState->mSelection[2]*width, s1f);
        nvgLineTo(vg, mState->mSelection[2]*width, s3f);
        nvgLineTo(vg, mState->mSelection[0]*width, s3f);
        nvgLineTo(vg, mState->mSelection[0]*width, s1f);
        
        nvgStroke(vg);
    }
}

void
PanogramCustomDrawer::DrawPlayBar(NVGcontext *vg, int width, int height)
{
    if (!mState->mPlayBarActive)
        return;
    
    BL_FLOAT strokeWidths[2] = { 2.0, 1.0 };
    
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1],
                                   colors[i][2], colors[i][3]));
        
        // Draw the bar
        nvgBeginPath(vg);
        
        // Draw the line
        BL_FLOAT x = mState->mPlayBarPos*width;
        
#if FIX_PLAY_BAR_OUTSIDE_RIGHT
        if (x > mState->mSelection[2]*width - 3)
            x = mState->mSelection[2]*width - 3;
#endif
        
#if !CLIP_PLAY_BAR
        nvgMoveTo(vg, x, (1.0 - mBounds[1])*height);
        nvgLineTo(vg, x, (1.0 - mBounds[3])*height);
#else
        
        BL_GUI_FLOAT b1f = (1.0 - mBounds[1])*height;
        BL_GUI_FLOAT b3f = (1.0 - mBounds[3])*height;
        BL_GUI_FLOAT s1f = (1.0 - mState->mSelection[1])*height;
        BL_GUI_FLOAT s3f = (1.0 - mState->mSelection[3])*height;
#if GRAPH_CONTROL_FLIP_Y
        b1f = height - b1f;
        b3f = height - b3f;
        
        s1f = height - s1f;
        s3f = height - s3f;
#endif
        
        if (!mState->mSelectionActive)
        {
            nvgMoveTo(vg, x, b1f);
            nvgLineTo(vg, x, b3f);
        }
        else
        {
            nvgMoveTo(vg, x, s1f);
            nvgLineTo(vg, x, s3f);
        }
#endif
        
        nvgStroke(vg);
    }
}

#endif // IGRAPHICS_NANOVG
