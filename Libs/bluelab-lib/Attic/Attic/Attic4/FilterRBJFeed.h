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
    
    void SetCutoffFreq(BL_FLOAT freq);
    
    void SetQFactor(BL_FLOAT q);
    
    void SetSampleRate(BL_FLOAT sampleRate);
    
    BL_FLOAT Process(BL_FLOAT sample);
    
    //void Process(WDL_TypedBuf<BL_FLOAT> *result,
    //             const WDL_TypedBuf<BL_FLOAT> &samples);
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples);
    
protected:
    void FeedWithPrevSamples();
    
    FilterRBJ *mFilter;
    int mNumPrevSamples;
    
    WDL_TypedBuf<BL_FLOAT> mPrevSamples;
};

#endif /* defined(__BL_InfraSynth__FilterRBJFeed__) */
