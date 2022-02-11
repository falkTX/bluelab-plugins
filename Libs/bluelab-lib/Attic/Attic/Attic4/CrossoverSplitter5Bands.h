//
//  CrossoverSplitter5Bands.h
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __UST__CrossoverSplitter5Bands__
#define __UST__CrossoverSplitter5Bands__

#include "IPlug_include_in_plug_hdr.h"

class FilterLR2Crossover;
class FilterLR4Crossover;

// BAD (not transparent)
//#define FILTER_CLASS FilterLR2Crossover

// GOOD
#define FILTER_CLASS FilterLR4Crossover

// Num filters
#define CS5B_NUM_FILTERS 4

class CrossoverSplitter5Bands
{
public:
    CrossoverSplitter5Bands(BL_FLOAT cutoffFreqs[CS5B_NUM_FILTERS],
                            BL_FLOAT sampleRate);
    
    virtual ~CrossoverSplitter5Bands();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetCutoffFreqs(BL_FLOAT freqs[CS5B_NUM_FILTERS]);
    
    BL_FLOAT GetCutoffFreq(int freqNum);
    void SetCutoffFreq(int freqNum, BL_FLOAT freq);
    
    void Split(BL_FLOAT sample, BL_FLOAT result[5]);
    
    void Split(const WDL_TypedBuf<BL_FLOAT> &samples, WDL_TypedBuf<BL_FLOAT> result[5]);
    
    int GetNumFilters();
    
    // CRASHES
    //void GetFilter(int index, FILTER_CLASS *filter);
    
    const FILTER_CLASS *GetFilter(int index);
    
protected:
    void FeedPrevSamples();
    
    FILTER_CLASS *mFilters[CS5B_NUM_FILTERS];
    
    BL_FLOAT mSampleRate;
    
    // For feeding the filters when cutoff freq changes
    // (to ensure continuity)
    WDL_TypedBuf<BL_FLOAT> mPrevSamples;
    bool mFeedPrevSamples;
};

#endif /* defined(__UST__CrossoverSplitter5Bands__) */
