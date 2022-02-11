//
//  MelScale.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/15/20.
//
//

#ifndef MelScale_hpp
#define MelScale_hpp

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class MelScale
{
public:
    MelScale();
    virtual ~MelScale();
    
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
    static void HzToMelFilter_old(WDL_TypedBuf<BL_FLOAT> *result,
                                  const WDL_TypedBuf<BL_FLOAT> &magns,
                                  BL_FLOAT sampleRate, int numFilters);
    
    void HzToMelFilter(WDL_TypedBuf<BL_FLOAT> *result,
                       const WDL_TypedBuf<BL_FLOAT> &magns,
                       BL_FLOAT sampleRate, int numFilters);
    
protected:
    static BL_FLOAT ComputeTriangleAreaBetween(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                                               BL_FLOAT x0, BL_FLOAT x1);
    static BL_FLOAT ComputeTriangleY(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                                     BL_FLOAT x);
    
    class FilterBank
    {
    public:
        FilterBank();
        
        FilterBank(int dataSize, BL_FLOAT sampleRate, int numFilters);
        
        virtual ~FilterBank();
        
    protected:
        friend class MelScale;
        
        int mDataSize;
        BL_FLOAT mSampleRate;
        int mNumFilters;
        
        struct Filter
        {
            WDL_TypedBuf<BL_FLOAT> mData;
            int mBounds[2];
        };
        
        vector<Filter> mFilters;
    };
    
    void CreateFilterBand(FilterBank *filterBank, int dataSize,
                          BL_FLOAT sampleRate, int numFilters);
    void ApplyFilterBank(WDL_TypedBuf<BL_FLOAT> *result,
                         const WDL_TypedBuf<BL_FLOAT> &magns,
                         const FilterBank &filterBank);
    
    FilterBank mHzToMelFilterBank;
};

#endif /* MelScale_hpp */
