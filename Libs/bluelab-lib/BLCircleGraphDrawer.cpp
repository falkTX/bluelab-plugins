//
//  BLCircleGraphDrawer
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphSwapColor.h>

#include <GUIHelper12.h>
#undef DrawText

#include "BLCircleGraphDrawer.h"

#define FIX_CIRCLE_DRAWER_BOTTOM_LINE 1

//#define OFFSET_X 8.0

#if !FIX_CIRCLE_DRAWER_BOTTOM_LINE
#define OFFSET_Y 2.0
#else
#define OFFSET_Y 1.0
#endif

//#define TITLE_POS_X FONT_SIZE*0.5
#define TITLE_POS_Y FONT_SIZE*0.5


BLCircleGraphDrawer::BLCircleGraphDrawer(GUIHelper12 *guiHelper,
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
        mLinesWidth = 1.0;
        
        mLinesColor = IColor(255, 128, 128, 128);
        mTextColor = IColor(255, 128, 128, 128);

        mOffsetX = 8;
        mTitleOffsetY = TITLE_POS_Y;
    }
    else
    {
        guiHelper->GetCircleGDCircleLineWidth(&mCircleLineWidth);
        guiHelper->GetCircleGDLinesWidth(&mLinesWidth);
        guiHelper->GetCircleGDLinesColor(&mLinesColor);
        guiHelper->GetCircleGDTextColor(&mTextColor);

        guiHelper->GetCircleGDOffsetX(&mOffsetX);
        guiHelper->GetCircleGDOffsetY(&mTitleOffsetY);
    }
}

void
BLCircleGraphDrawer::PreDraw(NVGcontext *vg, int width, int height)
{
    /*BL_FLOAT circleStrokeWidth = 2.0; //3.0;
    BL_FLOAT linesStrokeWidth = 1.0;
    
    int color[4] = { 128, 128, 128, 255 };
    int fontColor[4] = { 128, 128, 128, 255 };
    */
    
    int fillColor[4] = { 0, 128, 255, 255 };
    
    nvgStrokeWidth(vg, mCircleLineWidth);

    int color[4] = { mLinesColor.R, mLinesColor.G,
                     mLinesColor.B, mLinesColor.A };
    SWAP_COLOR(color);

#define OFFSET_X mOffsetX
    
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    SWAP_COLOR(fillColor);
    nvgFillColor(vg, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
    
    // Draw the circle
    nvgBeginPath(vg);
    
#if !GRAPH_CONTROL_FLIP_Y
    nvgCircle(vg, width/2.0, 0.0, width/2.0 - OFFSET_X);
#else
    nvgCircle(vg, width/2.0, height - OFFSET_Y, width/2.0 - OFFSET_X);
#endif
    
    // Draw line
#if !GRAPH_CONTROL_FLIP_Y
    nvgMoveTo(vg, OFFSET_X, OFFSET_Y);
    nvgLineTo(vg, width - OFFSET_X, OFFSET_Y);
#else
    nvgMoveTo(vg, OFFSET_X, height - OFFSET_Y);
    nvgLineTo(vg, width - OFFSET_X, height - OFFSET_Y);
#endif
    
    nvgClosePath(vg);
    
    nvgStroke(vg);
    
    // TODO: check the location of the 2 lines (accurate ?),
    // with full panned sounds
    
#define RADIUS (width*0.5 - OFFSET_X)
#define COS_PI4 0.707106781186548
#define ADJUST_OFFSET 1
    
    nvgStrokeWidth(vg, mLinesWidth);

    BL_FLOAT offsetYf = OFFSET_Y;
    //BL_FLOAT lYf = COS_PI4*RADIUS - ADJUST_OFFSET + OFFSET_Y;
    BL_FLOAT lYf = COS_PI4*(RADIUS + mCircleLineWidth*0.5) - ADJUST_OFFSET + OFFSET_Y;
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
    int fontColor[4] = { mTextColor.R, mTextColor.G,
                         mTextColor.B, mTextColor.A };
    SWAP_COLOR(fontColor);
    
    //
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
                             (1.0 - COS_PI4)*RADIUS*radiusRatio +
                             //OFFSET_X - TEXT_OFFSET_X,
                             OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, leftText, fontColor,
                             halignL, NVG_ALIGN_MIDDLE);
    
    // Right
    GraphControl12::DrawText(vg,
                             (1.0 + COS_PI4)*RADIUS*radiusRatio +
                             //OFFSET_X + TEXT_OFFSET_X,
                             OFFSET_X,
                             COS_PI4*RADIUS*radiusRatio + TEXT_OFFSET_Y,
                             width, height,
                             FONT_SIZE, rightText, fontColor,
                             halignR, NVG_ALIGN_MIDDLE);
    
    if (mTitleSet)
    {
        GraphControl12::DrawText(vg,
                                 //TITLE_POS_X,
                                 TEXT_OFFSET_X,
                                 //height - TITLE_POS_Y,
                                 height - mTitleOffsetY,
                                 width, height,
                                 FONT_SIZE, mTitleText, fontColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_TOP);
    }
}

#endif // IGRAPHICS_NANOVG

