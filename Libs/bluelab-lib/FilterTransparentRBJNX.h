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
//  FilterTransparentRBJNX.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterTransparentRBJNX__
#define __UST__FilterTransparentRBJNX__

#include <vector>
using namespace std;

#include <FilterRBJ.h>

#include "IPlug_include_in_plug_hdr.h"

// Low pass and high pass filters summed
// in order to have only the delay

class FilterRBJNX;
class FilterTransparentRBJNX : public FilterRBJ
{
public:
    FilterTransparentRBJNX(int numFilters,
                         BL_FLOAT sampleRate,
                         BL_FLOAT cutoffFreq);
    
    FilterTransparentRBJNX(const FilterTransparentRBJNX &other);
    
    virtual ~FilterTransparentRBJNX();
    
    void SetCutoffFreq(BL_FLOAT freq) override;
    
    void SetQFactor(BL_FLOAT q) override;
    
    void SetSampleRate(BL_FLOAT sampleRate) override;
    
    BL_FLOAT Process(BL_FLOAT sample) override;
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) override;
    
protected:
    int mNumFilters;
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    
    FilterRBJNX *mFilters[2];
};

#endif /* defined(__UST__BL_FLOATRBJFilter__) */
