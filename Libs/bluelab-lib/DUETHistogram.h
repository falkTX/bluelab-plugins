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
//  DUETHistogram.h
//  BL-DUET
//
//  Created by applematuer on 5/3/20.
//
//

#ifndef __BL_DUET__DUETHistogram__
#define __BL_DUET__DUETHistogram__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class DUETHistogram
{
public:
    DUETHistogram(int width, int height, BL_FLOAT maxValue);
    
    virtual ~DUETHistogram();
    
    void Reset();
    
    void Clear();
    
    void AddValue(BL_FLOAT u, BL_FLOAT v, BL_FLOAT value, int sampleIndex = -1);
    
    void Process();
    
    int GetWidth();
    int GetHeight();
    void GetData(WDL_TypedBuf<BL_FLOAT> *data);
    
    void GetIndices(int histoIndex, vector<int> *indices);
    
    void SetTimeSmooth(BL_FLOAT smoothFactor);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;
    
    int mWidth;
    int mHeight;
    
    BL_FLOAT mMaxValue;
    
    // Smooth
    BL_FLOAT mSmoothFactor;
    WDL_TypedBuf<BL_FLOAT> mPrevData;
    
    // Indices of the samples
    vector<vector<int> > mIndices;
};

#endif /* defined(__BL_DUET__DUETHistogram__) */
