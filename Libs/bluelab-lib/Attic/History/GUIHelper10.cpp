//
//  GUIHelper10.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/06/17.
//
//

#include <Utils.h>

#include <Debug.h>

#include "GUIHelper10.h"

#if USE_GRAPH_OGL
#include "GraphControl10.h"
#endif

#define USE_SHADOWS 0

#define USE_DIFFUSE 0

// Do not display diffuse and shadows
// (avoids some flickering of the labels,
// due to old diffuse or shadows with that overlap things)
// (e.g EQHack, firt mode label)
#define BLACK_GUI_HACK 1

// For UST
//
#define UST_PLUGIN              1

#if UST_PLUGIN

#define TEXT_VALUES_BLACK_RECTANGLE 0

#define DISPLAY_TITLE_TEXT          0
#define UST_TEXT_FIELD_WIDTH        73
//#define UST_TEXT_FIELD_HEIGHT       16

// Text style (re-define)
#define VALUE_TEXT_SIZE 12 //9 //14
#define VALUE_TEXT_OFFSET 3

// "before"
//#define VALUE_TEXT_COLOR_R 73
//#define VALUE_TEXT_COLOR_G 80
//#define VALUE_TEXT_COLOR_B 106

#if 0
// Proposition "after"
#define VALUE_TEXT_COLOR_R 123
#define VALUE_TEXT_COLOR_G 135
#define VALUE_TEXT_COLOR_B 176
#endif

#if 0
// Mofied to be less "agressive"
#define VALUE_TEXT_COLOR_R 135
#define VALUE_TEXT_COLOR_G 135
#define VALUE_TEXT_COLOR_B 135
#endif

#if 1
// Mofied to be darker than labels
#define VALUE_TEXT_COLOR_R 100
#define VALUE_TEXT_COLOR_G 100
#define VALUE_TEXT_COLOR_B 100
#endif


// UST
#define USE_GRAPH_OVERLAY 1

#endif

// FIX: when using GHOST_OPTIM_GL (mac),
// png graph background have red and blue
// swapped when rendered directly by GL
#define GHOST_OPTIM_GL_FIX_GRAPH_BACKGROUND 1

// From GuiHelpers9
//
// Done during GHOST_OPTIM_GL
// FIX: fix the region size of the label text fields
// (voids flickering of label sometimes)
#define FIX_TEXT_LABEL_RECT 1

// From GuiHelpers9
//
// Added during Infra release
// Avoid trial text bleed when GUI is refreshed
// => compute correctly the text rectangle
#define FIX_TRIAL_TEXT_BLEED 1


IColor GUIHelper10::mValueTextColor = IColor(255,
                                             VALUE_TEXT_COLOR_R,
                                             VALUE_TEXT_COLOR_G,
                                             VALUE_TEXT_COLOR_B);

GUIHelper10::GUIHelper10()
{
    mTitleTextColor = IColor(255,
                             TITLE_TEXT_COLOR_R,
                             TITLE_TEXT_COLOR_G,
                             TITLE_TEXT_COLOR_B);
    
    mRadioLabelTextColor = IColor(255,
                                  RADIO_LABEL_TEXT_COLOR_R,
                                  RADIO_LABEL_TEXT_COLOR_G,
                                  RADIO_LABEL_TEXT_COLOR_B);
    
    mVersionTextColor = IColor(255,
                               VERSION_TEXT_COLOR_R,
                               VERSION_TEXT_COLOR_G,
                               VERSION_TEXT_COLOR_B);
    
    mHilightTextColor = IColor(255,
                               HILIGHT_TEXT_COLOR_R,
                               HILIGHT_TEXT_COLOR_G,
                               HILIGHT_TEXT_COLOR_B);
    
    mEditTextColor = IColor(255,
                            EDIT_TEXT_COLOR_R,
                            EDIT_TEXT_COLOR_G,
                            EDIT_TEXT_COLOR_B);
}


GUIHelper10::~GUIHelper10()
{
#if GUI_OBJECTS_SORTING
    mBackObjects.clear();
    mWidgets.clear();
    mTexts.clear();
    mDiffuse.clear();
    mShadows.clear();
    mTriggers.clear();
#endif
}

void
GUIHelper10::AddAllObjects(IGraphics *graphics)
{
#if GUI_OBJECTS_SORTING
    for (int i = 0; i < mBackObjects.size(); i++)
    {
        IControl *control = mBackObjects[i];
        graphics->AttachControl(control);
    }
    
#if 0
    for (int i = 0; i < mTexts.size(); i++)
    {
        IControl *control = mTexts[i];
        graphics->AttachControl(control);
    }
#endif
    
#if !BLACK_GUI_HACK
    for (int i = 0; i < mDiffuse.size(); i++)
    {
        IControl *control = mDiffuse[i];
        graphics->AttachControl(control);
    }
#endif
    
    for (int i = 0; i < mWidgets.size(); i++)
    {
        IControl *control = mWidgets[i];
        graphics->AttachControl(control);
    }
    
#if 1 // Add texts after widget, for buttons labels
    for (int i = 0; i < mTexts.size(); i++)
    {
        IControl *control = mTexts[i];
        graphics->AttachControl(control);
    }
#endif
    
#if !BLACK_GUI_HACK
    for (int i = 0; i < mShadows.size(); i++)
    {
        IControl *control = mShadows[i];
        graphics->AttachControl(control);
    }
#endif
    
    for (int i = 0; i < mTriggers.size(); i++)
    {
        IControl *control = mTriggers[i];
        graphics->AttachControl(control);
    }
#endif
}

ITextControl *
GUIHelper10::CreateValueText(IPlug *plug, IGraphics *graphics,
                             int width, int height, int x, int y,
                             const char *title)
{
    IText text(VALUE_TEXT_SIZE, &mValueTextColor,
               VALUE_TEXT_FONT, IText::kStyleBold, IText::kAlignCenter,
               0, IText::kQualityDefault);
    
    IRECT rect(x, y + height + VALUE_TEXT_OFFSET,
               (x + width),
               (y + height + VALUE_TEXT_OFFSET + VALUE_TEXT_SIZE));
    
    ITextControl *textControl = new ITextControl(plug, rect, &text, title);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

IKnobMultiControl *
GUIHelper10::CreateKnob(IPlug *plug, IGraphics *graphics,
                       int bmpId, const char *bmpFn, int bmpFrames,
                       int x, int y, int param, const char *title,
                       int shadowId, const char *shadowFn,
                       int diffuseId, const char *diffuseFn,
                       int textFieldId, const char *textFieldFn,
                       Size size,
                       IText **ioValueText, bool addValueText,
                       ITextControl **ioValueTextControl, int yoffset,
                       bool doubleTextField, BL_FLOAT sensivity)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap, kVertical,
                                                       sensivity*DEFAULT_GEARING);
    
    CreateKnobInternal(plug, graphics, &bitmap, control, x, y, param, title,
                       shadowId, shadowFn, diffuseId, diffuseFn,
                       textFieldId, textFieldFn, size, ioValueText, addValueText,
                       ioValueTextControl, yoffset, doubleTextField);
    
    return control;
}

IKnobMultiControl *
GUIHelper10::CreateKnob2(IPlug *plug, IGraphics *graphics,
                        int bmpId, const char *bmpFn, int bmpFrames,
                        int x, int y, int param, const char *title,
                        int shadowId, const char *shadowFn,
                        int diffuseId, const char *diffuseFn,
                        int textFieldId, const char *textFieldFn,
                        IText **ioValueText, bool addValueText,
                        ITextControl **ioValueTextControl, int yoffset,
                        bool doubleTextField, BL_FLOAT sensivity)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap, kVertical,
                                                       sensivity*DEFAULT_GEARING);
    
    CreateKnobInternal2(plug, graphics, &bitmap, control, x, y, param, title,
                        shadowId, shadowFn, diffuseId, diffuseFn,
                        textFieldId, textFieldFn, ioValueText, addValueText,
                        ioValueTextControl, yoffset, doubleTextField);
    
    return control;
}

IKnobMultiControl *
GUIHelper10::CreateKnob4(IPlug *plug, IGraphics *graphics,
                        int bmpId, const char *bmpFn, int bmpFrames,
                        int x, int y, int param, const char *title,
                        int shadowId, const char *shadowFn,
                        int diffuseId, const char *diffuseFn,
                        int textFieldId, const char *textFieldFn,
                        IText **ioValueText, bool addValueText,
                        ITextControl **ioValueTextControl, int yoffset,
                        bool doubleTextField, BL_FLOAT sensivity)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap, kVertical,
                                                       sensivity*DEFAULT_GEARING);
    
    CreateKnobInternal4(plug, graphics, &bitmap, control, x, y, param, title,
                        shadowId, shadowFn, diffuseId, diffuseFn,
                        textFieldId, textFieldFn, ioValueText, addValueText,
                        ioValueTextControl, yoffset, doubleTextField);
    
    return control;
}


IKnobMultiControl *
GUIHelper10::CreateEnumKnob(IPlug *plug, IGraphics *graphics,
                           int bmpId, const char *bmpFn, int bmpFrames,
                           int x, int y, int param, BL_FLOAT sensivity,
                           const char *title,
                           int shadowId, const char *shadowFn,
                           int diffuseId, const char *diffuseFn,
                           int textFieldId, const char *textFieldFn,
                           IText **ioValueText, bool addValueText,
                           ITextControl **ioValueTextControl, int yoffset)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap,
                                                       kVertical, sensivity*DEFAULT_GEARING);
    
    
    CreateKnobInternal(plug, graphics, &bitmap, control, x, y, param, title,
                       shadowId, shadowFn, diffuseId, diffuseFn,
                       textFieldId, textFieldFn, SIZE_STANDARD,
                       ioValueText, addValueText,
                       ioValueTextControl, yoffset);
    
    return control;
}

IKnobMultiControl *
GUIHelper10::CreateKnob3(IPlug *plug, IGraphics *graphics,
                        int bmpId, const char *bmpFn, int bmpFrames,
                        int x, int y, int param, const char *title,
                        int shadowId, const char *shadowFn,
                        int diffuseId, const char *diffuseFn,
                        int textFieldId, const char *textFieldFn,
                        Size size,
                        IText **ioValueText, bool addValueText,
                        ICaptionControl **ioValueTextControl, int yoffset,
                        bool doubleTextField, BL_FLOAT sensivity)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap, kVertical,
                                                       sensivity*DEFAULT_GEARING);
    
    CreateKnobInternal3(plug, graphics, &bitmap, control, x, y, param, title,
                        shadowId, shadowFn, diffuseId, diffuseFn,
                        textFieldId, textFieldFn, size, ioValueText, addValueText,
                        ioValueTextControl, yoffset, doubleTextField);
    
    return control;
}

IKnobMultiControl *
GUIHelper10::CreateKnob3Ex(IPlug *plug, IGraphics *graphics,
                           int bmpId, const char *bmpFn, int bmpFrames,
                           int x, int y, int param, const char *title,
                           int shadowId, const char *shadowFn,
                           int diffuseId, const char *diffuseFn,
                           int textFieldId, const char *textFieldFn,
                           Size size, int titleYOffset,
                           IText **ioValueText, bool addValueText,
                           ICaptionControl **ioValueTextControl, int textValueYoffset,
                           bool doubleTextField, BL_FLOAT sensivity,
                           EDirection dragDirection)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap, dragDirection,
                                                       sensivity*DEFAULT_GEARING);
    
    CreateKnobInternal3Ex(plug, graphics, &bitmap, control, x, y, param, title,
                          shadowId, shadowFn, diffuseId, diffuseFn,
                          textFieldId, textFieldFn, size, titleYOffset, ioValueText, addValueText,
                          ioValueTextControl, textValueYoffset, doubleTextField);
    
    return control;
}

IKnobMultiControl *
GUIHelper10::CreateKnob5(IPlug *plug, IGraphics *graphics,
                        int bmpId, const char *bmpFn, int bmpFrames,
                        int x, int y, int param, const char *title,
                        int shadowId, const char *shadowFn,
                        int diffuseId, const char *diffuseFn,
                        int textFieldId, const char *textFieldFn,
                        Size size,
                        IText **ioValueText, bool addValueText,
                        ICaptionControl **ioValueTextControl, int yoffset,
                        bool doubleTextField, BL_FLOAT sensivity)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap, kVertical,
                                                       sensivity*DEFAULT_GEARING);
    
    CreateKnobInternal5(plug, graphics, &bitmap, control, x, y, param, title,
                        shadowId, shadowFn, diffuseId, diffuseFn,
                        textFieldId, textFieldFn, size, ioValueText, addValueText,
                        ioValueTextControl, yoffset, doubleTextField);
    
    return control;
}

IRadioButtonsControl *
GUIHelper10::CreateRadioButtons(IPlug *plug, IGraphics *graphics,
                                int bmpId, const char *bmpFn, int bmpFrames,
                                int x, int y, int numButtons, int size, int param,
                                bool horizontalFlag, const char *title,
                                int diffuseId, const char *diffuseFn,
                                IText::EAlign align,
                                IText::EAlign titleAlign,
                                const char **radioLabels, int numRadioLabels,
                                BL_FLOAT titleTextOffsetY)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    // Title
    AddTitleText(plug, graphics, &bitmap, title, x, y, titleAlign, SIZE_STANDARD, titleTextOffsetY);
    
    // Radio labels
    if (numRadioLabels > 0)
    {
        if (!horizontalFlag)
        {
            int labelX = x;
            if (align == IText::kAlignFar)
                labelX = labelX - bitmap.W - RADIO_LABEL_TEXT_OFFSET;
            
            if (align == IText::kAlignNear)
            {
                labelX = labelX + bitmap.W + RADIO_LABEL_TEXT_OFFSET;
            }
            
            int spaceBetween = (size - numButtons*bitmap.H/2)/(numButtons - 1);
            int stepSize = bitmap.H/2 + spaceBetween;
            
            for (int i = 0; i < numRadioLabels; i++)
            {
                // new
                IText labelText(RADIO_LABEL_TEXT_SIZE,
                                &mRadioLabelTextColor,
                                RADIO_LABEL_TEXT_FONT,
                                IText::kStyleBold,
                                align,
                                0, IText::kQualityDefault);
                
                int labelY = y + i*stepSize;
                const char *labelStr = radioLabels[i];
                AddRadioLabelText(plug, graphics, &bitmap, &labelText, labelStr, labelX, labelY);
            }
        }
        else
        {
            // NOTE: not really tested
            
            int labelY = x;
            //if (titleAlign == IText::kAlignFar)
            //    labelX = labelX - bitmap.W - RADIO_LABEL_TEXT_OFFSET;
            
            if (align == IText::kAlignNear)
            {
                // TODO: implement this
            }
            
            int spaceBetween = (size - numButtons*bitmap.H/2)/(numButtons - 1);
            int stepSize = bitmap.H/2 + spaceBetween;
            
            for (int i = 0; i < numRadioLabels; i++)
            {
                // new
                IText labelText(RADIO_LABEL_TEXT_SIZE,
                                &mRadioLabelTextColor,
                                RADIO_LABEL_TEXT_FONT,
                                IText::kStyleBold,
                                align,
                                0, IText::kQualityDefault);
                
                int labelX = x + i*stepSize;
                const char *labelStr = radioLabels[i];
                AddRadioLabelText(plug, graphics, &bitmap, &labelText, labelStr, labelX, labelY);
            }
        }
    }
    
    // Buttons
    IRECT rect(x, y, x + size, y + bitmap.H);
    EDirection direction = kHorizontal;
    if (!horizontalFlag)
    {
        rect = IRECT(x, y, x + bitmap.W, y + size);
        
        direction = kVertical;
    }
    
    IRadioButtonsControlExt *radioButtons =
                    new IRadioButtonsControlExt(plug, rect,
                                                param, numButtons, &bitmap, direction);
    
    // Hard coded because we have margins in the interface in Inkscape
    //graphics->AttachControl(radioButtons);
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(radioButtons);
#else
    graphics->AttachControl(radioButtons);
#endif
    
#if USE_DIFFUSE
    if ((diffuseId >= 0) && (diffuseFn != NULL))
    {
        WDL_TypedBuf<IRECT> rects;
        radioButtons->GetRects(&rects);
        
        vector<IBitmapControlExt *> diffuseBmps;

        
        CreateMultiDiffuse(plug, graphics, diffuseId, diffuseFn,
                           rects, &bitmap, bmpFrames, &diffuseBmps);
        
        radioButtons->SetDiffuseBitmapList(diffuseBmps);
    }
#endif
    
    return radioButtons;
}

IToggleButtonControl *
GUIHelper10::CreateToggleButton(IPlug *plug, IGraphics *graphics,
                               int bmpId, const char *bmpFn, int bmpFrames,
                               int x, int y, int param, const char *title,
                               int diffuseId, const char *diffuseFn,
                               Size size)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    // Title
    AddTitleText(plug, graphics, &bitmap, title, x, y, IText::kAlignCenter, size);
    
    IToggleButtonControl *control = new IToggleButtonControl(plug, x, y, param, &bitmap);
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
#if 0
    IText text(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
               IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
    
    
    int bmpWidth = bitmap.W;
    int width = VALUE_TEXT_SIZE*strlen(title);
    
    IRECT rect(x - width/2 + bmpWidth/2,
               (y -  VALUE_TEXT_SIZE - VALUE_TEXT_OFFSET),
               x + width/2 + bmpWidth/2, y - VALUE_TEXT_OFFSET);
    
    ITextControl *textControl = new ITextControl(plug, rect, &text, title);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif

#endif
    
    return control;
}

//IToggleButtonControl *
IControl *
GUIHelper10::CreateToggleButtonEx(IPlug *plug, IGraphics *graphics,
                                  int bmpId, const char *bmpFn, int bmpFrames,
                                  int x, int y, int param, const char *title,
                                  int diffuseId, const char *diffuseFn,
                                  Size size,
                                  int textOffsetX, int textOffsetY,
                                  TitlePosition position, bool oneWay)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
#if DISPLAY_TITLE_TEXT
    // Title
    if (position == TITLE_TOP)
    {
        AddTitleText(plug, graphics, &bitmap, title, x, y + textOffsetY, IText::kAlignCenter, size);
    }
    else if (position == TITLE_RIGHT)
    {
        AddTitleTextRight(plug, graphics, &bitmap, title,
                          x + textOffsetX, y + textOffsetY,
                          IText::kAlignNear, size);
    }
#endif
    
    IControl *control = NULL;
    if (!oneWay)
    {
        control = new IToggleButtonControl(plug, x, y, param, &bitmap);
    }
    else
    {
        control = new IOneWayToggleButtonControl(plug, x, y, param, &bitmap);
    }
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
#if 0
    IText text(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
               IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
    
    
    int bmpWidth = bitmap.W;
    int width = VALUE_TEXT_SIZE*strlen(title);
    
    IRECT rect(x - width/2 + bmpWidth/2,
               (y -  VALUE_TEXT_SIZE - VALUE_TEXT_OFFSET),
               x + width/2 + bmpWidth/2, y - VALUE_TEXT_OFFSET);
    
    ITextControl *textControl = new ITextControl(plug, rect, &text, title);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
#endif
    
    return control;
}

VuMeterControl *
GUIHelper10::CreateVuMeter(IPlug *plug, IGraphics *graphics,
                          int bmpId, const char *bmpFn, int bmpFrames,
                          int x, int y, int param, const char *title, bool alignLeft,
                          bool decreaseWhenIdle,
                          bool informHostParamChange)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    VuMeterControl *control = new VuMeterControl(plug, x, y, param, &bitmap,
                                                 IChannelBlend::kBlendNone, decreaseWhenIdle,
                                                 informHostParamChange);

#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    enum IText::EAlign align = alignLeft ? IText::kAlignNear : IText::kAlignFar;
    
    //IText text(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
    //           IText::kStyleNormal, align, 0, IText::kQualityDefault);
    
    int titleTextSize = TITLE_TEXT_SIZE;
    IText text(titleTextSize, &mTitleTextColor, TITLE_TEXT_FONT,
               //IText::kStyleNormal
               IText::kStyleBold, align,
               0, IText::kQualityDefault);
    
    
    int bmpWidth = bitmap.W;
    int width = VALUE_TEXT_SIZE*strlen(title);
    
    int yy = y + bitmap.frameHeight() + VALUE_TEXT_OFFSET;
    int xx = alignLeft ? x : x + bmpWidth - VALUE_TEXT_SIZE*strlen(title);
    
    IRECT rect(xx, yy, xx + width, yy);
    
    ITextControl *textControl = new ITextControl(plug, rect, &text, title);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return control;
}

#if USE_GRAPH_OGL
GraphControl10 *
GUIHelper10::CreateGraph10(IPlug *plug, IGraphics *graphics,
                           int param, const char *paramName,
                           int bmpId, const char *bmpFn, int bmpFrames,
                           int x , int y,
                           int numCurves, int numPoints,
                           int shadowsId, const char *shadowsFn,
                           IBitmap **oBitmap, bool useBgBitmap)
{
#if GHOST_OPTIM_GL
    
#if GHOST_OPTIM_GL_FIX_GRAPH_BACKGROUND
    LICE_SetLoadPNGForGL(true);
#endif
    
#endif
    
    plug->GetParam(param)->InitInt(paramName, 0, 0, 1);
    
    // Added for BL-Waves
    // Failed auval validation (Logic Pro X) => parmeter did not retain value 
    plug->GetParam(param)->SetIsMeta(true);
    
    IBitmap graphBmp = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    if (oBitmap != NULL)
        *oBitmap = &graphBmp;

    IBitmap *graphBackground = NULL;
    if (useBgBitmap)
        graphBackground = &graphBmp;
    
    GraphControl10 *control = CreateGraph10(plug, graphics, param, x, y,
                                            graphBmp.W, graphBmp.H, numCurves, numPoints,
                                            graphBackground);

    if ((shadowsId >= 0) && (shadowsFn != NULL))
    {
#if USE_SHADOWS
        CreateShadow(plug, graphics, shadowsId, shadowsFn, x, y, &graphBmp, graphBmp.N);
#endif
        
#if USE_GRAPH_OVERLAY
        IBitmap *graphOverlay = NULL;
        CreateGraphOverlay(plug, graphics, shadowsId, shadowsFn, x, y, &graphBmp, graphBmp.N,
                           &graphOverlay);
        if (graphOverlay != NULL)
        {
            LICE_IBitmap *liceBmp = (LICE_IBitmap *)graphOverlay->mData;
            
            control->SetOverlayImage(liceBmp);
        }
#endif
    }

#if GHOST_OPTIM_GL
    
#if GHOST_OPTIM_GL_FIX_GRAPH_BACKGROUND
    LICE_SetLoadPNGForGL(false);
#endif
    
#endif
    
    return control;
}

GraphControl10 *
GUIHelper10::CreateGraph10(IPlug *plug, IGraphics *graphics, int param,
                         int x , int y, int width, int height,
                         int numCurves, int numPoints, IBitmap *bgBitmap)
{
    IRECT rect(x, y, x + width, y + height);
    
    // Get the path to the font
    WDL_String pPath;
    graphics->ResourcePath(&pPath, "font.ttf");

    GraphControl10 *control = new GraphControl10(plug, graphics, rect, param, numCurves, numPoints, pPath.Get());

    if (bgBitmap != NULL)
    {
        LICE_IBitmap *liceBmp = (LICE_IBitmap *)bgBitmap->mData;
        
        //LICE_IBitmap *liceBmp = new LICE_MemBitmap();
        //LICE_Copy(liceBmp, (LICE_IBitmap *)bgBitmap->mData);
        
        control->SetBackgroundImage(liceBmp);
    }
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif

    return control;
}

#endif

IControl *
GUIHelper10::CreateFileSelector(IPlug *plug, IGraphics *graphics,
                               int bmpId, const char *bmpFn, int bmpFrames,
                               int x, int y, int param,
                               EFileAction action,
                               char *label,
                               char* dir, char* extensions,
                               bool promptForFile)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IRECT pR;
    pR.L = x;
    pR.T = y;
    pR.R = x + bitmap.W;
    pR.B = y + bitmap.H/bmpFrames; //
    
    IControl *control = NULL;
    if (promptForFile)
    {        
        // We will open a file selection when clicked
        control = new IFileSelectorControl(plug, pR, param, &bitmap,
                                           action, dir, extensions);
        
        // Create rollover button AFTER
        //CreateRolloverButton(plug, graphics,
        //                     bmpId, bmpFn, bmpFrames,
        //                     x, y, param, label);

        
#if GUI_OBJECTS_SORTING
        mWidgets.push_back(control);
#else
        graphics->AttachControl(control);
#endif
        
        // Create rollover button AFTER added file selector
        // (to enable rollover effect)
        //
        // And create it as container, to transmit mouse down to the file selector
        CreateRolloverButtonContainer(plug, graphics,
                                      bmpId, bmpFn, bmpFrames,
                                      x, y, param, label, control, false);
        
        // But avoid catching mouse clicks
        // Mouse clicks must be catched by file selector
        //rolloverButton->SetInteractionDisabled(true);
    }
    else
    {
        // We will open a file selection when clicked
        control = new IButtonControl(plug, x, y, param, &bitmap);
        //control = new ICaptionControl(plug, pR, param, &bitmap);
        
#if GUI_OBJECTS_SORTING
        mWidgets/*mBackObjects*/.push_back(control);
#else
        graphics->AttachControl(control);
#endif
        
        // Add the label
        AddTitleText(plug, graphics, &bitmap, label,
                     x + FILE_SELECTOR_TEXT_OFFSET_X,
                     y + bitmap.H*1.5 + FILE_SELECTOR_TEXT_OFFSET_Y,
                     IText::kAlignNear);
    }
    
    return control;
}

IControl *
GUIHelper10::CreateButton(IPlug *plug, IGraphics *graphics,
                         int bmpId, const char *bmpFn, int bmpFrames,
                         int x, int y, int param,
                         char *label)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IRECT pR;
    pR.L = x;
    pR.T = y;
    pR.R = x + bitmap.W;
    pR.B = y + bitmap.H;
    
    IControl *control = new IButtonControl(plug, x, y, param, &bitmap);
  
#if GUI_OBJECTS_SORTING
        mWidgets/*mBackObjects*/.push_back(control);
#else
        graphics->AttachControl(control);
#endif
    
    // Add the label
    AddTitleText(plug, graphics, &bitmap, label,
                 x + BUTTON_LABEL_TEXT_OFFSET_X,
                 y + bitmap.H*1.5 + BUTTON_LABEL_TEXT_OFFSET_Y, //
                 IText::kAlignNear);
    
    return control;
}

IControl *
GUIHelper10::CreateGUIResizeButton(IPlug *plug, IGraphics *graphics,
                                  int bmpId, const char *bmpFn, int bmpFrames,
                                  int x, int y, int param,
                                  int resizeWidth, int resizeHeight,
                                  char *label)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IRECT pR;
    pR.L = x;
    pR.T = y;
    pR.R = x + bitmap.W;
    pR.B = y + bitmap.H;
    
#if 0 // Origin
    IControl *control =
            new IGUIResizeButtonControl(plug, x, y, param, &bitmap,
                                        resizeWidth, resizeHeight);
#endif
    
#if 1 // With rollover
    IControl *control =
    new IGUIResizeButtonControl2(plug, x, y, param, &bitmap,
                                 resizeWidth, resizeHeight);
#endif

    
#if GUI_OBJECTS_SORTING
    mWidgets/*mBackObjects*/.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    // Add the label
    AddTitleText(plug, graphics, &bitmap, label,
                 x + BUTTON_LABEL_TEXT_OFFSET_X,
                 y + bitmap.H*1.5/((BL_FLOAT)bmpFrames) + BUTTON_LABEL_TEXT_OFFSET_Y,
                 IText::kAlignNear);
    
    return control;
}

IControl *
GUIHelper10::CreateRolloverButton(IPlug *plug, IGraphics *graphics,
                                 int bmpId, const char *bmpFn, int bmpFrames,
                                 int x, int y, int param,
                                 char *label, bool toggleFlag)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IRolloverButtonControl *control =
        new IRolloverButtonControl(plug, x, y, param, &bitmap, toggleFlag);
    
#if GUI_OBJECTS_SORTING
    mWidgets/*mBackObjects*/.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    // Add the label
    ITextControl *text = AddTitleText(plug, graphics, &bitmap, label,
                                      x + BUTTON_LABEL_TEXT_OFFSET_X,
                                      y + bitmap.H*1.5/((BL_FLOAT)bmpFrames) + BUTTON_LABEL_TEXT_OFFSET_Y, //
                                      IText::kAlignNear);
    
    control->LinkText(text, mTitleTextColor, mHilightTextColor);
    
    return control;
}

IControl *
GUIHelper10::CreateRolloverButtonContainer(IPlug *plug, IGraphics *graphics,
                                          int bmpId, const char *bmpFn, int bmpFrames,
                                          int x, int y, int param,
                                          char *label, IControl *child, bool toggleFlag)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IRolloverContainerControl *control =
    new IRolloverContainerControl(plug, x, y, param, &bitmap, child, toggleFlag);
    
#if GUI_OBJECTS_SORTING
    mWidgets/*mBackObjects*/.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    // Add the label
    ITextControl *text = AddTitleText(plug, graphics, &bitmap, label,
                                      x + BUTTON_LABEL_TEXT_OFFSET_X,
                                      y + bitmap.H*1.5/((BL_FLOAT)bmpFrames) + BUTTON_LABEL_TEXT_OFFSET_Y, //
                                      IText::kAlignNear);
    
    control->LinkText(text, mTitleTextColor, mHilightTextColor);
    
    return control;
}

void
GUIHelper10::CreateVersion(IPlug *plug, IGraphics *graphics,
                          const char *versionStr, Position pos)
{
    // Lower left corner
    char versionStrV[256];
    sprintf(versionStrV, "v%s", versionStr);
 
    int x  = 0;
    int y  = 0;

    IText::EAlign textAlign = IText::kAlignNear;
    
    if (pos == LOWER_LEFT)
    {
        x = 0;
        y = graphics->Height() - VERSION_TEXT_SIZE - VERSION_TEXT_OFFSET;
    
        textAlign = IText::kAlignNear;
    }

    
    if (pos == LOWER_RIGHT)
    {
        int strWidth = strlen(versionStrV)*VERSION_TEXT_SIZE;
        
        x = graphics->Width() - strWidth/2 - VERSION_TEXT_OFFSET + VERSION_TEXT_SIZE/2;
        x += VERSION_TEXT_SIZE/2;
        
        y = graphics->Height() - VERSION_TEXT_SIZE - VERSION_TEXT_OFFSET;
    
        textAlign = IText::kAlignNear;
        
#if UST_PLUGIN
        x -= 86; // HACK
#endif
    }

    if (pos == BOTTOM)
    {
        x = graphics->Width()/2;
        y = graphics->Height() - VERSION_TEXT_SIZE - VERSION_TEXT_OFFSET;
    
        //y -= TRIAL_Y_OFFSET;
        
        textAlign = IText::kAlignCenter;
    }
    
    IText versionText(VERSION_TEXT_SIZE, &mVersionTextColor, VERSION_TEXT_FONT,
                      IText::kStyleNormal, textAlign);
    
    AddVersionText(plug, graphics, &versionText, versionStrV, x, y);
}

IBitmapControl *
GUIHelper10::CreateBitmap(IPlug *plug, IGraphics *graphics,
                         int x, int y,
                         int bmpId, const char *bmpFn,
                         int shadowsId, const char *shadowsFn,
                         IBitmap **ioBmp)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
    if (ioBmp != NULL)
        *ioBmp = &bmp;
    
    IBitmapControl *control = new IBitmapControl(plug, x, y, -1, &bmp);
    control->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    if ((shadowsId >= 0) && (shadowsFn != NULL))
    {
#if USE_SHADOWS
        CreateShadow(plug, graphics, shadowsId, shadowsFn, x, y, &bmp, bmp.N);
#endif
    }
    
    return control;
}

IBitmapControl *
GUIHelper10::CreateShadows(IPlug *plug, IGraphics *graphics, int bmpId, const char *bmpFn)
{
    IBitmapControl *shadowControl = NULL;
    
#if USE_SHADOWS
    IBitmap shadowBmp = graphics->LoadIBitmap(bmpId, bmpFn);
    shadowControl = new IBitmapControl(plug, 0, 0, -1, &shadowBmp);
    shadowControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mShadows.push_back(shadowControl);
#else
    graphics->AttachControl(shadowControl);
#endif
    
#endif
    
    return shadowControl;
}

// Static logo
IBitmapControl *
GUIHelper10::CreateLogo(IPlug *plug, IGraphics *graphics,
                       int bmpId, const char *bmpFn, Position pos)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
#if 0
    // Lower left corner
    int x = 0;
    int y = graphics->Height() - bmp.H;
#endif
    
    int x = 0;
    int y = 0;
    if (pos == TOP)
    {
        // Upper right corner
        x = graphics->Width() - bmp.W;
        y = 0;
    }

    if (pos == BOTTOM)
    {
        // Lower right corner
        x = graphics->Width() - bmp.W;
        y = graphics->Height() - bmp.H - LOGO_OFFSET_Y;
    }

    
    IBitmapControl *control = new IBitmapControl(plug, x, y, -1, &bmp);
    control->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

// Animated logo
IBitmapControl *
GUIHelper10::CreateLogoAnim(IPlug *plug, IGraphics *graphics,
                           int bmpId, const char *bmpFn, Position pos)
{
#define NUM_FRAMES 31
    
    // How many cycles per second
#define SPEED 0.5 //1.0
    
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn, NUM_FRAMES);
    
#if 0
    // Lower left corner
    int x = 0;
    int y = graphics->Height() - bmp.H;
#endif
    
    int x = 0;
    int y = 0;
    if (pos == TOP)
    {
        // Upper right corner
        x = graphics->Width() - bmp.W;
        y = 0;
    }
    
    if (pos == BOTTOM)
    {
        // Lower right corner
        x = graphics->Width() - bmp.W;
        y = graphics->Height() - bmp.H/bmp.N - LOGO_OFFSET_Y;
    }
    
    
    //IBitmapControlAnim *control = new IBitmapControlAnim(plug, x, y, -1, &bmp, SPEED, true, true);
    IBitmapControlAnim2 *control = new IBitmapControlAnim2(plug, x, y, -1, &bmp, SPEED, true, true);
    control->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

IBitmapControl *
GUIHelper10::CreatePlugName(IPlug *plug, IGraphics *graphics,
                           int bmpId, const char *bmpFn, Position pos)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);

    int x = 0;
    int y = 0;
    
    if (pos == TOP)
    {
        // Upper left corner
        x = PLUG_NAME_OFFSET_X;
        y = PLUG_NAME_OFFSET_Y;
    }

    if (pos == BOTTOM)
    {
        // Lower left corner
        x = PLUG_NAME_OFFSET_X;
        y = graphics->Height() - bmp.H - PLUG_NAME_OFFSET_Y;
        
        y -= TRIAL_Y_OFFSET;
    }

    
    IBitmapControl *control = new IBitmapControl(plug, x, y, -1, &bmp);
    control->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

IBitmapControl *
GUIHelper10::CreateHelpButton(IPlug *plug, IGraphics *graphics,
                             int bmpId, const char *bmpFn,
                             int manResId, const char *fileName,
                             Position pos)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
    int x = 0;
    int y = 0;
#if 0 // TODO
    if (pos == TOP)
    {
        // Upper right corner
        x = graphics->Width() - bmp.W;
        y = 0;
    }
#endif
    
    if (pos == BOTTOM)
    {
        // Lower right corner
        x = graphics->Width() - bmp.W - HELP_BUTTON_X_OFFSET;
        y = graphics->Height() - bmp.H - HELP_BUTTON_Y_OFFSET;
    }
    
    char fullFileName[1024];

#ifndef WIN32 // Mac
    WDL_String wdlResDir;
    graphics->GetResourceDir(&wdlResDir);
    const char *resDir = wdlResDir.Get();
    
    // Remove the "manual" directory from the path
	fileName = Utils::GetFileName(fileName);
    
    sprintf(fullFileName, "%s/%s", resDir, fileName);
#else
    // On windows, we must load the resource from dll, save it to the temp file
	// before re-opning it
	IGraphicsWin *graphWin = (IGraphicsWin *)graphics;
	void *resBuf;
	long resSize;
	bool res = graphWin->LoadWindowsResource(manResId, "PDF",
											 &resBuf, &resSize);
	if (!res)
		return NULL;

	TCHAR tempPathBuffer[MAX_PATH];
	GetTempPath(MAX_PATH, tempPathBuffer);

	// Remove the "manual" directory from the path
	fileName = Utils::GetFileName(fileName);

	sprintf(fullFileName, "%s%s", tempPathBuffer, fileName);

	FILE *file = fopen(fullFileName, "wb");
	fwrite(resBuf, 1, resSize, file);
	fclose(file);
#endif
    
    IBitmapControl *control = new IHelpButtonControl(plug, x, y, -1, &bmp,
                                                     fullFileName);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

ITextControl *
GUIHelper10::AddValueText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                        IText *text, int x, int y, int param, IBitmap *tfBitmap)
{
    //int width = bitmap->W;
    int height = bitmap->frameHeight();
 
    int tfWidth = tfBitmap->W;
    x = x - tfBitmap->W/2 + bitmap->W/2;
    
    IRECT rect(x, y + height + VALUE_TEXT_OFFSET + VALUE_TEXT_OFFSET2,
               (x + tfWidth),
               (y + height + VALUE_TEXT_OFFSET + VALUE_TEXT_OFFSET2 + VALUE_TEXT_SIZE));
    
    ITextControl *textControl = new ICaptionControl(plug, rect, param, text, "");
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ICaptionControl *
GUIHelper10::AddValueText3(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                          IText *text, int x, int y, int param, IBitmap *tfBitmap)
{
    //int width = bitmap->W;
    int height = bitmap->frameHeight();
    
#if TEXT_VALUES_BLACK_RECTANGLE
    int tfWidth = tfBitmap->W;
    x = x - tfBitmap->W/2 + bitmap->W/2;
#else
    int tfWidth = UST_TEXT_FIELD_WIDTH;
    x = x - UST_TEXT_FIELD_WIDTH/2 + bitmap->W/2;
#endif
    
    IRECT rect(x, y + height + VALUE_TEXT_OFFSET + VALUE_TEXT_OFFSET2,
               (x + tfWidth),
               (y + height + VALUE_TEXT_OFFSET + VALUE_TEXT_OFFSET2 + VALUE_TEXT_SIZE));
    
    ICaptionControl *textControl = new ICaptionControl(plug, rect, param, text, "");

    // Here is the magic !
    textControl->DisablePrompt(false);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

// UST
// NOT TESTED
int
GUIHelper10::MeasureText(IGraphics *graphics, const char *text, Size size)
{
    int titleTextSize = TITLE_TEXT_SIZE;
    if (size == SIZE_SMALL)
    {
        titleTextSize *= SMALL_SIZE_RATIO;
    }
    
    if (size == SIZE_TINY)
    {
        titleTextSize *= TINY_SIZE_RATIO;
    }
    
    if (size == SIZE_HUGE)
    {
        titleTextSize *= HUGE_SIZE_RATIO;
    }
    
    IText titleText(titleTextSize, NULL, TITLE_TEXT_FONT,
                    IText::kStyleBold, IText::kAlignCenter,
                    0, IText::kQualityDefault);
    
    bool measure = true;
    IRECT textRect;
    graphics->DrawIText(&titleText, (char *)text, &textRect, measure);
    
    int width = textRect.R - textRect.L;
    
    return width;
}

ITextControl *
GUIHelper10::AddTitleText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                          const char *title, int x, int y, IText::EAlign align,
                          Size size, BL_FLOAT titleTextOffsetY)
{
    int titleTextSize = TITLE_TEXT_SIZE;
    //int titleTextOffsetY = TITLE_TEXT_OFFSET_Y;
    
    if (size == SIZE_SMALL)
    {
        titleTextSize *= SMALL_SIZE_RATIO;
        titleTextOffsetY *= SMALL_SIZE_RATIO;
    }
    
    if (size == SIZE_TINY)
    {
        titleTextSize *= TINY_SIZE_RATIO;
        titleTextOffsetY *= TINY_SIZE_RATIO;
    }
    
    if (size == SIZE_HUGE)
    {
        titleTextSize *= HUGE_SIZE_RATIO;
        titleTextOffsetY *= HUGE_SIZE_RATIO;
    }
    
    // new
    IText titleText(titleTextSize, &mTitleTextColor, TITLE_TEXT_FONT,
                    //IText::kStyleNormal
                    IText::kStyleBold, align,
                    0, IText::kQualityDefault);
    
#if !FIX_TEXT_LABEL_RECT
    int width = bitmap->W;
    IRECT rect(x, y + titleTextOffsetY - titleTextSize,
               (x + width), (y  + titleTextOffsetY /*- TITLE_TEXT_SIZE*/));
#else
    // Measure the real text size in pixels
    bool measure = true;
    IRECT textRect;
    graphics->DrawIText(&titleText, (char *)title, &textRect, measure);
    
    int width = textRect.R - textRect.L;
    
    // Adjust for aligne center
    int dx = 0;
    if (align == IText::kAlignCenter)
    {
        int bmpWidth = bitmap->W;
        dx = bmpWidth/2 - width/2;
    }
    
    // Adrjust for align far
    if (align == IText::kAlignFar)
    {
        int bmpWidth = bitmap->W;
        dx = -width + bmpWidth;
    }
    
    IRECT rect(x + dx, y + titleTextOffsetY - titleTextSize,
               (x + width) + dx, (y  + titleTextOffsetY));
#endif
    
    ITextControl *textControl = new ITextControl(plug, rect, &titleText, title);
    
    textControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelper10::AddTitleTextRight(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                               const char *title, int x, int y, IText::EAlign align,
                               Size size, BL_FLOAT titleTextOffsetX, BL_FLOAT titleTextOffsetY)
{
    int titleTextSize = TITLE_TEXT_SIZE;
    //int titleTextOffsetY = TITLE_TEXT_OFFSET_Y;
    
    if (size == SIZE_SMALL)
    {
        titleTextSize *= SMALL_SIZE_RATIO;
        titleTextOffsetX *= SMALL_SIZE_RATIO;
    }
    
    if (size == SIZE_TINY)
    {
        titleTextSize *= TINY_SIZE_RATIO;
        titleTextOffsetX *= TINY_SIZE_RATIO;
    }
    
    if (size == SIZE_HUGE)
    {
        titleTextSize *= HUGE_SIZE_RATIO;
        titleTextOffsetX *= HUGE_SIZE_RATIO;
    }
    
    // new
    IText titleText(titleTextSize, &mTitleTextColor, TITLE_TEXT_FONT,
                    //IText::kStyleNormal
                    IText::kStyleBold, align,
                    0, IText::kQualityDefault);
    
    int width = bitmap->W;
    int height = bitmap->H;
    int nFrames = bitmap->N;
    IRECT rect(x + width,
               y - titleTextSize/2 + (height/2)/nFrames + titleTextOffsetY,
               (x + width + titleTextOffsetX),
               y + titleTextSize/2 + (height/2)/nFrames + titleTextOffsetY);
    
    ITextControl *textControl = new ITextControl(plug, rect, &titleText, title);
    
    textControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelper10::AddRadioLabelText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                               IText *text, const char *labelStr, int x, int y,
                               IText::EAlign align)
{
#if !FIX_TEXT_LABEL_RECT
    int width = bitmap->W;
    IRECT rect(x, y, (x + width), y);
#else
    // Measure the real text size in pixels
    bool measure = true;
    IRECT textRect;
    graphics->DrawIText(text, (char *)labelStr, &textRect, measure);
    
    int width = textRect.R - textRect.L;
    
    // Adjust for aligne center
    int dx = 0;
    
    // Adrjust for align far
    if (align == IText::kAlignFar)
    {
        int bmpWidth = bitmap->W;
        dx = -width + bmpWidth;
    }
    
    IRECT rect(x + dx, y,
               (x + width) + dx, y + text->mSize);
#endif
    
    ITextControl *textControl = new ITextControl(plug, rect, text, labelStr);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelper10::AddVersionText(IPlug *plug, IGraphics *graphics,
                            IText *text, const char *versionStr, int x, int y)
{
    // The following is not centered
    //IRECT rect(x, y, x + VERSION_TEXT_SIZE, y);
    
    IRECT rect(x-VERSION_TEXT_SIZE/2, y, x + VERSION_TEXT_SIZE/2, y);
    
    ITextControl *textControl = new ITextControl(plug, rect, text, versionStr);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

void
GUIHelper10::AddTrigger(ITriggerControl *trigger)
{
#if GUI_OBJECTS_SORTING
    mTriggers.push_back(trigger);
#else
    graphics->AttachControl(trigger);
#endif
}

void
GUIHelper10::UpdateText(IPlugBase *plug, int paramIdx)
{
    if (plug->GetGUI())
        plug->GetGUI()->SetParameterFromPlug(paramIdx, plug->GetParam(paramIdx)->Value(), false);
    
    // Comment the following line otherwise it will make an infinite loop under Protools
    
    //inform host of new normalized value
    //plug->InformHostOfParamChange(paramIdx, (plug->GetParam(paramIdx)->Value()));
}

void
GUIHelper10::ResetParameter(IPlugBase *plug, int paramIdx)
{
    if (plug->GetGUI())
    {
        plug->GetParam(paramIdx)->SetToDefault();
        
        plug->GetGUI()->SetParameterFromPlug(paramIdx, plug->GetParam(paramIdx)->Value(), false);
        
        // TEST-NIKO
        //plug->GetGUI()->SetParameterFromGUI(paramIdx, plug->GetParam(paramIdx)->GetNormalized());
    }
}

bool
GUIHelper10::PromptForFile(IPlugBase *plug, EFileAction action, WDL_String *result,
                          char* dir, char* extensions)
{
    WDL_String file;
    IFileSelectorControl::EFileSelectorState state = IFileSelectorControl::kFSNone;
    
    if (plug && plug->GetGUI())
    {
        WDL_String wdlDir(dir);
        
        state = IFileSelectorControl::kFSSelecting;
        plug->GetGUI()->PromptForFile(&file, action, &wdlDir, extensions);
        state = IFileSelectorControl::kFSDone;
    }
    
    result->Set(file.Get());
    
    if (result->GetLength() == 0)
        return false;
    
    return true;
}

IBitmapControl *
GUIHelper10::CreateShadow(IPlug *plug, IGraphics *graphics,
                          int bmpId, const char *bmpFn,
                          int x, int y, IBitmap *objectBitmap,
                          int nObjectFrames)
{
    IBitmap shadowBmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
    int centerObjectX = x + objectBitmap->W/2;
    int centerObjectY = y + (objectBitmap->H/nObjectFrames)/2;
    
    int shadowX = centerObjectX - shadowBmp.W/2;
    int shadowY = centerObjectY - shadowBmp.H/2;
    
    IBitmapControl *shadowControl = new IBitmapControl(plug, shadowX, shadowY, -1, &shadowBmp);
    shadowControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mShadows.push_back(shadowControl);
#else
    graphics->AttachControl(shadowControl);
#endif

    return shadowControl;
}

void
GUIHelper10::CreateGraphOverlay(IPlug *plug, IGraphics *graphics,
                                int bmpId, const char *bmpFn,
                                int x, int y, IBitmap *objectBitmap,
                                int nObjectFrames, IBitmap **overlayBmp)
{
    *overlayBmp = NULL;
    
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
    if (bmp.W != objectBitmap->W)
    {
        return;
    }
    
    if (bmp.H != objectBitmap->H)
    {
        return;
    }
    
    IBitmap *result = new IBitmap();
    *result = bmp;
    
    *overlayBmp = result;
}

IBitmapControl *
GUIHelper10::CreateDiffuse(IPlug *plug, IGraphics *graphics,
                           int bmpId, const char *bmpFn,
                           int x, int y, IBitmap *objectBitmap,
                           int nObjectFrames)
{
    IBitmap diffuseBmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
    int centerObjectX = x + objectBitmap->W/2;
    int centerObjectY = y + (objectBitmap->H/nObjectFrames)/2;
    
    int diffuseX = centerObjectX - diffuseBmp.W/2;
    int diffuseY = centerObjectY - diffuseBmp.H/2;
    
    IBitmapControl *diffuseControl =
            new IBitmapControl(plug, diffuseX, diffuseY, -1, &diffuseBmp);
    
    diffuseControl->SetBlendMethod(IChannelBlend::kBlendMul, 1.0);
    
    diffuseControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mDiffuse.push_back(diffuseControl);
#else
    graphics->AttachControl(diffuseControl);
#endif
    
    return diffuseControl;
}

bool
GUIHelper10::CreateMultiDiffuse(IPlug *plug, IGraphics *graphics,
                                int bmpId, const char *bmpFn,
                                const WDL_TypedBuf<IRECT> &rects, IBitmap *objectBitmap,
                                int nObjectFrames,
                                vector<IBitmapControlExt *> *diffuseBmps)
{
    IBitmap diffuseBmp = graphics->LoadIBitmap(bmpId, bmpFn, nObjectFrames);
    
    for (int i = 0; i < rects.GetSize(); i++)
    {
        IRECT rect = rects.Get()[i];
        
        int centerX = (rect.L + rect.R)/2;
        int centerY = (rect.T + rect.B)/2;
    
        int diffuseX = centerX - diffuseBmp.W/2;
        int diffuseY = centerY - (diffuseBmp.H/nObjectFrames)/2;
    
        IBitmapControlExt *diffuseControl =
            new IBitmapControlExt(plug, diffuseX, diffuseY, -1, &diffuseBmp);
    
        diffuseControl->SetBlendMethod(IChannelBlend::kBlendMul, 1.0);
    
        diffuseControl->SetInteractionDisabled(true);
    
        diffuseBmps->push_back(diffuseControl);
        
#if GUI_OBJECTS_SORTING
        mDiffuse.push_back(diffuseControl);
#else
        graphics->AttachControl(diffuseControl);
#endif
    }
    
    return true;
}

IBitmapControl *
GUIHelper10::CreateTextField(IPlug *plug, IGraphics *graphics,
                            int bmpId, const char *bmpFn,
                            int x, int y, IBitmap *objectBitmap,
                            int nObjectFrames, IBitmap *outBmp)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
    BL_FLOAT resultX = x;
    BL_FLOAT resultY = y - VALUE_TEXT_OFFSET/2;
    
    if (objectBitmap != NULL)
    {
        int centerObjectX = x + objectBitmap->W/2;
        int centerObjectY = y + (objectBitmap->H/nObjectFrames)/2;
    
        resultX = centerObjectX - bmp.W/2;
        resultY = centerObjectY + (objectBitmap->H/nObjectFrames)/2 + bmp.H - VALUE_TEXT_OFFSET/2;
    }
    
    IBitmapControl *control = new IBitmapControl(plug, resultX, resultY, -1, &bmp);
    control->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    if (outBmp != NULL)
        *outBmp = bmp;
    
    return control;
}

int
GUIHelper10::GetTextOffset()
{
    return VALUE_TEXT_SIZE + VALUE_TEXT_OFFSET2;
}

ITextControl *
GUIHelper10::CreateText(IPlug *plug, IGraphics *graphics,
                       const char *textStr,
                       int x, int y,
                       int size, char *font,
                       IText::EStyle style,
                       const IColor &color,
                       IText::EAlign align)
{    
    IText text(size, &color, font, style, align);
    
#if !FIX_TRIAL_TEXT_BLEED
    int len = strlen(textStr);
    IRECT rect(x, y, x + size*len, y + size);
#else
    bool measure = true;
    IRECT textRect;
    graphics->DrawIText(&text, (char *)textStr, &textRect, measure);
    
    IRECT rect(x, y, x + textRect.R - textRect.L, y + size);
#endif
    
    ITextControl *textControl = new ITextControl(plug, rect, &text, textStr);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelper10::CreateTitleText(IPlug *plug, IGraphics *graphics, int width,
                            const char *title, int x, int y, IText::EAlign align,
                            Size size, BL_FLOAT titleTextOffsetY)
{
    int titleTextSize = TITLE_TEXT_SIZE;
    //int titleTextOffsetY = TITLE_TEXT_OFFSET_Y;
    
    if (size == SIZE_SMALL)
    {
        titleTextSize *= SMALL_SIZE_RATIO;
        titleTextOffsetY *= SMALL_SIZE_RATIO;
    }
    
    // new
    IText titleText(titleTextSize, &mTitleTextColor, TITLE_TEXT_FONT,
                    //IText::kStyleNormal
                    IText::kStyleBold, align,
                    0, IText::kQualityDefault);
    
    IRECT rect(x, y + titleTextOffsetY - titleTextSize,
               (x + width), (y  + titleTextOffsetY /*- TITLE_TEXT_SIZE*/));
    
    
    ITextControl *textControl = new ITextControl(plug, rect, &titleText, title);
    
    textControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelper10::CreateUSTText(IPlug *plug, IGraphics *graphics, int textSize, int width,
                           const IColor &color,
                           const char *str, int x, int y, IText::EAlign align,
                           BL_FLOAT textOffsetX, BL_FLOAT textOffsetY)
{
    // new
    IText text(textSize, &color, TITLE_TEXT_FONT,
                    //IText::kStyleNormal
                    IText::kStyleBold, align,
                    0, IText::kQualityDefault);
    
    IRECT rect(x + textOffsetX, y + textOffsetY,
               (x + textOffsetX + width), (y  + textSize + textOffsetY));
    
    
    ITextControl *textControl = new ITextControl(plug, rect, &text, str);
    
    textControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

// UST
IControl *
GUIHelper10::CreateSwitch(IPlug *plug, IGraphics *graphics,
                          int bmpId, const char *bmpFn, int bmpFrames,
                          int x, int y, int param)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    
    IControl *control = new ISwitchControl(plug, x, y, param, &bitmap);
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

void
GUIHelper10::AddControl(IControl *control)
{
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
}

void
GUIHelper10::UpdateFrequencyAxis(GraphControl10 *graph,
                                int nativeBufferSize,
                                BL_FLOAT sampleRate,
                                bool highResLogCurves)
{
#if USE_GRAPH_OGL
    if (graph == NULL)
        return;
    
    // Like Logic
#define NUM_AXIS_DATA 10
    char *AXIS_DATA [NUM_AXIS_DATA][2] =
    {
        { "20.0", "" }, //Do not display 20Hz, because the text overlaps the "80dB" text
        { "50.0", "50Hz" },
        { "100.0", "100Hz" },
        { "200.0", "200Hz" },
        { "500.0", "500Hz" },
        { "1000.0", "1KHz" },
        { "2000.0", "2KHz" },
        { "5000.0", "5KHz" },
        { "10000.0", "10KHz" },
        { "20000.0", "" }
    };
    
    // Strange when not using variable buffer size, but works like that
    int bufferSize = Utils::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    
    BL_FLOAT minHzValue;
    BL_FLOAT maxHzValue;
    Utils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                   bufferSize, sampleRate);
    
    int offsetI = 0;
#if 1 // Set to 1 if AddHAxis0 (to avoid label near overlap)
    
    // HACK: Adjust the first values
    // (otherwise, they will be half displayed and half out of screen)
    // Compute the index of the first value we will display
    // (skip one value each time we multiply freq by 2)
    BL_FLOAT offset = std::log((BL_FLOAT)(maxHzValue/22050.0))/std::log((BL_FLOAT)2.0);
    if (offset < 0.0)
        offset = 0.0;
    offset = bl_round(offset);
    offsetI = offset;
    
    // Set the first text to null
    AXIS_DATA[offsetI][1] = "";
#endif
    
    if (!highResLogCurves)
        graph->SetXScale(true, minHzValue, maxHzValue);
    else
        graph->SetXScale(false, minHzValue, maxHzValue);

    
    graph->RemoveHAxis();
    
    // AddHAxis0: fix light shift in low frequencies
#if !UST_PLUGIN
    int axisColor[4] = { 48, 48, 48, 255 };
    int axisLabelColor[4] = { 170, 170, 170, 255 };
#else
    int axisColor[4] = { 48, 48, 48, 0/*255*/ };
    int axisLabelColor[4] = { 255, 255, 255, 255 };
#endif
    
    graph->AddHAxis0(&AXIS_DATA[offsetI], NUM_AXIS_DATA - offsetI, true, axisColor, axisLabelColor);
#endif
}

void
GUIHelper10::CreateKnobInternal(IPlug *plug, IGraphics *graphics,
                               IBitmap *bitmap, IKnobMultiControl *control,
                               int x, int y, int param, const char *title,
                               int shadowId, const char *shadowFn,
                               int diffuseId, const char *diffuseFn,
                               int textFieldId, const char *textFieldFn,
                               Size size,
                               IText **ioValueText, bool addValueText,
                               ITextControl **ioValueTextControl, int yoffset,
                               bool doubleTextField)
{
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif

    //CreateTitleText(plug, graphics, x, y, title, bitmap);
    
    // Title
    AddTitleText(plug, graphics, bitmap, title, x, y, IText::kAlignCenter, size);

    // Value
    IBitmap tfBmp;
    CreateTextFieldInternal(plug, graphics, textFieldId, textFieldFn,
                            x, y, bitmap, bitmap->N, doubleTextField,
                            &tfBmp);
    
    ITextControl *valCtrl = AddValueTextInternal(plug, graphics, bitmap,
                                                 x, y + yoffset,
                                                 param, addValueText, NULL, &tfBmp);
    if (ioValueTextControl != NULL)
        *ioValueTextControl = valCtrl;
    
    
    CreateKnobLightingInternal(plug, graphics, shadowId, shadowFn,
                               diffuseId, diffuseFn, x, y, bitmap, bitmap->N);
    
}

// Make some soup in this function
// this is for Spatializer
void
GUIHelper10::CreateKnobInternal2(IPlug *plug, IGraphics *graphics,
                               IBitmap *bitmap, IKnobMultiControl *control,
                               int x, int y, int param, const char *title,
                               int shadowId, const char *shadowFn,
                               int diffuseId, const char *diffuseFn,
                               int textFieldId, const char *textFieldFn,
                               IText **ioValueText, bool addValueText,
                               ITextControl **ioValueTextControl, int yoffset,
                               bool doubleTextField)
{
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    CreateKnobLightingInternal(plug, graphics, shadowId, shadowFn,
                               diffuseId, diffuseFn, x, y, bitmap, bitmap->N);
    
    // Title
    AddTitleText(plug, graphics, bitmap, title, x,
                 y + yoffset + 2*TITLE_TEXT_SIZE, IText::kAlignCenter);
    
    // Value
    IBitmap tfBmp;
    CreateTextFieldInternal(plug, graphics, textFieldId, textFieldFn, x, y - TITLE_TEXT_SIZE,
                            bitmap, bitmap->N, doubleTextField, &tfBmp);
    
    ITextControl *valCtrl =
    AddValueTextInternal(plug, graphics, bitmap, x, y - TITLE_TEXT_SIZE,
                         param, addValueText, NULL, &tfBmp);
    if (ioValueTextControl != NULL)
        *ioValueTextControl = valCtrl;
    
    
}

void
GUIHelper10::CreateKnobInternal3(IPlug *plug, IGraphics *graphics,
                                IBitmap *bitmap, IKnobMultiControl *control,
                                int x, int y, int param, const char *title,
                                int shadowId, const char *shadowFn,
                                int diffuseId, const char *diffuseFn,
                                int textFieldId, const char *textFieldFn,
                                Size size,
                                IText **ioValueText, bool addValueText,
                                ICaptionControl **ioValueTextControl, int yoffset,
                                bool doubleTextField)
{
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    //CreateTitleText(plug, graphics, x, y, title, bitmap);
    
    // Title
    AddTitleText(plug, graphics, bitmap, title, x, y, IText::kAlignCenter, size);
    
    // Value
    IBitmap tfBmp;
    CreateTextFieldInternal(plug, graphics, textFieldId, textFieldFn,
                            x, y, bitmap, bitmap->N, doubleTextField,
                            &tfBmp);
    
    ICaptionControl *valCtrl = AddValueTextInternal3(plug, graphics, bitmap,
                                                     x, y + yoffset,
                                                     param, addValueText, NULL, &tfBmp);
    if (ioValueTextControl != NULL)
        *ioValueTextControl = valCtrl;
    
    
    CreateKnobLightingInternal(plug, graphics, shadowId, shadowFn,
                               diffuseId, diffuseFn, x, y, bitmap, bitmap->N);
    
}

void
GUIHelper10::CreateKnobInternal3Ex(IPlug *plug, IGraphics *graphics,
                                   IBitmap *bitmap, IKnobMultiControl *control,
                                   int x, int y, int param, const char *title,
                                   int shadowId, const char *shadowFn,
                                   int diffuseId, const char *diffuseFn,
                                   int textFieldId, const char *textFieldFn,
                                   Size size, int titleYOffset,
                                   IText **ioValueText, bool addValueText,
                                   ICaptionControl **ioValueTextControl, int textValueYoffset,
                                   bool doubleTextField)
{
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    //CreateTitleText(plug, graphics, x, y, title, bitmap);
    
#if DISPLAY_TITLE_TEXT
    // Title
    AddTitleText(plug, graphics, bitmap, title, x, y + titleYOffset, IText::kAlignCenter, size);
#endif
    
#if TEXT_VALUES_BLACK_RECTANGLE
    // Value
    IBitmap tfBmp;
    CreateTextFieldInternal(plug, graphics, textFieldId, textFieldFn,
                            x, y, bitmap, bitmap->N, doubleTextField,
                            &tfBmp);
    
    ICaptionControl *valCtrl = AddValueTextInternal3(plug, graphics, bitmap,
                                                     x, y + textValueYoffset,
                                                     param, addValueText, NULL, &tfBmp);
#else
    ICaptionControl *valCtrl = AddValueTextInternal3(plug, graphics, bitmap,
                                                     x, y + textValueYoffset,
                                                     param, addValueText, NULL, NULL);
#endif
    
    if (ioValueTextControl != NULL)
        *ioValueTextControl = valCtrl;
    
    
    CreateKnobLightingInternal(plug, graphics, shadowId, shadowFn,
                               diffuseId, diffuseFn, x, y, bitmap, bitmap->N);
    
}

// Make some soup in this function
// this is for Spatializer
// Added exitable text field
void
GUIHelper10::CreateKnobInternal4(IPlug *plug, IGraphics *graphics,
                                IBitmap *bitmap, IKnobMultiControl *control,
                                int x, int y, int param, const char *title,
                                int shadowId, const char *shadowFn,
                                int diffuseId, const char *diffuseFn,
                                int textFieldId, const char *textFieldFn,
                                IText **ioValueText, bool addValueText,
                                ITextControl **ioValueTextControl, int yoffset,
                                bool doubleTextField)
{
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    CreateKnobLightingInternal(plug, graphics, shadowId, shadowFn,
                               diffuseId, diffuseFn, x, y, bitmap, bitmap->N);
    
    // Title
    AddTitleText(plug, graphics, bitmap, title, x,
                 y + yoffset + 2*TITLE_TEXT_SIZE, IText::kAlignCenter);
    
    // Value
    IBitmap tfBmp;
    CreateTextFieldInternal(plug, graphics, textFieldId, textFieldFn, x, y - TITLE_TEXT_SIZE,
                            bitmap, bitmap->N, doubleTextField, &tfBmp);
    
    ICaptionControl *valCtrl = AddValueTextInternal3(plug, graphics, bitmap, x, y - TITLE_TEXT_SIZE,
                                                     param, addValueText, NULL, &tfBmp);
    if (ioValueTextControl != NULL)
        *ioValueTextControl = valCtrl;
    
    
}

// For Precedence
// Do not add text field with the value of text field here
void
GUIHelper10::CreateKnobInternal5(IPlug *plug, IGraphics *graphics,
                                IBitmap *bitmap, IKnobMultiControl *control,
                                int x, int y, int param, const char *title,
                                int shadowId, const char *shadowFn,
                                int diffuseId, const char *diffuseFn,
                                int textFieldId, const char *textFieldFn,
                                Size size,
                                IText **ioValueText, bool addValueText,
                                ICaptionControl **ioValueTextControl, int yoffset,
                                bool doubleTextField)
{
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    //CreateTitleText(plug, graphics, x, y, title, bitmap);
    
    // Title
    AddTitleText(plug, graphics, bitmap, title, x, y, IText::kAlignCenter, size);
    
    // Value
    IBitmap tfBmp;
    CreateTextFieldInternal(plug, graphics, textFieldId, textFieldFn,
                            x, y, bitmap, bitmap->N, doubleTextField,
                            &tfBmp);
    
    ICaptionControl *valCtrl = AddValueTextInternal5(plug, graphics, bitmap,
                                                     x, y + yoffset,
                                                     param, addValueText, NULL, &tfBmp);
    if (ioValueTextControl != NULL)
        *ioValueTextControl = valCtrl;
    
    
    CreateKnobLightingInternal(plug, graphics, shadowId, shadowFn,
                               diffuseId, diffuseFn, x, y, bitmap, bitmap->N);
    
}

ITextControl *
GUIHelper10::AddValueTextInternal(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                 int x, int y, int param,
                                 bool addValueText, IText **ioValueText,
                                 IBitmap *tfBitmap)
{
    IText *valueText = new IText(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
                                 //IText::kStyleNormal
                                 IText::kStyleBold, IText::kAlignCenter,
                                 0, IText::kQualityDefault);
    
    
    if (ioValueText != NULL)
        *ioValueText = valueText;
    
    ITextControl *valueTextControl = NULL;
    
    if ((ioValueText == NULL) || addValueText)
    {
        valueTextControl = AddValueText(plug, graphics, bitmap, valueText, x, y, param, tfBitmap);
    }
    
    return valueTextControl;
}

ICaptionControl *
GUIHelper10::AddValueTextInternal3(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                  int x, int y, int param,
                                  bool addValueText, IText **ioValueText,
                                  IBitmap *tfBitmap)
{
    // Colors for text entries for text edit with keyboard
    // Set color invert
    //IColor pTextEntreBGColor = mValueTextColor;
    //IColor pTextEntreFGColor = mEditTextColor; //IColor(255, 0, 0, 0); // Black
    IColor pTextEntreBGColor = IColor(255, 0, 0, 0); // Black;
    IColor pTextEntreFGColor = mValueTextColor;
    
    IText *valueText = new IText(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
                                 //IText::kStyleNormal
                                 IText::kStyleBold, IText::kAlignCenter,
                                 0, IText::kQualityDefault,
                                 &pTextEntreBGColor, &pTextEntreFGColor);
    
    
    if (ioValueText != NULL)
        *ioValueText = valueText;
    
    ICaptionControl *valueTextControl = NULL;
    
    if ((ioValueText == NULL) || addValueText)
    {
        valueTextControl = AddValueText3(plug, graphics, bitmap, valueText, x, y, param, tfBitmap);
    }
    
    return valueTextControl;
}

ICaptionControl *
GUIHelper10::AddValueTextInternal5(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                  int x, int y, int param,
                                  bool addValueText, IText **ioValueText,
                                  IBitmap *tfBitmap)
{
    // Colors for text entries for text edit with keyboard
    // Set color invert
    //IColor pTextEntreBGColor = mValueTextColor;
    //IColor pTextEntreFGColor = mEditTextColor; //IColor(255, 0, 0, 0); // Black
    IColor pTextEntreBGColor = IColor(255, 0, 0, 0); // Black;
    IColor pTextEntreFGColor = mValueTextColor;
    
    IText *valueText = new IText(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
                                 //IText::kStyleNormal
                                 IText::kStyleBold, IText::kAlignCenter,
                                 0, IText::kQualityDefault,
                                 &pTextEntreBGColor, &pTextEntreFGColor);
    
    
    if (ioValueText != NULL)
        *ioValueText = valueText;
    
    ICaptionControl *valueTextControl = NULL;
    
    //if ((ioValueText == NULL) || addValueText)
    if (addValueText)
    {
        valueTextControl = AddValueText3(plug, graphics, bitmap, valueText, x, y, param, tfBitmap);
    }
    
    return valueTextControl;
}

void
GUIHelper10::CreateKnobLightingInternal(IPlug *plug, IGraphics *graphics,
                                       int shadowId, const char *shadowFn,
                                       int diffuseId, const char *diffuseFn,
                                       int x, int y, IBitmap *bitmap, int nObjectFrames)
{
    if ((shadowId >= 0) && (shadowFn != NULL))
    {
#if USE_SHADOWS
        CreateShadow(plug, graphics, shadowId, shadowFn, x, y, bitmap, bitmap->N);
#endif
    }

    if ((diffuseId >= 0) && (diffuseFn != NULL))
    {
#if USE_SHADOWS
        CreateDiffuse(plug, graphics, diffuseId, diffuseFn, x, y, bitmap, bitmap->N);
#endif
    }
}

IBitmapControl *
GUIHelper10::CreateTextFieldInternal(IPlug *plug, IGraphics *graphics,
                                    int textFieldId, const char *textFieldFn,
                                    int x, int y, IBitmap *bitmap, int nObjectFrames,
                                    bool doubleTextField, IBitmap *outBmp)
{
    IBitmapControl *result = NULL;
    
    if ((textFieldId >= 0) && (textFieldFn != NULL))
    {
        result = CreateTextField(plug, graphics,
                                 textFieldId, textFieldFn, x, y, bitmap, bitmap->N, outBmp);
    
        if (doubleTextField)
        {
            int y2 = y + VALUE_TEXT_SIZE + VALUE2_TEXT_OFFSET;
        
            CreateTextField(plug, graphics, textFieldId, textFieldFn, x, y2, bitmap, bitmap->N);
        }
    }
    
    return result;
}

#if 0 // Not tested
        // FIX: (this one is just in case): resize and fill the dummy input
        // (otherwise when saving to stereo, one channel would often be garbage)
        dummyIn.resize(in.size());
        for (int i = 0; i < in.size(); i++)
        {
            WDL_TypedBuf<BL_FLOAT> &chan = dummyIn[i];
            Utils::ResizeFillZeros(&chan, BUFFER_SIZE);
        }
#endif
