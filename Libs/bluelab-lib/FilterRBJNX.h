//
//  FilterRBJNX.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterRBJNX__
#define __UST__FilterRBJNX__

#include <vector>
using namespace std;

#include <CFxRbjFilter.h>

#include "IPlug_include_in_plug_hdr.h"

#include <FilterRBJ.h>

class FilterRBJNX : public FilterRBJ
{
public:
    FilterRBJNX(int numFilters, int type,
               BL_FLOAT sampleRate, BL_FLOAT cutoffFreq,
               BL_FLOAT QFactor = 0.707, BL_FLOAT gain = 0.0);
    
    FilterRBJNX(const FilterRBJNX &other);
    
    virtual ~FilterRBJNX();
    
    void SetCutoffFreq(BL_FLOAT freq) override;
    
    void SetQFactor(BL_FLOAT q) override; // NEW
    
    void SetSampleRate(BL_FLOAT sampleRate) override;
    
    BL_FLOAT Process(BL_FLOAT sample) override;
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) override;
    
protected:
    void CalcFilterCoeffs();
    
    int mNumFilters;
    int mType;
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    BL_FLOAT mQFactor;
    BL_FLOAT mGain;
    
    vector<CFxRbjFilter *> mFilters;
};

#endif /* defined(__UST__BL_FLOATFilterRBJ__) */
