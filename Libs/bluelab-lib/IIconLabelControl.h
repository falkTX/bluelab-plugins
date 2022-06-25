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
 
#ifndef IICON_LABEL_CONTROL_H
#define IICON_LABEL_CONTROL_H

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IIconLabelControl : public IBitmapControl
{
public:
    IIconLabelControl(float x, float y,
                      float iconOffsetX, float iconOffsetY,
                      float textOffsetX, float textOffsetY,
                      const IBitmap &bgBitmap,
                      const IBitmap &iconBitmap,
                      const IText &text = DEFAULT_TEXT,
                      int paramIdx = kNoParameter,
                      EBlend blend = EBlend::Default);

    virtual ~IIconLabelControl();
    
    virtual void Draw(IGraphics& g) override;

    void SetLabelText(const char *text);
    void SetIconNum(int iconNum);
    
 protected:
    ITextControl *mTextControl;
    IBitmap mIconBitmap;

    float mIconOffsetX;
    float mIconOffsetY;

    float mTextOffsetX;
    float mTextOffsetY;
    
    int mIconNum;
};

#endif
