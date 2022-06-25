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
//  MiniView2.h
//  BL-Ghost
//
//  Created by Pan on 15/06/18.
//
//

#ifndef __BL_Ghost__MiniView2__
#define __BL_Ghost__MiniView2__

#ifdef IGRAPHICS_NANOVG

#include <BLTypes.h>
#include <GraphControl12.h>

#include "IPlug_include_in_plug_hdr.h"

class NVGcontext;

class MiniView2 : public GraphCustomDrawer
{
public:
    struct State
    {
        BL_FLOAT mMinNormX;
        BL_FLOAT mMaxNormX;
    };
    
    MiniView2(int maxNumPoints,
              BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1,
              State *state = NULL);
    
    virtual ~MiniView2();

    State *GetState();
    
    void PostDraw(NVGcontext *vg, int width, int height) override;
    
    bool IsPointInside(int x, int y, int width, int height);
    
    void SetSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void GetWaveForm(WDL_TypedBuf<BL_FLOAT> *waveform);
    
    void SetBounds(BL_FLOAT minNormX, BL_FLOAT maxNormX);
    
    BL_FLOAT GetDrag(int dragX, int width);
    
protected:
    void DrawBackground(NVGcontext *vg, int width, int height);
    
    BL_FLOAT mBounds[4];
    
    int mMaxNumPoints;
    
    WDL_TypedBuf<BL_FLOAT> mWaveForm;

    State *mState;
};

#endif

#endif /* defined(__BL_Ghost__MiniView2__) */
