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
//  PanogramGraphDrawer.h
//  BL-Pano
//
//  Created by applematuer on 8/9/19.
//
//

#ifndef __BL_Pano__PanogramGraphDrawer__
#define __BL_Pano__PanogramGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

class PanogramGraphDrawer : public GraphCustomDrawer
{
public:
    enum ViewOrientation
    {
        HORIZONTAL = 0,
        VERTICAL
    };
    
    PanogramGraphDrawer();
    
    virtual ~PanogramGraphDrawer();

    // The graph will destroy it automatically
    bool IsOwnedByGraph() override { return true; }
    
    // Draw after everything
    void PostDraw(NVGcontext *vg, int width, int height) override;

    void SetViewOrientation(ViewOrientation orientation);

protected:
    ViewOrientation mViewOrientation;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Pano__PanogramGraphDrawer__) */
