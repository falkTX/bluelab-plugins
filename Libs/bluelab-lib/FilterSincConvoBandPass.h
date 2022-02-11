//
//  FilterSincConvoBandPass.h
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#ifndef __UST__FilterSincConvoBandPass__
#define __UST__FilterSincConvoBandPass__

#define DEFAULT_FILTER_SIZE 64

#include "IPlug_include_in_plug_hdr.h"

// See: https://tomroelandts.com/articles/how-to-create-simple-band-pass-and-band-reject-filters
// and (simulator): https://fiiir.com/
//
class FastRTConvolver3;
class FilterSincConvoBandPass
{
public:
    FilterSincConvoBandPass();
    
    FilterSincConvoBandPass(const FilterSincConvoBandPass &other);
    
    virtual ~FilterSincConvoBandPass();
    
    void Init(BL_FLOAT fl, BL_FLOAT fh,
              BL_FLOAT sampleRate, int filterSize = DEFAULT_FILTER_SIZE);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples);
    
protected:
    void ComputeFilter(BL_FLOAT fc, int filterSize,
                       BL_FLOAT sampleRate,
                       WDL_TypedBuf<BL_FLOAT> *filterData,
                       bool highPass);

    
    WDL_TypedBuf<BL_FLOAT> mFilterData;
    
    FastRTConvolver3 *mConvolver;
    
    BL_FLOAT mFL;
    BL_FLOAT mFH;
    
    int mFilterSize;
};

#endif /* defined(__UST__FilterSincConvoBandPass__) */
