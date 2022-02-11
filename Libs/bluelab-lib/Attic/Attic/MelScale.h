//
//  MelScale.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/15/20.
//
//

#ifndef MelScale_hpp
#define MelScale_hpp

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class MelScale
{
public:
    static BL_FLOAT HzToMel(BL_FLOAT freq);
    static BL_FLOAT MelToHz(BL_FLOAT mel);
    
    // Quick transformations, without filtering
    static void HzToMel(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                        const WDL_TypedBuf<BL_FLOAT> &magns,
                        BL_FLOAT sampleRate);
    static void MelToHz(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                        const WDL_TypedBuf<BL_FLOAT> &magns,
                        BL_FLOAT sampleRate);
    
    // Use real Hz to Mel conversion, using filters
    //static void HzToMelMfcc(WDL_TypedBuf<BL_FLOAT> *result,
    //                        const WDL_TypedBuf<BL_FLOAT> &magns,
    //                        BL_FLOAT sampleRate, int numMelBins);
    
    // Use real Hz to Mel conversion, using filters
    static void HzToMelFilter(WDL_TypedBuf<BL_FLOAT> *result,
                              const WDL_TypedBuf<BL_FLOAT> &magns,
                              BL_FLOAT sampleRate, int numFilters);
    
protected:
    static BL_FLOAT ComputeTriangleAreaBetween(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                                               BL_FLOAT x0, BL_FLOAT x1);

    static BL_FLOAT ComputeTriangleY(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                                     BL_FLOAT x);
    
};

#endif /* MelScale_hpp */
