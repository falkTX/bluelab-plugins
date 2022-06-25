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
//  StereoWidthGraphDrawer3.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__StereoWidthGraphDrawer3__
#define __BL_StereoWidth__StereoWidthGraphDrawer3__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

// From StereoWidthGraphDrawer2
// BLCircleGraphDrawer: from USTCircleGraphDrawer
//
// StereoWidthGraphDrawer3 from BLCircleGraphDrawer (to get the same style)
//
class GUIHelper12;
class StereoWidthGraphDrawer3 : public GraphCustomDrawer
{
public:
    StereoWidthGraphDrawer3(GUIHelper12 *guiHelper = NULL,
                            const char *title = NULL);
    
    virtual ~StereoWidthGraphDrawer3() {}
    
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
protected:
    bool mTitleSet;
    char mTitleText[256];

    // Style
    float mCircleLineWidth;
    IColor mLinesColor;
    IColor mTextColor;
    int mOffsetX;
    int mTitleOffsetY;
};

#endif

#endif /* defined(__BL_StereoWidth__StereoWidthGraphDrawer3__) */
