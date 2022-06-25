/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#ifndef IBL_TOOLTIP_CONTROL_H
#define IBL_TOOLTIP_CONTROL_H

#include <ITooltipControl.h>

class IBLTooltipControl : public ITooltipControl
{
 public:
    IBLTooltipControl(const IColor& BGColor = COLOR_WHITE,
                      const IColor& borderColor = COLOR_BLACK,
                      const IText& text = DEFAULT_TEXT)
    : ITooltipControl(BGColor, text),
      mBorderColor(borderColor) {}

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
