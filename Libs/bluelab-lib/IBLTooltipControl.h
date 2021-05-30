#ifndef IBL_TOOLTIP_CONTROL_H
#define IBL_TOOLTIP_CONTROL_H

#include <ITooltipControl.h>

class IBLTooltipControl : public ITooltipControl
{
 public:
    IBLTooltipControl(const IColor& BGColor = COLOR_WHITE,
                      const IColor& BorderColor = COLOR_BLACK,
                      const IText& text = DEFAULT_TEXT)
        : ITooltipControl(BGColor, text) {}

    void Draw(IGraphics& g) override
    {
        IRECT innerRECT = mRECT.GetPadded(-10);
        g.FillRect(mBGColor, innerRECT);
        g.DrawRect(mBorderColor, innerRECT);
        g.DrawText(mText, mDisplayStr.Get(), mRECT.GetPadded(-2));
    }

 protected:
    IColor mBorderColor;
};

#endif
