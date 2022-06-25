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
//  FilterTransparentRBJ2X.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterTransparentRBJ2X__
#define __UST__FilterTransparentRBJ2X__

#include <vector>
using namespace std;

#include <FilterRBJ.h>

#include "IPlug_include_in_plug_hdr.h"

// Use 1 HP and 1 LP, and sum the result
// Each of the LP and HP are the chain of two filters (e.g 2 chained HP).
//
// NOTE: this works, but requires 2 filters
// (prefer using 1 single all pass filter, not chained)

//#define FILTER_2X_CLASS FilterRBJ2X
#define TRANSPARENT_RBJ_2X_FILTER_2X_CLASS FilterRBJ2X2

// Low pass and high pass filters summed
// in order to have only the delay
class FilterRBJ2X;
class FilterRBJ2X2;
class FilterTransparentRBJ2X : public FilterRBJ
{
public:
    FilterTransparentRBJ2X(BL_FLOAT sampleRate,
                           BL_FLOAT cutoffFreq);
    
    FilterTransparentRBJ2X(const FilterTransparentRBJ2X &other);
    
    virtual ~FilterTransparentRBJ2X();
    
    void SetCutoffFreq(BL_FLOAT freq) override;
    
    void SetQFactor(BL_FLOAT q) override;
    
    void SetSampleRate(BL_FLOAT sampleRate) override;
    
    BL_FLOAT Process(BL_FLOAT sample) override;
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) override;
                 
protected:
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    
    FilterRBJ2X2 *mFilters[2];
};

#endif /* defined(__UST__BL_FLOATRBJFilter__) */
