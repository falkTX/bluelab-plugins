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
 
#ifndef ZOOM_CUSTOM_CONTROL_H
#define ZOOM_CUSTOM_CONTROL_H

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

class ZoomListener
{
public:
    virtual void UpdateZoom(BL_FLOAT zoomChange) = 0;
    virtual void ResetZoom() = 0;
};


class ZoomCustomControl : public GraphCustomControl
{
public:
    ZoomCustomControl(ZoomListener *listener);
    
    virtual ~ZoomCustomControl();
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &pMod) override;
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &pMod) override;
    
    virtual void OnMouseWheel(float x, float y, const IMouseMod &pMod,
                              float d) override;

protected:
    ZoomListener *mListener;

    int mStartDrag[2];
    
    BL_FLOAT mPrevMouseY;
};

#endif

#endif
