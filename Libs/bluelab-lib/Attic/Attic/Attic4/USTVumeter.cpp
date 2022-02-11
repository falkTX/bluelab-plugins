//
//  USTVumeter.cpp
//  UST
//
//  Created by applematuer on 8/14/19.
//
//

#include <USTVumeterProcess.h>

//#include <VuMeterControl.h>
#include <GUIHelper11.h>

#include <ParamSmoother.h>
#include <BLUtils.h>

#include <LUFSMeter.h>

#include "USTVumeter.h"

// TODO: check well for needle overlap !!
#define TEST_VUMETER_NEEDLE_ALIGN 0 //1 //0

// Do not compute short term loudness too often
// (it consumes a lot of resource, computed by the lib for intervals of 3s)
#define OPTIM_LUFS_SHORT_TERM 1
#define OPTIM_LUFS_SHORT_TERM_INTERVAL 0.5 // seconds

// With this, UpdateGUI() will be called from the drawing thread
// (and less often than in the audio thread at each buffer processing)
#define OPTIM_UPDATE_GUI 0 //1


USTVumeter::USTVumeter(BL_FLOAT sampleRate)
{
    mBars[0] = NULL;
    mBars[1] = NULL;
    
    mBarTexts[0] = NULL;
    mBarTexts[1] = NULL;
    
    mDBText = NULL;
    mLUFSText = NULL;
    
    for (int i = 0; i < 2; i++)
        mProcess[i] = new USTVumeterProcess(sampleRate);
    
    mMode = RMS;
    
#if USE_SMOOTHERS // Use smoothers
    for (int i = 0; i < 2; i++)
        mLRSmoothers[i] = new ParamSmoother(0.0, 0.95);
#endif

#if USE_PEAK_SMOOTHERS
    for (int i = 0; i < 2; i++)
        mLRPeakSmoothers[i] = new ParamSmoother(0.0, 0.95);
#endif
    
    mLUFSMeter = new LUFSMeter(2, sampleRate);
    
    mSampleRate = sampleRate;
    
#if OPTIM_LUFS_SHORT_TERM
    mNumSamples = 0;
#endif
    
    mLUFSValue = VUMETER_MIN_GAIN;
}

USTVumeter::~USTVumeter()
{
    for (int i = 0; i < 2; i++)
        delete mProcess[i];
    
#if USE_SMOOTHERS
    for (int i = 0; i < 2; i++)
        delete mLRSmoothers[i];
#endif
    
#if USE_PEAK_SMOOTHERS
    for (int i = 0; i < 2; i++)
        delete mLRPeakSmoothers[i];
#endif
}

void
USTVumeter::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < 2; i++)
        mProcess[i]->Reset(sampleRate);
    
#if USE_SMOOTHERS
    for (int i = 0; i < 2; i++)
        mLRSmoothers[i]->Reset();
#endif
    
#if USE_PEAK_SMOOTHERS
    for (int i = 0; i < 2; i++)
        mLRPeakSmoothers[i]->Reset()
#endif
    
    if (mLUFSMeter != NULL)
        mLUFSMeter->Reset(sampleRate);
    
    mSampleRate = sampleRate;

#if OPTIM_LUFS_SHORT_TERM
    mNumSamples = 0;
#endif
}

void
//USTVumeter::SetBarControls(VumeterControl *leftBar, VumeterControl *rightBar)
USTVumeter::SetBarControls(IControl *leftBar, IControl *rightBar)
{
    mBars[0] = leftBar;
    mBars[1] = rightBar;
}

void
//USTVumeter::SetBarPeakControls(VumeterControl *leftPeakBar, VumeterControl *rightPeakBar)
USTVumeter::SetBarPeakControls(IControl *leftPeakBar, IControl *rightPeakBar)
{
    mPeakBars[0] = leftPeakBar;
    mPeakBars[1] = rightPeakBar;
}

void
USTVumeter::SetBarTextControls(ITextControl *leftText, ITextControl *rightText)
{
    mBarTexts[0] = leftText;
    mBarTexts[1] = rightText;
    
    mOriginTextColor = mBarTexts[0]->GetTextColor();
}

void
USTVumeter::SetDBTextControl(ITextControl *dbControl)
{
    mDBText = dbControl;
}

void
USTVumeter::SetLUFSTextControl(ITextControl *lufsControl)
{
    mLUFSText = lufsControl;
}

void
USTVumeter::SetMode(Mode mode)
{
    mMode = mode;
}

void
USTVumeter::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    for (int i = 0; i < 2; i++)
        mProcess[i]->AddSamples(samples[i]);
    
    if (mLUFSMeter != NULL)
        mLUFSMeter->AddSamples(samples);
    
#if OPTIM_LUFS_SHORT_TERM
    mNumSamples += samples[0].GetSize();
#endif
    
    ComputeLUFSValue();
    
#if !OPTIM_UPDATE_GUI
    //UpdateGUI();
#endif
}

void
USTVumeter::UpdateGUI()
{
    BL_FLOAT values[2];
    values[0] = mProcess[0]->GetValue();
    values[1] = mProcess[1]->GetValue();
        
    BL_FLOAT peakValues[2];
    peakValues[0] = mProcess[0]->GetPeakValue();
    peakValues[1] = mProcess[1]->GetPeakValue();
    
#if USE_SMOOTHERS
    for (int i = 0; i < 2; i++)
    {
        mLRSmoothers[i]->SetNewValue(values[i]);
        mLRSmoothers[i]->Update();
        values[i] = mLRSmoothers[i]->GetCurrentValue();
    }
#endif
    
#if USE_PEAK_SMOOTHERS
    for (int i = 0; i < 2; i++)
    {
        mLRPeakSmoothers[i]->SetNewValue(peakValues[i]);
        mLRPeakSmoothers[i]->Update();
        peakValues[i] = mLRPeakSmoothers[i]->GetCurrentValue();
    }
#endif
    
    SetBarValues(mBars, values);
    
    // Do not display peak vumeter needle if value is less than non-peak vumter
    // (would be over the other bars)
    BL_FLOAT peakValuesFixed[2] = { peakValues[0], peakValues[1] };
    for (int i = 0; i < 2; i++)
    {
        if ((peakValuesFixed[i] <= values[i]) || TEST_VUMETER_NEEDLE_ALIGN)
        {
            peakValuesFixed[i] = values[i]; //VUMETER_MIN_GAIN;
        }
    }
    
    SetBarValues(mPeakBars, peakValuesFixed);
    
    if (mMode == RMS)
        SetTextValues(values);
    else
    {
        if (mMode == PEAK)
            SetTextValues(peakValues);
    }
    
    SetTextValuesLUFS(mLUFSValue);
    
    CheckSaturation(peakValues);
}

void
//USTVumeter::SetBarValues(VumeterControl *bars[2], BL_FLOAT gains[2])
USTVumeter::SetBarValues(IControl *bars[2], BL_FLOAT gains[2])
{
    for (int i = 0; i < 2; i++)
    {
        BL_FLOAT normGain = (gains[i] - VUMETER_MIN_GAIN)/(VUMETER_MAX_GAIN - VUMETER_MIN_GAIN);
        
#if !OPTIM_UPDATE_GUI
        //bars[i]->SetValueFromPlug(normGain);
        bars[i]->SetValue(normGain);
        bars[i]->SetDirty(false); // new
#else
        bars[i]->SetValueFromGUI(normGain);
#endif
    }
}

void
USTVumeter::SetTextValues(BL_FLOAT gains[2])
{
    // Quick and dirty compute mono gain
    BL_FLOAT monoGain;
    if (mMode == RMS)
    {
        // Take avg
        monoGain = (gains[0] + gains[1])*0.5;
    }
    else if (mMode == PEAK)
    {
        // take max
        monoGain = (gains[0] >= gains[1] ? gains[0] : gains[1]);
    }
    
    for (int i = 0; i < 2; i++)
    {
        char text[256];
        GainToText(gains[i], text, i);
        
#if !OPTIM_UPDATE_GUI
        //mBarTexts[i]->SetTextFromPlug(text);
        mBarTexts[i]->SetStr(text);
        mBarTexts[i]->SetDirty(false); // new
#else
        mBarTexts[i]->SetTextFromGUI(text);
#endif
    }
    
    char text[256];
    GainToText(monoGain, text, 1);
    
#if !OPTIM_UPDATE_GUI
    //mDBText->SetTextFromPlug(text);
    mDBText->SetStr(text);
    mDBText->SetDirty(false);
#else
    mDBText->SetTextFromGUI(text);
#endif
}

void
USTVumeter::SetTextValuesLUFS(BL_FLOAT lufs)
{
    char textLUFS[256];
    GainToText(lufs, textLUFS, 1);
    
#if !OPTIM_UPDATE_GUI
    //mLUFSText->SetTextFromPlug(textLUFS);
    mLUFSText->SetStr(textLUFS);
    mLUFSText->SetDirty(false);
#else
    mLUFSText->SetTextFromGUI(textLUFS);
#endif
}

void
USTVumeter::GainToText(BL_FLOAT gain, char *text, int vumeterNum)
{
#define EPS 0.1 // For RMS, that leads to 89.999
    if (gain <= VUMETER_MIN_GAIN + EPS)
    {
        if (vumeterNum == 0)
            sprintf(text, "  -inf");
        else
            sprintf(text, " -inf"); // Default
        
        return;
    }
    
    int numDigits = 1;
    if (std::fabs(gain) >= 10.0)
        numDigits++;
    if (gain < 0.0)
        numDigits++;
    
    // add space digits to stay centered
    if (numDigits >= 3)
    {
        sprintf(text, "%.1f", gain);
    }
    else if (numDigits >= 2)
    {
        sprintf(text, " %.1f", gain);
    } else
    {
        sprintf(text, "  %.1f", gain);
    }
}

void
USTVumeter::CheckSaturation(BL_FLOAT peakValues[2])
{
    for (int i = 0; i < 2; i++)
    {
        if (peakValues[i] >= 0.0)
        {
            IColor red(255, 255, 0, 0);
      
#if !OPTIM_UPDATE_GUI
            //mBarTexts[i]->SetTextColorFromPlug(red);
            mBarTexts[i]->SetTextColor(red);
            mBarTexts[i]->SetDirty(false);
#else
            mBarTexts[i]->SetTextColorFromGUI(red);
#endif
        }
        else
        {
#if !OPTIM_UPDATE_GUI
            //mBarTexts[i]->SetTextColorFromPlug(mOriginTextColor);
            mBarTexts[i]->SetTextColor(mOriginTextColor);
            mBarTexts[i]->SetDirty(false);
#else
            mBarTexts[i]->SetTextColorFromGUI(mOriginTextColor);
#endif
        }
    }
}

void
USTVumeter::ComputeLUFSValue()
{
    bool computeLoudnessShortTerm = true;
    
#if OPTIM_LUFS_SHORT_TERM
    computeLoudnessShortTerm = false;
    long numSamplesUpdate = OPTIM_LUFS_SHORT_TERM_INTERVAL*mSampleRate;
    if (mNumSamples >= numSamplesUpdate)
    {
        mNumSamples -= numSamplesUpdate;
        
        computeLoudnessShortTerm = true;
    }
#endif
    
    if (computeLoudnessShortTerm)
    {
        mLUFSValue = VUMETER_MIN_GAIN;
        
        if (mLUFSMeter != NULL)
            mLUFSMeter->GetLoudnessShortTerm(&mLUFSValue);
        
        SetTextValuesLUFS(mLUFSValue);
    }
}
