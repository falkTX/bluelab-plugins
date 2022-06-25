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
//  StereoWidthGraphDrawer3
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphSwapColor.h>

#include <GUIHelper12.h>
#undef DrawText

#include "StereoWidthGraphDrawer3.h"

#define FIX_CIRCLE_DRAWER_BOTTOM_LINE 1

//#define OFFSET_X 8.0

#if !FIX_CIRCLE_DRAWER_BOTTOM_LINE
#define OFFSET_Y 2.0
#else
#define OFFSET_Y 1.0
#endif

#define TITLE_POS_X FONT_SIZE*0.5
#define TITLE_POS_Y FONT_SIZE*0.5

StereoWidthGraphDrawer3::StereoWidthGraphDrawer3(GUIHelper12 *guiHelper,
                                                 const char *title)
{
    mTitleSet = false;
    
    if (title != NULL)
    {
        mTitleSet = true;
        
        sprintf(mTitleText, "%s", title);
    }

    // Style
    if (guiHelper == NULL)
    {
        mCircleLineWidth = 2.0;
       
        mLinesColor = IColor(255, 128, 128, 128);
        mTextColor = IColor(255, 128, 128, 128);

        mOffsetX = 8;
        mTitleOffsetY = TITLE_POS_Y;
    }
    else
    {
        guiHelper->GetCircleGDCircleLineWidth(&mCircleLineWidth);
        guiHelper->GetCircleGDLinesColor(&mLinesColor);
        guiHelper->GetCircleGDTextColor(&mTextColor);

        guiHelper->GetCircleGDOffsetX(&mOffsetX);
        guiHelper->GetCircleGDOffsetY(&mTitleOffsetY);
    }
}

void
StereoWidthGraphDrawer3::PreDraw(NVGcontext *vg, int width, int height)
{
    BL_FLOAT circleStrokeWidth = mCircleLineWidth;

    int color[4] = { mLinesColor.R, mLinesColor.G,
                     mLinesColor.B, mLinesColor.A };
    
    int fontColor[4] = { mTextColor.R, mTextColor.G,
                         mTextColor.B, mTextColor.A };

    int fillColor[4] = { 0, 128, 255, 255 };
    
    //
    nvgStrokeWidth(vg, circleStrokeWidth);
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    SWAP_COLOR(fillColor);
    nvgFillColor(vg, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
    
    // Draw the circle
    BL_FLOAT yf = 0.0;
    BL_FLOAT yf2 = OFFSET_Y;
#if GRAPH_CONTROL_FLIP_Y
    yf = height - yf;
    yf2 = height - yf2;
#endif

#define OFFSET_X mOffsetX
    
    nvgBeginPath(vg);
    nvgCircle(vg, width/2.0, yf, width/2.0 - OFFSET_X);
        
    // Draw line
    nvgMoveTo(vg, OFFSET_X, yf2);
    nvgLineTo(vg, width - OFFSET_X, yf2);
    
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
#define RADIUS (width*0.5 - OFFSET_X)
#define COS_PI4 0.707106781186548
#define ADJUST_OFFSET 1
    
    // Draw the texts
    SWAP_COLOR(fontColor); // ?
    
    //#define TEXT_OFFSET_X 6.0
#define TEXT_OFFSET_X mOffsetX
#define TEXT_OFFSET_Y 20.0
    
    char *leftText = "LEFT";
    char *rightText = "RIGHT";
    BL_FLOAT radiusRatio = 1.0; //1;
    int halignL = NVG_ALIGN_RIGHT;
    int halignR = NVG_ALIGN_LEFT;
    
    // Left
    GraphControl12::DrawText(vg,
                             (1.0 - COS_PI4)*RADIUS*radiusRatio + OFFSET_X - TEXT_OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, leftText, fontColor,
                             halignL, NVG_ALIGN_MIDDLE /*NVG_ALIGN_BOTTOM*/);
    
    // Right
    GraphControl12::DrawText(vg,
                             (1.0 + COS_PI4)*RADIUS*radiusRatio +
                             OFFSET_X + TEXT_OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, rightText, fontColor,
                             halignR, NVG_ALIGN_MIDDLE/*NVG_ALIGN_BOTTOM*/);
    
    if (mTitleSet)
    {
        GraphControl12::DrawText(vg,
                                 //TITLE_POS_X,
                                 TEXT_OFFSET_X,
                                 //height - TITLE_POS_Y,
                                 height - mTitleOffsetY,
                                 width, height,
                                 FONT_SIZE, mTitleText, fontColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_TOP /*NVG_ALIGN_BOTTOM*/);
    }
}

#endif // IGRAPHICS_NANOVG

