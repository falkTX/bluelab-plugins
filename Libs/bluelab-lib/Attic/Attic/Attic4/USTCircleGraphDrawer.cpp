//
//  USTCircleGraphDrawer
//  UST
//
//  Created by Pan on 24/04/18.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include "USTCircleGraphDrawer.h"

// Fix
#define FIX_CIRCLE_DRAWER_BOTTOM_LINE 1

#define OFFSET_X 8.0 //6.0 //2.0

#if !FIX_CIRCLE_DRAWER_BOTTOM_LINE
#define OFFSET_Y 2.0
#else
#define OFFSET_Y 1.0
#endif

#define MALIK_CIRCLE 1

void
USTCircleGraphDrawer::PreDraw(NVGcontext *vg, int width, int height)
{
#if !MALIK_CIRCLE
    BL_FLOAT circleStrokeWidth = 2.0; //3.0;
    BL_FLOAT linesStrokeWidth = 2.0;
    
    int color[4] = { 64, 64, 64, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
#else
    BL_FLOAT circleStrokeWidth = 2.0; //3.0;
    BL_FLOAT linesStrokeWidth = 1.0;
    
    int color[4] = { 128, 128, 128, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
#endif
    
    nvgStrokeWidth(vg, circleStrokeWidth);
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    SWAP_COLOR(fillColor);
    nvgFillColor(vg, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
    
    // Draw the circle
    BL_FLOAT y0 = 0.0;
    BL_FLOAT yOff = OFFSET_Y;
    BL_FLOAT a0 = 0.0;
    BL_FLOAT a1 = M_PI;
#if GRAPH_CONTROL_FLIP_Y
    y0 = height - y0;
    yOff = height - yOff;
    
    a0 = - a0;
    a1 = - a1;
    
    BL_FLOAT tmp = a0;
    a0 = a1;
    a1 = tmp;
#endif
    
    nvgBeginPath(vg);
    //nvgCircle(vg, width/2.0, y0, width/2.0 - OFFSET_X);
    nvgArc(vg, width/2.0, y0, width/2.0 - OFFSET_X, a0, a1, NVG_CW);
    
    // Draw line
    nvgMoveTo(vg, OFFSET_X, yOff);
    nvgLineTo(vg, width - OFFSET_X, yOff);
    
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    // TODO: check the location of the 2 lines (accurate ?),
    // with full panned sounds
    
#define RADIUS (width*0.5 - OFFSET_X)
#define COS_PI4 0.707106781186548
#define ADJUST_OFFSET 1
    
    nvgStrokeWidth(vg, linesStrokeWidth);
    
    // Draw the left line
    BL_FLOAT yf = COS_PI4*RADIUS - ADJUST_OFFSET + OFFSET_Y;
#if GRAPH_CONTROL_FLIP_Y
    yf = height - yf;
#endif
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, 0.5*width, yOff);
    nvgLineTo(vg, (1.0 - COS_PI4)*RADIUS + OFFSET_X + ADJUST_OFFSET, yf);
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    
    // Draw the right line
    nvgBeginPath(vg);
    nvgMoveTo(vg, 0.5*width, yOff);
    nvgLineTo(vg, (1.0 + COS_PI4)*RADIUS + OFFSET_X -/*+*/ ADJUST_OFFSET, yf);
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    
    // Draw the texts
    SWAP_COLOR(fontColor); // ?
    
#if !MALIK_CIRCLE
    
#define TEXT_OFFSET_X 6.0
#define TEXT_OFFSET_Y 6.0
    
    char *leftText = "L";
    char *rightText = "R";
    BL_FLOAT radiusRatio = 1.0;
    int halignL = NVG_ALIGN_CENTER;
    int halignR = NVG_ALIGN_CENTER;
#else
    
#define TEXT_OFFSET_X 6.0
#define TEXT_OFFSET_Y 20.0
    
    char *leftText = "LEFT";
    char *rightText = "RIGHT";
    BL_FLOAT radiusRatio = 1.0; //1;
    int halignL = NVG_ALIGN_RIGHT;
    int halignR = NVG_ALIGN_LEFT;
#endif

    
    // Left
    GraphControl11::DrawText(vg,
                             (1.0 - COS_PI4)*RADIUS*radiusRatio + OFFSET_X - TEXT_OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, leftText, fontColor,
                             halignL, NVG_ALIGN_MIDDLE /*NVG_ALIGN_BOTTOM*/);
    
    // Right
    GraphControl11::DrawText(vg,
                             (1.0 + COS_PI4)*RADIUS*radiusRatio + OFFSET_X + TEXT_OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, rightText, fontColor,
                             halignR, NVG_ALIGN_MIDDLE/*NVG_ALIGN_BOTTOM*/);
    
#if 0
    // Left
    GraphControl11::DrawText(vg,
                             FONT_SIZE + OFFSET_X, FONT_SIZE/2 + OFFSET_Y,
                             width, height,
                             FONT_SIZE, "L", fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE /*NVG_ALIGN_BOTTOM*/);
    
    // Right
    GraphControl11::DrawText(vg,
                             width - FONT_SIZE - OFFSET_X, FONT_SIZE/2 + OFFSET_Y,
                             width, height,
                             FONT_SIZE, "R", fontColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE/*NVG_ALIGN_BOTTOM*/);
#endif
    
#if 0 // TEST
    nvgBeginPath(vg);
    
    BL_FLOAT rad2 = width/8.0;
    BL_FLOAT corners2[4][2] =
    { { -rad2/2 + width/2, -rad2/2 + height/2 },
      { rad2/2 + width/2, -rad2/2 + height/2 },
      //{ rad2/2 + width/2, rad2/2 + height/2 },
      { -rad2/2 + width/2 + 10, -rad2/2 + height/2 + 10 }, //
      { -rad2/2 + width/2, rad2/2 + height/2 } };
#if 1 // Ok
    nvgMoveTo(vg, corners2[0][0], corners2[0][1]);
    nvgLineTo(vg, corners2[1][0], corners2[1][1]);
    nvgLineTo(vg, corners2[2][0], corners2[2][1]);
    nvgLineTo(vg, corners2[3][0], corners2[3][1]);
#else // Ok too
    nvgMoveTo(vg, corners2[3][0], corners2[3][1]);
    nvgLineTo(vg, corners2[2][0], corners2[2][1]);
    nvgLineTo(vg, corners2[1][0], corners2[1][1]);
    nvgLineTo(vg, corners2[0][0], corners2[0][1]);
#endif
    
    nvgClosePath(vg);
    
    //nvgStroke(vg);
    
    nvgFill(vg);
#endif
}


