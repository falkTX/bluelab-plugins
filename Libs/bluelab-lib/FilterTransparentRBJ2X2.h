//
//  FilterTransparentRBJ2X2.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterTransparentRBJ2X2__
#define __UST__FilterTransparentRBJ2X2__

#include <vector>
using namespace std;

#include <FilterRBJ.h>

#include "IPlug_include_in_plug_hdr.h"

//#define FILTER_2X_CLASS FilterRBJ2X

// Origin: chain 2 all pass filters (doesn't work)
//#define FILTER_2X_CLASS FilterRBJ2X2

// Seems to works well with a single all pass filter
// (no need to chain 2 allpass filter)
#define TRANSPARENT_RBJ_2X2_FILTER_2X_CLASS FilterRBJ1X

// FilterTransparentRBJ2X:
// Low pass and high pass filters summed
// in order to have only the delay
//
// FilterTransparentRBJ2X:
// Use a single all pass filter instead of two filters
//

class FilterRBJ1X;
class FilterRBJ2X;
class FilterRBJ2X2;
class FilterTransparentRBJ2X2 : public FilterRBJ
{
public:
    FilterTransparentRBJ2X2(BL_FLOAT sampleRate,
                           BL_FLOAT cutoffFreq);
    
    FilterTransparentRBJ2X2(const FilterTransparentRBJ2X2 &other);
    
    virtual ~FilterTransparentRBJ2X2();
    
    void SetCutoffFreq(BL_FLOAT freq) override;
    
    void SetQFactor(BL_FLOAT q) override;
    
    void SetSampleRate(BL_FLOAT sampleRate) override;
    
    BL_FLOAT Process(BL_FLOAT sample) override;
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) override;
                 
protected:
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    
    TRANSPARENT_RBJ_2X2_FILTER_2X_CLASS *mFilter;
};

#endif /* defined(__UST__BL_FLOATRBJFilter__) */
