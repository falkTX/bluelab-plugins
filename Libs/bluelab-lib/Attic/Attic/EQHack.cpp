#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <FftProcessObj16.h>

#include <EQHackFftObj.h>
#include <FftProcessBufObj.h>

#include <GUIHelper12.h>
#include <SecureRestarter.h>

#include <AirProcess2.h>

#include <BLUtils.h>
#include <BLDebug.h>
#include <BlaTimer.h>

#include <GraphControl12.h>
#include <GraphFreqAxis2.h>
#include <GraphAxis2.h>
#include <GraphAmpAxis.h>

#include <GraphCurve5.h>
#include <SmoothCurveDB.h>

#include <Scale.h>

#include <CMA2Smoother.h>

#include "IControl.h"
#include "config.h"
#include "bl_config.h"

#include "EQHack.h"

#include "IPlug_include_in_plug_src.h"
#include "IGraphics_include_in_plug_src.h"

//
#define BUFFER_SIZE 2048
#define OVERSAMPLING 4 // With 32, result is better (less low freq vibrations in noise)
#define FREQ_RES 1
#define KEEP_SYNTHESIS_ENERGY 0
#define VARIABLE_HANNING 1

//#define GRAPH_MIN_DB -119.0 // Take care of the noise/harmo bottom dB
//#define GRAPH_MAX_DB 10.0

// TEST
#define GRAPH_MIN_DB -40.0 // Take care of the noise/harmo bottom dB
#define GRAPH_MAX_DB 40.0

// NOTE: maybe decrease a bit this value, to 512 or 256
#define GRAPH_CURVE_NUM_VALUES 1024 //512
//#define CURVE_SMOOTH_COEFF 0.95
#define CURVE_SMOOTH_COEFF_SLOW 0.975
#define CURVE_SMOOTH_COEFF_FAST 0.9

#define STRENGTH_COEFF 4.0

// Logic equalizer max dB is +/- 24dB
// For Protools, this is +/- 18dB
#define MIN_DB -40.0
#define MAX_DB 40.0

#define APPLY_MIN_DB -100.0
#define APPLY_MAX_DB 0.0

#define DEFAULT_VALUE_RATIO 1.0

// Origin learn and guess too a very long time to stabilize
#define SMOOTH_FASTER 1

// Display instant curve in guess mode
#define DISPLAY_INSTANT_GUESS 1

#define SMOOTHER_WINDOW_SIZE 10

// Optimization
#define OPTIM_SKIP_IFFT 1

// BUG: preset factory reset doesn't clear the curve 

// NOTE: on protools, need "allZero" flag, othersie the learn curve will return slightly to zero

#if 0
TODO: add a "MON" checkbox

NOTE: for debugging au, launch:
/usr/bin/auvaltool
with arguments:
-v aufx 'tqz9' 'BlLa'

TODO: draw the main horiz axis in another color or biggest depth compared to the other grid lines

TODO/OPTIM: make a full float version, to see if it optimizes

USER REQUEST: "real time EQHack"/Analog EQ plugin + curve
fredguest@mac.com (mail support)
"What I mean by "real time" is using EQHACK without audio files, but rather an incoming audio stream. The use case would be if you are mixing virtual instrument tracks that have not been printed to audio files yet, and you are tuning them analog EQ emulations while arranging/mixing. "
AND if possible: use a single track and side chain pre/post fx
(no need to use a second track) Niko: may be possible in some hosts.
SUMMARY: use a "monitor" mode, to process eqhack without audio region
"I’m on Logic Pro X. It’s pretty easy to sidechain tracks in Logic, and I believe you can side chain a track to itself, so I think it might work!"
=> send him a mail when ready?
Fred => See the Bertom Analyzer which makes exactly what we want: https://www.bertomaudio.com/eqca.html
Sandwitch a plugin to analyze between two Bertom EQ Curve Analyzer
Niko - 20200921: tests sidechain => conclusion: for the moment, can t get the pre-fx side chain from the same track
BUG: on Reaper + AU, input is null and side chain is filled (or reverse, depending on the place in the code)
BUG: Logic(my version): if no sidechain is connected, then Logic copy the side input data to the sidechain data
(there is currently a workaround in IPlugBase, using memcmp)

IDEA: try with kick tracks => does the curve dimishes between kicks ? => in this case freeze the curve when the sound level is very low
"yeah there are a lot of tools that can be used for similar purpose like isotope matchEQ, IK multimedia, TDR plugins, new fangled audio "

NOTE: other similar plugins: "MatchEQ", "eq matching"

Emrah: "TDR NOVA GE can match/learn the EQ dynamically and match"


TODO: recompile, to benefit from FIX_TOO_MANY_INPUT_CHANNELS

NOTE: StudioOne, Sierra, VST: side chain does not enter in the plugins
=> must use AU or VST3 for sidechain

NOTE: Cubase Pro 10, Sierra: Cubase supports side chain for third party plugins only in VST3 format

TODO: tweak well allZero threshold EPS (test with Confusius-Stereo-EQ1000)
(Problem on Orion, Windows only ?
 maybe the two clip were a bit shifted, or small sidechain delay...)
(checked with Reaper, Logic => no problem)
#endif


enum EParams
{
    kMode = 0,
    kGraph,
      
    kNumParams
};
 
const int kNumPresets = 1;

enum ELayout
{
    kWidth = PLUG_WIDTH,
    kHeight = PLUG_HEIGHT,

    kGraphX = 0,
    kGraphY = 0,

    kRadioButtonsModeX = 595,
    kRadioButtonsModeY = 95,
    kRadioButtonsModeVSize = 72,
    //kRadioButtonModeNumButtons = 4,
    kRadioButtonsModeNumButtons = 3,
   
    kLogoAnimFrames = 31,
};


//
EQHack::EQHack(const InstanceInfo &info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    TRACE;
    
    InitNull();
    InitParams();

    Init(OVERSAMPLING, FREQ_RES);
  
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
    mMakeGraphicsFunc = [&]() { return this->MyMakeGraphics(); };
    
    mLayoutFunc = [&](IGraphics* pGraphics) { this->MyMakeLayout(pGraphics); };
#endif
    
    BL_PROFILE_RESET;
}

EQHack::~EQHack()
{
    if (mGUIHelper != NULL)
        delete mGUIHelper;
    
    if (mSignalFftObj != NULL)
        delete mSignalFftObj;

    if (mSignalObj != NULL)
        delete mSignalObj;

    if (mEQSourceFftObj != NULL)
        delete mEQSourceFftObj;

    if (mEQSourceObj != NULL)
        delete mEQSourceObj;

    if (mApplySignalFftObj != NULL)
        delete mApplySignalFftObj;
    
    for (int i = 0; i < 2; i++)
    {     
        if (mApplySignalObjs[i] != NULL)
            delete mApplySignalObjs[i];
    }
    
    //
    if (mFreqAxis != NULL)
        delete mFreqAxis;
    
    if (mHAxis != NULL)
        delete mHAxis;

    if (mAmpAxis != NULL)
        delete mAmpAxis;

    if (mVAxis != NULL)
        delete mVAxis;
    
    //
    if (mEQLearnCurve != NULL)
        delete mEQLearnCurve;
    
    if (mEQLearnCurveSmooth != NULL)
        delete mEQLearnCurveSmooth;
    
    if (mEQInstantCurve != NULL)
        delete mEQInstantCurve;
    
    if (mEQInstantCurveSmooth != NULL)
        delete mEQInstantCurveSmooth;
    
    if (mEQGuessCurve != NULL)
        delete mEQGuessCurve;
    
    if (mEQGuessCurveSmooth != NULL)
        delete mEQGuessCurveSmooth;

    for (int i = 0; i < 2; i++)
    {
        if (mSmoothers[i] != NULL)
            delete mSmoothers[i];
    }
}

IGraphics *
EQHack::MyMakeGraphics()
{
    IGraphics *graphics = MakeGraphics(*this,
                                       PLUG_WIDTH, PLUG_HEIGHT,
                                       PLUG_FPS,
                                       GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
    
    return graphics;
}

void
EQHack::MyMakeLayout(IGraphics *pGraphics)
{
    ENTER_PARAMS_MUTEX;
    
    pGraphics->LoadFont("font-regular", FONT_REGULAR_FN);
    pGraphics->LoadFont("font-light", FONT_LIGHT_FN);
    pGraphics->LoadFont("font-bold", FONT_BOLD_FN);

    // Style: BLULAB_V3
    pGraphics->LoadFont("OpenSans-ExtraBold", FONT_OPENSANS_EXTRA_BOLD_FN);
    pGraphics->LoadFont("Roboto-Bold", FONT_ROBOTO_BOLD_FN);
    
    pGraphics->AttachBackground(BACKGROUND_FN);

#ifdef __linux__
    pGraphics->AttachTextEntryControl();
#endif
    
#if 0 // Debug
    pGraphics->ShowControlBounds(true);
#endif
    
    // For rollover buttons
    pGraphics->EnableMouseOver(true);
    
    CreateControls(pGraphics);

    ApplyParams();
    
    // Demo mode
    mDemoManager.Init(this, pGraphics);
    
    mUIOpened = true;
    
    LEAVE_PARAMS_MUTEX;
}

void
EQHack::InitNull()
{
    BLUtils::PlugInits();
    
    mUIOpened = false;
    mControlsCreated = false;
    mIsInitialized = false;
    
    // Init WDL FFT
    FftProcessObj16::Init();

    mPrevSampleRate = GetSampleRate();
    
    mSignalObj = NULL;
    mEQSourceObj = NULL;
  
    mApplySignalObjs[0] = NULL;
    mApplySignalObjs[1] = NULL;
    
    mSignalFftObj = NULL;
    mEQSourceFftObj = NULL;
    mApplySignalFftObj = NULL;
    
    mGUIHelper = NULL;
    
    //
    mGraph = NULL;
    
    mFreqAxis = NULL;
    mHAxis = NULL;

    mAmpAxis = NULL;
    mVAxis = NULL;
    
    //
    mEQLearnCurve = NULL;
    mEQLearnCurveSmooth = NULL;
    
    mEQInstantCurve = NULL;
    mEQInstantCurveSmooth = NULL;
    
    mEQGuessCurve = NULL;
    mEQGuessCurveSmooth = NULL;

    for (int i = 0; i < 2; i++)
        mSmoothers[i] = NULL;
}

void
EQHack::InitParams()
{
    //arguments are: name, defaultVal, minVal, maxVal, step, label
    EQHackPluginInterface::Mode defaultMode = EQHackPluginInterface::LEARN;
    mMode = defaultMode;
  
    GetParam(kMode)->InitInt("Mode", (int)defaultMode, 0, 2);
}

void
EQHack::ApplyParams()
{
    ModeChanged();
    
    if (mEQLearnCurveSmooth != NULL)
        mEQLearnCurveSmooth->SetValues(mLearnCurve);
}

void
EQHack::Init(int oversampling, int freqRes)
{
    if (mIsInitialized)
        return;
    
    BL_FLOAT sampleRate = GetSampleRate();
    
    if (mEQSourceFftObj == NULL)
    {
        int numChannels = 1;
        int numScInputs = 0;
        
        vector<ProcessObj *> processObjs;
        mEQSourceObj = new FftProcessBufObj(BUFFER_SIZE, OVERSAMPLING,
                                            FREQ_RES, sampleRate);
        processObjs.push_back(mEQSourceObj);
        
        
        mEQSourceFftObj = new FftProcessObj16(processObjs,
                                              numChannels, numScInputs,
                                              BUFFER_SIZE, oversampling, freqRes,
                                              sampleRate);

#if OPTIM_SKIP_IFFT
        mEQSourceFftObj->SetSkipIFft(-1, true);
#endif
        
#if !VARIABLE_HANNING
        mEQSourceFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                           FftProcessObj16::WindowHanning);
        
        mEQSourceFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                            FftProcessObj16::WindowHanning);
#else
        mEQSourceFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                           FftProcessObj16::WindowVariableHanning);
        mEQSourceFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                            FftProcessObj16::WindowVariableHanning);
#endif
        
        mEQSourceFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS,
                                                KEEP_SYNTHESIS_ENERGY);
    }
    else
    {
        mEQSourceFftObj->Reset(BUFFER_SIZE, oversampling, freqRes, sampleRate);
    }

    // Signal Obj
    if (mSignalFftObj == NULL)
    {
        int numChannels = 1;
        int numScInputs = 0;
        
        vector<ProcessObj *> processObjs;
        mSignalObj = new EQHackFftObj(BUFFER_SIZE,
                                      oversampling, freqRes,
                                      sampleRate, mEQSourceObj);
        processObjs.push_back(mSignalObj);
        
        
        mSignalFftObj = new FftProcessObj16(processObjs,
                                            numChannels, numScInputs,
                                            BUFFER_SIZE, oversampling, freqRes,
                                            sampleRate);

#if OPTIM_SKIP_IFFT
        mSignalFftObj->SetSkipIFft(-1, true);
#endif
        
#if !VARIABLE_HANNING
        mSignalFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                         FftProcessObj16::WindowHanning);
        
        mSignalFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                          FftProcessObj16::WindowHanning);
#else
        mSignalFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                         FftProcessObj16::WindowVariableHanning);
        mSignalFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                          FftProcessObj16::WindowVariableHanning);
#endif
        
        mSignalFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS,
                                              KEEP_SYNTHESIS_ENERGY);
    }
    else
    {
        mSignalFftObj->Reset(BUFFER_SIZE, oversampling, freqRes, sampleRate);
    }

#if 0 // For the moment, ingore apply totally
    // Apply Signal Objs
    if (mApplySignalFftObj == NULL)
    {
        vector<ProcessObj *> processObjs;
	
        for (int i = 0; i < 2; i++)
        {
            mApplySignalObjs[i] = new EQHackFftObj(BUFFER_SIZE, OVERSAMPLING, FREQ_RES,
                                                   sampleRate, NULL);
	    
            processObjs.push_back(mApplySignalObjs[i]);
        }
    
        int numChannels = 2;
        int numScInputs = 0;
    
        mApplySignalFftObj = new FftProcessObj16(processObjs,
                                                 numChannels, numScInputs,
                                                 BUFFER_SIZE, oversampling, freqRes,
                                                 sampleRate);

#if OPTIM_SKIP_IFFT
        mApplySignalFftObj->SetSkipIFft(-1, true);
#endif
	
#if !VARIABLE_HANNING
        mApplySignalFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                              FftProcessObj16::WindowHanning);
        mApplySignalFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                               FftProcessObj16::WindowHanning);
#else
        mApplySignalFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                              FftProcessObj16::WindowVariableHanning);
        mApplySignalFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                               FftProcessObj16::WindowVariableHanning);
#endif
	
        mApplySignalFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS,
                                                   KEEP_SYNTHESIS_ENERGY);
    }
    else
    {
        FftProcessObj16 *fftObj = mApplySignalFftObj;
        fftObj->Reset(BUFFER_SIZE, oversampling, freqRes, mSampleRate);
    }
#endif

    for (int i = 0; i < 2; i++)
        mSmoothers[i] = new CMA2Smoother(BUFFER_SIZE, SMOOTHER_WINDOW_SIZE);
    
    ApplyParams();
    
    mIsInitialized = true;
}

void
EQHack::ProcessBlock(sample **inputs, sample **outputs, int nFrames)
{
    // Mutex is already locked for us.

    // Be sure to have sound even when the UI is closed
    BLUtils::BypassPlug(inputs, outputs, nFrames);
    
    if (!mIsInitialized)
        return;

    if (mGraph != NULL)
        mGraph->Lock();
    
    BL_PROFILE_BEGIN;
    
    FIX_FLT_DENORMAL_INIT()
    
    vector<WDL_TypedBuf<BL_FLOAT> > &in = mTmpBuf0;
    vector<WDL_TypedBuf<BL_FLOAT> > &scIn = mTmpBuf1;
    vector<WDL_TypedBuf<BL_FLOAT> > &out = mTmpBuf2;
    BLUtils::GetPlugIOBuffers(this, inputs, outputs, nFrames,
                              &in, &scIn, &out);

    // Cubase 10, Sierra
    // in or out can be empty...
    if (in.empty() || out.empty())
    {
        if (mGraph != NULL)
            mGraph->Unlock();
        
        return;
    }
  
    // Warning: there is a bug in Logic EQ plugin:
    // - when not playing, ProcessDoubleReplacing is still called continuously
    // - and the values are not zero ! (1e-5 for example)
    // This is the same for Protools, and if the plugin consumes, this slows all without stop
    // For example when selecting "offline"
    // Can be the case if we switch to the offline quality option:
    // All slows down, and Protools or Logix doesn't prompt for insufficient resources
    mSecureRestarter.Process(in);
    
    // Set the outputs to 0
    //
    // FIX: on Protools, in render mode, after play is finished,
    // there is a buzz sound
    for (int i = 0; i < out.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &out0 = out[i];
        BLUtils::FillAllZero(&out0);
    }

    if (IsTransportPlaying())
    {
        if (mSignalObj == NULL)
            return;
  
        if (mEQSourceObj == NULL)
            return;
  
        WDL_TypedBuf<BL_FLOAT> monoIn;
        if (in.size() > 0)
        {
            monoIn = in[0];
      
            if (in.size() > 1)
                BLUtils::StereoToMono(&monoIn, in[0], in[1]);
        }
    
        WDL_TypedBuf<BL_FLOAT> monoScIn;
        if (scIn.size() > 0)
            // At least one side chain !
        {
            monoScIn = scIn[0];
            if (scIn.size() > 1)
            {
                BLUtils::StereoToMono(&monoScIn, scIn[0], scIn[1]);
            }
        }
  
        // Analyse or apply
        if (monoScIn.GetSize() > 0)
        {
            vector< WDL_TypedBuf<BL_FLOAT> > monoScInVec;
            monoScInVec.push_back(monoScIn);
      
            vector< WDL_TypedBuf<BL_FLOAT> > dummy;
            mEQSourceFftObj->Process(monoScInVec, dummy, NULL);
        }
    
        vector< WDL_TypedBuf<BL_FLOAT> > monoInVec;
        monoInVec.push_back(monoIn);
    
        vector< WDL_TypedBuf<BL_FLOAT> > dummy;
        mSignalFftObj->Process(monoInVec, dummy, NULL);
  
        DL_TypedBuf<BL_FLOAT> eqBuf;
        mEQSourceObj->GetBuffer(&eqBuf);
  
        WDL_TypedBuf<BL_FLOAT> signalBuf;
        mSignalObj->GetBuffer(&signalBuf);
    
        if ((mMode != EQHackPluginInterface::APPLY) && (mMode != EQHackPluginInterface::APPLY_INV))
        {
            // Output the same thing as input
            BLUtils::BypassPlug(inputs, outputs, nFrames);
        }
        else
            // mMode == APPLY
        {
            // Apply the lear curve to signal, possibly stereo
            mApplySignalFftObj->Process(in, out, NULL);
      
            WDL_TypedBuf<BL_FLOAT> buf0;
            mApplySignalObjs[0]->GetBuffer(&buf0);
      
            WDL_TypedBuf<BL_FLOAT> buf1 = buf0;
      
            if ((in.size() > 1) && (out.size() > 1))
            {
                mApplySignalObjs[1]->GetBuffer(&buf1);
            }
      
            BLUtils::StereoToMono(&signalBuf, buf0.Get(), buf1.Get(), buf0.GetSize());
        }
    
        // Take the first half of the FFT, since it is symetric
        //TakeHalf(&signalBuf);
    
        // Do not update the learn curve is side chain is not connected.
        // This will avoid reseting the learn curve when we use the plugin
        // only for applying a recorded learn curve
        bool updateLearnCurve = (scIn.size() > 0);
    
        UpdateCurves(&signalBuf, updateLearnCurve);
    }

    // FIX: (in Orion for example)
    // Avoid playing the last part of samples indefinitely
    // when stopping the transport (with new AllZero)
    // (in fact, prev "allZero + bypass" was not safe)
    BLUtils::BypassPlug(inputs, outputs, nFrames);
    
    // Demo mode
    if (mDemoManager.MustProcess())
    {
        mDemoManager.Process(outputs, nFrames);
    }

    if (mGraph != NULL)
        mGraph->Unlock();
  
    BL_PROFILE_END;
}

void
EQHack::CreateControls(IGraphics *pGraphics)
{
    if (mGUIHelper == NULL)
        mGUIHelper = new GUIHelper12(GUIHelper12::STYLE_BLUELAB);
    
    mGraph = mGUIHelper->CreateGraph(this, pGraphics,
                                     kGraphX, kGraphY,
                                     GRAPH_FN, kGraph);
    int sepColor[4] = { 24, 24, 24, 255 };
    mGraph->SetSeparatorY0(2.0, sepColor);
    
    CreateGraphAxes();
    CreateGraphCurves();
    
    int graphSize[2];
    mGraph->GetSize(&graphSize[0], &graphSize[1]);
    int offsetY = graphSize[1];

    const char *modeRadioLabels[] = { "LEARN", "GUESS", "RESET"};
    mGUIHelper->CreateRadioButtons(pGraphics,
                                   kRadioButtonsModeX,
                                   kRadioButtonsModeY,
                                   RADIOBUTTON_FN,
                                   kRadioButtonsModeNumButtons,
                                   kRadioButtonsModeVSize,
                                   kMode,
                                   false,
                                   "MODE",
                                   EAlign::Near,
                                   EAlign::Near,
                                   modeRadioLabels);
    
    // Version
    mGUIHelper->CreateVersion(this, pGraphics, PLUG_VERSION_STR); //, GUIHelper12::BOTTOM);
    
    // Logo
    mGUIHelper->CreateLogoAnim(this, pGraphics, LOGO_FN,
                               kLogoAnimFrames, GUIHelper12::BOTTOM);
    
    // Plugin name
    mGUIHelper->CreatePlugName(this, pGraphics, PLUGNAME_FN, GUIHelper12::BOTTOM);
    
    // Help button
    mGUIHelper->CreateHelpButton(this, pGraphics,
                                 HELP_BUTTON_FN, MANUAL_FN,
                                 GUIHelper12::BOTTOM);
    
    mGUIHelper->CreateDemoMessage(pGraphics);
    
    //
    mControlsCreated = true;
}

void
EQHack::OnReset()
{
    TRACE;
    ENTER_PARAMS_MUTEX;
      
    // Logic X has a bug: when it restarts after stop,
    // sometimes it provides full volume directly, making a sound crack
    mSecureRestarter.Reset();
    
    //if (mUIOpened)
    //{
    BL_FLOAT sampleRate = GetSampleRate();
    if (sampleRate != mPrevSampleRate)
    {
        mPrevSampleRate = sampleRate;

        //ResetInstantCurve();

        mEQLearnCurveSmooth->ClearValues();
        mEQInstantCurveSmooth->ClearValues();
        mEQGuessCurveSmooth->ClearValues();

        // Force reset mode before UpdateGraph()
        EQHackPluginInterface::Mode prevMode = mMode;
        mMode = EQHackPluginInterface::RESET;
        UpdateCurves(NULL, false);
        mMode = prevMode;
    }

    if (mFreqAxis != NULL)
        mFreqAxis->Reset(BUFFER_SIZE, sampleRate);
    
    if (mEQLearnCurve != NULL)
        mEQLearnCurve->SetXScale(Scale::LOG, 0.0, sampleRate*0.5); //
    if (mEQInstantCurve != NULL)
        mEQInstantCurve->SetXScale(Scale::LOG, 0.0, sampleRate*0.5);
    if (mEQGuessCurve != NULL)
        mEQGuessCurve->SetXScale(Scale::LOG, 0.0, sampleRate*0.5);
    
    LEAVE_PARAMS_MUTEX;
    
    BL_PROFILE_RESET;
}

void
EQHack::OnParamChange(int paramIdx)
{
    if (!mIsInitialized)
        return;
  
    ENTER_PARAMS_MUTEX;
    
    switch (paramIdx)
    {
        case kMode:
        {
            EQHackPluginInterface::Mode mode =
            (EQHackPluginInterface::Mode)GetParam(paramIdx)->Int();
            if (mode != mMode)
            {
                mMode = mode;
	    
                ModeChanged();
            }
        }
      
        default:
            break;
    }
    
    LEAVE_PARAMS_MUTEX;
}

void
EQHack::OnUIOpen()
{
    IEditorDelegate::OnUIOpen();
    
    // Make sure we are not currently processing block
    // (process block send stuff to UI)
    ENTER_PARAMS_MUTEX;
    
    LEAVE_PARAMS_MUTEX;
}

void
EQHack::OnUIClose()
{
    // Make sure we are not currently processing block
    // (process block send stuff to UI)
    ENTER_PARAMS_MUTEX;

    mGraph = NULL;
    
    // Make sure we won't go to ProcessBlock() again
    mUIOpened = false;
    
    LEAVE_PARAMS_MUTEX;
}

void
EQHack::CreateGraphAxes()
{
    // Create
    if (mHAxis == NULL)
    {
        mHAxis = new GraphAxis2();
        mFreqAxis = new GraphFreqAxis2(true, Scale::LOG);
    }

    if (mVAxis == NULL)
    {
        mVAxis = new GraphAxis2();
        mAmpAxis = new GraphAmpAxis(true, GraphAmpAxis::DENSITY_10DB);
    }
    
    // Update
    mGraph->SetHAxis(mHAxis);
    mGraph->SetVAxis(mVAxis);
    
    BL_FLOAT sampleRate = GetSampleRate();
    int graphWidth = mGraph->GetRECT().W();
    
    bool horizontal = true;
    mFreqAxis->Init(mHAxis, mGUIHelper, horizontal, BUFFER_SIZE, sampleRate, graphWidth);
    mFreqAxis->Reset(BUFFER_SIZE, sampleRate);

    // Added dB offsets to avoid displaying the extremity axis labels
    mAmpAxis->Init(mVAxis, mGUIHelper,
                   GRAPH_MIN_DB + 0.1,
                   GRAPH_MAX_DB - 0.1, graphWidth);
}

void
EQHack::CreateGraphCurves()
{
    // TODO: styles !
    
    if (mEQLearnCurve == NULL)
        // Not yet created
    {
        BL_FLOAT sampleRate = GetSampleRate();
        
        int descrColor[4];
        mGUIHelper->GetGraphCurveDescriptionColor(descrColor);
    
        //float fillAlpha = mGUIHelper->GetGraphCurveFillAlpha();
        
        // EQHack curve
        int eqLearnColor[4];
        mGUIHelper->GetGraphCurveColorBlue(eqLearnColor);
        
        mEQLearnCurve = new GraphCurve5(GRAPH_CURVE_NUM_VALUES);
        mEQLearnCurve->SetDescription("EQ (learn)", descrColor);
        mEQLearnCurve->SetXScale(Scale::LOG, 0.0, sampleRate*0.5); //
        mEQLearnCurve->SetYScale(Scale::DB, GRAPH_MIN_DB, GRAPH_MAX_DB);
        //mEQLearnCurve->SetFill(true);
        //mEQLearnCurve->SetFillAlpha(fillAlpha);
        mEQLearnCurve->SetColor(eqLearnColor[0], eqLearnColor[1], eqLearnColor[2]);
        mEQLearnCurve->SetLineWidth(2.0);
        
        mEQLearnCurveSmooth = new SmoothCurveDB(mEQLearnCurve,
                                                CURVE_SMOOTH_COEFF_SLOW,
                                                GRAPH_CURVE_NUM_VALUES,
                                                1.0, // 1 in amp is 0 in dB (middle)
                                                GRAPH_MIN_DB, GRAPH_MAX_DB,
                                                sampleRate);
    
        // Harmo curve
        int instantColor[4];
        mGUIHelper->GetGraphCurveColorLightBlue(instantColor);
        
        mEQInstantCurve = new GraphCurve5(GRAPH_CURVE_NUM_VALUES);
        mEQInstantCurve->SetDescription("EQ (instant)", descrColor);
        mEQInstantCurve->SetXScale(Scale::LOG, 0.0, sampleRate*0.5);
        mEQInstantCurve->SetYScale(Scale::DB, GRAPH_MIN_DB, GRAPH_MAX_DB);
        //mEQInstantCurve->SetFill(true);
        //mEQInstantCurve->SetFillAlpha(fillAlpha);
        mEQInstantCurve->SetColor(instantColor[0], instantColor[1], instantColor[2]);
        
        mEQInstantCurveSmooth = new SmoothCurveDB(mEQInstantCurve,
                                                  CURVE_SMOOTH_COEFF_FAST,
                                                  GRAPH_CURVE_NUM_VALUES,
                                                  1.0, // 1 in amp is 0 in dB (middle)
                                                  GRAPH_MIN_DB, GRAPH_MAX_DB,
                                                  sampleRate);
	
        int guessColor[4];
        mGUIHelper->GetGraphCurveColorGreen(guessColor);
        
        mEQGuessCurve = new GraphCurve5(GRAPH_CURVE_NUM_VALUES);
        mEQGuessCurve->SetDescription("EQ (guess)", descrColor);
        mEQGuessCurve->SetXScale(Scale::LOG, 0.0, sampleRate*0.5); //
        mEQGuessCurve->SetYScale(Scale::DB, GRAPH_MIN_DB, GRAPH_MAX_DB);
        //mSumCurve->SetFill(true);
        //mEQGuessCurve->SetFillAlpha(fillAlpha);
        mEQGuessCurve->SetColor(guessColor[0], guessColor[1], guessColor[2]);
        
        mEQGuessCurveSmooth = new SmoothCurveDB(mEQGuessCurve,
                                                CURVE_SMOOTH_COEFF_SLOW,
                                                GRAPH_CURVE_NUM_VALUES,
                                                1.0, // 1 in amp is 0 in dB (middle)
                                                GRAPH_MIN_DB, GRAPH_MAX_DB,
                                                sampleRate);
    }

    int graphSize[2];
    mGraph->GetSize(&graphSize[0], &graphSize[1]);
    
    mEQInstantCurve->SetViewSize(graphSize[0], graphSize[1]);
    mGraph->AddCurve(mEQInstantCurve);
    
    mEQLearnCurve->SetViewSize(graphSize[0], graphSize[1]);
    mGraph->AddCurve(mEQLearnCurve);
    
    mEQGuessCurve->SetViewSize(graphSize[0], graphSize[1]);
    mGraph->AddCurve(mEQGuessCurve);
}

void
EQHack::UpdateCurves(const WDL_TypedBuf<BL_GUI_FLOAT> *currentEQ,
                     bool updateLearnCurve)
{
    if (!mUIOpened)
        return;
  
    WDL_TypedBuf<BL_GUI_FLOAT> learnCurve;
  
    if (mEQLearnCurveSmooth != NULL)
        mEQLearnCurveSmooth->GetHistogramValuesDB(&learnCurve);

    WDL_TypedBuf<BL_GUI_FLOAT> avgInstantEQ;

    if (currentEQ != NULL)
    {
        if (mEQInstantCurveSmooth != NULL)
        {
            mEQInstantCurveSmooth->SetValues(*currentEQ);
            mEQInstantCurveSmooth->GetHistogramValuesDB(&avgInstantEQ);
        }
        
        if (mMode == EQHackPluginInterface::LEARN)
        {
            if (mEQLearnCurveSmooth != NULL)
            {
                mEQLearnCurveSmooth->SetValues(*currentEQ);
                mEQLearnCurveSmooth->GetHistogramValuesDB(&learnCurve);
                
                // Smooth the data spatially
                CMASmooth(0, &learnCurve);
                
                if (updateLearnCurve)
                    // Keep track of the curve
                    mLearnCurve = learnCurve;
            }
        }
        else if (mMode == EQHackPluginInterface::APPLY)
        {
            // Nothing to do ?
        }
        else if (mMode == EQHackPluginInterface::GUESS)
        {
            if (mLearnCurve.GetSize() != 0)
                // Test to avoid a crash
            {
                if (mEQGuessCurveSmooth != NULL)
                {
                    mEQGuessCurveSmooth->SetValues(*currentEQ);
                    mEQGuessCurveSmooth->GetHistogramValuesDB(&learnCurve);
                    //mEQGuessCurveSmooth->GetHistogramValuesDB(&learnCurve);
    
                    // Smooth the data spatially
                    CMASmooth(1, &learnCurve);
    
                    // Compute the guess curve color, depending on the confidence of the match
                    BL_FLOAT matchCoeff = BLUtils::ComputeCurveMatchCoeff(learnCurve.Get(),
                                                                          mLearnCurve.Get(),
                                                                          learnCurve.GetSize());
                    
                    unsigned int matchColor0[3] = { 255, 64, 64 };
                    unsigned int matchColor1[3] = { 64, 255, 64 };
	
                    unsigned int resultColor[3];
                    for (int i = 0; i < 3; i++)
                        resultColor[i] = (1.0 - matchCoeff)*matchColor0[i] +
                        matchCoeff*matchColor1[i];
	
                    if (mEQGuessCurve != NULL)
                        mEQGuessCurve->SetColor(resultColor[0], resultColor[1], resultColor[2]);
                }
            }
        }
    }

    //
    if (mMode == EQHackPluginInterface::LEARN)
    {
        if (mEQInstantCurveSmooth != NULL)
            mEQInstantCurveSmooth->SetValues(avgInstantEQ);
      
        if (mEQLearnCurveSmooth != NULL)
            mEQLearnCurveSmooth->SetValues(mLearnCurve);
    }
    else if (mMode == EQHackPluginInterface::GUESS)
    {
        if (mEQGuessCurveSmooth != NULL)
            mEQGuessCurveSmooth->SetValues(learnCurve);
        if (mEQInstantCurveSmooth != NULL)
            mEQInstantCurveSmooth->SetValues(avgInstantEQ);
    }
    else if (mMode == EQHackPluginInterface::RESET)
    {
        if (mEQLearnCurveSmooth != NULL)
            mEQLearnCurveSmooth->SetValues(learnCurve);
        if (mEQGuessCurveSmooth != NULL)
            mEQGuessCurveSmooth->SetValues(learnCurve);
    }
}

void
EQHack::ModeChanged(bool fromUnserial)
{
    // Could have been serialized and restored...
    /*if (!fromUnserial)
      {
      if (mEQLearnCurveSmooth != NULL)
      mEQLearnCurveSmooth->ClearValues();
      }*/
  
    if (mEQInstantCurveSmooth != NULL)
        mEQInstantCurveSmooth->ClearValues();
    if (mEQGuessCurveSmooth != NULL)
        mEQGuessCurveSmooth->ClearValues();
  
    if (mSignalObj != NULL)
        mSignalObj->SetMode(mMode);
  
    for (int i = 0; i < 2; i++)
    {
        if (mApplySignalObjs[i] != NULL)
            mApplySignalObjs[i]->SetMode(mMode);
    }
  
    if (mMode == EQHackPluginInterface::GUESS)
    {
        SetCurveSpecsModeGuess();
        
        if (mSignalObj != NULL)
            mSignalObj->SetLearnCurve(&mLearnCurve);
    
        // Just in case
        for (int i = 0; i < 2; i++)
        {
            if (mApplySignalObjs[i] != NULL)
                mApplySignalObjs[i]->SetLearnCurve(&mLearnCurve);
            
        }
        
        if (mEQLearnCurveSmooth != NULL)
            mEQLearnCurveSmooth->SetValues(mLearnCurve, true);
    }
    else if (mMode == EQHackPluginInterface::LEARN)
    {
        SetCurveSpecsModeLearn();

        // TEST
        if (!fromUnserial)
            ResetLearnCurve();
        
        // Could have been serialized and restored...
        //if (!fromUnserial)
        //    mLearnCurve.Resize(0);

        // 1.0 = 0dB
        //BLUtils::FillAllValue(&mLearnCurve, (BL_FLOAT)1.0); // TEST
    }
    else if (mMode == EQHackPluginInterface::RESET)
    {
        if (mEQLearnCurveSmooth != NULL)
            mEQLearnCurveSmooth->ClearValues();
        if (mEQInstantCurveSmooth != NULL)
            mEQInstantCurveSmooth->ClearValues();
        if (mEQGuessCurveSmooth != NULL)
            mEQGuessCurveSmooth->ClearValues();

        ResetLearnCurve(); // TEST
    }
    // Keep this for later (maybe...)
#if 0
    else if (mMode == EQHackPluginInterface::APPLY_INV)
    {
        SetCurveSpecsModeApplyInv();
    
        mSignalObj0->SetLearnCurve(&mLearnCurve);
        mSignalObj1->SetLearnCurve(&mLearnCurve);
    
        // Just in case
        mSignalObj->SetLearnCurve(&mLearnCurve);
    }
    else if (mMode == EQHackPluginInterface::APPLY)
    {
        SetCurveSpecsModeApply();
    
        mSignalObj0->SetLearnCurve(&mLearnCurve);
        mSignalObj1->SetLearnCurve(&mLearnCurve);
    
        // Just in case
        mSignalObj->SetLearnCurve(&mLearnCurve);
    }
#endif
  
    UpdateCurves(NULL, false);
}

void
EQHack::SetCurveSpecsModeLearn()
{
    // Avg EQ Hack
    if (mEQGuessCurve != NULL)
        mEQGuessCurve->SetAlpha(0.0);

#if 0
    mEQGraph->SetCurveAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, 0.0);
    mEQGraph->SetCurveFillAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, 0.0);
#endif
  
    if (mEQLearnCurve != NULL)
    {
        mEQLearnCurve->SetAlpha(1.0);
        float fillAlpha = mGUIHelper->GetGraphCurveFillAlpha();
        mEQLearnCurve->SetFillAlpha(fillAlpha);
    }
}

void
EQHack::SetCurveSpecsModeApply()
{
    // Avg EQ Hack
    if (mEQGuessCurve != NULL)
        mEQGuessCurve->SetAlpha(0.0);

#if 0
    mEQGraph->SetCurveAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, 0.0);
    mEQGraph->SetCurveFillAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, 0.0);
#endif
  
    if (mEQLearnCurve != NULL)
    {
        mEQLearnCurve->SetAlpha(1.0);
        float fillAlpha = mGUIHelper->GetGraphCurveFillAlpha();
        mEQLearnCurve->SetFillAlpha(fillAlpha);
    }
}

void
EQHack::SetCurveSpecsModeApplyInv()
{
    // Avg EQ Hack
    if (mEQGuessCurve != NULL)
        mEQGuessCurve->SetAlpha(0.0);

#if 0
    mEQGraph->SetCurveAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, 1.0);
    mEQGraph->SetCurveFillAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, CURVE_FILL_ALPHA);
#endif
  
    if (mEQLearnCurve != NULL)
    {
        mEQLearnCurve->SetAlpha(0.0); // 0.5
        mEQLearnCurve->SetFillAlpha(0.0);
    }
}

void
EQHack::SetCurveSpecsModeGuess()
{
    if (mEQGuessCurve != NULL)
        mEQGuessCurve->SetAlpha(1.0);

#if 0
    mEQGraph->SetCurveAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, 0.0);
    mEQGraph->SetCurveFillAlpha(AVG_EQ_MODE_LEARN_INV_CURVE, 0.0);
#endif
  
    if (mEQLearnCurve != NULL)
    {
        mEQLearnCurve->SetAlpha(1.0);
        float fillAlpha = mGUIHelper->GetGraphCurveFillAlpha();
        mEQLearnCurve->SetFillAlpha(fillAlpha);
    }
}

void
EQHack::CMASmooth(int smootherIndex, WDL_TypedBuf<BL_FLOAT> *buf)
{  
    WDL_TypedBuf<BL_FLOAT> bufSmooth = *buf;
    //CMA2Smoother::ProcessOne(buf->Get(), bufSmooth.Get(), buf->GetSize(), SMOOTHER_WINDOW_SIZE);
    if (mSmoothers[smootherIndex] != NULL)
        mSmoothers[smootherIndex]->ProcessOne(buf->Get(),
                                              bufSmooth.Get(),
                                              buf->GetSize(),
                                              SMOOTHER_WINDOW_SIZE);
    *buf = bufSmooth;
}

#if 1
// Threshold, to avoid computing when we have silence + hiss
// Problem on Protools and Orion: when silence + hiss,
// the EQ (instant) curve jumps up (especially in stereo)
// (No problem on Reaper and Logic)
// Example: BlueLabAudio-EQHack-TestProtools-Test2.ptx
bool
EQHack::IsAllZero(const WDL_TypedBuf<BL_FLOAT> &buf)
{
    // Greater EPS than in Utils
    //#define EPS_DB -100 // Still jump a bit
#define EPS_DB -80 // Good: jump a very few but i's ok

    BL_FLOAT eps = DBToAmp(EPS_DB);
  
    for (int i = 0; i < buf.GetSize(); i++)
    {
        BL_FLOAT val = fabs(buf.Get()[i]);
        if (val > eps)
            return false;
    }
  
    return true;
}
#endif

bool
EQHack::SerializeState(IByteChunk &pChunk) const
{
    TRACE;
    ((EQHack *)this)->ENTER_PARAMS_MUTEX;
  
    // Previosu version was to save "avgValues"
    // It as wrong
  
    // Serialize the avg values curve
    int size = mLearnCurve.GetSize();
    pChunk.Put(&size);
  
    for (int i = 0; i < size; i++)
    {
        BL_FLOAT v = mLearnCurve.Get()[i];
        pChunk.Put(&v);
    }
  
    bool res = SerializeParams(pChunk);

    ((EQHack *)this)->LEAVE_PARAMS_MUTEX;
    
    return res;
}

int
EQHack::UnserializeState(const IByteChunk &pChunk, int startPos)
{
    TRACE;
    ENTER_PARAMS_MUTEX;
  
    // Unserialize avg values curve
    WDL_TypedBuf<BL_GUI_FLOAT> avgValues;
      
    int size;
    startPos = pChunk.Get(&size, startPos);
  
    avgValues.Resize(size);
  
    for (int i = 0; i < size; i++)
    {
        BL_FLOAT v;
        startPos = pChunk.Get(&v, startPos);
    
        avgValues.Get()[i] = v;
    }
  
    // Security: set values only if it is coherent
    // This avoid problems if one day we change the buffer size, and we have old
    // saved states of the plugin
    if (size == BUFFER_SIZE/2)
    {
        mLearnCurve = avgValues;
      
        if (mEQLearnCurveSmooth != NULL)
            mEQLearnCurveSmooth->SetValues(mLearnCurve);
    
        // Update the interface
        ModeChanged(true);
    }
  
    int res = UnserializeParams(pChunk, startPos);

    LEAVE_PARAMS_MUTEX;
    
    return res;
}

void
EQHack::ResetLearnCurve()
{
    // 1.0 = 0dB
    BLUtils::FillAllValue(&mLearnCurve, (BL_FLOAT)0.5); //1.0);
    if (mEQLearnCurveSmooth != NULL)
    {
        mEQLearnCurveSmooth->SetValues(mLearnCurve, true); //
        mEQLearnCurveSmooth->GetHistogramValuesDB(&mLearnCurve);
    }

    // Just in case
    for (int i = 0; i < 2; i++)
    {
        if (mApplySignalObjs[i] != NULL)
            mApplySignalObjs[i]->SetLearnCurve(&mLearnCurve);
        
    }
}
