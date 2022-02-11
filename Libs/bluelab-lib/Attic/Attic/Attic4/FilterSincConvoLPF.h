//
//  FilterSincConvoLPF.h
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#ifndef __UST__FilterSincConvoLPF__
#define __UST__FilterSincConvoLPF__

#define DEFAULT_FILTER_SIZE 64

#include "IPlug_include_in_plug_hdr.h"

// See: https://tomroelandts.com/articles/how-to-create-a-simple-low-pass-filter

class FastRTConvolver3;
class FilterSincConvoLPF
{
public:
    FilterSincConvoLPF();
    
    virtual ~FilterSincConvoLPF();
    
    void Init(BL_FLOAT fc, BL_FLOAT sampleRate, int filterSize = DEFAULT_FILTER_SIZE);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mFilterData;
    
    FastRTConvolver3 *mConvolver;
};

#endif /* defined(__UST__FilterSincConvoLPF__) */
