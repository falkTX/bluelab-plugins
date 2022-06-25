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
//  FilterRBJFeed.h
//  BL-InfraSynth
//
//  Created by applematuer on 9/15/19.
//
//

#ifndef __BL_InfraSynth__FilterRBJFeed__
#define __BL_InfraSynth__FilterRBJFeed__

#include "IPlug_include_in_plug_hdr.h"

#include <FilterRBJ.h>

// Decorator
// Feed the filter with previous data when resetting params
class FilterRBJFeed : public FilterRBJ
{
public:
    FilterRBJFeed(FilterRBJ *filter,
                  int numPrevSamples);
    
    FilterRBJFeed(const FilterRBJFeed &other);
    
    virtual ~FilterRBJFeed();
    
    void SetCutoffFreq(BL_FLOAT freq) override;
    
    void SetQFactor(BL_FLOAT q) override;
    
    void SetSampleRate(BL_FLOAT sampleRate) override;
    
    BL_FLOAT Process(BL_FLOAT sample) override;
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) override;
    
protected:
    void FeedWithPrevSamples();
    
    FilterRBJ *mFilter;
    int mNumPrevSamples;
    
    WDL_TypedBuf<BL_FLOAT> mPrevSamples;
};

#endif /* defined(__BL_InfraSynth__FilterRBJFeed__) */
