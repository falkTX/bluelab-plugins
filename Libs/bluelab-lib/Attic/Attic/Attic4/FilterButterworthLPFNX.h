//
//  FilterButterworthLPFNX.h
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#ifndef __UST__FilterButterworthLPFNX__
#define __UST__FilterButterworthLPFNX__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class FilterButterworthLPF;
class FilterButterworthLPFNX
{
public:
    FilterButterworthLPFNX(int numFilters);
    
    virtual ~FilterButterworthLPFNX();
    
    void Init(BL_FLOAT resoFreq, BL_FLOAT sampleRate);
    
    BL_FLOAT Process(BL_FLOAT sample);
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    vector<FilterButterworthLPF *> mFilters;
};

#endif /* defined(__UST__FilterButterworthLPFNX__) */
