//
//  FilterRBJ2X2.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterRBJ2X2__
#define __UST__FilterRBJ2X2__

#include <FilterRBJ.h>
#include <CFxRbjFilter.h>

#include "IPlug_include_in_plug_hdr.h"

// Optimization: use CFxRbjFilter2, which chains the coefficients automatically
class FilterRBJ2X2 : public FilterRBJ
{
public:
    FilterRBJ2X2(int type, BL_FLOAT sampleRate, BL_FLOAT cutoffFreq);
    
    FilterRBJ2X2(const FilterRBJ2X2 &other);
    
    virtual ~FilterRBJ2X2();
    
    void SetCutoffFreq(BL_FLOAT freq) override;
    
    void SetSampleRate(BL_FLOAT sampleRate) override;
    
    void SetQFactor(BL_FLOAT q) override;
    
    BL_FLOAT Process(BL_FLOAT sample) override;
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) override;
                 
protected:
    void CalcFilterCoeffs();
    
    int mType;
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    BL_FLOAT mQFactor;
    
    CFxRbjFilter2X *mFilter;
};

#endif /* defined(__UST__FilterRBJ2X2__) */
