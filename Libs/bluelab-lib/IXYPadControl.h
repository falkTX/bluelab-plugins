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
 
#ifndef IXY_PAD_CONTROL_H
#define IXY_PAD_CONTROL_H

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IXYPadControl : public IControl
{
 public:
    IXYPadControl(const IRECT& bounds,
                  const std::initializer_list<int>& params,
                  const IBitmap& trackBitmap,
                  const IBitmap& handleBitmap,
                  float borderSize = 0.0,
                  bool reverseY = false);

    virtual ~IXYPadControl();
    
    void Draw(IGraphics& g) override;

    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    void OnMouseUp(float x, float y, const IMouseMod& mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod& mod) override;
  
 protected:
    void DrawTrack(IGraphics& g);
    void DrawHandle(IGraphics& g);

    // Ensure that the handle doesn't go out of the track at all 
    void PixelsToParams(float *x, float *y);
    void ParamsToPixels(float *x, float *y);

    bool MouseOnHandle(float x, float y,
                       float *offsetX, float *offsetY);
        
    //
    IBitmap mTrackBitmap;
    IBitmap mHandleBitmap;

    // Border size, or "stroke width"
    float mBorderSize;

    bool mReverseY;
    
    bool mMouseDown;

    float mOffsetX;
    float mOffsetY;

    float mPrevX;
    float mPrevY;
};

#endif
