//
//  FilterRBJNXEx.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterRBJNXEx__
#define __UST__FilterRBJNXEx__

#include <vector>
using namespace std;

#include <CFxRbjFilter.h>

#include "IPlug_include_in_plug_hdr.h"

#include <FilterRBJ.h>

#if 0
TODO: integrate param smoothers directly inside this class
#endif

// Add the possibilty to mix between filtered data and dry data
// Do it cleanly, with adding all pass filters
class FilterRBJNXEx : public FilterRBJ
{
public:
    FilterRBJNXEx(int numFilters, int type,
               BL_FLOAT sampleRate, BL_FLOAT cutoffFreq,
               BL_FLOAT QFactor = 0.707, BL_FLOAT gain = 0.0);
    
    FilterRBJNXEx(const FilterRBJNXEx &other);
    
    virtual ~FilterRBJNXEx();
    
    void SetCutoffFreq(BL_FLOAT freq) override;
    
    void SetQFactor(BL_FLOAT q) override;
    
    void SetSampleRate(BL_FLOAT sampleRate) override;
    
    BL_FLOAT Process(BL_FLOAT sample) override;
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) override;

    void SetMix(BL_FLOAT mix);
    
protected:
    void CalcFilterCoeffs();
    
    int mNumFilters;
    int mType;
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    BL_FLOAT mQFactor;
    BL_FLOAT mGain;
    
    BL_FLOAT mMix;
    
    vector<CFxRbjFilter *> mFilters;
    vector<CFxRbjFilter *> mBypassFilters;
};

#endif /* defined(__UST__BL_FLOATFilterRBJ__) */
