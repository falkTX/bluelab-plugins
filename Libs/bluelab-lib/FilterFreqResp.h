//
//  FilterFreqResp.h
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#ifndef __UST__FilterFreqResp__
#define __UST__FilterFreqResp__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class FilterLR4Crossover;

class FilterFreqResp
{
public:
    FilterFreqResp();
    
    virtual ~FilterFreqResp();
    
    // For other filters, implement other methods
    void GetFreqResp(WDL_TypedBuf<BL_FLOAT> *freqRespLo,
                     WDL_TypedBuf<BL_FLOAT> *freqRespHi,
                     int numSamples,
                     FilterLR4Crossover *filter);
    
    void GenImpulse(WDL_TypedBuf<BL_FLOAT> *impulse);
    
    void GetFreqResp(const WDL_TypedBuf<BL_FLOAT> &filteredImpulse,
                     WDL_TypedBuf<BL_FLOAT> *freqResp);
    
    // For float GUI
#if (BL_GUI_TYPE_FLOAT!=BL_TYPE_FLOAT)
    void GetFreqResp(const WDL_TypedBuf<BL_FLOAT> &filteredImpulse,
                     WDL_TypedBuf<BL_GUI_FLOAT> *freqResp);
#endif
};

#endif /* defined(__UST__FilterFreqResp__) */
