//
//  BLLissajousGraphDrawer
//  UST
//
//  Created by Pan on 24/04/18.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include "BLLissajousGraphDrawer.h"

#define TITLE_POS_X FONT_SIZE*0.5
#define TITLE_POS_Y FONT_SIZE*0.5

BLLissajousGraphDrawer::BLLissajousGraphDrawer(BL_FLOAT scale, const char *title)
{
    mScale = scale;
    
    mTitleSet = false;
    
    if (title != NULL)
    {
        mTitleSet = true;
        
        sprintf(mTitleText, "%s", title);
    }
}

BLLissajousGraphDrawer::~BLLissajousGraphDrawer() {}

void
BLLissajousGraphDrawer::PreDraw(NVGcontext *vg, int width, int height)
{
    BL_FLOAT strokeWidthOut = 2.0;
    BL_FLOAT strokeWidthIn = 1.0;
    
    int color[4] = { 128, 128, 128, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };

    
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

    BL_FLOAT c01Yf = corners[0][1];
#if GRAPH_CONTROL_FLIP_Y
    c01Yf = height - c01Yf;
#endif
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, corners[0][0], c01Yf);
    for (int i = 0; i < 4; i++)
    {
        BL_FLOAT ci1Yf = corners[i][1];
#if GRAPH_CONTROL_FLIP_Y
        ci1Yf = height - ci1Yf;
#endif
        
        nvgLineTo(vg, corners[i][0], ci1Yf);
    }
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    // Draw cross
    nvgStrokeWidth(vg, strokeWidthIn);
    
    BL_FLOAT cc01Yf = crossCorners[0][1];
    BL_FLOAT cc21Yf = crossCorners[2][1];
    BL_FLOAT cc11Yf = crossCorners[1][1];
    BL_FLOAT cc31Yf = crossCorners[3][1];
#if GRAPH_CONTROL_FLIP_Y
    cc01Yf = height - cc01Yf;
    cc21Yf = height - cc21Yf;
    cc11Yf = height - cc11Yf;
    cc31Yf = height - cc31Yf;
#endif
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, crossCorners[0][0], cc01Yf);
    nvgLineTo(vg, crossCorners[2][0], cc21Yf);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, crossCorners[1][0], cc11Yf);
    nvgLineTo(vg, crossCorners[3][0], cc31Yf);
    nvgStroke(vg);
    
    // Draw texts
    SWAP_COLOR(fontColor); // ?
    
#define TEXT_OFFSET_X 6.0
#define TEXT_OFFSET_Y 6.0 //2.0
    
    char *leftText = "LEFT";
    char *rightText = "RIGHT";
    
    BL_FLOAT textY = height/2.0 - FONT_SIZE/2.0 - FONT_SIZE/4.0;
    
    // Left
    GraphControl11::DrawText(vg, FONT_SIZE + TEXT_OFFSET_X,
                             textY + FONT_SIZE/2 + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, leftText, fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE);
    
    // Right
    GraphControl11::DrawText(vg, width - FONT_SIZE - TEXT_OFFSET_X,
                             textY + FONT_SIZE/2 + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, rightText, fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE);
    
    if (mTitleSet)
    {
        GraphControl11::DrawText(vg,
                                 TITLE_POS_X, height - TITLE_POS_Y,
                                 width, height,
                                 FONT_SIZE, mTitleText, fontColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_TOP);
    }
}


