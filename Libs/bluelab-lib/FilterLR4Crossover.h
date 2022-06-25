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
//  FilterLR4Crossover.h
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#ifndef __UST__FilterLR4Crossover__
#define __UST__FilterLR4Crossover__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

// See: https://www.musicdsp.org/en/latest/Filters/266-4th-order-linkwitz-riley-filters.html?highlight=Linkwitz
// Implementation of LR4 by moc.liamg@321tiloen

// GOOD: doing multiband crossover with this filter
// is transparent when summing all bands


class LRCrossoverFilter;

class FilterLR4Crossover
{
public:
    FilterLR4Crossover(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate);
    
    FilterLR4Crossover();
    
    FilterLR4Crossover(const FilterLR4Crossover &other);
    
    virtual ~FilterLR4Crossover();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate);
    
    BL_FLOAT GetSampleRate() const;
    
    BL_FLOAT GetCutoffFreq() const;
    void SetCutoffFreq(BL_FLOAT freq);
    
    void Process(BL_FLOAT inSample, BL_FLOAT *lpOutSample, BL_FLOAT *hpOutSample);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                 WDL_TypedBuf<BL_FLOAT> *lpOutSamples,
                 WDL_TypedBuf<BL_FLOAT> *hpOutSamples);
    
protected:
    void Init();
    
    BL_FLOAT mCutoffFreq;
    BL_FLOAT mSampleRate;
    
    LRCrossoverFilter *mFilter;
};

#endif /* defined(__UST__FilterLR4Crossover__) */
