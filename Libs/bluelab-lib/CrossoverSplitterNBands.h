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
