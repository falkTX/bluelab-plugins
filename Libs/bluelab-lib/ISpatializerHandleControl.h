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
 
#ifndef ISPATIALIZER_HANDLE_CONTROL_H
#define ISPATIALIZER_HANDLE_CONTROL_H

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class ISpatializerHandleControl : public IControl
{
 public:
    ISpatializerHandleControl(const IRECT& bounds,
                              float minAngle, float maxAngle, bool reverseY,
                              const IBitmap& handleBitmap,
                              int paramIdx = kNoParameter);

    virtual ~ISpatializerHandleControl();
    
    void Draw(IGraphics& g) override;

    void OnMouseDown(float x, float y, const IMouseMod& mod);
    
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod& mod) override;
        
 protected:
    void DrawHandle(IGraphics& g);

    void PixelsToParam(float *y);
    void ParamToPixels(float val, float *x, float *y);
    
    //
    IBitmap mHandleBitmap;

    float mMinAngle;
    float mMaxAngle;

    // For fine ma,nip with shift
    float mPrevX;
    float mPrevY;

    bool mReverseY;
};

#endif
