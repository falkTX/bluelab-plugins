//
//  GUIHelper11.cpp
//  UST-macOS
//
//  Created by applematuer on 9/25/20.
//
//

#include <lice.h>

#include <GraphControl11.h>

#include "GUIHelper11.h"

// Text
//#define TITLE_TEXT_FONT "Tahoma"
#define TITLE_TEXT_FONT "font"
//#define TITLE_TEXT_FONT NULL

#define VALUE_TEXT_FONT "font"

// Version text
#define VERSION_TEXT_FONT "font"

#define TEXTFIELD_BITMAP "textfield.png"


GUIHelper11::GUIHelper11(Style style)
{
    mStyle = style;
    
    if (style == STYLE_UST)
    {
        mCreateTitles = false;
        
        mTitleTextSize = 13.0;
        mTitleTextOffsetX = 0.0;
        mTitleTextOffsetY = -18.0;
        mTitleTextColor = IColor(255, 110, 110, 110);
        
        mValueCaptionOffset = 0.0;
        mValueTextSize = 14.0;
        mValueTextOffsetX = -1.0;
        mValueTextOffsetY = 1.0;
        mValueTextColor = IColor(255, 240, 240, 255);
        mValueTextFGColor = mValueTextColor; //IColor(255, 255, 255, 255);
        mValueTextBGColor = IColor(0/*255*/, 0, 0, 0);
        
        mVersionTextSize = 12.0;
        mVersionTextOffset = 3.0;
        mVersionTextColor = IColor(255, 110 , 110);
    }
}

GUIHelper11::~GUIHelper11() {}

IBKnobControl *
GUIHelper11::CreateKnob(IGraphics *graphics,
                        float x, float y,
                        const char *bitmapFname, int nStates,
                        int paramIdx, const char *title,
                        ICaptionControl **caption)
{
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, nStates);
                       
    IBKnobControl *knob = new IBKnobControl(x, y, bitmap, paramIdx);
    graphics->AttachControl(knob);
    
    if (mCreateTitles)
    {
        // Title
        if ((title != NULL) && (strlen(title) > 0))
        {
            CreateTitle(graphics, x + bitmap.W()/2, y, title);
        }
    }
    
    CreateValue(graphics, x - bitmap.W()/2, y + bitmap.H()/bitmap.N(),
                paramIdx, caption);
    
    return knob;
}

GraphControl11 *
GUIHelper11::CreateGraph(Plugin *plug, IGraphics *graphics,
                         float x, float y,
                         const char *bitmapFname, int paramIdx,
                         int numCurves, int numPoints,
                         const char *overlayFname)
{
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname);
    
    const char *resPath = graphics->GetSharedResourcesSubPath();
    char fontPath[MAX_PATH];
    sprintf(fontPath, "%s/%s", resPath, "font.ttf");
    
    IRECT rect(x, y, x + bitmap.W(), y + bitmap.H());
    GraphControl11 *graph = new GraphControl11(plug, graphics, rect, paramIdx,
                                               numCurves, numPoints, fontPath);
    
    graph->SetBackgroundImage(bitmap);
    
    if (overlayFname != NULL)
    {
        char bmpPath[MAX_PATH];
        sprintf(bmpPath, "%s/%s", resPath, overlayFname);
        
        IBitmap overlayBitmap = graphics->LoadBitmap(bmpPath);
        
        graph->SetOverlayImage(overlayBitmap);
    }
    
    graphics->AttachControl(graph);
    
    return graph;
}

IBSwitchControl *
GUIHelper11::CreateSwitchButton(IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname, int nStates,
                                int paramIdx, const char *title)
{
    IBitmap bitmap = graphics->LoadBitmap(bitmapFname, nStates);
    
    IBSwitchControl *button = new IBSwitchControl(x, y, bitmap, paramIdx);
    graphics->AttachControl(button);
    
    return button;
}

IBSwitchControl *
GUIHelper11::CreateToggleButton(IGraphics *graphics,
                                float x, float y,
                                const char *bitmapFname,
                                int paramIdx, const char *title)
{
    /*IBitmap bitmap = graphics->LoadBitmap(bitmapFname, 2);
    IBSwitchControl *button = new IBSwitchControl(x, y, bitmap, paramIdx);
    graphics->AttachControl(button);*/
    
    IBSwitchControl *button = CreateSwitchButton(graphics, x, y,
                                                 bitmapFname, 2,
                                                 paramIdx, title);
    
    return button;
}

VumeterControl *
GUIHelper11::CreateVumeter(IGraphics *graphics,
                           float x, float y,
                           const char *bitmapFname, int nStates,
                           int paramIdx, const char *title)
{
    // TODO
    VumeterControl *vumeter = CreateKnob(graphics,
                                         x, y,
                                         bitmapFname, nStates,
                                         paramIdx, title);
    
    graphics->AttachControl(vumeter);
    
    return vumeter;
}

ITextControl *
GUIHelper11::CreateText(IGraphics *graphics, float x, float y,
                        const char *textStr, float size,
                        const IColor &color, EAlign align,
                        float offsetX, float offsetY)
{
    IText text(size, color, TITLE_TEXT_FONT, align);
    
    float width = GetTextWidth(graphics, text, textStr);
    
    IRECT rect(x + offsetX, y + offsetY,
               (x + offsetX + width), (y  + size + offsetY));

    
    ITextControl *textControl = new ITextControl(rect, textStr, text);
    
    graphics->AttachControl(textControl);
    
    return textControl;
}

IBitmapControl *
GUIHelper11::CreateBitmap(IGraphics *graphics,
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
        *height = bitmap.H()/bitmap.N();;
    
    //if (align == EAlign::Center)
    //    x -= bitmap.W()/2;
    
    IBitmapControl *result = new IBitmapControl(x + offsetX, y + offsetY, bitmap);
    result->SetInteractionDisabled(true);
    graphics->AttachControl(result);
    
    return result;
}

void
GUIHelper11::CreateVersion(Plugin *plug, IGraphics *graphics,
                           const char *versionStr, Position pos)
{
    // Lower left corner
    char versionStr0[256];
    sprintf(versionStr0, "v%s", versionStr);
    
    float x  = 0.0;
    float y  = 0.0;
    
    EAlign textAlign = EAlign::Near;
    
    if (pos == LOWER_LEFT)
    {
        x = 0;
        y = graphics->Height() - mVersionTextSize - mVersionTextOffset;
        
        textAlign = EAlign::Near;
    }
    
    
    if (pos == LOWER_RIGHT)
    {
        int strWidth = (int)(strlen(versionStr0)*mVersionTextSize);
        
        x = graphics->Width() - strWidth/2 - mVersionTextOffset + mVersionTextSize/2;
        x += mVersionTextSize/2;
        
        y = graphics->Height() - mVersionTextOffset - mVersionTextSize;
        
        textAlign = EAlign::Near;
        
#if UST_PLUGIN
        x -= 86; // HACK
#endif
    }
    
    if (pos == BOTTOM)
    {
        x = graphics->Width()/2;
        y = graphics->Height() - mVersionTextSize - mVersionTextOffset;
        
        //y -= TRIAL_Y_OFFSET;
        
        textAlign = EAlign::Center;
    }
    
    IText versionText(mVersionTextSize, mVersionTextColor, VERSION_TEXT_FONT, textAlign);
    
    IRECT rect(x - mVersionTextSize/2, y, x + mVersionTextSize/2, y);
    
    ITextControl *textControl = new ITextControl(rect, versionStr0, versionText);
    
    graphics->AttachControl(textControl);
}

void
GUIHelper11::UpdateText(Plugin *plug, int paramIdx)
{
    // bl-iplug2: makes infinite loop with OnParamChange()
    //double normValue = plug->GetParam(paramIdx)->Value();
    //plug->SetParameterValue(paramIdx, normValue);
}

void
GUIHelper11::CreateTitle(IGraphics *graphics, float x, float y, const char *title)
{
    IText text(mTitleTextSize, mTitleTextColor, TITLE_TEXT_FONT, EAlign::Center);
    float width = GetTextWidth(graphics, text, title);
    
    x -= width/2.0;
    
    ITextControl *control = CreateText(graphics, x, y,
                                       title, text,
                                       mTitleTextOffsetX, mTitleTextOffsetY);
    
    control->SetInteractionDisabled(true);
    
    graphics->AttachControl(control);
}

float
GUIHelper11::GetTextWidth(IGraphics *graphics, const IText &text, const char *textStr)
{
    // Measure
    IRECT textRect;
    graphics->MeasureText(text, (char *)textStr, textRect);

    float width = textRect.W();

    return width;
}

ITextControl *
GUIHelper11::CreateText(IGraphics *graphics, float x, float y,
                        const char *textStr, const IText &text,
                        float offsetX, float offsetY)
{
    float width = GetTextWidth(graphics, text, textStr);
    
    IRECT rect(x + offsetX, y + offsetY,
               (x + offsetX + width), (y  + mTitleTextSize + offsetY));
    
    ITextControl *textControl = new ITextControl(rect, textStr, text);
    
    graphics->AttachControl(textControl);
    
    return textControl;
}

ITextControl *
GUIHelper11::CreateValue(IGraphics *graphics, float x, float y,
                         int paramIdx,
                         ICaptionControl **caption)
{
    // Bitmap
    float width;
    float height;
    CreateBitmap(graphics, x, y, TEXTFIELD_BITMAP,
                 //EAlign::Near,
                 mValueTextOffsetX, mValueTextOffsetY,
                 &width, &height);
                 //EAlign::Center);

    // Value
    IRECT bounds(x + mValueTextOffsetX,
                 y + mValueTextOffsetY,
                 x + width + mValueTextOffsetX,
                 y + height + mValueTextOffsetY);
    
    IText text(mValueTextSize, mValueTextColor, VALUE_TEXT_FONT,
               EAlign::Center, EVAlign::Middle, 0.0,
               mValueTextBGColor, mValueTextFGColor);
    ICaptionControl *caption0 = new ICaptionControl(bounds, paramIdx, text,
                                                    mValueTextBGColor);
    caption0->DisablePrompt(false); // Here is the magic !
    graphics->AttachControl(caption0);
    
    if (caption != NULL)
        *caption = caption0;
    
    // Text
    //ITextControl *textControl = CreateValueText(graphics, x, y, paramIdx);
    
    //return textControl;
    return NULL;
}

/*ITextControl *
GUIHelper11::CreateValueText(IGraphics *graphics,
                             float x, float y, int paramIdx)
{
    // Text
    IText text(mValueTextSize, mValueTextColor, VALUE_TEXT_FONT,
               EAlign::Center, EVAlign::Middle, 0.0,
               mValueTextBGColor, mValueTextFGColor);
    ITextControl *textControl = CreateText(graphics, x, y,
                                           "test", text,
                                           mValueTextOffsetX, mValueTextOffsetY);
    //mValueTextBGColor
    graphics->AttachControl(textControl);
    
    return textControl;
}
*/
