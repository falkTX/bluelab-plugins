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
