//
//  USTVumeter.h
//  UST
//
//  Created by applematuer on 8/14/19.
//
//

#ifndef __UST__USTVumeter__
#define __UST__USTVumeter__

#include <vector>

#include "IPlug_include_in_plug_hdr.h"

class USTVumeterProcess;

class ParamSmoother;
class LUFSMeter;


#define USE_SMOOTHERS 1 //0
#define USE_PEAK_SMOOTHERS 0

// NOTE: this is ok if the vumeter grows slowly at begin (Protools does like that)
// NOTE: LUFSMeter must be applied to all channels at the same time
class USTVumeter
{
public:
    enum Mode
    {
        RMS,
        PEAK
    };
    
    USTVumeter(BL_FLOAT sampleRate);
    
    virtual ~USTVumeter();
    
    void Reset(BL_FLOAT sampleRate);
    
    //void SetBarControls(VumeterControl *leftBar, VumeterControl *rightBar);
    //void SetBarPeakControls(VumeterControl *leftPeakBar, VumeterControl *rightPeakBar);
    void SetBarControls(IControl *leftBar, IControl *rightBar);
    void SetBarPeakControls(IControl *leftPeakBar, IControl *rightPeakBar);
    
    void SetBarTextControls(ITextControl *leftText, ITextControl *rightText);
    void SetDBTextControl(ITextControl *valueControl);
    void SetLUFSTextControl(ITextControl *valueControl);
    
    //
    void SetMode(Mode mode);
    
    //
    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);
    
    void UpdateGUI();
    
protected:
    //void SetBarValues(VumeterControl *bars[2], BL_FLOAT gains[2]);
    void SetBarValues(IControl *bars[2], BL_FLOAT gains[2]);
    void SetTextValues(BL_FLOAT gains[2]);
    void SetTextValuesLUFS(BL_FLOAT lufs);
    
    void GainToText(BL_FLOAT gain, char *text, int vumeterNum);
    void CheckSaturation(BL_FLOAT peakValues[2]);
    
    void ComputeLUFSValue();

    bool NeedUpdateTexts();
    
    //
    //VumeterControl *mBars[2];
    //VumeterControl *mPeakBars[2];
    IControl *mBars[2];
    IControl *mPeakBars[2];
    
    ITextControl *mBarTexts[2];
    ITextControl *mDBText;
    ITextControl *mLUFSText;
    
    IColor mOriginTextColor;
    
    Mode mMode;
    
    //
    USTVumeterProcess *mProcess[2];
    
#if USE_SMOOTHERS
    ParamSmoother *mLRSmoothers[2];
#endif
    
#if ENABLE_PEAK_SMOOTHERS
    ParamSmoother *mLRPeakSmoothers[2];
#endif

    LUFSMeter *mLUFSMeter;
    BL_FLOAT mLUFSValue;
    
    // For optimization
    unsigned long mNumSamples;
    BL_FLOAT mSampleRate;
    
    long int mPrevMillis;
    long int mTextMillisFPS;
};

#endif /* defined(__UST__USTVumeter__) */
