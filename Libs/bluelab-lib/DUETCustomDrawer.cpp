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
//  DUETCustomDrawer.cpp
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifdef IGRAPHICS_NANOVG

//#include <DUET.h>
#include <GraphControl12.h>
#include <GraphSwapColor.h>

#include "DUETCustomDrawer.h"

DUETCustomDrawer::DUETCustomDrawer()
{    
    Reset();
}

void
DUETCustomDrawer::Reset()
{
    mPickCursorActive = false;
    
    mPickCursorX = 0;
    mPickCursorY = 0;
}

void
DUETCustomDrawer::PostDraw(NVGcontext *vg, int width, int height)
{
#define CURSOR_SIZE 8.0
    
    if (!mPickCursorActive)
        return;
    
    BL_FLOAT strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    BL_FLOAT x = mPickCursorX*width;
    BL_FLOAT y = (1.0 - mPickCursorY)*height;
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1], colors[i][2], colors[i][3]));
        
        BL_FLOAT yf = y;
        BL_FLOAT yfm = y - CURSOR_SIZE;
        BL_FLOAT yfp = y + CURSOR_SIZE;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
        yfm = height - yfm;
        yfp = height - yfp;
#endif
        
        // Draw the circle
        nvgBeginPath(vg);
        
        nvgMoveTo(vg, x - CURSOR_SIZE, yf);
        nvgLineTo(vg, x + CURSOR_SIZE, yf);
        
        nvgMoveTo(vg, x, yfm);
        nvgLineTo(vg, x, yfp);
        
        nvgStroke(vg);
    }
}

void
DUETCustomDrawer::SetPickCursorActive(bool flag)
{
    mPickCursorActive = flag;
}

void
DUETCustomDrawer::SetPickCursor(BL_FLOAT x, BL_FLOAT y)
{
    mPickCursorX = x;
    mPickCursorY = y;
}

#endif // IGRAPHICS_NANOVG
