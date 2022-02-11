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

#include "IPlug_include_in_plug_hdr.h"


using namespace iplug;
using namespace iplug::igraphics;

class GraphControl11;

// TODO
#define VumeterControl IBKnobControl

class GUIHelper11
{
public:
    enum Style
    {
        STYLE_UST
    };
    
    enum Position
    {
        TOP,
        BOTTOM,
        LOWER_LEFT,
        LOWER_RIGHT
    };
    
    GUIHelper11(Style style);
    
    virtual ~GUIHelper11();
    
    IBKnobControl *CreateKnob(IGraphics *graphics,
                              float x, float y,
                              const char *bitmapFname, int nStates,
                              int paramIdx, const char *title = NULL,
                              ICaptionControl **caption = NULL);
    
    GraphControl11 *CreateGraph(Plugin *plug, IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname, int paramIdx,
                                int numCurves, int numPoints,
                                const char *overlayFname = NULL);

    IBSwitchControl *CreateSwitchButton(IGraphics *graphics,
                                        float x, float y,
                                        const char *bitmapFname, int nStates,
                                        int paramIdx, const char *title = NULL);
    
    IBSwitchControl *CreateToggleButton(IGraphics *graphics,
                                        float x, float y,
                                        const char *bitmapFname,
                                        int paramIdx, const char *title = NULL);
    
    VumeterControl *CreateVumeter(IGraphics *graphics,
                                  float x, float y,
                                  const char *bitmapFname, int nStates,
                                  int paramIdx, const char *title = NULL);
    
    ITextControl *CreateText(IGraphics *graphics,
                             float x, float y,
                             const char *textStr, float size,
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
    
    // NOTE: not sure it is still useful
    static void UpdateText(Plugin *plug, int paramIdx);
    
protected:
    void CreateTitle(IGraphics *graphics, float x, float y, const char *title);
    
    ITextControl *CreateText(IGraphics *graphics, float x, float y,
                             const char *textStr, const IText &text,
                             float offsetX, float offsetY);
    
    ITextControl *CreateValue(IGraphics *graphics, float x, float y,
                              int paramIdx,
                              ICaptionControl **caption);

    
    ITextControl *CreateValueText(IGraphics *graphics, float x, float y,
                                  int paramIdx);

    float GetTextWidth(IGraphics *graphics, const IText &text, const char *textStr);

    
    
    //
    Style mStyle;
    
    bool mCreateTitles;
    
    float mTitleTextSize;
    float mTitleTextOffsetX;
    float mTitleTextOffsetY;
    IColor mTitleTextColor;
    
    float mValueCaptionOffset;
    float mValueTextSize;
    float mValueTextOffsetX;
    float mValueTextOffsetY;
    IColor mValueTextColor;
    IColor mValueTextFGColor;
    IColor mValueTextBGColor;
    
    float mVersionTextSize;
    float mVersionTextOffset;
    IColor mVersionTextColor;
};

#endif /* GUIHelper11_hpp */
