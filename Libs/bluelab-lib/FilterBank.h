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
//  FilterBank.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/15/20.
//
//

#ifndef FilterBank_hpp
#define FilterBank_hpp

#include <vector>
using namespace std;

#include <BLTypes.h>

#include <Scale.h>

#include "IPlug_include_in_plug_hdr.h"

// From MelScale
// But generalized to arbitrary scale type, not only Mel

class FilterBank
{
public:
    FilterBank(Scale::Type targetScaleType);
    virtual ~FilterBank();

    // NOTE: can decimate or increase the data size
    // as the same time as scaling!
    
    // Use real Hz to Mel conversion, using filters
    void HzToTarget(WDL_TypedBuf<BL_FLOAT> *result,
                    const WDL_TypedBuf<BL_FLOAT> &magns,
                    BL_FLOAT sampleRate, int numFilters);
    // Inverse
    void TargetToHz(WDL_TypedBuf<BL_FLOAT> *result,
                    const WDL_TypedBuf<BL_FLOAT> &magns,
                    BL_FLOAT sampleRate, int numFilters);
    
protected:
    BL_FLOAT ComputeTriangleAreaBetween(BL_FLOAT txmin,
                                        BL_FLOAT txmid,
                                        BL_FLOAT txmax,
                                        BL_FLOAT x0, BL_FLOAT x1);
    static BL_FLOAT ComputeTriangleY(BL_FLOAT txmin, BL_FLOAT txmid, BL_FLOAT txmax,
                                     BL_FLOAT x);

    BL_FLOAT ApplyScale(BL_FLOAT val, BL_FLOAT minFreq, BL_FLOAT maxFreq);
    BL_FLOAT ApplyScaleInv(BL_FLOAT val, BL_FLOAT minFreq, BL_FLOAT maxFreq);

    //
    
    class FilterBankObj
    {
    public:
        FilterBankObj();
        
        FilterBankObj(int dataSize, BL_FLOAT sampleRate, int numFilters);
        
        virtual ~FilterBankObj();
        
    protected:
        friend class FilterBank;
        
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
    
    void CreateFilterBankHzToTarget(FilterBankObj *filterBank, int dataSize,
                                    BL_FLOAT sampleRate, int numFilters);
    void CreateFilterBankTargetToHz(FilterBankObj *filterBank, int dataSize,
                                    BL_FLOAT sampleRate, int numFilters);
    void ApplyFilterBank(WDL_TypedBuf<BL_FLOAT> *result,
                         const WDL_TypedBuf<BL_FLOAT> &magns,
                         const FilterBankObj &filterBank);

    void FixSmallTriangles(BL_FLOAT *fmin, BL_FLOAT *fmax, int dataSize);
        
    // DEBUG
    void DBG_DumpFilterBank(const FilterBankObj &filterBank);
    
    //
    FilterBankObj mHzToTargetFilterBank;
    FilterBankObj mTargetToHzFilterBank;

    Scale::Type mTargetScaleType;
    Scale *mScale;
    
private:
    // Tmp buffers
    vector<BL_FLOAT> mTmpBuf0;
};

#endif /* FilterBank_hpp */
