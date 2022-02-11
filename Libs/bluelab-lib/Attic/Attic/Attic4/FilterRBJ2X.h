//
//  FilterRBJ2X.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterRBJ2X__
#define __UST__FilterRBJ2X__

#include <FilterRBJ.h>
#include <CFxRbjFilter.h>

#include "IPlug_include_in_plug_hdr.h"

class FilterRBJ2X : public FilterRBJ
{
public:
    FilterRBJ2X(int type, BL_FLOAT sampleRate, BL_FLOAT cutoffFreq);
    
    FilterRBJ2X(const FilterRBJ2X &other);
    
    virtual ~FilterRBJ2X();
    
    void SetCutoffFreq(BL_FLOAT freq);
    
    void SetSampleRate(BL_FLOAT sampleRate);
    
    void SetQFactor(BL_FLOAT q);
    
    BL_FLOAT Process(BL_FLOAT sample);
    
    //void Process(WDL_TypedBuf<BL_FLOAT> *result,
    //             const WDL_TypedBuf<BL_FLOAT> &samples);
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples);
                 
protected:
    void CalcFilterCoeffs();
    
    int mType;
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    BL_FLOAT mQFactor;
    
    CFxRbjFilter *mFilters[2];
};

#endif /* defined(__UST__FilterRBJ2X__) */
