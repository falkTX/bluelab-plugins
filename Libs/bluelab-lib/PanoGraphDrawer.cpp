//
//  PanoGraphDrawer.cpp
//  BL-Pano
//
//  Created by applematuer on 8/9/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphSwapColor.h>

#include "PanoGraphDrawer.h"


PanoGraphDrawer::PanoGraphDrawer() {}

PanoGraphDrawer::~PanoGraphDrawer() {}

void
PanoGraphDrawer::PostDraw(NVGcontext *vg, int width, int height)
{
    BL_FLOAT strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1], colors[i][2], colors[i][3]));
        
        // Draw the circle
        nvgBeginPath(vg);
        
        // Draw the line
        BL_FLOAT y = 0.5;
        
        BL_FLOAT yf = y*height;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        nvgMoveTo(vg, 0.0*width, yf);
        nvgLineTo(vg, 1.0*width, yf);
        
        nvgStroke(vg);
    }
}

#endif // IGRAPHICS_NANOVG
