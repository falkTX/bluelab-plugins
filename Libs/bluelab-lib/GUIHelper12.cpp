//
//  GUIHelper12.cpp
//  UST-macOS
//
//  Created by applematuer on 9/25/20.
//
//

#include <GraphControl12.h>

#include <BLUtils.h>
#include <BLUtilsPlug.h>

#include <IBitmapControlAnim.h>

#include <IHelpButtonControl.h>
#include <IHelpButtonControl2.h>

#include <IRadioButtonsControl.h>
#include <IGUIResizeButtonControl.h>

#include <BLVumeterControl.h>
#include <BLVumeterNeedleControl.h>
#include <BLVumeter2SidesControl.h>
#include <ResizeGUIPluginInterface.h>
#include <ITextButtonControl.h>
#include <IXYPadControl.h>
#include <IBLSwitchControl.h>
#include <ITabsBarControl.h>
#include <IBLTooltipControl.h>
#include <ISpatializerHandleControl.h>
#include <IRadioButtonsControlCustom.h>

#include "GUIHelper12.h"

// Available fonts:
// - "font-regular"
// - "font-light"
// - "font-bold"

// Demo mode
#if DEMO_VERSION
#define DEMO_MODE 1
#endif

#ifndef DEMO_MODE
#define DEMO_MODE 0
#endif

#define DEMO_MESSAGE "[DEMO] Please consider buying if you like it!"

// FIX: when plugin with a graph is resized, the background bitmap is not resized
// and the extents are displayed in dark grey
// (detected with Ghost)
#define FIX_RESIZE_GRAPH 1

// Use a fixed height, so that plugs with "g" e.g
// will be centered like plugs with only small letters
// (plug name bitmap height can change depending on the letters of the plug name,
// some letters like g ar taller than other, like a
#define BLUELAB_V3_PLUG_NAME_HEIGHT 14


GUIHelper12::GUIHelper12(Style style)
{
    mStyle = style;
    
    if (style == STYLE_UST)
    {
        mCreateTitles = false;

        mCreatePlugName = false;
        mCreateLogo = false;
        mCreateHelpButton = false;
    
        mTitleTextSize = 13.0;
        mTitleTextOffsetX = 0.0;
        mTitleTextOffsetY = -18.0;
        mTitleTextColor = IColor(255, 110, 110, 110);
        mTitleFont = "font-bold";
        
        mValueCaptionOffset = 0.0;
        mValueTextSize = 14.0;
        mValueTextOffsetX = -1.0;
        mValueTextOffsetY = 1.0;
        mValueTextColor = IColor(255, 240, 240, 255);
        mValueTextFGColor = mValueTextColor;
        mValueTextBGColor = IColor(0, 0, 0, 0);
        mValueTextFont = "font-bold";

        mValueTextColorLight = IColor(255, 209, 216, 223);
        
        mVersionPosition = BOTTOM;
        
        mVersionTextSize = 12.0;
        mVersionTextOffsetX = 100.0;
        mVersionTextOffsetY = 3.0;
        mVersionTextColor = IColor(255, 110 , 110, 110);
        mVersionTextFont = "font-regular";
        
        mVumeterColor = IColor(255, 131, 152, 214);
        mVumeterNeedleColor = IColor(255, 237, 120, 31);
        mVumeterNeedleDepth = 2.0;

        mGraphSeparatorColor = IColor(255, 24, 24, 24);

        mCreateRadioLabels = true;
    }
    
    if (style == STYLE_BLUELAB)
    {
        mCreateTitles = true;

        mCreatePlugName = true;
        mCreateLogo = true;
        mCreateHelpButton = true;
        
        mTitleTextSize = 16.0;
        mTitleTextOffsetX = 0.0;
        mTitleTextOffsetY = -23.0;
        mTitleTextColor = IColor(255, 110, 110, 110);
        
        mHilightTextColor = IColor(255, 248, 248, 248);
        
        mTitleTextSizeBig = 20.0;
        mTitleTextOffsetXBig = 0.0;
        mTitleTextOffsetYBig = -32.0;
        
        mDefaultTitleSize = SIZE_SMALL;
        mTitleFont = "font-bold";
        
        mValueCaptionOffset = 0.0;
        mValueTextSize = 16.0;
        mValueTextOffsetX = -1.0;
        mValueTextOffsetY = 14.0;
        mValueTextColor = IColor(255, 240, 240, 255);
        mValueTextFGColor = mValueTextColor;
        mValueTextBGColor = IColor(0, 0, 0, 0);
        //mValueTextFont = "font-regular";
        mValueTextFont = "font-bold";

        mValueTextColorLight = IColor(255, 209, 216, 223);
        
        mVersionPosition = BOTTOM;
        
        mVersionTextSize = 12.0;
        mVersionTextOffsetX = 100.0;
        mVersionTextOffsetY = 3.0;
        mVersionTextColor = IColor(255, 110 , 110, 110);
        mVersionTextFont = "font-regular";
        
        mVumeterColor = IColor(255, 131 , 152, 214);
        mVumeterNeedleColor = IColor(255, 237, 120, 31);
        mVumeterNeedleDepth = 2.0;
        
        mLogoOffsetX = 0.0;
        mLogoOffsetY = -1.0;
        mAnimLogoSpeed = 0.5;
        
        mPlugNameOffsetX = 5.0;
        mPlugNameOffsetY = 6.0;
        
        mTrialOffsetX = 0.0;
        mTrialOffsetY = 7.0;
        
        mHelpButtonOffsetX = -44.0;
        mHelpButtonOffsetY = -4.0;
        
        mDemoTextSize = 10.0;
        mDemoTextOffsetX = 2.0;
        mDemoTextOffsetY = 2.0;
        mDemoTextColor = IColor(255, 200, 0, 0);
        mDemoFont = "font-regular";
        
        mWatermarkTextSize = 10.0;
        mWatermarkTextOffsetX = 2.0;
        mWatermarkTextOffsetY = 2.0;
        mWatermarkTextColor = IColor(255, 0, 128, 255);
        mWatermarkFont = "font-regular";

        mCreateRadioLabels = true;
        mRadioLabelTextSize = 15;
        mRadioLabelTextOffsetX = 6.0;
        mRadioLabelTextColor = IColor(255, 100, 100, 161);
        mLabelTextFont = "font-bold";
        
        mButtonLabelTextOffsetX = 3.0;
        mButtonLabelTextOffsetY = 3.0;
        
        // Graph
        mGraphAxisColor = IColor(255, 48, 48, 48);
        
        //mGraphAxisColor = IColor(255, 21, 21, 117); // Dark blue
        // Choose maximum brightness color for labels,
        // to see them well over clear spectrograms
        mGraphAxisOverlayColor = IColor(255, 255, 255, 255);
        mGraphAxisLabelColor = IColor(255, 255, 255, 255);
        mGraphAxisLabelOverlayColor = IColor(255, 48, 48, 48);
        
        //mGraphAxisLineWidth = 2.0; //1.0;
        //mGraphAxisLineWidthBold = 3.0; //2.0;

        // Since axis lines are now aligned to pixels,
        // can choose thiner line width
        // (every line will be displayed the same correctly anyway)
        // Grow a bit the size, to be sure it will be 1 pixel at the minimum
        //
        // Choose 0.25 more. Otherwise the lines will be too dark,
        // with more clear dots at the intersections.
        //
        // With 0.5 more, it is too fat...
        mGraphAxisLineWidth = 1.25; //1.5; //1.0;
        mGraphAxisLineWidthBold = 2.25; //2.5; //2.0;
        
        mGraphCurveDescriptionColor = IColor(255, 170, 170, 170);
        mGraphCurveColorBlue = IColor(255, 64, 64, 255);
        mGraphCurveColorGreen = IColor(255, 194, 243, 61);
        mGraphCurveColorPurple = IColor(255, 162, 61, 243);
        mGraphCurveColorLightBlue = IColor(255, 200, 200, 255);
        mGraphCurveFillAlpha = 0.5;
        mGraphCurveFillAlphaLight = 0.2;

        mGraphCurveColorGray = IColor(255, 64, 64, 64);
        mGraphCurveColorRed = IColor(255, 255, 64, 64);

        mGraphCurveColorLightRed = IColor(255, 255, 200, 200);
        
        mGraphCurveColorBlack = IColor(255, 0, 0, 0);

        mGraphCurveColorDarkBlue = IColor(255, 25, 25, 59);
        mGraphCurveColorDarkBlueRollover = IColor(255, 40, 40, 94);

        mGraphCurveColorBlueEnabled = IColor(255, 106 , 106, 255);

        mGraphCurveColorWhiteNames = IColor(255, 209, 216, 223);
        
        // Menu
        mMenuTextSize = 16.0;
        mMenuTextColor = IColor(255, 248, 248, 248);
        mMenuTextFGColor = IColor(255, 248, 248, 248); 
        mMenuTextBGColor = IColor(0, 0, 0, 0);;
        mMenuTextFont = "font-regular";
        mMenuCaptionTextFont = "font-bold";

        mGraphSeparatorColor = IColor(255, 24, 24, 24);
    }

    if (style == STYLE_BLUELAB_V3)
    {
        mCreateTitles = false;

        mCreatePlugName = true; //false;
        mCreateLogo = false;
        //mCreateHelpButton = false;
        mCreateHelpButton = true;
        
        mTitleTextSize = 16.0;
        mTitleTextOffsetX = 0.0;
        mTitleTextOffsetY = -23.0;
        mTitleTextColor = IColor(255, 110, 110, 110);
        
        mHilightTextColor = IColor(255, 248, 248, 248);
        
        mTitleTextSizeBig = 20.0;
        mTitleTextOffsetXBig = 0.0;
        mTitleTextOffsetYBig = -32.0;
        
        mDefaultTitleSize = SIZE_SMALL;
        mTitleFont = "font-bold";
        
        mValueCaptionOffset = 0.0;
        // This is 10 in Inkscape, but here, must use a bigger size
        // to get the same result (don't know why...)
        mValueTextSize = 19.0; //10.0;
        mValueTextOffsetX = -1.0;
        mValueTextOffsetY = 27.0; //31.0; //14.0;
        //mValueTextColor = IColor(255, 235, 242, 250); //v3
        mValueTextColor = IColor(255, 147, 147, 147); //Design v3.6.8
        mValueTextFGColor = mValueTextColor;
        mValueTextBGColor = IColor(0, 0, 0, 0);
        mValueTextFont = "OpenSans-ExtraBold";

        mValueTextColorLight = IColor(255, 209, 216, 223);
        
        mVersionPosition = LOWER_RIGHT;
        
        mVersionTextSize = 13.0; //8.0; //12.0;
        mVersionTextOffsetX = 48.0; //100.0;
        mVersionTextOffsetY = 8.0; //7.0; //5.0; //3.0;
        mVersionTextColor = IColor(255, 147, 147, 147);
        mVersionTextFont = "Roboto-Bold";
        
        //mVumeterColor = IColor(255, 131 , 152, 214);
        mVumeterColor = IColor(255, 106 , 106, 255);
        mVumeterNeedleColor = IColor(255, 237, 120, 31);
        mVumeterNeedleDepth = 2.0;
        
        mLogoOffsetX = 0.0;
        mLogoOffsetY = -1.0;
        mAnimLogoSpeed = 0.5;
        
        mPlugNameOffsetX = 5.0;
        mPlugNameOffsetY = 14.0; //6.0;
        
        mTrialOffsetX = 0.0;
        mTrialOffsetY = 7.0;
        
        mHelpButtonOffsetX = -13.0; //-14.0; //-30.0; //-44.0;
        mHelpButtonOffsetY = -11.0; //-10.0; //-4.0;
        
        mDemoTextSize = 10.0;
        mDemoTextOffsetX = 2.0;
        mDemoTextOffsetY = 2.0;
        mDemoTextColor = IColor(255, 200, 0, 0);
        mDemoFont = "font-regular";
        
        mWatermarkTextSize = 10.0;
        mWatermarkTextOffsetX = 2.0;
        mWatermarkTextOffsetY = 2.0;
        mWatermarkTextColor = IColor(255, 0, 128, 255);
        mWatermarkFont = "font-regular";

        mCreateRadioLabels = false;
        mRadioLabelTextSize = 15;
        mRadioLabelTextOffsetX = 6.0;
        mRadioLabelTextColor = IColor(255, 100, 100, 161);
        mLabelTextFont = "font-bold";
        
        mButtonLabelTextOffsetX = 3.0;
        mButtonLabelTextOffsetY = 3.0;
        
        // Graph
        mGraphAxisColor = IColor(255, 48, 48, 48);
        //mGraphAxisColor = IColor(255, 21, 21, 117); // Dark blue
        // Choose maximum brightness color for labels,
        // to see them well over clear spectrograms
        mGraphAxisOverlayColor = IColor(255, 255, 255, 255);
        mGraphAxisLabelColor = IColor(255, 255, 255, 255);
        mGraphAxisLabelOverlayColor = IColor(255, 48, 48, 48);

        //mGraphAxisLineWidth = 2.0; //1.0;
        //mGraphAxisLineWidthBold = 3.0; //2.0;

        // Since axis lines are now aligned to pixels,
        // can choose thiner line width
        // (every line will be displayed the same correctly anyway)
        // Grow a bit the size, to be sure it will be 1 pixel at the minimum
        //
        // Choose 0.25 more. Otherwise the lines will be too dark,
        // with more clear dots at the intersections.
        //
        // With 0.5 more, it is too fat...
        mGraphAxisLineWidth = 1.25; //1.5; //1.0;
        mGraphAxisLineWidthBold = 2.25; //2.5; //2.0;
        
        mGraphCurveDescriptionColor = IColor(255, 170, 170, 170);
        mGraphCurveColorBlue = IColor(255, 64, 64, 255);
        mGraphCurveColorGreen = IColor(255, 194, 243, 61);
        mGraphCurveColorPurple = IColor(255, 162, 61, 243);
        mGraphCurveColorLightBlue = IColor(255, 200, 200, 255);
        mGraphCurveFillAlpha = 0.5;
        mGraphCurveFillAlphaLight = 0.2;

        mGraphCurveColorGray = IColor(255, 64, 64, 64);
        mGraphCurveColorRed = IColor(255, 255, 64, 64);

        mGraphCurveColorLightRed = IColor(255, 255, 200, 200);
        
        mGraphCurveColorBlack = IColor(255, 0, 0, 0);

        mGraphCurveColorDarkBlue = IColor(255, 25, 25, 59);
        mGraphCurveColorDarkBlueRollover = IColor(255, 40, 40, 94);
 
        mGraphCurveColorBlueEnabled = IColor(255, 106 , 106, 255);

        mGraphCurveColorWhiteNames = IColor(255, 209, 216, 223);
        
        // Circle drawer
        mCircleGDCircleLineWidth = 2.5; //3.0; //2.0;
        mCircleGDLinesWidth = 2.0; //1.5; //1.0;
        mCircleGDLinesColor = IColor(255, 147, 147, 147);
        //IColor(255, 128, 128, 128);
        mCircleGDTextColor = IColor(255, 147, 147, 147);
        // IColor(255, 128, 128, 128);
        mCircleGDOffsetX = 18; //15; //8;
        mCircleGDOffsetY = 14; //16;

        // Menu
        mMenuTextSize = 16.0;
        //mMenuTextColor = IColor(255, 248, 248, 248);
        mMenuTextColor = IColor(255, 147, 147, 147);
        mMenuTextFGColor = IColor(255, 147, 147, 147);
        mMenuTextBGColor = IColor(0, 0, 0, 0); // Black
        //mMenuTextBGColor = IColor(255, 25, 25, 59); // Dark blue
        //mMenuTextEntryFGColor = IColor(255, 248, 248, 248);
        mMenuTextEntryFGColor = IColor(255, 147, 147, 147);
        //mMenuTextEntryBGColor = IColor(0, 0, 0, 0); // Black
        mMenuTextEntryBGColor = IColor(255, 25, 25, 59); // Dark blue
        mMenuTextFont = "OpenSans-ExtraBold";
        mMenuCaptionTextFont = "Roboto-Bold";
        mMenuCellBGColor = IColor(255, 106, 106, 255); 
        mMenuBorderColor = IColor(255, 248, 248, 248);
            
        mGraphSeparatorColor = IColor(255, 147, 147, 147);

        mTooltipTextSize = 18.0; //16.0; //20.0;
    }
}

GUIHelper12::~GUIHelper12() {}

IBKnobControl *
GUIHelper12::CreateKnob(IGraphics *graphics,
                        float x, float y,
                        const char *bitmapFname, int nStates,
                        int paramIdx,
                        const char *tfBitmapFname,
                        const char *title,
                        Size titleSize,
                        ICaptionControl **caption,
                        bool createValue)
{
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, nStates);
    IBKnobControl *knob = new IBKnobControl(x, y, bitmap, paramIdx);
    graphics->AttachControl(knob);
    
    if (mCreateTitles)
    {
        // Title
        if ((title != NULL) && (strlen(title) > 0))
        {
            CreateTitle(graphics, x + bitmap.W()/2, y, title, titleSize);
        }
    }

    if (createValue)
    {
        ICaptionControl *caption0 =
        CreateValue(graphics,
                    x + bitmap.W()/2, y + bitmap.H()/bitmap.N(),
                    tfBitmapFname,
                    paramIdx);
        if (caption != NULL)
            *caption = caption0;
    }
    
    return knob;
}

ISVGKnobControl *
GUIHelper12::CreateKnobSVG(IGraphics *graphics,
                           float x, float y,
                           float width, float height,
                           const char *svgFname,
                           int paramIdx,
                           const char *tfBitmapFname,
                           const char *title,
                           Size titleSize,
                           ICaptionControl **caption,
                           bool createValue,
                           const char *tooltip)
{
    const ISVG knobSVG = graphics->LoadSVG(svgFname);
    IRECT bounds(x, y, x + width, y + height);
    ISVGKnobControl *knob = new ISVGKnobControl(bounds, knobSVG, paramIdx);
    graphics->AttachControl(knob);
    
    if (mCreateTitles)
    {
        // Title
        if ((title != NULL) && (strlen(title) > 0))
        {
            CreateTitle(graphics, x + width/2, y, title, titleSize);
        }
    }

    if (createValue)
    {
        ICaptionControl *caption0 =
        CreateValue(graphics,
                    x + width/2, y + height,
                    tfBitmapFname,
                    paramIdx);
        if (caption != NULL)
            *caption = caption0;
    }

    if (tooltip != NULL)
        knob->SetTooltip(tooltip);
    
    return knob;
}

#ifdef IGRAPHICS_NANOVG
GraphControl12 *
GUIHelper12::CreateGraph(Plugin *plug, IGraphics *graphics,
                         float x, float y,
                         const char *bitmapFname, int paramIdx,
                         const char *overlayFname)
{
    const char *resPath = graphics->GetSharedResourcesSubPath();
    char fontPath[MAX_PATH];
    sprintf(fontPath, "%s/%s", resPath, "font.ttf");
    
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname);
    
    IRECT rect(x, y, x + bitmap.W(), y + bitmap.H());
    GraphControl12 *graph = new GraphControl12(plug, graphics, rect,
                                               paramIdx, fontPath);
    
#if !FIX_RESIZE_GRAPH
    graph->SetBackgroundImage(graphics, bitmap);
#endif
    
    if (overlayFname != NULL)
    {
        const char *resPath = graphics->GetSharedResourcesSubPath();
        
        char bmpPath[MAX_PATH];
        sprintf(bmpPath, "%s/%s", resPath, overlayFname);
        
        IBitmap overlayBitmap = graphics->LoadBitmap(bmpPath);
        
        graph->SetOverlayImage(graphics, overlayBitmap);
    }
    
    graphics->AttachControl(graph);
    
    return graph;
}

#endif // IGRAPHICS_NANOVG


IBLSwitchControl *
GUIHelper12::CreateSwitchButton(IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname, int nStates,
                                int paramIdx, const char *title,
                                Size titleSize, bool clickToggleOff)
{
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, nStates);
 
    if (mCreateTitles)
        CreateTitle(graphics, x + bitmap.W()/2, y, title, titleSize);
    
    IBLSwitchControl *button = new IBLSwitchControl(x, y, bitmap, paramIdx);

    button->SetClickToggleOff(clickToggleOff);
    
    graphics->AttachControl(button);
    
    return button;
}

IBLSwitchControl *
GUIHelper12::CreateToggleButton(IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname,
                                int paramIdx, const char *title,
                                Size titleSize, bool clickToggleOff,
                                const char *tooltip)
{
    IBLSwitchControl *button = CreateSwitchButton(graphics, x, y,
                                                  bitmapFname, 2,
                                                  paramIdx,
                                                  title, titleSize);
    button->SetClickToggleOff(clickToggleOff);

    if (tooltip != NULL)
        button->SetTooltip(tooltip);
    
    return button;
}

VumeterControl *
GUIHelper12::CreateVumeter(IGraphics *graphics,
                           float x, float y,
                           const char *bitmapFname, int nStates,
                           int paramIdx, const char *title)
{
    // TODO
    /*VumeterControl *vumeter = CreateKnob(graphics,
                                         x, y,
                                         bitmapFname, nStates,
                                         paramIdx, title);*/
    
    //graphics->AttachControl(vumeter);
    
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, nStates);
    VumeterControl *vumeter = new IBKnobControl(x, y, bitmap, paramIdx);
    vumeter->SetInteractionDisabled(true);
    
    graphics->AttachControl(vumeter);
    
    return vumeter;
}

BLVumeterControl *
GUIHelper12::CreateVumeterV(IGraphics *graphics,
                            float x, float y,
                            const char *bitmapFname,
                            int paramIdx, const char *title)

{
    // Background bitmap
    float width;
    float height;
    CreateBitmap(graphics,
                 x, y,
                 bitmapFname,
                 0.0f, 0.0f,
                 &width, &height);
    
    IRECT rect(x, y, x + width, y + height);
    BLVumeterControl *result = new BLVumeterControl(rect,
                                                    mVumeterColor,
                                                    paramIdx);
    result->SetInteractionDisabled(true);
    graphics->AttachControl(result);
    
    return result;
}

BLVumeter2SidesControl *
GUIHelper12::CreateVumeter2SidesV(IGraphics *graphics,
                                  float x, float y,
                                  const char *bitmapFname,
                                  int paramIdx, const char *title,
                                  float marginMin, float marginMax,
                                  const char *tooltip)
{
    // Background bitmap
    float width;
    float height;
    IBitmapControl *bitmap = CreateBitmap(graphics,
                                          x, y,
                                          bitmapFname,
                                          0.0f, 0.0f,
                                          &width, &height);

    // For tooltips
    bitmap->SetInteractionDisabled(false);
    
    // Margin
    x += marginMin;
    width -= (marginMin + marginMax);
    
    IRECT rect(x, y, x + width, y + height);
    BLVumeter2SidesControl *result = new BLVumeter2SidesControl(rect,
                                                                mVumeterColor,
                                                                paramIdx);

    if (tooltip != NULL)
        bitmap->SetTooltip(tooltip);
    
    result->SetInteractionDisabled(true);
    
    graphics->AttachControl(result);
    
    return result;
}

BLVumeterNeedleControl *
GUIHelper12::CreateVumeterNeedleV(IGraphics *graphics,
                                  float x, float y,
                                  const char *bitmapFname,
                                  int paramIdx, const char *title)

{
    // Background bitmap
    float width;
    float height;
    CreateBitmap(graphics,
                 x, y,
                 bitmapFname,
                 0.0f, 0.0f,
                 &width, &height);
    
    IRECT rect(x, y, x + width, y + height);
    BLVumeterNeedleControl *result = new BLVumeterNeedleControl(rect,
                                                                mVumeterNeedleColor,
                                                                mVumeterNeedleDepth,
                                                                paramIdx);
    result->SetInteractionDisabled(true);
    graphics->AttachControl(result);
    
    return result;
    
}

ITextControl *
GUIHelper12::CreateText(IGraphics *graphics, float x, float y,
                        const char *textStr, float size,
                        const char *font,
                        const IColor &color, EAlign align,
                        float offsetX, float offsetY)
{
    IText text(size, color, font, align);
    
    float width = GetTextWidth(graphics, text, textStr);
    
    IRECT rect(x + offsetX, y + offsetY,
               (x + offsetX + width), (y  + size + offsetY));

    ITextControl *textControl = new ITextControl(rect, textStr, text);
    
    graphics->AttachControl(textControl);
    
    return textControl;
}

ITextButtonControl *
GUIHelper12::CreateTextButton(IGraphics *graphics, float x, float y,
                              int paramIdx,
                              const char *textStr, float size,
                              const char *font,
                              const IColor &color, EAlign align,
                              float offsetX, float offsetY,
                              const IColor &borderColor,
                              float borderWidth)
{
    IText text(size, color, font, align);
    
    float width = GetTextWidth(graphics, text, textStr);
    
    IRECT rect(x + offsetX, y + offsetY,
               (x + offsetX + width), (y  + size + offsetY));

    
    ITextButtonControl *textControl = new ITextButtonControl(rect, paramIdx,
                                                             textStr, text,
                                                             DEFAULT_BGCOLOR,
                                                             //color,
                                                             false,
                                                             borderColor,
                                                             borderWidth);
    
    graphics->AttachControl(textControl);
    
    return textControl;
}

IBitmapControl *
GUIHelper12::CreateBitmap(IGraphics *graphics,
                          float x, float y,
                          const char *bitmapFname,
                          //EAlign align,
                          float offsetX, float offsetY,
                          float *width, float *height)
{
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, 1);
    
    if (width != NULL)
        *width = bitmap.W();
    if (height != NULL)
        *height = bitmap.H()/bitmap.N();
    
    IBitmapControl *result = new IBitmapControl(x + offsetX, y + offsetY, bitmap);
    result->SetInteractionDisabled(true);
    graphics->AttachControl(result);
    
    return result;
}

IXYPadControl *
GUIHelper12::CreateXYPad(IGraphics *graphics,
                         float x, float y,
                         const char *trackBitmapFname,
                         const char *handleBitmapFname,
                         int xParamIdx, int yParamIdx,
                         float borderSize, bool reverseY,
                         const char *tooltip)
{
    std::initializer_list<int> params( { xParamIdx, yParamIdx } );
    
    IBitmap trackBitmap = graphics->LoadBitmap(trackBitmapFname, 1);
    IBitmap handleBitmap = graphics->LoadBitmap(handleBitmapFname, 1);

    IRECT rect(x, y, x + trackBitmap.W(), y + trackBitmap.H());
    IXYPadControl *result =
        new IXYPadControl(rect, params, trackBitmap, handleBitmap,
                          borderSize, reverseY);

    if (tooltip != NULL)
        result->SetTooltip(tooltip);
    
    graphics->AttachControl(result);
    
    return result;
}

ISpatializerHandleControl *
GUIHelper12::CreateSpatializerHandle(IGraphics *graphics,
                                     float x, float y, float rad,
                                     float minAngle, float maxAngle,
                                     bool reverseY,
                                     const char *handleBitmapFname,
                                     const char *tfBitmapFname,
                                     int paramIdx,
                                     float valueXOffset, float valueYOffset,
                                     const char *tooltip)
{
    IBitmap handleBitmap = graphics->LoadBitmap(handleBitmapFname, 1);

    IRECT rect(x, y, x + rad*2.0, y + rad*2.0);
    ISpatializerHandleControl *result =
        new ISpatializerHandleControl(rect, minAngle, maxAngle, reverseY,
                                      handleBitmap, paramIdx);
    graphics->AttachControl(result);

    // Text field
    CreateValue(graphics,
                x + rect.W()/2 + valueXOffset,
                y + rect.H() + valueYOffset,
                tfBitmapFname, paramIdx);

    if (tooltip != NULL)
        result->SetTooltip(tooltip);
    
    return result;
}

void
GUIHelper12::CreateVersion(Plugin *plug, IGraphics *graphics,
                           const char *versionStr)
{
    // Lower left corner
    char versionStr0[256];
    sprintf(versionStr0, "v%s", versionStr);
    
    float x  = 0.0;
    float y  = 0.0;
    
    EAlign textAlign = EAlign::Near;
    
    if (mVersionPosition == LOWER_LEFT)
    {
        x = mVersionTextOffsetX;
        y = graphics->Height() - mVersionTextSize - mVersionTextOffsetY;
        
        textAlign = EAlign::Near;
    }
    
    if (mVersionPosition == LOWER_RIGHT)
    {
        int strWidth = (int)(strlen(versionStr0)*mVersionTextSize);
        
        x = graphics->Width() - strWidth/2 - mVersionTextOffsetX + mVersionTextSize/2;
        x += mVersionTextSize/2;
        
        y = graphics->Height() - mVersionTextOffsetY - mVersionTextSize;
        
        textAlign = EAlign::Near;
    }
    
    IText versionText(mVersionTextSize,
                      mVersionTextColor, mVersionTextFont, textAlign);
    
    if (mVersionPosition == BOTTOM)
    {
        float textWidth = GetTextWidth(graphics, versionText, versionStr0);
        
        x = graphics->Width()/2 - textWidth/2.0;
        y = graphics->Height() - mVersionTextSize - mVersionTextOffsetY;
    }
    
    float textWidth = GetTextWidth(graphics, versionText, versionStr0);
    
    IRECT rect(x, y, x + textWidth, y + mVersionTextSize);
    
    ITextControl *textControl = new ITextControl(rect, versionStr0, versionText);
    
    graphics->AttachControl(textControl);
}

// Static logo
void
GUIHelper12::CreateLogo(Plugin *plug, IGraphics *graphics,
                        const char *logoFname, Position pos)
{
    if (!mCreateLogo)
        return;
    
    IBitmap bmp = graphics->LoadBitmap(logoFname, 1);
    
    float x = 0.0;
    float y = 0.0;
    
    if (pos == TOP)
    {
        // Upper right corner
        x = graphics->Width() - bmp.W();
        y = 0;
    }
    
    if (pos == BOTTOM)
    {
        // Lower right corner
        x = graphics->Width() - bmp.W();
        y = graphics->Height() - bmp.H();
    }
    
    IBitmapControl *control = new IBitmapControl(x + mLogoOffsetX,
                                                 y + mLogoOffsetY, bmp);
    control->SetInteractionDisabled(true);
    
    graphics->AttachControl(control);
}

// Static logo
void
GUIHelper12::CreateLogoAnim(Plugin *plug, IGraphics *graphics,
                            const char *logoFname, int nStates, Position pos)
{
    if (!mCreateLogo)
        return;
    
    IBitmap bmp = graphics->LoadBitmap(logoFname, nStates);
    
    float x = 0.0;
    float y = 0.0;
    
    if (pos == TOP)
    {
        // Upper right corner
        x = graphics->Width() - bmp.W();
        y = 0;
    }
    
    if (pos == BOTTOM)
    {
        // Lower right corner
        x = graphics->Width() - bmp.W();
        y = graphics->Height() - bmp.H()/bmp.N();
    }
    
    IBitmapControlAnim *control = new IBitmapControlAnim(x + mLogoOffsetX,
                                                         y + mLogoOffsetY,
                                                         bmp, kNoValIdx,
                                                         mAnimLogoSpeed,
                                                         true, true);
    control->SetInteractionDisabled(true);
    
    graphics->AttachControl(control);
}

void
GUIHelper12::GetManualFullPath(Plugin *plug, IGraphics *graphics,
                               const char *manualFileName,
                               char fullFileName[1024])
{
#ifndef WIN32 // Mac
    WDL_String wdlResDir;
    BLUtilsPlug::GetFullPlugResourcesPath(*plug, &wdlResDir);
    const char *resDir = wdlResDir.Get();
    
    sprintf(fullFileName, "%s/%s", resDir, manualFileName);
#else
    // On windows, we must load the resource from dll, save it to the temp file
    // before re-opening it
    WDL_TypedBuf<uint8_t> resBuf = graphics->LoadResource(MANUAL_FN, "RCDATA");
    long resSize = resBuf.GetSize();
    if (resSize == 0)
        return;

    TCHAR tempPathBuffer[MAX_PATH];
    GetTempPath(MAX_PATH, tempPathBuffer);

    sprintf(fullFileName, "%s%s", tempPathBuffer, MANUAL_FN);
    
    FILE *file = fopen(fullFileName, "wb");
    fwrite(resBuf.Get(), 1, resSize, file);
    fclose(file);
#endif
}

void
GUIHelper12::CreateHelpButton(Plugin *plug, IGraphics *graphics,
                              const char *bmpFname,
                              const char *manualFileName,
                              Position pos,
                              const char *tooltip)
{
    if (!mCreateHelpButton)
        return;
    
    IBitmap bitmap;
    if (mStyle != STYLE_BLUELAB_V3)
        bitmap = graphics->LoadBitmap(bmpFname, 1);
    else
        bitmap = graphics->LoadBitmap(bmpFname, 2);
    
    float x = 0.0;
    float y = 0.0;
    
    //if (pos == TOP) // TODO
    
    if (pos == BOTTOM)
    {
        // Lower right corner
        x = graphics->Width() - bitmap.W() + mHelpButtonOffsetX;
        y = graphics->Height() - bitmap.H()/bitmap.N() + mHelpButtonOffsetY;
    }
    
    char fullFileName[1024];
    GetManualFullPath(plug, graphics, manualFileName, fullFileName);

    IControl *control = NULL;

    if (mStyle != STYLE_BLUELAB_V3)
        control = new IHelpButtonControl(x, y, bitmap,
                                         kNoValIdx,
                                         fullFileName);
    else
        control = new IHelpButtonControl2(x, y, bitmap,
                                          kNoValIdx,
                                          fullFileName);

    if (tooltip != NULL)
        control->SetTooltip(tooltip);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
}

void
GUIHelper12::ShowHelp(Plugin *plug, IGraphics *graphics,
                      const char *manualFileName)
{
    char fullFileName[1024];
    GetManualFullPath(plug, graphics,
                      manualFileName, fullFileName);
    
    IHelpButtonControl::ShowManual(fullFileName);
}

void
GUIHelper12::CreatePlugName(Plugin *plug, IGraphics *graphics,
                            const char *plugNameFname, Position pos)
{
    if (!mCreatePlugName)
        return;
    
    IBitmap bmp = graphics->LoadBitmap(plugNameFname, 1);
    
    float x = 0;
    float y = 0;
    
    if (pos == TOP)
    {
        // Upper left corner
        x = mPlugNameOffsetX;
        y = mPlugNameOffsetY;
    }

    if (mStyle != STYLE_BLUELAB_V3)
    {
        if (pos == BOTTOM)
        {
            // Lower left corner
            x = mPlugNameOffsetX;
            y = graphics->Height() - bmp.H() - mPlugNameOffsetY;
            
            y -= mTrialOffsetY;
        }
    }
    else
    {
        // Center
        int plugWidth = graphics->Width(); //plug->WindowWidth();
        int titleWidth = bmp.W();
        x = plugWidth/2 - titleWidth/2;

        int plugNameHeight = BLUELAB_V3_PLUG_NAME_HEIGHT; // bmp.H()
        y = graphics->Height() - plugNameHeight - mPlugNameOffsetY;
    }
    
    IBitmapControl *control = new IBitmapControl(x, y, bmp);
    control->SetInteractionDisabled(true);
    
    graphics->AttachControl(control);
}

void
GUIHelper12::CreateDemoMessage(IGraphics *graphics)
{
#if DEMO_MODE
    int x = mDemoTextOffsetX;
    int y = graphics->Height() - mDemoTextSize - mDemoTextOffsetY;
    
    ITextControl *textControl = CreateText(graphics,
                                           x, y,
                                           DEMO_MESSAGE, mDemoTextSize,
                                           mDemoFont,
                                           mDemoTextColor, EAlign::Near);
    
    // Avoids that the trial message masks the interaction on the help button for example
    textControl->SetInteractionDisabled(true);
#endif
}

void
GUIHelper12::CreateWatermarkMessage(IGraphics *graphics,
                                    const char *message,
                                    IColor *color)
{
    IColor textColor = mWatermarkTextColor;
    if (color != NULL)
        textColor = *color;
    
    int x = mWatermarkTextOffsetX;
    int y = graphics->Height() - mWatermarkTextSize - mWatermarkTextOffsetY;
    
    ITextControl *textControl = CreateText(graphics,
                                           x, y,
                                           message, mWatermarkTextSize,
                                           mWatermarkFont,
                                           textColor, EAlign::Near);
    
    // Avoids that the trial message masks the interaction on the help button for example
    textControl->SetInteractionDisabled(true);
}

IRadioButtonsControl *
GUIHelper12::CreateRadioButtons(IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname,
                                int numButtons, float size, int paramIdx,
                                bool horizontalFlag, const char *title,
                                EAlign align,
                                EAlign titleAlign,
                                const char **radioLabels,
                                const char *tooltip)
{
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, 2);
    
    float titleX = x;
    if (titleAlign == EAlign::Center)
        titleX = x + bitmap.W()/2;
    else if (titleAlign == EAlign::Far)
        titleX = x + bitmap.W();

    if (mCreateTitles)
    {
        // Title
        CreateTitle(graphics, titleX, y, title, SIZE_DEFAULT, titleAlign);
    }
    
    if (mCreateRadioLabels)
    {
        // Radio labels
        if (numButtons > 0)
        {
            if (!horizontalFlag)
            {
                int labelX = x;
                if (align == EAlign::Far)
                    labelX = labelX - bitmap.W() - mRadioLabelTextOffsetX;
                
                if (align == EAlign::Near)
                {
                    labelX = labelX + bitmap.W() + mRadioLabelTextOffsetX;
                }
                
                int spaceBetween = (size - numButtons*bitmap.H()/2)/(numButtons - 1);
                int stepSize = bitmap.H()/2 + spaceBetween;
                
                for (int i = 0; i < numButtons; i++)
                {
                    IText labelText(mRadioLabelTextSize,
                                    mRadioLabelTextColor,
                                    mLabelTextFont,
                                    align);
                    
                    int labelY = y + i*stepSize;
                    const char *labelStr = radioLabels[i];
                    CreateRadioLabelText(graphics,
                                         labelX, labelY,
                                         labelStr,
                                         bitmap,
                                         labelText,
                                         align);
                }
            }
            else
            {
                // NOTE: not really tested
                
                int labelY = x;
                //if (titleAlign == IText::kAlignFar)
                //    labelX = labelX - bitmap.W - RADIO_LABEL_TEXT_OFFSET;
                
                if (align == EAlign::Near)
                {
                    // TODO: implement this
                }
                
                int spaceBetween = (size - numButtons*bitmap.H()/2)/(numButtons - 1);
                int stepSize = bitmap.H()/2 + spaceBetween;
                
                for (int i = 0; i < numButtons; i++)
                {
                    // new
                    IText labelText(mRadioLabelTextSize,
                                    mRadioLabelTextColor,
                                    mLabelTextFont,
                                    align);
                    
                    int labelX = x + i*stepSize;
                    const char *labelStr = radioLabels[i];
                    CreateRadioLabelText(graphics,
                                         labelX, labelY,
                                         labelStr,
                                         bitmap,
                                         labelText,
                                         align);
                }
            }
        }
    }
    
    // Buttons
    IRECT rect(x, y, x + size, y + bitmap.H());
    EDirection direction = EDirection::Horizontal;
    if (!horizontalFlag)
    {
        rect = IRECT(x, y, x + bitmap.W(), y + size);
        
        direction = EDirection::Vertical;
    }
    
    IRadioButtonsControl *control =
        new IRadioButtonsControl(rect,
                                 paramIdx, numButtons,
                                 bitmap, direction);

    if (tooltip != NULL)
        control->SetTooltip(tooltip);
    
    graphics->AttachControl(control);
    
    return control;
}

IRadioButtonsControlCustom *
GUIHelper12::CreateRadioButtonsCustom(IGraphics *graphics,
                                      float x, float y,
                                      const char *bitmapFnames[], int numButtons,
                                      int bitmapNFrames,
                                      float size, int paramIdx,
                                      bool horizontalFlag,
                                      const char *tooltip)
{
#define MAX_NUM_BITMAPS 32
    
    if (numButtons< 1)
        return NULL;
    
    IBitmap bitmaps[MAX_NUM_BITMAPS];

    for (int i = 0; i < numButtons; i++)
        bitmaps[i] = graphics->LoadBitmap(bitmapFnames[i], bitmapNFrames);
    
    // Buttons
    IRECT rect(x, y, x + size, y + bitmaps[0].H()/bitmaps[0].N());
    EDirection direction = EDirection::Horizontal;
    if (!horizontalFlag)
    {
        rect = IRECT(x, y, x + bitmaps[0].W(), y + size);
        
        direction = EDirection::Vertical;
    }
    
    IRadioButtonsControlCustom *control =
        new IRadioButtonsControlCustom(rect,
                                       paramIdx, numButtons,
                                       bitmaps, direction);

    if (tooltip != NULL)
        control->SetTooltip(tooltip);
    
    graphics->AttachControl(control);
        
    return control;
}

void
GUIHelper12::ResetParameter(Plugin *plug, int paramIdx)
{
    if (plug->GetUI())
    {
        plug->GetParam(paramIdx)->SetToDefault();
        plug->SendParameterValueFromAPI(paramIdx,
                                        plug->GetParam(paramIdx)->Value(), false);
    }
}

IControl *
GUIHelper12::CreateGUIResizeButton(ResizeGUIPluginInterface *plug,
                                   IGraphics *graphics,
                                   float x, float y,
                                   const char *bitmapFname,
                                   int paramIdx,
                                   char *label,
                                   int guiSizeIdx,
                                   const char *tooltip)
{
    int bmpFrames = 3;
    
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, bmpFrames);
    
    IRECT pR;
    pR.L = x;
    pR.T = y;
    pR.R = x + bitmap.W();
    pR.B = y + bitmap.H();
    
    // With rollover
    IControl *control =
        new IGUIResizeButtonControl(plug, x, y, bitmap,
                                    paramIdx, guiSizeIdx);

    if (tooltip != NULL)
        control->SetTooltip(tooltip);
    
    graphics->AttachControl(control);

    if ((label != NULL) && (strlen(label) != 0))
    {
        // Add the label
        CreateTitle(graphics,
                    x + mButtonLabelTextOffsetX,
                    y + bitmap.H()*1.5/((BL_FLOAT)bmpFrames) +
                    mButtonLabelTextOffsetY,
                    label, Size::SIZE_DEFAULT);
    }
    
    return control;
}

IControl *
GUIHelper12::CreateRolloverButton(IGraphics *graphics,
                                  float x, float y,
                                  const char *bitmapFname,
                                  int paramIdx,
                                  char *label,
                                  bool toggleFlag,
                                  bool toggleOffFlag,
                                  bool disableDlbClick,
                                  const char *tooltip)
{
  int bmpFrames = 3;
  
  IBitmap bitmap = graphics->LoadBitmap(bitmapFname, bmpFrames);
  
  IRECT pR;
  pR.L = x;
  pR.T = y;
  pR.R = x + bitmap.W();
  pR.B = y + bitmap.H();
    
  IRolloverButtonControl *control = new IRolloverButtonControl(x, y, bitmap,
                                                               paramIdx,
                                                               toggleFlag,
                                                               toggleOffFlag,
                                                               disableDlbClick);

  if (tooltip != NULL)
      control->SetTooltip(tooltip);
  
  graphics->AttachControl(control);

  if (mCreateTitles)
  {
      if (label != NULL)
      {
          // Add the label
          ITextControl *text = CreateTitle(graphics,
                                           x + mButtonLabelTextOffsetX,
                                           y + bitmap.H()*1.5/((BL_FLOAT)bmpFrames) +
                                           mButtonLabelTextOffsetY,
                                           label,
                                           // NOTE: with small/default size, text is not well centered
                                           //Size::SIZE_DEFAULT,
                                           Size::SIZE_BIG,
                                           EAlign::Near);
          
          control->LinkText(text, mTitleTextColor, mHilightTextColor);
      }
  }
  
  return control;
}

void
GUIHelper12::GetValueTextColor(IColor *valueTextColor) const
{
    *valueTextColor = mValueTextColor;
}

void
GUIHelper12::GetValueTextColorLight(IColor *valueTextColor) const
{
    *valueTextColor = mValueTextColorLight;
}

void
GUIHelper12::GetGraphAxisColor(int color[4])
{
    color[0] = mGraphAxisColor.R;
    color[1] = mGraphAxisColor.G;
    color[2] = mGraphAxisColor.B;
    color[3] = mGraphAxisColor.A;
}

void
GUIHelper12::GetGraphAxisOverlayColor(int color[4])
{
    color[0] = mGraphAxisOverlayColor.R;
    color[1] = mGraphAxisOverlayColor.G;
    color[2] = mGraphAxisOverlayColor.B;
    color[3] = mGraphAxisOverlayColor.A;
}

void
GUIHelper12::GetGraphAxisLabelColor(int color[4])
{
    color[0] = mGraphAxisLabelColor.R;
    color[1] = mGraphAxisLabelColor.G;
    color[2] = mGraphAxisLabelColor.B;
    color[3] = mGraphAxisLabelColor.A;
}

void
GUIHelper12::GetGraphAxisLabelOverlayColor(int color[4])
{
    color[0] = mGraphAxisLabelOverlayColor.R;
    color[1] = mGraphAxisLabelOverlayColor.G;
    color[2] = mGraphAxisLabelOverlayColor.B;
    color[3] = mGraphAxisLabelOverlayColor.A;
}

float
GUIHelper12::GetGraphAxisLineWidth()
{
    return mGraphAxisLineWidth;
}

float
GUIHelper12::GetGraphAxisLineWidthBold()
{
    return mGraphAxisLineWidthBold;
}

void
GUIHelper12::GetGraphCurveDescriptionColor(int color[4])
{
    color[0] = mGraphCurveDescriptionColor.R;
    color[1] = mGraphCurveDescriptionColor.G;
    color[2] = mGraphCurveDescriptionColor.B;
    color[3] = mGraphCurveDescriptionColor.A;
}

void
GUIHelper12::GetGraphCurveColorBlue(int color[4])
{
    color[0] = mGraphCurveColorBlue.R;
    color[1] = mGraphCurveColorBlue.G;
    color[2] = mGraphCurveColorBlue.B;
    color[3] = mGraphCurveColorBlue.A;
}

void
GUIHelper12::GetGraphCurveColorGreen(int color[4])
{
    color[0] = mGraphCurveColorGreen.R;
    color[1] = mGraphCurveColorGreen.G;
    color[2] = mGraphCurveColorGreen.B;
    color[3] = mGraphCurveColorGreen.A;
}

void
GUIHelper12::GetGraphCurveColorPurple(int color[4])
{
    color[0] = mGraphCurveColorPurple.R;
    color[1] = mGraphCurveColorPurple.G;
    color[2] = mGraphCurveColorPurple.B;
    color[3] = mGraphCurveColorPurple.A;
}

void
GUIHelper12::GetGraphCurveColorLightBlue(int color[4])
{
    color[0] = mGraphCurveColorLightBlue.R;
    color[1] = mGraphCurveColorLightBlue.G;
    color[2] = mGraphCurveColorLightBlue.B;
    color[3] = mGraphCurveColorLightBlue.A;
}

float
GUIHelper12::GetGraphCurveFillAlpha()
{
    return mGraphCurveFillAlpha;
}

float
GUIHelper12::GetGraphCurveFillAlphaLight()
{
    return mGraphCurveFillAlphaLight;
}

void
GUIHelper12::GetGraphCurveColorGray(int color[4])
{
  color[0] = mGraphCurveColorGray.R;
  color[1] = mGraphCurveColorGray.G;
  color[2] = mGraphCurveColorGray.B;
  color[3] = mGraphCurveColorGray.A;
}

void
GUIHelper12::GetGraphCurveColorRed(int color[4])
{
    color[0] = mGraphCurveColorRed.R;
    color[1] = mGraphCurveColorRed.G;
    color[2] = mGraphCurveColorRed.B;
    color[3] = mGraphCurveColorRed.A;
}

void
GUIHelper12::GetGraphCurveColorLightRed(int color[4])
{
    color[0] = mGraphCurveColorLightRed.R;
    color[1] = mGraphCurveColorLightRed.G;
    color[2] = mGraphCurveColorLightRed.B;
    color[3] = mGraphCurveColorLightRed.A;
}

void
GUIHelper12::GetGraphCurveColorBlack(int color[4])
{
    color[0] = mGraphCurveColorBlack.R;
    color[1] = mGraphCurveColorBlack.G;
    color[2] = mGraphCurveColorBlack.B;
    color[3] = mGraphCurveColorBlack.A;
}

void
GUIHelper12::GetGraphCurveDarkBlue(int color[4])
{
    color[0] = mGraphCurveColorDarkBlue.R;
    color[1] = mGraphCurveColorDarkBlue.G;
    color[2] = mGraphCurveColorDarkBlue.B;
    color[3] = mGraphCurveColorDarkBlue.A;
}

void
GUIHelper12::GetGraphCurveDarkBlueRollover(int color[4])
{
    color[0] = mGraphCurveColorDarkBlueRollover.R;
    color[1] = mGraphCurveColorDarkBlueRollover.G;
    color[2] = mGraphCurveColorDarkBlueRollover.B;
    color[3] = mGraphCurveColorDarkBlueRollover.A;
}

void
GUIHelper12::GetGraphCurveBlueEnabled(int color[4])
{
    color[0] = mGraphCurveColorBlueEnabled.R;
    color[1] = mGraphCurveColorBlueEnabled.G;
    color[2] = mGraphCurveColorBlueEnabled.B;
    color[3] = mGraphCurveColorBlueEnabled.A;
}

void
GUIHelper12::GetGraphCurveColorWhiteNames(int color[4])
{
    color[0] = mGraphCurveColorWhiteNames.R;
    color[1] = mGraphCurveColorWhiteNames.G;
    color[2] = mGraphCurveColorWhiteNames.B;
    color[3] = mGraphCurveColorWhiteNames.A;
}

void
GUIHelper12::RefreshAllParameters(Plugin *plug, int numParams)
{
#ifndef __linux__
    for (int i = 0; i < numParams; i++)
        plug->SendParameterValueFromAPI(i, plug->GetParam(i)->Value(), false);
#else // __linux__
    IGraphics *graphics = plug->GetUI();
    if (graphics == NULL)
        return;
    
    for (int i = 0; i < numParams; i++)
    {
        double normValue = plug->GetParam(i)->GetNormalized();

        for (int j = 0; j < graphics->NControls(); j++)
        {
            IControl *control = graphics->GetControl(j);
            if (control == NULL)
                continue;

            int idx = control->GetParamIdx();

            if (idx == i)
            {
                control->SetValue(normValue);
                control->SetDirty(false);
            }
        }
    }
#endif
}

bool
GUIHelper12::PromptForFile(Plugin *plug, EFileAction action, WDL_String *result,
                           char* dir, char* extensions)
{
    //WDL_String file;
    // For zenity, to open on the right directory
    WDL_String file(dir);
 
    //IFileSelectorControl::EFileSelectorState state = IFileSelectorControl::kFSNone;
    
    if (plug && plug->GetUI())
    {
        WDL_String wdlDir(dir);
        
        //state = IFileSelectorControl::kFSSelecting;
        plug->GetUI()->PromptForFile(file, wdlDir, action, extensions);
        //state = IFileSelectorControl::kFSDone;
    }
    
    result->Set(file.Get());
    
    if (result->GetLength() == 0)
        return false;
    
    return true;
}

ITextControl *
GUIHelper12::CreateTitle(IGraphics *graphics, float x, float y,
                         const char *title, Size size, EAlign align)
{
    if (size == SIZE_DEFAULT)
        size = mDefaultTitleSize;
    
    float titleSize = mTitleTextSize;
    float textOffsetX = mTitleTextOffsetX;
    float textOffsetY = mTitleTextOffsetY;
    
    if (size == SIZE_BIG)
    {
        titleSize = mTitleTextSizeBig;
        textOffsetX = mTitleTextOffsetXBig;
        textOffsetY = mTitleTextOffsetYBig;
    }
    
    IText text(titleSize, mTitleTextColor, mTitleFont, align);
    float width = GetTextWidth(graphics, text, title);
    
    if (align == EAlign::Center)
        x -= width/2.0;
    else if (align == EAlign::Far)
        x -= width;
    
    ITextControl *control = CreateText(graphics, x, y,
                                       title, text,
                                       textOffsetX, textOffsetY);
    
    control->SetInteractionDisabled(true);
    
    return control;
}

ITextControl *
GUIHelper12::CreateValueText(IGraphics *graphics,
                             float x, float y,
                             const char *textValue)
{
    IText text(mValueTextSize, mValueTextColor, mValueTextFont,
               EAlign::Center, EVAlign::Middle, 0.0,
               mValueTextBGColor, mValueTextFGColor);
    
    float width = GetTextWidth(graphics, text, textValue);
    // Align center
    x -= width/2.0;
    
    float textOffsetX = 0.0;
    float textOffsetY = 0.0;
    
    ITextControl *control = CreateText(graphics, x, y,
                                       textValue, text,
                                       textOffsetX, textOffsetY);
    
    control->SetInteractionDisabled(true);
    
    return control;
}    

ITabsBarControl *
GUIHelper12::CreateTabsBar(IGraphics *graphics,
                           float x, float y, float w, float h)
{
    IRECT bounds(x, y, w, h);
    ITabsBarControl *tabsBar = new ITabsBarControl(bounds);

    if (mStyle == STYLE_BLUELAB_V3)
    {
        tabsBar->SetBackgroundColor(mGraphCurveColorDarkBlue);
        tabsBar->SetTabEnabledColor(mGraphCurveColorBlueEnabled);
        tabsBar->SetTabDisabledColor(mGraphCurveColorDarkBlue);
        tabsBar->SetTabsRolloverColor(mGraphCurveColorDarkBlueRollover);
        tabsBar->SetNameColor(mGraphCurveColorWhiteNames);
        tabsBar->SetCrossColor(mValueTextColor);
        tabsBar->SetCrossRolloverColor(mGraphCurveColorWhiteNames);
        tabsBar->SetTabLinesColor(mValueTextColor);
            
        tabsBar->SetCrossLineWidth(2.0);
    }
    
    graphics->AttachControl(tabsBar);

    return tabsBar;
}

ICaptionControl *
GUIHelper12::CreateDropDownMenu(IGraphics *graphics,
                                float x, float y, float width,
                                int paramIdx,
                                const char *title,
                                Size titleSize,
                                const char *tooltip)
{
    float height = mMenuTextSize;

    // Colors
    IColor bgColor = mMenuTextBGColor; // DEFAULT_FGCOLOR;
    IColor textFGColor = mMenuTextFGColor; // DEFAULT_TEXT_FGCOLOR;
    IColor textEntryBGColor = mMenuTextEntryBGColor; // DEFAULT_TEXTENTRY_BGCOLOR;
    textEntryBGColor.A = 255; // Not transparent menu
    IColor textEntryFGColor = mMenuTextEntryFGColor; // DEFAULT_TEXTENTRY_FGCOLOR;
    IColor textEntryCellBackgroundColor = mMenuCellBGColor;

    IColor menuBorderColor = mMenuBorderColor; // DEFAULT_FGCOLOR;
    
    IText menuText(mMenuTextSize,
                   textFGColor, mMenuTextFont,
                   EAlign::Center, EVAlign::Middle,
                   0.0, 
                   textEntryBGColor,
                   textEntryFGColor);
    
    // Popup menu e.g for colormap
    // NOTE: the popup menu seems to be managed for CaptionControl automatically,
    // if we have an enum parameter
    graphics->AttachPopupMenuControl(menuText/*DEFAULT_LABEL_TEXT*/);

    // Popup menu style
    IPopupMenuControl *menu = graphics->GetPopupMenuControl();
    menu->SetPanelColor(textEntryBGColor);
    menu->SetItemColor(textEntryFGColor);
    menu->SetSeparatorColor(textEntryFGColor);
    //menu->SetBorderStyle(textEntryFGColor, 2.0);
    menu->SetBorderStyle(menuBorderColor, 2.0);
    menu->SetCellBackgroundColor(textEntryCellBackgroundColor); // blue
    menu->SetDropShadow(false);

    IText captionText(mMenuTextSize,
                      textFGColor, mMenuCaptionTextFont,
                      EAlign::Center, EVAlign::Middle,
                      0.0, 
                      textEntryBGColor,
                      textEntryFGColor);
    
    IRECT rect(x, y, x + width, y + height);    
    ICaptionControl *control =
        new ICaptionControl(rect, paramIdx, captionText, bgColor, false);

    if (tooltip != NULL)
        control->SetTooltip(tooltip);
    
    graphics->AttachControl(control, kNoTag, "");

    if (mCreateTitles)
    {
        // Title
        if ((title != NULL) && (strlen(title) > 0))
        {
            CreateTitle(graphics, x + width/2, y, title, titleSize);
        }
    }
    
    return control;
}

void
GUIHelper12::GetCircleGDCircleLineWidth(float *circleLineWidth)
{
    *circleLineWidth = mCircleGDCircleLineWidth;
}

void
GUIHelper12::GetCircleGDLinesWidth(float *linesWidth)
{
    *linesWidth = mCircleGDLinesWidth;
}

void
GUIHelper12::GetCircleGDLinesColor(IColor *linesColor)
{
    *linesColor = mCircleGDLinesColor;
}

void
GUIHelper12::GetCircleGDTextColor(IColor *textColor)
{
    *textColor = mCircleGDTextColor;
}

void
GUIHelper12::GetCircleGDOffsetX(int *x)
{
    *x = mCircleGDOffsetX;
}

void
GUIHelper12::GetCircleGDOffsetY(int *x)
{
    *x = mCircleGDOffsetY;
}

void
GUIHelper12::AttachToolTipControl(IGraphics *graphics)
{
    //IBLTooltipControl *control = new IBLTooltipControl();

    IText text(mTooltipTextSize, //DEFAULT_TEXT_SIZE,
               mValueTextColorLight);
    IBLTooltipControl *control = new IBLTooltipControl(mGraphCurveColorDarkBlue,
                                                       //mValueTextColorLight,
                                                       mValueTextColor,
                                                       text);

    graphics->AttachToolTipControl(control);
    
}

void
GUIHelper12::GetGraphSeparatorColor(IColor *sepColor)
{
    *sepColor = mGraphSeparatorColor;
}

float
GUIHelper12::GetTextWidth(IGraphics *graphics, const IText &text, const char *textStr)
{
    // Measure
    IRECT textRect;
    graphics->MeasureText(text, (char *)textStr, textRect);

    float width = textRect.W();

    return width;
}

ITextControl *
GUIHelper12::CreateText(IGraphics *graphics, float x, float y,
                        const char *textStr, const IText &text,
                        float offsetX, float offsetY,
                        EAlign align)
{
    float width = GetTextWidth(graphics, text, textStr);
    
    IRECT rect(x + offsetX, y + offsetY,
               (x + offsetX + width), (y  + mTitleTextSize + offsetY));
    
    ITextControl *textControl = new ITextControl(rect, textStr, text);
    
    graphics->AttachControl(textControl);
    
    return textControl;
}

ICaptionControl *
GUIHelper12::CreateValue(IGraphics *graphics,
                         float x, float y,
                         const char *bitmapFname,
                         int paramIdx)
{
    // Bitmap
    //float width;
    //float height;
    //CreateBitmap(graphics, x, y, bitmapFname,
    //             mValueTextOffsetX, mValueTextOffsetY,
    //             &width, &height);

    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, 1);
    float width = bitmap.W();
    float height = bitmap.H()/bitmap.N();
    
    // Value
    IRECT bounds(x - width/2.0 + mValueTextOffsetX,
                 y + mValueTextOffsetY,
                 x + width/2.0 + mValueTextOffsetX,
                 y + height + mValueTextOffsetY);
    
    IText text(mValueTextSize, mValueTextColor, mValueTextFont,
               EAlign::Center, EVAlign::Middle, 0.0,
               mValueTextBGColor, mValueTextFGColor);
    ICaptionControl *caption = new ICaptionControl(bounds, paramIdx, text,
                                                   mValueTextBGColor);
    caption->DisablePrompt(false); // Here is the magic !
    graphics->AttachControl(caption);
    
    return caption;
}

ITextControl *
GUIHelper12::CreateRadioLabelText(IGraphics *graphics,
                                  float x, float y,
                                  const char *textStr,
                                  const IBitmap &bitmap,
                                  const IText &text,
                                  EAlign align)
{
    float width = GetTextWidth(graphics, text, textStr);
    
    // Adjust for aligne center
    float dx = 0.0;
    
    // Adjust for align far
    if (align == EAlign::Far)
    {
        int bmpWidth = bitmap.W();
        dx = -width + bmpWidth;
    }
    
    IRECT rect(x + dx, y,
               (x + width) + dx, y + text.mSize);
    
    ITextControl *control = new ITextControl(rect, textStr, text);
    
    graphics->AttachControl(control);
    
    return control;
}
