//
//  GUIHelper4.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/06/17.
//
//

#include "GUIHelper4.h"

#if USE_GRAPH_OGL
#include "GraphControl6.h"
#endif

#define USE_SHADOWS 0

IColor GUIHelper4::mValueTextColor = IColor(255,
                                             VALUE_TEXT_COLOR_R,
                                             VALUE_TEXT_COLOR_G,
                                             VALUE_TEXT_COLOR_B);

GUIHelper4::GUIHelper4()
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
}


GUIHelper4::~GUIHelper4()
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
GUIHelper4::AddAllObjects(IGraphics *graphics)
{
#if GUI_OBJECTS_SORTING
    for (int i = 0; i < mBackObjects.size(); i++)
    {
        IControl *control = mBackObjects[i];
        graphics->AttachControl(control);
    }
    
    for (int i = 0; i < mTexts.size(); i++)
    {
        IControl *control = mTexts[i];
        graphics->AttachControl(control);
    }
    
    for (int i = 0; i < mDiffuse.size(); i++)
    {
        IControl *control = mDiffuse[i];
        graphics->AttachControl(control);
    }
    
    for (int i = 0; i < mWidgets.size(); i++)
    {
        IControl *control = mWidgets[i];
        graphics->AttachControl(control);
    }
    
    for (int i = 0; i < mShadows.size(); i++)
    {
        IControl *control = mShadows[i];
        graphics->AttachControl(control);
    }
    
    for (int i = 0; i < mTriggers.size(); i++)
    {
        IControl *control = mTriggers[i];
        graphics->AttachControl(control);
    }
#endif
}

ITextControl *
GUIHelper4::CreateValueText(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateKnob(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateKnob2(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateEnumKnob(IPlug *plug, IGraphics *graphics,
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

IRadioButtonsControl *
GUIHelper4::CreateRadioButtons(IPlug *plug, IGraphics *graphics,
                                int bmpId, const char *bmpFn, int bmpFrames,
                                int x, int y, int numButtons, int size, int param,
                                bool horizontalFlag, const char *title,
                                int diffuseId, const char *diffuseFn,
                                IText::EAlign align,
                                IText::EAlign titleAlign,
                                const char **radioLabels, int numRadioLabels)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    // Title
    AddTitleText(plug, graphics, &bitmap, title, x, y, titleAlign);
    
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
                IText *labelText = new IText(RADIO_LABEL_TEXT_SIZE,
                                             &mRadioLabelTextColor,
                                             RADIO_LABEL_TEXT_FONT,
                                             IText::kStyleBold,
                                             align,
                                             0, IText::kQualityDefault);
                
                int labelY = y + i*stepSize;
                const char *labelStr = radioLabels[i];
                AddRadioLabelText(plug, graphics, &bitmap, labelText, labelStr, labelX, labelY);
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
                IText *labelText = new IText(RADIO_LABEL_TEXT_SIZE,
                                             &mRadioLabelTextColor,
                                             RADIO_LABEL_TEXT_FONT,
                                             IText::kStyleBold,
                                             align,
                                             0, IText::kQualityDefault);
                
                int labelX = x + i*stepSize;
                const char *labelStr = radioLabels[i];
                AddRadioLabelText(plug, graphics, &bitmap, labelText, labelStr, labelX, labelY);
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
    
    if ((diffuseId >= 0) && (diffuseFn != NULL))
    {
        WDL_TypedBuf<IRECT> rects;
        radioButtons->GetRects(&rects);
        
        vector<IBitmapControlExt *> diffuseBmps;

        
        CreateMultiDiffuse(plug, graphics, diffuseId, diffuseFn,
                           rects, &bitmap, bmpFrames, &diffuseBmps);
        
        radioButtons->SetDiffuseBitmapList(diffuseBmps);
    }
    
    return radioButtons;
}

IToggleButtonControl *
GUIHelper4::CreateToggleButton(IPlug *plug, IGraphics *graphics,
                               int bmpId, const char *bmpFn, int bmpFrames,
                               int x, int y, int param, const char *title,
                               int diffuseId, const char *diffuseFn)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    // Title
    AddTitleText(plug, graphics, &bitmap, title, x, y, IText::kAlignCenter);
    
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

VuMeterControl *
GUIHelper4::CreateVuMeter(IPlug *plug, IGraphics *graphics,
                          int bmpId, const char *bmpFn, int bmpFrames,
                          int x, int y, int param, const char *title, bool alignLeft)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    VuMeterControl *control = new VuMeterControl(plug, x, y, param, &bitmap);

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
GraphControl6 *
GUIHelper4::CreateGraph6(IPlug *plug, IGraphics *graphics, int param,
                         int bmpId, const char *bmpFn, int bmpFrames,
                         int x , int y, int numCurves, int numPoints,
                         int shadowsId, const char *shadowsFn,
                         IBitmap **oBitmap)
{
    plug->GetParam(param)->InitInt("VoidGraph", 0, 0, 1);
    
    IBitmap graphBmp = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    if (oBitmap != NULL)
        *oBitmap = &graphBmp;
    
    
    GraphControl6 *control = CreateGraph6(plug, graphics, param, x, y,
                                          graphBmp.W, graphBmp.H, numCurves, numPoints);
    
    if ((shadowsId >= 0) && (shadowsFn != NULL))
    {
#if USE_SHADOWS
        CreateShadow(plug, graphics, shadowsId, shadowsFn, x, y, &graphBmp, graphBmp.N);
#endif
    }
    
    return control;
}

GraphControl6 *
GUIHelper4::CreateGraph6(IPlug *plug, IGraphics *graphics, int param,
                         int x , int y, int width, int height,
                         int numCurves, int numPoints)
{
    IRECT rect(x, y, x + width, y + height);
    
    // Get the path to the font
    WDL_String pPath;
    graphics->ResourcePath(&pPath, "font.ttf");
    
    GraphControl6 *control = new GraphControl6(plug, rect, param, numCurves, numPoints, pPath.Get());
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

#endif

void
GUIHelper4::CreateVersion(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateBitmap(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateShadows(IPlug *plug, IGraphics *graphics, int bmpId, const char *bmpFn)
{
    IBitmap shadowBmp = graphics->LoadIBitmap(bmpId, bmpFn);
    IBitmapControl *shadowControl = new IBitmapControl(plug, 0, 0, -1, &shadowBmp);
    shadowControl->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mShadows.push_back(shadowControl);
#else
    graphics->AttachControl(shadowControl);
#endif
    
    return shadowControl;
}

IBitmapControl *
GUIHelper4::CreateLogo(IPlug *plug, IGraphics *graphics,
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

IBitmapControl *
GUIHelper4::CreatePlugName(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateHelpButton(IPlug *plug, IGraphics *graphics,
                             int bmpId, const char *bmpFn,
                             const char *fileName,
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
    
    
    WDL_String wdlResDir;
    graphics->GetResourceDir(&wdlResDir);
    const char *resDir = wdlResDir.Get();
    
    char fullFileName[1024];
    sprintf(fullFileName, "%s/%s", resDir, fileName);
    
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
GUIHelper4::AddValueText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
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

ITextControl *
GUIHelper4::AddTitleText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                        const char *title, int x, int y, IText::EAlign align,
                         Size size)
{
    int titleTextSize = TITLE_TEXT_SIZE;
    int titleTextOffsetY = TITLE_TEXT_OFFSET_Y;
    
    if (size == SIZE_SMALL)
    {
        titleTextSize /= 2;
        titleTextOffsetY /= 2;
    }
    
    IText *titleText = new IText(titleTextSize, &mTitleTextColor, TITLE_TEXT_FONT,
                                 //IText::kStyleNormal
                                 IText::kStyleBold, align,
                                 0, IText::kQualityDefault);
    
    int width = bitmap->W;
    IRECT rect(x, y + titleTextOffsetY - titleTextSize,
               (x + width), (y  + titleTextOffsetY /*- TITLE_TEXT_SIZE*/));
    
    ITextControl *textControl = new ITextControl(plug, rect, titleText, title);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelper4::AddRadioLabelText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                               IText *text, const char *labelStr, int x, int y)
{
    int width = bitmap->W;
    IRECT rect(x, y, (x + width), y);
    
    ITextControl *textControl = new ITextControl(plug, rect, text, labelStr);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelper4::AddVersionText(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::AddTrigger(ITriggerControl *trigger)
{
#if GUI_OBJECTS_SORTING
    mTriggers.push_back(trigger);
#else
    graphics->AttachControl(trigger);
#endif
}

void
GUIHelper4::UpdateText(IPlugBase *plug, int paramIdx)
{
    if (plug->GetGUI())
        plug->GetGUI()->SetParameterFromPlug(paramIdx, plug->GetParam(paramIdx)->Value(), false);
    
    // Comment the following line otherwise it will make an infinite loop under Protools
    
    //inform host of new normalized value
    //plug->InformHostOfParamChange(paramIdx, (plug->GetParam(paramIdx)->Value()));
}

IBitmapControl *
GUIHelper4::CreateShadow(IPlug *plug, IGraphics *graphics,
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

IBitmapControl *
GUIHelper4::CreateDiffuse(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateMultiDiffuse(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateTextField(IPlug *plug, IGraphics *graphics,
                            int bmpId, const char *bmpFn,
                            int x, int y, IBitmap *objectBitmap,
                            int nObjectFrames, IBitmap *outBmp)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
    int centerObjectX = x + objectBitmap->W/2;
    int centerObjectY = y + (objectBitmap->H/nObjectFrames)/2;
    
    int resultX = centerObjectX - bmp.W/2;
    int resultY = centerObjectY + (objectBitmap->H/nObjectFrames)/2 + bmp.H - VALUE_TEXT_OFFSET/2;
    
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
GUIHelper4::GetTextOffset()
{
    return VALUE_TEXT_SIZE + VALUE_TEXT_OFFSET2;
}

void
GUIHelper4::CreateText(IPlug *plug, IGraphics *graphics,
                       const char *textStr,
                       int x, int y,
                       int size, char *font,
                       IText::EStyle style,
                       const IColor &color,
                       IText::EAlign align)
{    
    IText text(size, &color, font, style, align);
    
    int len = strlen(textStr);
    IRECT rect(x, y, x + size*len, y + size);
    
    ITextControl *textControl = new ITextControl(plug, rect, &text, textStr);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
}

void
GUIHelper4::CreateKnobInternal(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateKnobInternal2(IPlug *plug, IGraphics *graphics,
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

ITextControl *
GUIHelper4::AddValueTextInternal(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
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

void
GUIHelper4::CreateKnobLightingInternal(IPlug *plug, IGraphics *graphics,
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
GUIHelper4::CreateTextFieldInternal(IPlug *plug, IGraphics *graphics,
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
