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
//  BLLissajousGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__BLLissajousGraphDrawer__
#define __BL_StereoWidth__BLLissajousGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

// From StereoWidthGraphDrawer2
// BLLissajousGraphDrawer: from USTLissajousGraphDrawer
//
class GUIHelper12;
class BLLissajousGraphDrawer : public GraphCustomDrawer
{
public:
    BLLissajousGraphDrawer(BL_FLOAT scale,
                           GUIHelper12 *guiHelper = NULL,
                           const char *title = NULL);
    
    virtual ~BLLissajousGraphDrawer();
    
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
protected:
    BL_FLOAT mScale;
    
    bool mTitleSet;
    char mTitleText[256];

    // Style
    float mCircleLineWidth;
    float mLinesWidth;
    IColor mLinesColor;
    IColor mTextColor;
    int mOffsetX;
    int mTitleOffsetY;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__BLLissajousGraphDrawer__) */
