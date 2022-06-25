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
//  MiniView.h
//  BL-Ghost
//
//  Created by Pan on 15/06/18.
//
//

#ifndef __BL_Ghost__MiniView__
#define __BL_Ghost__MiniView__

#include "IPlug_include_in_plug_hdr.h"

class NVGcontext;

class MiniView
{
public:
    MiniView(int maxNumPoints,
             BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    virtual ~MiniView();
  
    void Display(NVGcontext *vg, int width, int height);
  
    bool IsPointInside(int x, int y, int width, int height);
    
    void SetData(const WDL_TypedBuf<BL_FLOAT> &data);
    
    void GetWaveForm(WDL_TypedBuf<BL_FLOAT> *waveform);
    
    void SetBounds(BL_FLOAT minNormX, BL_FLOAT maxNormX);
    
    BL_FLOAT GetDrag(int dragX, int width);
    
protected:
    BL_FLOAT mBounds[4];
    
    int mMaxNumPoints;
    
    WDL_TypedBuf<BL_FLOAT> mWaveForm;
    
    BL_FLOAT mMinNormX;
    BL_FLOAT mMaxNormX;
};

#endif /* defined(__BL_Ghost__MiniView__) */
