//
//  CrossoverSplitter3Bands.h
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __UST__CrossoverSplitter3Bands__
#define __UST__CrossoverSplitter3Bands__

#include "IPlug_include_in_plug_hdr.h"

class FilterLR2Crossover;
class FilterLR4Crossover;

// BAD (not transparent)
//#define FILTER_CLASS FilterLR2Crossover

// GOOD
#define FILTER_CLASS FilterLR4Crossover

// Num filters
#define CS3B_NUM_FILTERS 2

class CrossoverSplitter3Bands
{
public:
    CrossoverSplitter3Bands(BL_FLOAT cutoffFreqs[CS3B_NUM_FILTERS],
                            BL_FLOAT sampleRate);
    
    virtual ~CrossoverSplitter3Bands();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetCutoffFreqs(BL_FLOAT freqs[CS3B_NUM_FILTERS]);
    
    void SetCutoffFreq(int freqNum, BL_FLOAT freq);
    
    void Split(BL_FLOAT sample, BL_FLOAT result[3]);
    
    void Split(const WDL_TypedBuf<BL_FLOAT> &samples, WDL_TypedBuf<BL_FLOAT> result[3]);
    
    int GetNumFilters();
    
    // CRASHES
    //void GetFilter(int index, FILTER_CLASS *filter);
    
    const FILTER_CLASS *GetFilter(int index);
    
protected:
    void FeedPrevSamples();
    
    FILTER_CLASS *mFilters[CS3B_NUM_FILTERS];
    
    BL_FLOAT mSampleRate;
    
    // For feeding the filters when cutoff freq changes
    // (to ensure continuity)
    WDL_TypedBuf<BL_FLOAT> mPrevSamples;
    bool mFeedPrevSamples;
};

#endif /* defined(__UST__CrossoverSplitter3Bands__) */
