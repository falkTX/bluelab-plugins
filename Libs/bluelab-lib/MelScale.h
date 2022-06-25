/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
    void HzToMelFilter(WDL_TypedBuf<BL_FLOAT> *result,
                       const WDL_TypedBuf<BL_FLOAT> &magns,
                       BL_FLOAT sampleRate, int numFilters);
    // Inverse
    void MelToHzFilter(WDL_TypedBuf<BL_FLOAT> *result,
                       const WDL_TypedBuf<BL_FLOAT> &magns,
                       BL_FLOAT sampleRate, int numFilters);
    
    static void MelToMFCC(WDL_TypedBuf<BL_FLOAT> *result,
                          const WDL_TypedBuf<BL_FLOAT> &melMagns);
    static void MFCCToMel(WDL_TypedBuf<BL_FLOAT> *result,
                          const WDL_TypedBuf<BL_FLOAT> &mfccMagns);
    
protected:
    /*static*/ BL_FLOAT ComputeTriangleAreaBetween(BL_FLOAT txmin,
                                                   BL_FLOAT txmid,
                                                   BL_FLOAT txmax,
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
    
    void CreateFilterBankHzToMel(FilterBank *filterBank, int dataSize,
                                 BL_FLOAT sampleRate, int numFilters);
    void CreateFilterBankMelToHz(FilterBank *filterBank, int dataSize,
                                 BL_FLOAT sampleRate, int numFilters);
    void ApplyFilterBank(WDL_TypedBuf<BL_FLOAT> *result,
                         const WDL_TypedBuf<BL_FLOAT> &magns,
                         const FilterBank &filterBank);
    
    //
    FilterBank mHzToMelFilterBank;
    FilterBank mMelToHzFilterBank;

private:
    // Tmp buffers
    vector<BL_FLOAT> mTmpBuf0;
};

#endif /* MelScale_hpp */
