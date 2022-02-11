//
//  GUIHelper11.hpp
//  UST-macOS
//
//  Created by applematuer on 9/25/20.
//
//

#ifndef GUIHelper11_h
#define GUIHelper11_h

#include <IControls.h>

#include <BLVumeterControl.h>
#include <BLVumeterNeedleControl.h>
#include <ResizeGUIPluginInterface.h>

#include "IPlug_include_in_plug_hdr.h"


using namespace iplug;
using namespace iplug::igraphics;

class GraphControl11;
class IRadioButtonsControl;
class IGUIResizeButtonControl;

// TODO
#define VumeterControl IBKnobControl

class GUIHelper11
{
public:
    enum Style
    {
        STYLE_BLUELAB,
        STYLE_UST
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
    
    GUIHelper11(Style style);
    
    virtual ~GUIHelper11();
    
    IBKnobControl *CreateKnob(IGraphics *graphics,
                              float x, float y,
                              const char *bitmapFname, int nStates,
                              int paramIdx,
                              const char *tfBitmapFname,
                              const char *title = NULL,
                              Size titleSize = SIZE_DEFAULT,
                              ICaptionControl **caption = NULL);
    
#ifdef IGRAPHICS_NANOVG
    GraphControl11 *CreateGraph(Plugin *plug, IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname, int paramIdx,
                                int numCurves, int numPoints,
                                const char *overlayFname = NULL);
    
    // Must re-set the bitmaps after the plug window was closed
    // (because we get a new nanovg context, and bitmaps depend on it)
    void RefreshGraphGfx(IGraphics *graphics,
                         GraphControl11 *graph,
                         const char *bitmapFname,
                         const char *overlayFname);
    
#endif // IGRAPHICS_NANOVG
    
    // 2 states or more
    IBSwitchControl *CreateSwitchButton(IGraphics *graphics,
                                        float x, float y,
                                        const char *bitmapFname, int nStates,
                                        int paramIdx,
                                        const char *title = NULL,
                                        Size titleSize = SIZE_DEFAULT);
    
    // 2 states
    IBSwitchControl *CreateToggleButton(IGraphics *graphics,
                                        float x, float y,
                                        const char *bitmapFname,
                                        int paramIdx,
                                        const char *title = NULL,
                                        Size titleSize = SIZE_DEFAULT);
    
    VumeterControl *CreateVumeter(IGraphics *graphics,
                                  float x, float y,
                                  const char *bitmapFname, int nStates,
                                  int paramIdx, const char *title = NULL);
    
    BLVumeterControl *CreateVumeterV(IGraphics *graphics,
                                     float x, float y,
                                     const char *bitmapFname,
                                     int paramIdx, const char *title = NULL);
    
    BLVumeterNeedleControl *CreateVumeterNeedleV(IGraphics *graphics,
                                                 float x, float y,
                                                 const char *bitmapFname,
                                                 int paramIdx, const char *title = NULL);
    
    ITextControl *CreateText(IGraphics *graphics,
                             float x, float y,
                             const char *textStr, float size,
                             const char *font,
                             const IColor &color, EAlign align,
                             float offsetX = 0.0, float offsetY = 0.0);

    
    IBitmapControl *CreateBitmap(IGraphics *graphics,
                                 float x, float y,
                                 const char *bitmapFname,
                                 //EAlign align = EAlign::Near,
                                 float offsetX = 0.0, float offsetY = 0.0,
                                 float *width = NULL, float *height = NULL);

    
    void CreateVersion(Plugin *plug, IGraphics *graphics,
                       const char *versionStr, Position pos);
    
    void CreateLogo(Plugin *plug, IGraphics *graphics,
                    const char *logoFname, Position pos);
    
    void CreateLogoAnim(Plugin *plug, IGraphics *graphics,
                        const char *logoFname,
                        int nStates, Position pos);
    
    void CreatePlugName(Plugin *plug, IGraphics *graphics,
                        const char *plugNameFname, Position pos);

    
    void CreateHelpButton(Plugin *plug, IGraphics *graphics,
                          const char *bmpFname,
                          const char *manualFileName,
                          Position pos);

    void CreateDemoMessage(IGraphics *graphics);
    
    IRadioButtonsControl *
        CreateRadioButtons(IGraphics *graphics,
                           float x, float y,
                           const char *bitmapFname,
                           int numButtons, float size, int paramIdx,
                           bool horizontalFlag, const char *title,
                           EAlign align = EAlign::Center,
                           EAlign titleAlign = EAlign::Center,
                           const char **radioLabels = NULL);
    
    IControl *CreateGUIResizeButton(ResizeGUIPluginInterface *plug,
                                    IGraphics *graphics,
                                    float x, float y,
                                    const char *bitmapFname,
                                    int paramIdx,
                                    char *label,
                                    int guiSizeIdx);
    
    void GetValueTextColor(IColor *valueTextColor) const;
    
    static void ResetParameter(Plugin *plug, int paramIdx);
    
    // Refresh all the controls, from their values
    static void RefreshAllParameters(Plugin *plug, int numParams);
    
protected:
    void CreateTitle(IGraphics *graphics, float x, float y,
                     const char *title, Size size,
                     EAlign align = EAlign::Center);
    
    ITextControl *CreateText(IGraphics *graphics, float x, float y,
                             const char *textStr, const IText &text,
                             float offsetX, float offsetY,
                             EAlign align = EAlign::Center);
    
    ICaptionControl *CreateValue(IGraphics *graphics,
                                 float x, float y,
                                 const char *bitmapFname,
                                 int paramIdx);
    
    ITextControl *CreateValueText(IGraphics *graphics, float x, float y,
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
    
    float mTitleTextSize;
    float mTitleTextOffsetX;
    float mTitleTextOffsetY;
    IColor mTitleTextColor;
    
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
    
    float mRadioLabelTextSize;
    float mRadioLabelTextOffsetX;
    IColor mRadioLabelTextColor;
    char *mLabelTextFont;
    
    float mButtonLabelTextOffsetX;
    float mButtonLabelTextOffsetY;
};

#endif /* GUIHelper11_hpp */
