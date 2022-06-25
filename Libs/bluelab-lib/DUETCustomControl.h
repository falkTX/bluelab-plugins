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
 
//
//  DUETCustomControl.h
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifndef __BL_Panogram__DUETCustomControl__
#define __BL_Panogram__DUETCustomControl__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>
#include <DUETPlugInterface.h>

using namespace iplug;

class SpectrogramDisplayScroll;

class DUETCustomControl : public GraphCustomControl
{
public:
    DUETCustomControl(DUETPlugInterface *plug);
    
    virtual ~DUETCustomControl() {}
    
    void Reset();
    
    void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod) override;
    
    bool OnKeyDown(float x, float y, const IKeyPress& key) override;
    
protected:
    DUETPlugInterface *mPlug;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Panogram__DUETCustomControl__) */
