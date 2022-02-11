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
