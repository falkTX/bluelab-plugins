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
