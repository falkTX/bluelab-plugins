//
//  FilterLR2Crossover.h
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __UST__FilterLR2Crossover__
#define __UST__FilterLR2Crossover__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

// See: https://www.musicdsp.org/en/latest/Filters/266-4th-order-linkwitz-riley-filters.html?highlight=Linkwitz
// Implementation of LR2 with DFII by moc.liamg@321tiloen

// PROBLEM: doing multiband crossover with this filter
// is not transparent at all when summing all bands

class FilterLR2Crossover
{
public:
    FilterLR2Crossover(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate);
    
    virtual ~FilterLR2Crossover();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate);
    
    void SetCutoffFreq(BL_FLOAT freq);
    
    void Process(BL_FLOAT inSample, BL_FLOAT *lpOutSample, BL_FLOAT *hpOutSample);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                 WDL_TypedBuf<BL_FLOAT> *lpOutSamples,
                 WDL_TypedBuf<BL_FLOAT> *hpOutSamples);
    
protected:
    void Init();
    
    BL_FLOAT mCutoffFreq;
    BL_FLOAT mSampleRate;
    
    //
    BL_FLOAT mLp_xm0;
    BL_FLOAT mLp_xm1;
    
    BL_FLOAT mHp_xm0;
    BL_FLOAT mHp_xm1;
    
    BL_FLOAT mA0_lp;
    BL_FLOAT mA1_lp;
    BL_FLOAT mA2_lp;
    
    BL_FLOAT mA0_hp;
    BL_FLOAT mA1_hp;
    BL_FLOAT mA2_hp;
    
    BL_FLOAT mB1;
    BL_FLOAT mB2;
};

#endif /* defined(__UST__FilterLR2Crossover__) */
