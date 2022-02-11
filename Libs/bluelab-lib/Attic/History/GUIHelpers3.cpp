//
//  GUIHelpers3.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/06/17.
//
//

#include "GUIHelpers3.h"
#include "GraphControl6.h"

IColor GUIHelpers3::mTitleTextColor(255,
                                    TITLE_TEXT_COLOR_R,
                                    TITLE_TEXT_COLOR_G,
                                    TITLE_TEXT_COLOR_B);

IColor GUIHelpers3::mValueTextColor(255,
                                    VALUE_TEXT_COLOR_R,
                                    VALUE_TEXT_COLOR_G,
                                    VALUE_TEXT_COLOR_B);

IColor GUIHelpers3::mRadioLabelTextColor(255,
                                         RADIO_LABEL_TEXT_COLOR_R,
                                         RADIO_LABEL_TEXT_COLOR_G,
                                         RADIO_LABEL_TEXT_COLOR_B);

IColor GUIHelpers3::mVersionTextColor(255,
                                      VERSION_TEXT_COLOR_R,
                                      VERSION_TEXT_COLOR_G,
                                      VERSION_TEXT_COLOR_B);

#if GUI_OBJECTS_SORTING
vector<IControl *> GUIHelpers3::mBackObjects;
vector<IControl *> GUIHelpers3::mWidgets;
vector<IControl *> GUIHelpers3::mTexts;
vector<IControl *> GUIHelpers3::mDiffuse;
vector<IControl *> GUIHelpers3::mShadows;
#endif

void
GUIHelpers3::AddAllObjects(IGraphics *graphics)
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

#if 0
    // Clean
    mBackObjects.clear();
    mWidgets.clear();
    mTexts.clear();
    mDiffuse.clear();
    mShadows.clear();
#endif
    
#endif
}

void
GUIHelpers3::DirtyAllObjects()
{
#if GUI_OBJECTS_SORTING
    for (int i = 0; i < mBackObjects.size(); i++)
    {
        IControl *control = mBackObjects[i];
        control->SetDirty(false);
    }
    
    for (int i = 0; i < mTexts.size(); i++)
    {
        IControl *control = mTexts[i];
        control->SetDirty(false);
    }
    
    for (int i = 0; i < mDiffuse.size(); i++)
    {
        IControl *control = mDiffuse[i];
        control->SetDirty(false);
    }
    
    for (int i = 0; i < mWidgets.size(); i++)
    {
        IControl *control = mWidgets[i];
        control->SetDirty(false);
    }
    
    for (int i = 0; i < mShadows.size(); i++)
    {
        IControl *control = mShadows[i];
        control->SetDirty(false);
    }
#endif
}

ITextControl *
GUIHelpers3::CreateValueText(IPlug *plug, IGraphics *graphics,
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
GUIHelpers3::CreateKnob(IPlug *plug, IGraphics *graphics,
                        int bmpId, const char *bmpFn, int bmpFrames,
                        int x, int y, int param, const char *title,
                        int shadowId, const char *shadowFn,
                        int diffuseId, const char *diffuseFn,
                        int textFieldId, const char *textFieldFn,
                        IText **ioValueText, bool addValueText,
                        ITextControl **ioValueTextControl, int yoffset)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, &bitmap);
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif

    // Title
    IText *titleText = new IText(TITLE_TEXT_SIZE, &mTitleTextColor, TITLE_TEXT_FONT,
                                 //IText::kStyleNormal
                                 IText::kStyleBold, IText::kAlignCenter,
                                 0, IText::kQualityDefault);
    
    AddTitleText(plug, graphics, &bitmap, titleText, title, x, y);
    
    // Value
    IText *valueText = new IText(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
                                 //IText::kStyleNormal
                                 IText::kStyleBold, IText::kAlignCenter,
                                 0, IText::kQualityDefault);


    if (ioValueText != NULL)
        *ioValueText = valueText;
    
    if ((ioValueText == NULL) || addValueText)
    {
        ITextControl *valueTextControl = AddValueText(plug, graphics, &bitmap, valueText,
                                                      x, y + yoffset, param);
        if (ioValueTextControl != NULL)
            *ioValueTextControl = valueTextControl;
    }
    
    if ((shadowId >= 0) && (shadowFn != NULL))
    {
        CreateShadow(plug, graphics, shadowId, shadowFn, x, y, &bitmap, bmpFrames);
    }
    
    if ((diffuseId >= 0) && (diffuseFn != NULL))
    {
        CreateDiffuse(plug, graphics, diffuseId, diffuseFn, x, y, &bitmap, bmpFrames);
    }
    
    if ((textFieldId >= 0) && (textFieldFn != NULL))
    {
        CreateTextField(plug, graphics, textFieldId, textFieldFn, x, y, &bitmap, bmpFrames);
    }
    
    return control;
}

IKnobMultiControl *
GUIHelpers3::CreateEnumKnob(IPlug *plug, IGraphics *graphics,
                            IBitmap *bitmap,
                            int x, int y, int param, BL_FLOAT sensivity,
                            const char *title, IText **ioText,
                            int yoffset)
{
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, bitmap,
                                                       kVertical, sensivity*DEFAULT_GEARING);
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    IText *text = new IText(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
                            //IText::kStyleNormal
                            IText::kStyleBold, IText::kAlignCenter, 0, IText::kQualityDefault);
    
    
    if (ioText != NULL)
        *ioText = text;
    else
        AddValueText(plug, graphics, bitmap, text, x, y + yoffset, param);
    
    
    return control;
}

IRadioButtonsControl *
GUIHelpers3::CreateRadioButtons(IPlug *plug, IGraphics *graphics,
                                int bmpId, const char *bmpFn, int bmpFrames,
                                int x, int y, int numButtons, int size, int param,
                                bool horizontalFlag, const char *title,
                                int diffuseId, const char *diffuseFn,
                                IText::EAlign titleAlign,
                                const char **radioLabels, int numRadioLabels)
{
    IBitmap bitmap = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    // Title
    IText *titleText = new IText(TITLE_TEXT_SIZE, &mTitleTextColor, TITLE_TEXT_FONT,
                                 //IText::kStyleNormal
                                 IText::kStyleBold, titleAlign,
                                 0, IText::kQualityDefault);
    
    AddTitleText(plug, graphics, &bitmap, titleText, title, x, y);
    
    // Radio labels
    if (numRadioLabels > 0)
    {
        if (!horizontalFlag)
        {
            int labelX = x;
            if (titleAlign == IText::kAlignFar)
                labelX = labelX - bitmap.W - RADIO_LABEL_TEXT_OFFSET;
            
            if (titleAlign == IText::kAlignNear)
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
                                             titleAlign,
                                             0, IText::kQualityDefault);
                
                int labelY = y + i*stepSize;
                const char *labelStr = radioLabels[i];
                AddRadioLabelText(plug, graphics, &bitmap, labelText, labelStr, labelX, labelY);
            }
        }
        else
        {
            //TODO: implement this !
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
GUIHelpers3::CreateToggleButton(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                int x, int y, int param, const char *title)
{
    IToggleButtonControl *control = new IToggleButtonControl(plug, x, y, param, bitmap);
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    IText text(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
               IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
    
    
    int bmpWidth = bitmap->W;
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
    
    return control;
}

VuMeterControl *
GUIHelpers3::CreateVuMeter(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                           int x, int y, int param, const char *title, bool alignLeft)
{
    VuMeterControl *control = new VuMeterControl(plug, x, y, param, bitmap);

#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif

    enum IText::EAlign align = alignLeft ? IText::kAlignNear : IText::kAlignFar;
    
    IText text(VALUE_TEXT_SIZE, &mValueTextColor, VALUE_TEXT_FONT,
               IText::kStyleNormal, align, 0, IText::kQualityDefault);
    
    
    int bmpWidth = bitmap->W;
    int width = VALUE_TEXT_SIZE*strlen(title);
    
    int yy = y + bitmap->frameHeight() + VALUE_TEXT_OFFSET;
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

#if 0 // Old
GraphControl4 *
GUIHelpers3::CreateGraph4(IPlug *plug, IGraphics *graphics,
                          int bmpId, const char *bmpFn, int bmpFrames,
                          int x , int y, int numCurves, int numPoints)
{
    IBitmap graphBmp = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IRECT rect(x, y, x + graphBmp.W, y + graphBmp.H);

    // Get the path to the font
    WDL_String pPath;
    graphics->ResourcePath(&pPath, "font.ttf");
    
    GraphControl4 *control = new GraphControl4(plug, rect, numCurves, numPoints, pPath.Get());

#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}
#endif

GraphControl6 *
GUIHelpers3::CreateGraph6(IPlug *plug, IGraphics *graphics,
                          int bmpId, const char *bmpFn, int bmpFrames,
                          int x , int y, int numCurves, int numPoints)
{
    IBitmap graphBmp = graphics->LoadIBitmap(bmpId, bmpFn, bmpFrames);
    
    IRECT rect(x, y, x + graphBmp.W, y + graphBmp.H);
    
    // Get the path to the font
    WDL_String pPath;
    graphics->ResourcePath(&pPath, "font.ttf");
    
    GraphControl6 *control = new GraphControl6(plug, rect, numCurves, numPoints, pPath.Get());
    
#if GUI_OBJECTS_SORTING
    mWidgets.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

void
GUIHelpers3::CreateVersion(IPlug *plug, IGraphics *graphics, const char *versionStr)
{
    // Lower left corner
    char versionStrV[256];
    sprintf(versionStrV, "v%s", versionStr);
    
#if 0 // Lower right corner
    int strWidth = strlen(versionStrV)*VERSION_TEXT_SIZE;
    
    int x = graphics->Width() - strWidth/2 - VERSION_TEXT_OFFSET + VERSION_TEXT_SIZE/2;
    int y = graphics->Height() - VERSION_TEXT_SIZE - VERSION_TEXT_OFFSET;
    
    IText::EAlign textAlign = IText::kAlignNear;
#endif

#if 1 // Center bottom
    int x = graphics->Width()/2;
    int y = graphics->Height() - VERSION_TEXT_SIZE - VERSION_TEXT_OFFSET;
    
    IText::EAlign textAlign = IText::kAlignCenter;
#endif
    
    IText versionText(VERSION_TEXT_SIZE, &mVersionTextColor, VERSION_TEXT_FONT,
                      IText::kStyleNormal, textAlign);
    
    AddVersionText(plug, graphics, &versionText, versionStrV, x, y);
}

IBitmapControl *
GUIHelpers3::CreateBitmap(IPlug *plug, IGraphics *graphics,
                          int x, int y, int bmpId, const char *bmpFn)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
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
GUIHelpers3::CreateShadows(IPlug *plug, IGraphics *graphics, int bmpId, const char *bmpFn)
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
GUIHelpers3::CreateLogo(IPlug *plug, IGraphics *graphics, int bmpId, const char *bmpFn)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
#if 0
    // Lower left corner
    int x = 0;
    int y = graphics->Height() - bmp.H;
#endif
    
#if 0
    // Upper right corner
    int x = graphics->Width() - bmp.W;
    int y = 0;
#endif

#if 1
    // Lower right corner
    int x = graphics->Width() - bmp.W;
    int y = graphics->Height() - bmp.H;
#endif

    
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
GUIHelpers3::CreatePlugName(IPlug *plug, IGraphics *graphics, int bmpId, const char *bmpFn)
{
    IBitmap bmp = graphics->LoadIBitmap(bmpId, bmpFn);
    
#if 0
    // Upper left corner
    int x = 0;
    int y = 0;
#endif

#if 1
    // Lower left corner
    int x = 0;
    int y = graphics->Height() - bmp.H;
#endif

    
    IBitmapControl *control = new IBitmapControl(plug, x, y, -1, &bmp);
    control->SetInteractionDisabled(true);
    
#if GUI_OBJECTS_SORTING
    mBackObjects.push_back(control);
#else
    graphics->AttachControl(control);
#endif
    
    return control;
}

ITextControl *
GUIHelpers3::AddValueText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                          IText *text, int x, int y, int param)
{
    int width = bitmap->W;
    int height = bitmap->frameHeight();
    
    //graphics->AttachControl(new ITextControl(plug,
    //                                         IRECT(x, (y -  TEXT_SIZE - TEXT_OFFSET), (x + width), y - TEXT_OFFSET), &text, label));
    
    IRECT rect(x, y + height + VALUE_TEXT_OFFSET + VALUE_TEXT_OFFSET2,
               (x + width),
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
GUIHelpers3::AddTitleText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                          IText *text, const char *title, int x, int y)
{
    int width = bitmap->W;
    IRECT rect(x, y + TITLE_TEXT_OFFSET - TITLE_TEXT_SIZE,
               (x + width), (y  + TITLE_TEXT_OFFSET /*- TITLE_TEXT_SIZE*/));
    
    ITextControl *textControl = new ITextControl(plug, rect, text, title);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

ITextControl *
GUIHelpers3::AddRadioLabelText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
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
GUIHelpers3::AddVersionText(IPlug *plug, IGraphics *graphics,
                            IText *text, const char *versionStr, int x, int y)
{
    IRECT rect(x, y, x + VERSION_TEXT_SIZE, y);
    ITextControl *textControl = new ITextControl(plug, rect, text, versionStr);
    
#if GUI_OBJECTS_SORTING
    mTexts.push_back(textControl);
#else
    graphics->AttachControl(textControl);
#endif
    
    return textControl;
}

void
GUIHelpers3::UpdateText(IPlug *plug, int paramIdx)
{
    if (plug->GetGUI())
        plug->GetGUI()->SetParameterFromPlug(paramIdx, plug->GetParam(paramIdx)->Value(), false);
    
    // Comment the following line otherwise it will make an infinite loop under Protools
    
    //inform host of new normalized value
    //plug->InformHostOfParamChange(paramIdx, (plug->GetParam(paramIdx)->Value()));
}

IBitmapControl *
GUIHelpers3::CreateShadow(IPlug *plug, IGraphics *graphics,
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
GUIHelpers3::CreateDiffuse(IPlug *plug, IGraphics *graphics,
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
GUIHelpers3::CreateMultiDiffuse(IPlug *plug, IGraphics *graphics,
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
GUIHelpers3::CreateTextField(IPlug *plug, IGraphics *graphics,
                            int bmpId, const char *bmpFn,
                            int x, int y, IBitmap *objectBitmap,
                            int nObjectFrames)
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
    
    return control;
}

int
GUIHelpers3::GetTextOffset()
{
    return VALUE_TEXT_SIZE + VALUE_TEXT_OFFSET2;
}
