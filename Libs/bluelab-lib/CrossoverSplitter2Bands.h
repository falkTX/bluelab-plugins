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
//  CrossoverSplitter2Bands.h
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __UST__CrossoverSplitter2Bands__
#define __UST__CrossoverSplitter2Bands__

#include "IPlug_include_in_plug_hdr.h"

class FilterLR2Crossover;
class FilterLR4Crossover;

// BAD (not transparent)
//#define FILTER_CLASS FilterLR2Crossover

// GOOD
#define FILTER_CLASS FilterLR4Crossover

class CrossoverSplitter2Bands
{
public:
    CrossoverSplitter2Bands(BL_FLOAT sampleRate);
    
    virtual ~CrossoverSplitter2Bands();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetCutoffFreq(BL_FLOAT freq);
    
    void Split(BL_FLOAT sample, BL_FLOAT result[2]);
    
    void Split(const WDL_TypedBuf<BL_FLOAT> &samples, WDL_TypedBuf<BL_FLOAT> result[2]);
    
protected:
    void FeedPrevSamples();
    
    FILTER_CLASS *mFilter;
    
    BL_FLOAT mSampleRate;
    
    // For feeding the filters when cutoff freq changes
    // (to ensure continuity)
    WDL_TypedBuf<BL_FLOAT> mPrevSamples;
    bool mFeedPrevSamples;
};

#endif /* defined(__UST__CrossoverSplitter2Bands__) */
