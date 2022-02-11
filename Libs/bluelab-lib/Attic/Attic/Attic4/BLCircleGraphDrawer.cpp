//
//  BLCircleGraphDrawer
//  UST
//
//  Created by Pan on 24/04/18.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include "BLCircleGraphDrawer.h"

#define FIX_CIRCLE_DRAWER_BOTTOM_LINE 1

#define OFFSET_X 8.0

#if !FIX_CIRCLE_DRAWER_BOTTOM_LINE
#define OFFSET_Y 2.0
#else
#define OFFSET_Y 1.0
#endif

#define TITLE_POS_X FONT_SIZE*0.5
#define TITLE_POS_Y FONT_SIZE*0.5


BLCircleGraphDrawer::BLCircleGraphDrawer(const char *title)
{
    mTitleSet = false;
    
    if (title != NULL)
    {
        mTitleSet = true;
        
        sprintf(mTitleText, "%s", title);
    }
}

void
BLCircleGraphDrawer::PreDraw(NVGcontext *vg, int width, int height)
{
    BL_FLOAT circleStrokeWidth = 2.0; //3.0;
    BL_FLOAT linesStrokeWidth = 1.0;
    
    int color[4] = { 128, 128, 128, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
    
    nvgStrokeWidth(vg, circleStrokeWidth);
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    SWAP_COLOR(fillColor);
    nvgFillColor(vg, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
    
    // Draw the circle
    nvgBeginPath(vg);
    nvgCircle(vg, width/2.0, 0.0, width/2.0 - OFFSET_X);
        
    // Draw line
    nvgMoveTo(vg, OFFSET_X, OFFSET_Y);
    nvgLineTo(vg, width - OFFSET_X, OFFSET_Y);
    
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    // TODO: check the location of the 2 lines (accurate ?),
    // with full panned sounds
    
#define RADIUS (width*0.5 - OFFSET_X)
#define COS_PI4 0.707106781186548
#define ADJUST_OFFSET 1
    
    nvgStrokeWidth(vg, linesStrokeWidth);

    BL_FLOAT offsetYf = OFFSET_Y;
    BL_FLOAT lYf = COS_PI4*RADIUS - ADJUST_OFFSET + OFFSET_Y;
#if GRAPH_CONTROL_FLIP_Y
    offsetYf = height - offsetYf;
    lYf = height - lYf;
#endif
    
    // Draw the left line
    nvgBeginPath(vg);
    nvgMoveTo(vg, 0.5*width, offsetYf);
    nvgLineTo(vg, (1.0 - COS_PI4)*RADIUS + OFFSET_X + ADJUST_OFFSET, lYf);
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    
    // Draw the right line
    nvgBeginPath(vg);
    nvgMoveTo(vg, 0.5*width, offsetYf);
    nvgLineTo(vg, (1.0 + COS_PI4)*RADIUS + OFFSET_X - ADJUST_OFFSET, lYf);
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    
    // Draw the texts
    SWAP_COLOR(fontColor);
    
    //
#define TEXT_OFFSET_X 6.0
#define TEXT_OFFSET_Y 20.0
    
    char *leftText = "LEFT";
    char *rightText = "RIGHT";
    BL_FLOAT radiusRatio = 1.0; //1;
    int halignL = NVG_ALIGN_RIGHT;
    int halignR = NVG_ALIGN_LEFT;

    
    // Left
    GraphControl11::DrawText(vg,
                             (1.0 - COS_PI4)*RADIUS*radiusRatio + OFFSET_X - TEXT_OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, leftText, fontColor,
                             halignL, NVG_ALIGN_MIDDLE);
    
    // Right
    GraphControl11::DrawText(vg,
                             (1.0 + COS_PI4)*RADIUS*radiusRatio + OFFSET_X + TEXT_OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, rightText, fontColor,
                             halignR, NVG_ALIGN_MIDDLE);
    
    if (mTitleSet)
    {
        GraphControl11::DrawText(vg,
                                 TITLE_POS_X, height - TITLE_POS_Y,
                                 width, height,
                                 FONT_SIZE, mTitleText, fontColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_TOP);
    }
}


