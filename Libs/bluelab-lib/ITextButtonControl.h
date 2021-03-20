#ifndef ITEXT_BUTTON_CONTROL_H
#define ITEXT_BUTTON_CONTROL_H

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

// Text control, but we can click on it, and it will trigger OnParamChange()
class ITextButtonControl : public ITextControl
{
 public:
    ITextButtonControl(const IRECT& bounds, int paramIdx = kNoParameter,
                       const char* str = "", const IText& text = DEFAULT_TEXT,
                       const IColor& BGColor = DEFAULT_BGCOLOR,
                       bool setBoundsBasedOnStr = false)
        : ITextControl(bounds, str, text, BGColor, setBoundsBasedOnStr)
    {
        mIgnoreMouse = false;
        
        SetParamIdx(paramIdx);
    }

    void OnMouseDown(float x, float y, const IMouseMod& mod) override
    {
        ITextControl::OnMouseDown(x, y, mod);

        if(GetValue() < 0.5)
            SetValue(1.);
        else
            SetValue(0.);
        
        SetDirty(true);
    }
};

#endif
