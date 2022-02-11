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

#ifndef M_PI
#define M_PI 3.14159265358979323846264
#endif

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
class USTStereoWidener;
class CParamSmooth;
class CMAParamSmooth;
class CMAParamSmooth2;
class KalmanParamSmooth;

class InstantCompressor;

class USTWidthAdjuster9
{
public:
    USTWidthAdjuster9(double sampleRate);
    
    virtual ~USTWidthAdjuster9();
    
    void Reset(double sampleRate);
    
    bool IsEnabled();
    void SetEnabled(bool flag);
    
    void SetSmoothFactor(double smoothFactor);
    void SetWidth(double width);
    
    void Update(double l, double r);
    
    double GetLimitedWidth() const;
    
protected:
    void UpdateWidth(double width);
    
    //
    double CorrToComp(double corr);
    double CompToCorr(double sample);
    
    //
    double ApplyCompWidth(double width, double compGain,
                          double corr);

    
    //
    bool mIsEnabled;
    
    double mSampleRate;
    
    double mSmoothFactor;
    

    // Width from the user
    double mUserWidth;
    
    // Limited result width
    double mLimitedWidth;
    
    // Current detector for out of correlation
    USTCorrelationComputer2 *mCorrComputer;
    
    USTStereoWidener *mStereoWidener;
    
    // Compression algorithm
    InstantCompressor *mComp;
    
    // For debugging
    USTCorrelationComputer2 *mCorrComputerDbg;
};

#endif /* defined(__UST__USTWidthAdjuster9__) */
