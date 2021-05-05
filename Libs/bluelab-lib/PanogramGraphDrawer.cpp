//
//  PanogramGraphDrawer.cpp
//  BL-Pano
//
//  Created by applematuer on 8/9/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphSwapColor.h>

#include "PanogramGraphDrawer.h"


PanogramGraphDrawer::PanogramGraphDrawer()
{
    mViewOrientation = HORIZONTAL;
}

PanogramGraphDrawer::~PanogramGraphDrawer() {}

void
PanogramGraphDrawer::PostDraw(NVGcontext *vg, int width, int height)
{
    //BL_FLOAT strokeWidths[2] = { 3.0, 2.0 };
    BL_FLOAT strokeWidths[2] = { 2.0, 1.0 };
    
    // Two colors, for drawing two times, for overlay
    //int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    int colors[2][4] = { { 64, 64, 64, 255 }, { 128, 128, 128, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1],
                                   colors[i][2], colors[i][3]));
        
        nvgBeginPath(vg);

        if (mViewOrientation == HORIZONTAL)
        {
            // Draw the line
            BL_FLOAT y = 0.5;
            
            BL_FLOAT yf = y*height;
#if GRAPH_CONTROL_FLIP_Y
            yf = height - yf;
#endif
            
            nvgMoveTo(vg, 0.0*width, yf);
            nvgLineTo(vg, 1.0*width, yf);
        }
        else
        {
            // Draw the line
            BL_FLOAT x = 0.5;
            
            BL_FLOAT xf = x*width;
            
            nvgMoveTo(vg, xf, 0.0*height);
            nvgLineTo(vg, xf, 1.0*height);
        }
        
        nvgStroke(vg);
    }
}

void
PanogramGraphDrawer::SetViewOrientation(ViewOrientation orientation)
{
    mViewOrientation = orientation;
}


#endif // IGRAPHICS_NANOVG
