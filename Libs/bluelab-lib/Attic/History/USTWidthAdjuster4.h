//
//  USTWidthAdjuster4.h
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __UST__USTWidthAdjuster4__
#define __UST__USTWidthAdjuster4__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// More intuitive smoothing

// USTWidthAdjuster3:
// - FIX buffer size problem
// - Decimation to manage high sample rates
//
// - USTWidthAdjuster4: manage to avoid any correlation problem
// anytime (test with SPAN mid/side + SideMinder)
//

class CorrelationComputer;

class USTWidthAdjuster4
{
public:
    USTWidthAdjuster4(BL_FLOAT sampleRate);
    
    virtual ~USTWidthAdjuster4();
    
    void Reset(BL_FLOAT sampleRate);
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(BL_FLOAT smoothFactor);
    
    void SetWidth(BL_FLOAT width);
    
    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);
    
    void Update();
    
    BL_FLOAT GetLimitedWidth() const;
    
    void DBG_SetCorrSmoothCoeff(BL_FLOAT smooth);
    
protected:
    BL_FLOAT ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> samples[2],
                              BL_FLOAT width);

    BL_FLOAT ComputeFirstGoodCorrWidth(WDL_TypedBuf<BL_FLOAT> samples[2]);

    
    bool mIsEnabled;
    
    BL_FLOAT mSmoothFactor;
    BL_FLOAT mPrevWidth;
    
    // Width from the user
    BL_FLOAT mFixedWidth;
    
    // First width with no correlation
    BL_FLOAT mGoodCorrWidth;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
    
    BL_FLOAT mSampleRate;
    
    CorrelationComputer *mCorrComputer;
};

#endif /* defined(__UST__USTWidthAdjuster4__) */
