//
//  USTLissajousGraphDrawer
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphSwapColor.h>

#include "USTLissajousGraphDrawer.h"

#define MALIK_CIRCLE 1

USTLissajousGraphDrawer::USTLissajousGraphDrawer(BL_FLOAT scale)
{
    mScale = scale;
}

USTLissajousGraphDrawer::~USTLissajousGraphDrawer() {}

void
USTLissajousGraphDrawer::PreDraw(NVGcontext *vg, int width, int height)
{
#if !MALIK_CIRCLE
    BL_FLOAT strokeWidthOut = 2.0; //3.0;
    BL_FLOAT strokeWidthIn = 2.0;
    
    int color[4] = { 64, 64, 64, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
#else
    BL_FLOAT strokeWidthOut = 2.0; //3.0;
    BL_FLOAT strokeWidthIn = 1.0;
    
    int color[4] = { 128, 128, 128, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
#endif
    
    //nvgStrokeWidth(vg, strokeWidth);
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    SWAP_COLOR(fillColor);
    nvgFillColor(vg, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
    
    // Corners
    BL_FLOAT corners[4][2] = { { 0.0, 1.0 }, { 1.0, 0.0 }, { 0.0, -1.0 }, { -1.0, 0.0} };
    
    // Cross corners
    BL_FLOAT crossCorners[4][2];
    crossCorners[0][0] = (corners[0][0] + corners[1][0])*0.5;
    crossCorners[0][1] = (corners[0][1] + corners[1][1])*0.5;
    
    crossCorners[1][0] = (corners[1][0] + corners[2][0])*0.5;
    crossCorners[1][1] = (corners[1][1] + corners[2][1])*0.5;
    
    crossCorners[2][0] = (corners[2][0] + corners[3][0])*0.5;
    crossCorners[2][1] = (corners[2][1] + corners[3][1])*0.5;
    
    crossCorners[3][0] = (corners[3][0] + corners[0][0])*0.5;
    crossCorners[3][1] = (corners[3][1] + corners[0][1])*0.5;
    
    // Scale corners to mScale
    for (int i = 0; i < 4; i++)
    {
        corners[i][0] *= mScale;
        corners[i][1] *= mScale;
    }
    
    for (int i = 0; i < 4; i++)
    {
        crossCorners[i][0] *= mScale;
        crossCorners[i][1] *= mScale;
    }
    
    
    // Scale corners to graph size
    for (int i = 0; i < 4; i++)
    {
        corners[i][0] = (corners[i][0] + 1.0)*0.5*height; //width;
        corners[i][1] = (corners[i][1] + 1.0)*0.5*height;
    }
    
    // Scale cross corners to graph size
    for (int i = 0; i < 4; i++)
    {
        crossCorners[i][0] = (crossCorners[i][0] + 1.0)*0.5*height; //width;
        crossCorners[i][1] = (crossCorners[i][1] + 1.0)*0.5*height;
    }
    
    // Offset in order to center
    BL_FLOAT offsetPixels = (width - height)/2.0;
    for (int i = 0; i < 4; i++)
    {
        corners[i][0] += offsetPixels;
        crossCorners[i][0] += offsetPixels;
    }
    
    // Draw square
    nvgStrokeWidth(vg, strokeWidthOut);
    
    // Draw the circle
    BL_FLOAT c0f = corners[0][1];
#if GRAPH_CONTROL_FLIP_Y
    c0f = height - c0f;
#endif
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, corners[0][0], c0f);
    for (int i = 0; i < 4; i++)
    {
        BL_FLOAT cf = corners[i][1];
#if GRAPH_CONTROL_FLIP_Y
        cf = height - cf;
#endif
        nvgLineTo(vg, corners[i][0], cf);
    }
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    // Draw cross
    BL_FLOAT cc0f = crossCorners[0][1];
    BL_FLOAT cc1f = crossCorners[1][1];
    BL_FLOAT cc2f = crossCorners[2][1];
    BL_FLOAT cc3f = crossCorners[3][1];
#if GRAPH_CONTROL_FLIP_Y
    cc0f = height - cc0f;
    cc1f = height - cc1f;
    cc2f = height - cc2f;
    cc3f = height - cc3f;
#endif
    
    nvgStrokeWidth(vg, strokeWidthIn);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, crossCorners[0][0], cc0f);
    nvgLineTo(vg, crossCorners[2][0], cc2f);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, crossCorners[1][0], cc1f);
    nvgLineTo(vg, crossCorners[3][0], cc3f);
    nvgStroke(vg);
    
    
    // Draw texts
    SWAP_COLOR(fontColor); // ?
    
#if 1
    
#define TEXT_OFFSET_X 6.0
#define TEXT_OFFSET_Y 6.0 //2.0
    
    
#if !MALIK_CIRCLE
    char *leftText = "L";
    char *rightText = "R";
    
    BL_FLOAT textY = 0.0;
    
#else
    char *leftText = "LEFT";
    char *rightText = "RIGHT";
    
    BL_FLOAT textY = height/2.0 - FONT_SIZE/2.0 - FONT_SIZE/4.0;
#endif
    
    // Left
    GraphControl12::DrawText(vg, FONT_SIZE + TEXT_OFFSET_X,
                             textY + FONT_SIZE/2 + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, leftText, fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE);
    
    // Right
    GraphControl12::DrawText(vg, width - FONT_SIZE - TEXT_OFFSET_X,
                             textY + FONT_SIZE/2 + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, rightText, fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE);
#endif
}

#endif // IGRAPHICS_NANOVG
