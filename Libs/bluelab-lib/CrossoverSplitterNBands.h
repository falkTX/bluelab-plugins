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
//  CrossoverSplitterNBands.h
//  UST
//
//  Created by applematuer on 8/24/19.
//
//

#ifndef __UST__CrossoverSplitterNBands__
#define __UST__CrossoverSplitterNBands__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// NOTE: there seems to have mistakes in "DSP-Cpp-filters"
// So I don't know if there is a problem

// Crossover splitter with arbitrary number of bands
// Try to have perfect splitter
//
// See (for explanation): https://www.modernmetalproduction.com/linkwitz-riley-crossovers-digital-multiband-processing/
//
// and (for filters): https://github.com/dimtass/DSP-Cpp-filters
//
//
// and (for correct allpass 2nd order): https://forum.cockos.com/showthread.php?t=62016
//
// Use Linkwitz-Riley Crossover
//

// TODO: check for crackles when changing cutoff freqs,
// if problem, use the trick of feeding filters like in CrossoverSplitter5Bands

// TODO: check confusion fc / Q in AllPassFilter 2nd order

#define OPTIM_AVOID_NEW 1

class CrossoverSplitterNBands
{
public:
    CrossoverSplitterNBands(int numBands, BL_FLOAT cutoffFreqs[], BL_FLOAT sampleRate);
    
    virtual ~CrossoverSplitterNBands();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetCutoffFreqs(BL_FLOAT freqs[]);
    
    int GetNumBands();
    
    BL_FLOAT GetCutoffFreq(int freqNum);
    void SetCutoffFreq(int freqNum, BL_FLOAT freq);
    
    void Split(BL_FLOAT sample, BL_FLOAT result[]);
    
    void Split(const WDL_TypedBuf<BL_FLOAT> &samples, WDL_TypedBuf<BL_FLOAT> result[]);
    
protected:
    void CreateFilters(BL_FLOAT sampleRate);
    
    void SetFiltersValues();
    
    //
    typedef void *Filter;
    
    BL_FLOAT mSampleRate;
    
    int mNumBands;
    vector<BL_FLOAT> mCutoffFreqs;
    
    vector<vector<Filter> > mFilterChains;
    
#if OPTIM_AVOID_NEW
    BL_FLOAT *mTmpResultCross;
    BL_FLOAT *mTmpResultCross2;
#endif
};

#endif /* defined(__UST__CrossoverSplitterNBands__) */
