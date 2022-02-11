//
//  USTWidthAdjuster2.h
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __UST__USTWidthAdjuster2__
#define __UST__USTWidthAdjuster2__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// More intuitive smoothing

class USTWidthAdjuster2
{
public:
    USTWidthAdjuster2();
    
    virtual ~USTWidthAdjuster2();
    
    void Reset();
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(BL_FLOAT smoothFactor);
    
    void SetWidth(BL_FLOAT width);
    
    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);
    
    void Update();
    
    BL_FLOAT GetLimitedWidth() const;
    
protected:
    BL_FLOAT ComputeCorrelation(const WDL_TypedBuf<BL_FLOAT> mSamples[2],
                              BL_FLOAT width);

    BL_FLOAT ComputeFirstGoodCorrWidth();
    
    
    bool mIsEnabled;
    
    BL_FLOAT mSmoothFactor;
    BL_FLOAT mPrevWidth;
    
    // Width from the user
    BL_FLOAT mFixedWidth;
    
    // First width with no correlation
    BL_FLOAT mGoodCorrWidth;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
};

#endif /* defined(__UST__USTWidthAdjuster2__) */
