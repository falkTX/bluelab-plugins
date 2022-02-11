//
//  GUIHelpers.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/06/17.
//
//

#include "GUIHelpers2.h"

IColor GUIHelpers2::mTextColor(255, TEXT_COLOR_R, TEXT_COLOR_G, TEXT_COLOR_B);

ITextControl *
GUIHelpers2::CreateText(IPlug *plug, IGraphics *graphics, int width, int height, int x, int y, const char *label)
{
    IText text(TEXT_SIZE, &mTextColor, TEXT_FONT,
               IText::kStyleBold, IText::kAlignCenter, 0, IText::kQualityDefault);
    
    ITextControl *control = new ITextControl(plug,
                                             IRECT(x, y + height + TEXT_OFFSET, (x + width), (y + height + TEXT_OFFSET + TEXT_SIZE)),
                                             &text, label);
    
    graphics->AttachControl(control);
    
    return control;
}

IKnobMultiControl *
GUIHelpers2::CreateKnobWithText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                               int x, int y, int param, const char *label, IText **ioText, int yoffset)
{
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, bitmap);
    
    graphics->AttachControl(control);

    IText *text = new IText(TEXT_SIZE, &mTextColor, TEXT_FONT,
                           //IText::kStyleNormal
                           IText::kStyleBold, IText::kAlignCenter, 0, IText::kQualityDefault);


    if (ioText != NULL)
        *ioText = text;
    else
        AddText(plug, graphics, bitmap, text, x, y + yoffset, param);
    
    
    return control;
}

IKnobMultiControl *
GUIHelpers2::CreateEnumKnobWithText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                    int x, int y, int param, double sensivity,
                                    const char *label, IText **ioText,
                                    int yoffset)
{
    IKnobMultiControl *control = new IKnobMultiControl(plug, x, y, param, bitmap, kVertical, sensivity*DEFAULT_GEARING);
    
    graphics->AttachControl(control);
    
    IText *text = new IText(TEXT_SIZE, &mTextColor, TEXT_FONT,
                            //IText::kStyleNormal
                            IText::kStyleBold, IText::kAlignCenter, 0, IText::kQualityDefault);
    
    
    if (ioText != NULL)
        *ioText = text;
    else
        AddText(plug, graphics, bitmap, text, x, y + yoffset, param);
    
    
    return control;
}

void
GUIHelpers2::AddText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap, IText *text, int x, int y, int param)
{
    int width = bitmap->W;
    int height = bitmap->frameHeight();
    
    //graphics->AttachControl(new ITextControl(plug,
    //                                         IRECT(x, (y -  TEXT_SIZE - TEXT_OFFSET), (x + width), y - TEXT_OFFSET), &text, label));

    graphics->AttachControl(new ICaptionControl(plug,
                                                IRECT(x, y + height + TEXT_OFFSET + TEXT_OFFSET2, (x + width), (y + height + TEXT_OFFSET + TEXT_OFFSET2 + TEXT_SIZE))
                                                , param, text, ""));
}

void
GUIHelpers2::UpdateText(IPlug *plug, int paramIdx)
{
    if (plug->GetGUI())
        plug->GetGUI()->SetParameterFromPlug(paramIdx, plug->GetParam(paramIdx)->Value(), false);
    
    // Comment the following line otherwise it will make an infinite loop under Protools

    //inform host of new normalized value
    //plug->InformHostOfParamChange(paramIdx, (plug->GetParam(paramIdx)->Value()));
}

IToggleButtonControl *
GUIHelpers2::CreateToggleButtonWithText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                       int x, int y, int param, const char *label)
{
    IToggleButtonControl *control = new IToggleButtonControl(plug, x, y, param, bitmap);
    
    graphics->AttachControl(control);
    
    IText text(TEXT_SIZE, &mTextColor, TEXT_FONT,
               IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
    
    
    int bmpWidth = bitmap->W;
    int width = TEXT_SIZE*strlen(label);
    
    graphics->AttachControl(new ITextControl(plug,
                                             IRECT(x - width/2 + bmpWidth/2, (y -  TEXT_SIZE - TEXT_OFFSET), x + width/2 + bmpWidth/2, y - TEXT_OFFSET), &text, label));
    
    
    return control;
}

VuMeterControl *
GUIHelpers2::CreateVuMeterWithText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                  int x, int y, int param, const char *label, bool alignLeft)
{
    VuMeterControl *control = new VuMeterControl(plug, x, y, param, bitmap);
    graphics->AttachControl(control);

    enum IText::EAlign align = alignLeft ? IText::kAlignNear : IText::kAlignFar;
    
    IText text(TEXT_SIZE, &mTextColor, TEXT_FONT,
               IText::kStyleNormal, align, 0, IText::kQualityDefault);
    
    
    int bmpWidth = bitmap->W;
    int width = TEXT_SIZE*strlen(label);
    
    int yy = y + bitmap->frameHeight() + TEXT_OFFSET;
    int xx = alignLeft ? x : x + bmpWidth - TEXT_SIZE*strlen(label);
    
    graphics->AttachControl(new ITextControl(plug,
                                             IRECT(xx, yy, xx + width, yy), &text, label));
    
    return control;
}

int
GUIHelpers2::GetTextOffset()
{
    return TEXT_SIZE + TEXT_OFFSET2;
}
