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
//  MiniView2.cpp
//  BL-Ghost
//
//  Created by Pan on 15/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphSwapColor.h>

#include <BLUtils.h>
#include <BLUtilsDecim.h>

#include "MiniView2.h"

#define BORDER_OFFSET_WIDTH 1
#define BOTTOM_BORDER_OFFSET 3 //2

MiniView2::MiniView2(int maxNumPoints,
                     BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1,
                     State *state)
{
    mBounds[0] = x0;
    mBounds[1] = y0;
    mBounds[2] = x1;
    mBounds[3] = y1;
    
    mMaxNumPoints = maxNumPoints;

    mState = state;
    if (mState == NULL)
    {
        mState = new State();
        
        mState->mMinNormX = 0.0;
        mState->mMaxNormX = 1.0;
    }
}

MiniView2::~MiniView2() {}

MiniView2::State *
MiniView2::GetState()
{
    return mState;
}

void
//MiniView2::PreDraw(NVGcontext *vg, int width, int height)
MiniView2::PostDraw(NVGcontext *vg, int width, int height)
{
    // Draw black background, this will avoid that the graph big waveform
    // is drawn over the miniview
    //DrawBackground(vg, width, height);
    
    nvgSave(vg);
    
    BL_FLOAT strokeWidth = 2.0;
    nvgStrokeWidth(vg, strokeWidth);
    
    int color[4] = { 255, 255, 255, 255 };
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    nvgBeginPath(vg);
    
    // Draw the rectangle corresponding to the visible area
    
    BL_FLOAT boundsSizeX = mBounds[2] - mBounds[0];
    //BL_FLOAT boundsSizeY = mBounds[1] - mBounds[3];
    
    BL_GUI_FLOAT b1f = mBounds[1]*height;
    BL_GUI_FLOAT b3f = mBounds[3]*height;
#if 0 //GRAPH_CONTROL_FLIP_Y
    b1f = height - b1f;
    b3f = height - b3f;
#endif
    
    // FIX: the bottom miniview border was not displayed
    b3f -= BOTTOM_BORDER_OFFSET;
    
    nvgMoveTo(vg, (mBounds[0] + mState->mMinNormX*boundsSizeX)*width +
              BORDER_OFFSET_WIDTH, b1f);
    nvgLineTo(vg, (mBounds[0] + mState->mMaxNormX*boundsSizeX)*width /*-*/+
              BORDER_OFFSET_WIDTH, b1f);
    nvgLineTo(vg, (mBounds[0] + mState->mMaxNormX*boundsSizeX)*width /*-*/+
              BORDER_OFFSET_WIDTH, b3f);
    nvgLineTo(vg, (mBounds[0] + mState->mMinNormX*boundsSizeX)*width +
              BORDER_OFFSET_WIDTH, b3f);
    nvgLineTo(vg, (mBounds[0] + mState->mMinNormX*boundsSizeX)*width +
              BORDER_OFFSET_WIDTH, b1f);
    
    nvgStroke(vg);
    
    nvgRestore(vg);
}

bool
MiniView2::IsPointInside(int x, int y, int width, int height)
{
    // Warning: y is reversed !
    BL_FLOAT nx = ((BL_FLOAT)x)/width;
    //BL_FLOAT ny = 1.0 - ((BL_FLOAT)y)/height;
    BL_FLOAT ny = ((BL_FLOAT)y)/height;
    
    if (nx < mBounds[0])
        return false;
    
    if (ny < mBounds[1])
        return false;
    
    if (nx > mBounds[2])
        return false;
    
    if (ny > mBounds[3])
        return false;
    
    return true;

}

void
MiniView2::SetSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    if (samples.GetSize() == 0)
        return;
    
    mWaveForm = samples;
    
    BL_FLOAT decFactor = ((BL_FLOAT)mWaveForm.GetSize())/mMaxNumPoints;
    //BLUtils::DecimateSamples(&mWaveForm, (BL_FLOAT)(1.0/decFactor));
    BLUtilsDecim::DecimateSamples3(&mWaveForm, samples, (BL_FLOAT)(1.0/decFactor));
    
    BL_FLOAT maxVal = BLUtils::ComputeMaxAbs(mWaveForm);
    
    BL_FLOAT boundsSize = (mBounds[3] - mBounds[1]);
    BL_FLOAT coeff = boundsSize/maxVal;
    
    // Must divide, because we have positive and negative values (waveform)
    coeff /= 2.0;
    
    // To be sure this doesn't overdraw
    coeff *= 0.8;
    
    BLUtils::MultValues(&mWaveForm, coeff);
    
    //BLUtils::AddValues(&mWaveForm, (BL_FLOAT)(mBounds[3] - boundsSize/2.0));
    BLUtils::AddValues(&mWaveForm, (BL_FLOAT)(1.0 - mBounds[1] - boundsSize/2.0));
}

void
MiniView2::GetWaveForm(WDL_TypedBuf<BL_FLOAT> *waveform)
{
    *waveform = mWaveForm;
}

void
MiniView2::SetBounds(BL_FLOAT minNormX, BL_FLOAT maxNormX)
{
    mState->mMinNormX = minNormX;
    mState->mMaxNormX = maxNormX;
}

BL_FLOAT
MiniView2::GetDrag(int dragX, int width)
{
    BL_FLOAT dx = ((BL_FLOAT)-dragX)/width;
    
    BL_FLOAT res = dx/(mState->mMaxNormX - mState->mMinNormX);
    
    return res;
}

// Note used
//
// Draw black background, this will avoid that the graph big waveform
// is drawn over the miniview
void
MiniView2::DrawBackground(NVGcontext *vg, int width, int height)
{
    nvgSave(vg);
    
    int color[4] = { 255, 0, 255, 255 };   
    SWAP_COLOR(color);
    nvgFillColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    nvgBeginPath(vg);
    
    nvgRect(vg, mBounds[0]*width, mBounds[1]*height,
            1.0*width, //(mBounds[2] - mBounds[0])*width,
            (mBounds[3] - mBounds[1])*height);
            
    nvgFill(vg);
    
    nvgRestore(vg);
}

#endif // IGRAPHICS_NANOVG
