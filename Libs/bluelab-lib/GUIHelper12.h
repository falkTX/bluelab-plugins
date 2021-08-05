//
//  GUIHelper12.hpp
//  UST-macOS
//
//  Created by applematuer on 9/25/20.
//
//

#ifndef GUIHelper12_h
#define GUIHelper12_h

#include <IControls.h>

/*#include <BLVumeterControl.h>
#include <BLVumeterNeedleControl.h>
#include <BLVumeter2SidesControl.h>
#include <ResizeGUIPluginInterface.h>
#include <ITextButtonControl.h>
#include <IXYPadControl.h>
#include <IBLSwitchControl.h>
#include <ITabsBarControl.h>*/

#include <VumeterControl.h>

#include "IPlug_include_in_plug_hdr.h"

// Tooltipe delay: 0.5s
#define TOOLTIP_DELAY 0.5

using namespace iplug;
using namespace iplug::igraphics;

class GraphControl12;
class IRadioButtonsControl;
class IGUIResizeButtonControl;

// TODO
//#define VumeterControl IBKnobControl
//class VumeterControl;

// GUIHelper12: from GUIHelper11, for GraphControl12
//
class BLVumeterControl;
class BLVumeterNeedleControl;
class BLVumeter2SidesControl;
class ResizeGUIPluginInterface;
class ITextButtonControl;
class IXYPadControl;
class IBLSwitchControl;
class ITabsBarControl;
class ISpatializerHandleControl;
class IRadioButtonsControlCustom;

class GUIHelper12
{
public:
    enum Style
    {
        STYLE_BLUELAB,
        STYLE_UST,
        STYLE_BLUELAB_V3
    };
    
    enum Position
    {
        TOP,
        BOTTOM,
        LOWER_LEFT,
        LOWER_RIGHT
    };
    
    enum Size
    {
        SIZE_DEFAULT,
        SIZE_SMALL,
        SIZE_BIG
    };
    
    GUIHelper12(Style style);
    
    virtual ~GUIHelper12();
    
    IBKnobControl *CreateKnob(IGraphics *graphics,
                              float x, float y,
                              const char *bitmapFname, int nStates,
                              int paramIdx,
                              const char *tfBitmapFname,
                              const char *title = NULL,
                              Size titleSize = SIZE_DEFAULT,
                              ICaptionControl **caption = NULL,
                              bool createValue = true);

    ISVGKnobControl *CreateKnobSVG(IGraphics *graphics,
                                   float x, float y,
                                   float width, float height,
                                   const char *svgFname,
                                   int paramIdx,
                                   const char *tfBitmapFname,
                                   const char *title = NULL,
                                   Size titleSize = SIZE_DEFAULT,
                                   ICaptionControl **caption = NULL,
                                   bool createValue = true,
                                   const char *tooltip = NULL);
    
#ifdef IGRAPHICS_NANOVG
    GraphControl12 *CreateGraph(Plugin *plug, IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname, int paramIdx,
                                const char *overlayFname = NULL);
#endif // IGRAPHICS_NANOVG
    
    // 2 states or more
    IBLSwitchControl *CreateSwitchButton(IGraphics *graphics,
                                         float x, float y,
                                         const char *bitmapFname, int nStates,
                                         int paramIdx,
                                         const char *title = NULL,
                                         Size titleSize = SIZE_DEFAULT,
                                         bool clickToggleOff = true);
    
    // 2 states
    IBLSwitchControl *CreateToggleButton(IGraphics *graphics,
                                         float x, float y,
                                         const char *bitmapFname,
                                         int paramIdx,
                                         const char *title = NULL,
                                         Size titleSize = SIZE_DEFAULT,
                                         bool clickToggleOff = true,
                                         const char *tooltip = NULL);
    
    VumeterControl *CreateVumeter(IGraphics *graphics,
                                  float x, float y,
                                  const char *bitmapFname, int nStates,
                                  int paramIdx, const char *title = NULL,
                                  const char *tooltip = NULL);
    
    BLVumeterControl *CreateVumeterV(IGraphics *graphics,
                                     float x, float y,
                                     const char *bitmapFname,
                                     int paramIdx, const char *title = NULL);
    
    // Margins are in pixels
    BLVumeter2SidesControl *CreateVumeter2SidesV(IGraphics *graphics,
                                                 float x, float y,
                                                 const char *bitmapFname,
                                                 int paramIdx,
                                                 const char *title = NULL,
                                                 float marginMin = 0.0,
                                                 float marginMax = 0.0,
                                                 const char *tooltip = NULL);
    
    BLVumeterNeedleControl *CreateVumeterNeedleV(IGraphics *graphics,
                                                 float x, float y,
                                                 const char *bitmapFname,
                                                 int paramIdx,
                                                 const char *title = NULL);
    
    ITextControl *CreateText(IGraphics *graphics,
                             float x, float y,
                             const char *textStr, float size,
                             const char *font,
                             const IColor &color, EAlign align,
                             float offsetX = 0.0, float offsetY = 0.0);

    ITextButtonControl *CreateTextButton(IGraphics *graphics,
                                         float x, float y,
                                         int paramIdx,
                                         const char *textStr, float size,
                                         const char *font,
                                         const IColor &color, EAlign align,
                                         float offsetX = 0.0, float offsetY = 0.0,
                                         const IColor &borderColor = DEFAULT_BGCOLOR,
                                         float borderWidth = -1.0);
    
    IBitmapControl *CreateBitmap(IGraphics *graphics,
                                 float x, float y,
                                 const char *bitmapFname,
                                 //EAlign align = EAlign::Near,
                                 float offsetX = 0.0, float offsetY = 0.0,
                                 float *width = NULL, float *height = NULL);


    IXYPadControl *CreateXYPad(IGraphics *graphics,
                               float x, float y,
                               const char *trackBitmapFname,
                               const char *handleBitmapFname,
                               int xParamIdx, int yParamIdx,
                               float borderSize = 0.0,
                               bool reverseY = false,
                               const char *tooltip = NULL);

    // Circle around heads, for Spatializer
    // - (x, y) is the center
    // - minAngle and maxAngle are in degree [0, 360]
    ISpatializerHandleControl *
    CreateSpatializerHandle(IGraphics *graphics,
                            float x, float y, float rad,
                            float minAngle, float maxAngle,
                            bool reverseY,
                            const char *handleBitmapFname,
                            const char *tfBitmapFname,
                            int paramIdx,
                            float valueXOffset, float valueYOffset,
                            const char *tooltip = NULL);
    
    void CreateVersion(Plugin *plug, IGraphics *graphics,
                       const char *versionStr);
    
    void CreateLogo(Plugin *plug, IGraphics *graphics,
                    const char *logoFname, Position pos);
    
    void CreateLogoAnim(Plugin *plug, IGraphics *graphics,
                        const char *logoFname,
                        int nStates, Position pos);
    
    void CreatePlugName(Plugin *plug, IGraphics *graphics,
                        const char *plugNameFname,
                        Position pos = GUIHelper12::BOTTOM);

    
    void CreateHelpButton(Plugin *plug, IGraphics *graphics,
                          const char *bmpFname,
                          const char *manualFileName,
                          Position pos = GUIHelper12::BOTTOM,
                          const char *tooltip = NULL);
    
    void ShowHelp(Plugin *plug, IGraphics *graphics,
                  const char *manualFileName);
    
    void CreateDemoMessage(IGraphics *graphics);
    
    void CreateWatermarkMessage(IGraphics *graphics, const char *message,
                                IColor *color = NULL);
    
    IRadioButtonsControl *
        CreateRadioButtons(IGraphics *graphics,
                           float x, float y,
                           const char *bitmapFname,
                           int numButtons, float size, int paramIdx,
                           bool horizontalFlag, const char *title,
                           EAlign align = EAlign::Center,
                           EAlign titleAlign = EAlign::Center,
                           const char **radioLabels = NULL,
                           const char *tooltip = NULL);

    IRadioButtonsControlCustom *
        CreateRadioButtonsCustom(IGraphics *graphics,
                                 float x, float y,
                                 const char *bitmapFnames[], int numButtons,
                                 int bitmapNFrames,
                                 float size, int paramIdx,
                                 bool horizontalFlag, const char *tooltip);
    
    IControl *CreateGUIResizeButton(ResizeGUIPluginInterface *plug,
                                    IGraphics *graphics,
                                    float x, float y,
                                    const char *bitmapFname,
                                    int paramIdx,
                                    char *label,
                                    int guiSizeIdx,
                                    const char *tooltip = NULL);
    
    IControl *CreateRolloverButton(IGraphics *graphics,
                                   float x, float y,
                                   const char *bitmapFname,
                                   int paramIdx,
                                   char *label = NULL,
                                   bool toggleFlag = false,
                                   bool toggleOffFlag = true,
                                   bool disableDlbClick = false,
                                   const char *tooltip = NULL);

    
    void GetValueTextColor(IColor *valueTextColor) const;
    void GetValueTextColorLight(IColor *valueTextColor) const;
    
    // Graph
    void GetGraphAxisColor(int color[4]);
    void GetGraphAxisOverlayColor(int color[4]);
    void GetGraphAxisLabelColor(int color[4]);
    void GetGraphAxisLabelOverlayColor(int color[4]);
    float GetGraphAxisLineWidth();
    float GetGraphAxisLineWidthBold();
    
    void GetGraphCurveDescriptionColor(int color[4]);
    void GetGraphCurveColorBlue(int color[4]);
    void GetGraphCurveColorGreen(int color[4]);
    void GetGraphCurveColorPurple(int color[4]);
    void GetGraphCurveColorLightBlue(int color[4]);
    float GetGraphCurveFillAlpha();
    float GetGraphCurveFillAlphaLight();
    
    void GetGraphCurveColorGray(int color[4]);
    void GetGraphCurveColorRed(int color[4]);

    void GetGraphCurveColorLightRed(int color[4]);
    
    // For overlay curves
    void GetGraphCurveColorBlack(int color[4]);

    void GetGraphCurveDarkBlue(int color[4]);
    void GetGraphCurveDarkBlueRollover(int color[4]);
    
    void GetGraphCurveBlueEnabled(int color[4]);

    void GetGraphCurveColorWhiteNames(int color[4]);

    void GetGraphCurveColorOrange(int color[4]);
    // Fake Cyan is more beautiful than real Cyan
    void GetGraphCurveColorFakeCyan(int color[4]);
    
    static void ResetParameter(Plugin *plug, int paramIdx);
    
    // Refresh all the controls, from their values
    static void RefreshAllParameters(Plugin *plug, int numParams);

    static bool PromptForFile(Plugin *plug, EFileAction action, WDL_String *result,
                              char* dir = "", char* extensions = "");
    
    // NOTE: set it public, can be useful
    ITextControl *CreateTitle(IGraphics *graphics, float x, float y,
                              const char *title, Size size,
                              EAlign align = EAlign::Center);

    // public for Precedence
    ITextControl *CreateValueText(IGraphics *graphics,
                                  float x, float y,
                                  const char *textValue);

    ITabsBarControl *CreateTabsBar(IGraphics *graphics,
                                   float x, float y, float w, float h);

    // For the menu to appear
    // - the associated parameter must have been created as InitEnum()
    // - we must have calle pGraphics->AttachPopupMenuControl()
    // before before creating the menu
    ICaptionControl *CreateDropDownMenu(IGraphics *graphics,
                                        float x, float y,
                                        float width,
                                        int paramIdx,
                                        const char *title,
                                        Size titleSize = SIZE_DEFAULT,
                                        const char *tooltip = NULL);
    
    // Circle graph drawer
    void GetCircleGDCircleLineWidth(float *circleLineWidth);
    void GetCircleGDLinesWidth(float *linesWidth);
    void GetCircleGDLinesColor(IColor *linesColor);
    void GetCircleGDTextColor(IColor *textColor);
    void GetCircleGDOffsetX(int *x);
    void GetCircleGDOffsetY(int *y);

    void AttachToolTipControl(IGraphics *graphics);

    // Graph separator
    void GetGraphSeparatorColor(IColor *sepColor);
    
protected:
    bool GetManualFullPath(Plugin *plug, IGraphics *graphics,
                           const char *manualFileName,
                           char fullFileName[1024]);
        
    ITextControl *CreateText(IGraphics *graphics, float x, float y,
                             const char *textStr, const IText &text,
                             float offsetX, float offsetY,
                             EAlign align = EAlign::Center);
    
    ICaptionControl *CreateValue(IGraphics *graphics,
                                 float x, float y,
                                 const char *bitmapFname,
                                 int paramIdx);

    float GetTextWidth(IGraphics *graphics, const IText &text, const char *textStr);
    
    ITextControl *CreateRadioLabelText(IGraphics *graphics,
                                       float x, float y,
                                       const char *textStr,
                                       const IBitmap &bitmap,
                                       const IText &text,
                                       EAlign align = EAlign::Near);
    
    
    //
    Style mStyle;
    
    bool mCreateTitles;

    bool mCreatePlugName;
    bool mCreateLogo;
    bool mCreateHelpButton;
    
    float mTitleTextSize;
    float mTitleTextOffsetX;
    float mTitleTextOffsetY;
    IColor mTitleTextColor;
    IColor mHilightTextColor;
    
    Size mDefaultTitleSize;
    char *mTitleFont;
    
    float mTitleTextSizeBig;
    float mTitleTextOffsetXBig;
    float mTitleTextOffsetYBig;
    
    float mValueCaptionOffset;
    float mValueTextSize;
    float mValueTextOffsetX;
    float mValueTextOffsetY;
    IColor mValueTextColor;
    IColor mValueTextFGColor;
    IColor mValueTextBGColor;
    char *mValueTextFont;

    IColor mValueTextColorLight;
    
    Position mVersionPosition;
    
    float mVersionTextSize;
    float mVersionTextOffsetX;
    float mVersionTextOffsetY;
    IColor mVersionTextColor;
    char *mVersionTextFont;
    
    IColor mVumeterColor;
    IColor mVumeterNeedleColor;
    float mVumeterNeedleDepth;
    
    float mLogoOffsetX;
    float mLogoOffsetY;
    float mAnimLogoSpeed;
    
    float mPlugNameOffsetX;
    float mPlugNameOffsetY;
    
    float mTrialOffsetX;
    float mTrialOffsetY;
    
    float mHelpButtonOffsetX;
    float mHelpButtonOffsetY;
    
    float mDemoTextSize;
    float mDemoTextOffsetX;
    float mDemoTextOffsetY;
    IColor mDemoTextColor;
    char *mDemoFont;
    
    float mWatermarkTextSize;
    float mWatermarkTextOffsetX;
    float mWatermarkTextOffsetY;
    IColor mWatermarkTextColor;
    char *mWatermarkFont;

    bool mCreateRadioLabels;
    float mRadioLabelTextSize;
    float mRadioLabelTextOffsetX;
    IColor mRadioLabelTextColor;
    char *mLabelTextFont;
    
    float mButtonLabelTextOffsetX;
    float mButtonLabelTextOffsetY;
    
    //
    IColor mGraphAxisColor;
    IColor mGraphAxisOverlayColor;
    IColor mGraphAxisLabelColor;
    IColor mGraphAxisLabelOverlayColor;
    float mGraphAxisLineWidth;
    float mGraphAxisLineWidthBold;
    
    IColor mGraphCurveDescriptionColor;
    IColor mGraphCurveColorBlue;
    IColor mGraphCurveColorGreen;
    IColor mGraphCurveColorPurple;
    IColor mGraphCurveColorLightBlue;
    float mGraphCurveFillAlpha;
    float mGraphCurveFillAlphaLight;

    IColor mGraphCurveColorGray;
    IColor mGraphCurveColorRed;

    IColor mGraphCurveColorLightRed;
    
    IColor mGraphCurveColorBlack;

    IColor mGraphCurveColorDarkBlue;
    IColor mGraphCurveColorDarkBlueRollover;
    
    IColor mGraphCurveColorBlueEnabled;

    IColor mGraphCurveColorWhiteNames;

    IColor mGraphCurveColorOrange;
    IColor mGraphCurveColorFakeCyan;
    
    // Circle graph drawer
    float mCircleGDCircleLineWidth;
    float mCircleGDLinesWidth;
    IColor mCircleGDLinesColor;
    IColor mCircleGDTextColor;
    int mCircleGDOffsetX;
    int mCircleGDOffsetY;

    // Drop down menus
    float mMenuTextSize;
    IColor mMenuTextColor;
    IColor mMenuTextFGColor;
    IColor mMenuTextBGColor;
    IColor mMenuTextEntryFGColor;
    IColor mMenuTextEntryBGColor;
    char *mMenuTextFont;
    char *mMenuCaptionTextFont;
    IColor mMenuCellBGColor;
    IColor mMenuBorderColor;
    
    // Graph separator
    IColor mGraphSeparatorColor;

    float mTooltipTextSize;
};

#endif /* GUIHelper12_hpp */
