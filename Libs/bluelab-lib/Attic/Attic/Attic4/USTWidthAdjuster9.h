//
//  USTWidthAdjuster9.h
//  UST
//
//  Created by applematuer on 8/1/19.
//
//

#ifndef __UST__USTWidthAdjuster9__
#define __UST__USTWidthAdjuster9__

#include <vector>
#include <deque>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846264
#endif

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

//#define CORRELATION_COMPUTER_CLASS USTCorrelationComputer2
//#define CORRELATION_COMPUTER_CLASS USTCorrelationComputer3
#define CORRELATION_COMPUTER_CLASS USTCorrelationComputer4

// More intuitive smoothing

// USTWidthAdjuster3:
// - FIX buffer size problem
// - Decimation to manage high sample rates
//
// - USTWidthAdjuster4: manage to avoid any correlation problem
// anytime (test with SPAN mid/side + SideMinder)
//
// - USTWidthAdjuster5: update the width and check at each sample
// (instead of at each buffer)
// (not working well, debug...)
//
// - USTWidthAdjuster6: Use USTCorrelationComputer2
// - USTWidthAdjuster7: Try to fix, and to be closer to SideMinder
// - USTWidthAdjuster8: use real compression on correlation, then find corresponding width
// // Tests to try to get good smooth width after FindGoodCorrWidth() (not really convincing)
// Debug/WIP
//
// - USTWidthAdjuster9: get the gain used in the compressor, and directly apply it to the width
// (simple and looks efficient !)

class USTCorrelationComputer2;
class USTCorrelationComputer3;
class USTCorrelationComputer4;

class USTStereoWidener;
class CParamSmooth;
class CMAParamSmooth;
class CMAParamSmooth2;
class KalmanParamSmooth;

class InstantCompressor;

class USTWidthAdjuster9
{
public:
    USTWidthAdjuster9(BL_FLOAT sampleRate);
    
    virtual ~USTWidthAdjuster9();
    
    void Reset(BL_FLOAT sampleRate);
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(BL_FLOAT smoothFactor);
    void SetWidth(BL_FLOAT width);
    
    void Update(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetLimitedWidth() const;
    
protected:
    void UpdateWidth(BL_FLOAT width);
    
    //
    BL_FLOAT CorrToComp(BL_FLOAT corr);
    BL_FLOAT CompToCorr(BL_FLOAT sample);
    
    //
    BL_FLOAT ApplyCompWidth(BL_FLOAT width, BL_FLOAT compGain,
                          BL_FLOAT corr);

    
    //
    bool mIsEnabled;
    
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mSmoothFactor;
    

    // Width from the user
    BL_FLOAT mUserWidth;
    
    // Limited result width
    BL_FLOAT mLimitedWidth;
    
    // Current detector for out of correlation
    CORRELATION_COMPUTER_CLASS *mCorrComputer;
    
    USTStereoWidener *mStereoWidener;
    
    // Compression algorithm
    InstantCompressor *mComp;
    
    // For debugging
    CORRELATION_COMPUTER_CLASS *mCorrComputerDbg;
};

#endif /* defined(__UST__USTWidthAdjuster9__) */
