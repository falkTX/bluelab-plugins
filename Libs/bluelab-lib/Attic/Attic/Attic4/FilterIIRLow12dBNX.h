//
//  FilterIIRLow12dBNX.h
//  UST
//
//  Created by applematuer on 8/10/20.
//
//

#ifndef __UST__FilterIIRLow12dBNX__
#define __UST__FilterIIRLow12dBNX__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class FilterIIRLow12dB;
class FilterIIRLow12dBNX
{
public:
    FilterIIRLow12dBNX(int numFilters);
    
    virtual ~FilterIIRLow12dBNX();
    
    void Init(BL_FLOAT resoFreq, BL_FLOAT sampleRate);
    
    BL_FLOAT Process(BL_FLOAT sample);
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    vector<FilterIIRLow12dB *> mFilters;
};

#endif /* defined(__UST__FilterIIRLow12dBNX__) */
