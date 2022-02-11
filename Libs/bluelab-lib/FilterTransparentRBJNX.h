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
