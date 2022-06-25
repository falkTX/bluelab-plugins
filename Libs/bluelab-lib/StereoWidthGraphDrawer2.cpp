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
//  StereoWidthGraphDrawer.cpp
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphSwapColor.h>

#include "StereoWidthGraphDrawer2.h"

#define OFFSET_X 6.0 //2.0
#define OFFSET_Y 2.0

void
StereoWidthGraphDrawer2::PostDraw(NVGcontext *vg, int width, int height)
{
    BL_FLOAT strokeWidth = 2.0; //3.0;
    
    int color[4] = { 64, 64, 64, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
    
    nvgStrokeWidth(vg, strokeWidth);
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    SWAP_COLOR(fillColor);
    nvgFillColor(vg, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
    
    // Draw the circle
    BL_FLOAT yf = 0.0;
#if GRAPH_CONTROL_FLIP_Y
    yf = height - yf;
#endif
    
    nvgBeginPath(vg);
    nvgCircle(vg, width/2.0, yf, width/2.0 - OFFSET_X);
        
    // Draw the line
    nvgMoveTo(vg, OFFSET_X, yf);
    nvgLineTo(vg, width - OFFSET_X, yf);
    
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    
    // Draw the texts
    
    SWAP_COLOR(fontColor); // ?
    
    // Left
    GraphControl12::DrawText(vg,
                             FONT_SIZE + OFFSET_X, FONT_SIZE/2 + OFFSET_Y,
                             width, height,
                             FONT_SIZE, "L", fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE /*NVG_ALIGN_BOTTOM*/);
    
    // Right
    GraphControl12::DrawText(vg,
                             width - FONT_SIZE - OFFSET_X, FONT_SIZE/2 + OFFSET_Y,
                             width, height,
                             FONT_SIZE, "R", fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE/*NVG_ALIGN_BOTTOM*/);
}

#endif

