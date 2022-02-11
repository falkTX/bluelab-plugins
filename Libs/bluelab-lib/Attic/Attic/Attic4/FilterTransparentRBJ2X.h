//
//  FilterTransparentRBJ2X.h
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#ifndef __UST__FilterTransparentRBJ2X__
#define __UST__FilterTransparentRBJ2X__

#include <vector>
using namespace std;

#include <FilterRBJ.h>

#include "IPlug_include_in_plug_hdr.h"

//#define FILTER_2X_CLASS FilterRBJ2X
#define FILTER_2X_CLASS FilterRBJ2X2

// Low pass and high pass filters summed
// in order to have only the delay
class FilterRBJ2X;
class FilterRBJ2X2;
class FilterTransparentRBJ2X : public FilterRBJ
{
public:
    FilterTransparentRBJ2X(BL_FLOAT sampleRate,
                           BL_FLOAT cutoffFreq);
    
    FilterTransparentRBJ2X(const FilterTransparentRBJ2X &other);
    
    virtual ~FilterTransparentRBJ2X();
    
    void SetCutoffFreq(BL_FLOAT freq);
    
    void SetQFactor(BL_FLOAT q);
    
    void SetSampleRate(BL_FLOAT sampleRate);
    
    BL_FLOAT Process(BL_FLOAT sample);
    
    //void Process(WDL_TypedBuf<BL_FLOAT> *result,
    //             const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples);
                 
protected:
    BL_FLOAT mSampleRate;
    BL_FLOAT mCutoffFreq;
    
    FilterRBJ2X2 *mFilters[2];
};

#endif /* defined(__UST__BL_FLOATRBJFilter__) */
