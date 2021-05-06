//
//  CrossoverSplitterNBands4.h
//  UST
//
//  Created by applematuer on 8/24/19.
//
//

#ifndef __UST__CrossoverSplitterNBands4__
#define __UST__CrossoverSplitterNBands4__

#include <vector>
using namespace std;

#include <FilterRBJ.h>

#include "IPlug_include_in_plug_hdr.h"

// CrossoverSplitterNBands: use 2nd order and phase compensation
// CrossoverSplitterNBands2: use different filter class, and 4th order
//
// NOTE: this crossover seems ok !
//
// CrossoverSplitterNBands3: use NFilterRBJ
//
// CrossoverSplitterNBands4: use optimizations for 2 chained filters

#define OPTIM_AVOID_NEW 1

class CrossoverSplitterNBands4
{
public:
    CrossoverSplitterNBands4(int numBands, BL_FLOAT cutoffFreqs[],
                             BL_FLOAT sampleRate);
    
    CrossoverSplitterNBands4(const CrossoverSplitterNBands4 &other);
    
    virtual ~CrossoverSplitterNBands4();
    
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
    BL_FLOAT mSampleRate;
    
    int mNumBands;
    vector<BL_FLOAT> mCutoffFreqs;
    
    vector<vector<FilterRBJ *> > mFilterChains;
    
#if OPTIM_AVOID_NEW
    BL_FLOAT *mTmpResultCross;
    BL_FLOAT *mTmpResultCross2;
#endif
};

#endif /* defined(__UST__CrossoverSplitterNBands4__) */
