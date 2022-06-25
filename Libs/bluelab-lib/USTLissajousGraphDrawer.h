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
//  USTLissajousGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__USTLissajousGraphDrawer__
#define __BL_StereoWidth__USTLissajousGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

// From StereoWidthGraphDrawer2
class USTLissajousGraphDrawer : public GraphCustomDrawer
{
public:
    USTLissajousGraphDrawer(BL_FLOAT scale);
    
    virtual ~USTLissajousGraphDrawer();
    
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
protected:
    BL_FLOAT mScale;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__USTLissajousGraphDrawer__) */
