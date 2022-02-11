//
//  USTWidthAdjuster.h
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __UST__USTWidthAdjuster__
#define __UST__USTWidthAdjuster__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// Smoothing over time does not work well

class USTWidthAdjuster
{
public:
    USTWidthAdjuster();
    
    virtual ~USTWidthAdjuster();
    
    void Reset();
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetReleaseMillis(BL_FLOAT release);
    
    void SetWidth(BL_FLOAT width);
    
    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);
    
    BL_FLOAT GetLimitedWidth() const;
    
protected:
    BL_FLOAT ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> mSamples[2],
                              BL_FLOAT width);

    BL_FLOAT ComputeFirstGoodCorrWidth();
    
    bool mIsEnabled;
    
    // Release time
    unsigned long long mReleaseMillis;
    
    // Width from the user
    BL_FLOAT mFixedWidth;
    
    // First width with no correlation
    BL_FLOAT mGoodCorrWidth;
    
    unsigned long long mPrevTime;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
};

#endif /* defined(__UST__USTWidthAdjuster__) */
