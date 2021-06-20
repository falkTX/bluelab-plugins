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
                       bool setBoundsBasedOnStr = false,
                       const IColor& borderColor = DEFAULT_BGCOLOR,
                       float borderWidth = -1.0)
        : ITextControl(bounds, str, text, BGColor, setBoundsBasedOnStr)
    {
        mIgnoreMouse = false;
        
        SetParamIdx(paramIdx);

        mBorderColor = borderColor;
        mBorderWidth = borderWidth;

        // Grow the bounds
        if (mBorderWidth > 0.0)
        {
            IRECT borderRect = mRECT;
            borderRect.L -= mBorderWidth;
            borderRect.T -= mBorderWidth;
            borderRect.R += mBorderWidth;
            borderRect.B += mBorderWidth;

            mRECT = borderRect;
        }
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

    void Draw(IGraphics& g) override
    {
        ITextControl::Draw(g);

        // Draw the border
        if (mBorderWidth < 0.0)
            return;
        
        IRECT borderRect = mRECT;
        borderRect.T += mBorderWidth*0.5;
        borderRect.R -= mBorderWidth*0.5;

        g.DrawRect(mBorderColor, borderRect, &mBlend, mBorderWidth);
    }
    
protected:
    IColor mBorderColor;
    float mBorderWidth;
};

#endif
