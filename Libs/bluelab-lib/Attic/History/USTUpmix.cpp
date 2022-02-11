//
//  USTUpmix.cpp
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

#include <GraphControl11.h>
#include <USTUpmixGraphDrawer.h>
#include <USTProcess.h>
#include <CrossoverSplitter2Bands.h>
#include <DelayObj4.h>
#include <CrossoverSplitter3Bands.h>
//#include <CrossoverSplitterNBands.h>
#include <CrossoverSplitterNBands3.h>
#include <StereoWidenProcess.h>

#include <BLUtils.h>

#include <TestFilter.h>

#define USE_FREEVERB 1
#if USE_FREEVERB
#include "revmodel.hpp"
#endif

#include "USTUpmix.h"


#define BASS_FOCUS_FREQ 100.0

// For SplitDiffuse v1
//#define SPLIT_S_DELAY 10.0 //5.0

// For SplitDiffuse v2
#define SPLIT_S_DELAY 20.0

// Not transparent
#define BOOST_DIFFUSE_MIN_GAIN 0.25
#define BOOST_DIFFUSE_MAX_GAIN 1.0

// Transparent
#define BOOST_DIFFUSE_MIN_GAIN_DB 0.0
#define BOOST_DIFFUSE_MAX_GAIN_DB 6.0 //3.0


// For early reflections reverb (in meters)
#define REV_ROOM_SIZE 10.0
#define REV_INTERMIC_DIST 0.1
#define REV_SOUND_SPEED 343.0
#define REV_DEPTH_COEFF 2.0

// Brillance
// See: https://en.wikipedia.org/wiki/Audio_frequency
#define BRILLANCE_MIN_FREQ 8192.0
#define BRILLANCE_MAX_FREQ 16384.0
#define BRILLANCE_MAX_DB 12.0

// GOOD: avoid duplicating IR when default parameters
#define COMPENSATE_DELAY1 1

// GOOD: avoid black bands in high freqs
#define COMPENSATE_DELAY2 1

// In samples
#define DIFFUSE_COMP_DELAY 4 //3

// Compensate the lost when SplitDiffuse (which uses mono2stereo)
#define COMPENSATE_DIFFUSE_GAIN 1

// Tests for transparency
//

// Almost transparent
#define TEST_CROSSOVER0 0
// Almost transparent
#define TEST_CROSSOVER1 0
// Almost transparent (spectrogram only)
#define TEST_CROSSOVER2 0

// Almost transparent (only a small gain loss around 20KHz)
#define TEST_CROSSOVER3 0

// Transparent
#define TEST_CROSSOVER4 0

#define PROCESS_STEREO 1

// Brillance should be applied to the depth sound, not to all sound !
#define BRILLANCE_FIX 1


USTUpmix::USTUpmix(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mGraph = NULL;
    mIsEnabled = false;
    
    mGain = 0.0;
    mPan = 0.0;
    mDepth = 0.0;
    mBrillance = 0.0;
    
#if !SPLITTER_N_BANDS
    mBassSplitter = new CrossoverSplitter2Bands(sampleRate);
    mBassSplitter->SetCutoffFreq(BASS_FOCUS_FREQ);
#else
    BL_FLOAT bassFreqs[1] = { BASS_FOCUS_FREQ };
    mBassSplitter = new CrossoverSplitterNBands3(2, bassFreqs, sampleRate);
    
#if PROCESS_STEREO
    for (int i = 0; i < 2; i++)
        mBassSplitters[i] = new CrossoverSplitterNBands3(2, bassFreqs, sampleRate);
#endif
    
#endif
    
    BL_FLOAT delay = (SPLIT_S_DELAY/1000.0)*sampleRate;
    mSplitSDelay = new DelayObj4(delay);
    
    mCompDelay = new DelayObj4(delay);
    
    for (int i = 0; i < 3; i++)
        mCompDelays2[i] = new DelayObj4(DIFFUSE_COMP_DELAY);
    
    for (int i = 0; i < 4; i++)
    {
        mEarlyReflectDelays[i] = new DelayObj4(0.0);
    }
    
    BL_FLOAT brillanceFreqs[2] = { BRILLANCE_MIN_FREQ, BRILLANCE_MAX_FREQ };
    
#if !SPLITTER_N_BANDS
    mBrillanceSplitter = new CrossoverSplitter3Bands(brillanceFreqs, sampleRate);
#else
    //mBrillanceSplitter = new CrossoverSplitterNBands(3, brillanceFreqs, sampleRate);
    mBrillanceSplitter = new CrossoverSplitterNBands3(3, brillanceFreqs, sampleRate);
    
#if PROCESS_STEREO
    for (int i = 0; i < 2; i++)
        mBrillanceSplitters[i] = new CrossoverSplitterNBands3(3, brillanceFreqs, sampleRate);
#endif
    
#endif
    
#if USE_FREEVERB
    mRevModel = new revmodel();

    mRevModel->init(sampleRate);
    InitRevModel();
#endif
}

USTUpmix::~USTUpmix()
{
    delete mBassSplitter;
    
#if PROCESS_STEREO
    for (int i = 0; i < 2; i++)
        delete mBassSplitters[i];
#endif
    
    delete mSplitSDelay;
    
    delete mCompDelay;
    
    for (int i = 0; i < 3; i++)
        delete mCompDelays2[i];
    
    for (int i = 0; i < 4; i++)
    {
        delete mEarlyReflectDelays[i];
    }
    
    delete mBrillanceSplitter;
    
#if PROCESS_STEREO
    for (int i = 0; i < 2; i++)
        delete mBrillanceSplitters[i];
#endif
        
#if USE_FREEVERB
    delete mRevModel;
#endif
}

void
USTUpmix::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mBassSplitter->Reset(sampleRate);
    
#if PROCESS_STEREO
    for (int i = 0; i < 2; i++)
        mBassSplitters[i]->Reset(sampleRate);
#endif
    
    BL_FLOAT delay = (SPLIT_S_DELAY/1000.0)*sampleRate;
    mSplitSDelay->SetDelay(delay);
    
    mCompDelay->SetDelay(delay);
    
    // TODO: mCompDelays3
    
    for (int i = 0; i < 4; i++)
    {
        mEarlyReflectDelays[i]->Reset();
    }
    
    mBrillanceSplitter->Reset(sampleRate);
    
#if PROCESS_STEREO
    for (int i = 0; i < 2; i++)
        mBrillanceSplitters[i]->Reset(sampleRate);
#endif
        
#if USE_FREEVERB
    mRevModel->init(mSampleRate);
    InitRevModel();
#endif
}

void
USTUpmix::SetEnabled(bool flag)
{
    mIsEnabled = flag;
    
    // bl-iplug2
#if 0
    if (mIsEnabled)
        mGraph->SetInteractionDisabled(false);
    else
        mGraph->SetInteractionDisabled(true);
#endif
    
    // NEW
    mGraph->SetDirty(true);
    //mGraph->SetMyDirty(true);
}

int
USTUpmix::GetNumCurves()
{
    return 0;
}

int
USTUpmix::GetNumPoints()
{
    return 0;
}

void
USTUpmix::SetGraph(UST *plug, GraphControl11 *graph)
{
    mGraph = graph;
    
    if (mGraph != NULL)
    {
        mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraph->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraph->SetClearColor(0, 0, 0, 255);
    }
    
    mUpmixDrawer = new USTUpmixGraphDrawer(plug, graph);
    
    if (mGraph != NULL)
    {
        mGraph->AddCustomDrawer(mUpmixDrawer);
        mGraph->AddCustomControl(mUpmixDrawer);
    }
}

void
USTUpmix::SetGain(BL_FLOAT gain)
{
    mGain = gain;
    
    mUpmixDrawer->SetGain(gain);
    
    if (mGraph != NULL)
    {
        // mGraph->SetMyDirty(true);
        mGraph->SetDirty(true);
    }
}

void
USTUpmix::SetPan(BL_FLOAT pan)
{
    mPan = pan;
    
    mUpmixDrawer->SetPan(pan);
    
    if (mGraph != NULL)
    {
        // mGraph->SetMyDirty(true);
        mGraph->SetDirty(true);
    }
}

void
USTUpmix::SetDepth(BL_FLOAT depth)
{
    mDepth = depth;
    
    mUpmixDrawer->SetDepth(depth);
    
    if (mGraph != NULL)
    {
        // mGraph->SetMyDirty(true);
        mGraph->SetDirty(true);
    }
}

void
USTUpmix::SetBrillance(BL_FLOAT brillance)
{
    mBrillance = brillance;
    
    mUpmixDrawer->SetBrillance(brillance);
    
    if (mGraph != NULL)
    {
        // mGraph->SetMyDirty(true);
        mGraph->SetDirty(true);
    }
}

// See: https://www.plugin-alliance.com/en/products/schoeps_mono_upmix.html
// and: https://www.plugin-alliance.com/en/products/bx_stereomaker.html
//
// NOTE: Tried to get the same result as Waves
// (not totally transparent, less bass compared to bypass)
void
USTUpmix::Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples)
{
    if (!mIsEnabled)
        return;
    
    if (ioSamples->empty())
        return;
    
    // First, set to mono if not already
    USTProcess::StereoToMono(ioSamples);

    // Split in low and hi freqs
    WDL_TypedBuf<BL_FLOAT> loHiFreqs[2];
    mBassSplitter->Split((*ioSamples)[0], loHiFreqs);
    
    // Split diffuse and defined
    WDL_TypedBuf<BL_FLOAT> defined;
    WDL_TypedBuf<BL_FLOAT> diffuse;
    SplitDiffuse(loHiFreqs[1], &defined, &diffuse);
    
#if COMPENSATE_DELAY1
    mCompDelay->ProcessSamples(&diffuse);
#endif

    // TEST
#if 1 // Invert phase and ajust gain
    BLUtils::MultValues(&diffuse, -1.0);
    
#define GAIN_COMPENSATION_DIFFUSE_DB -3.0
    BL_FLOAT coeffDiffuse = DBToAmp(GAIN_COMPENSATION_DIFFUSE_DB);
    BLUtils::MultValues(&loHiFreqs[0], coeffDiffuse);
    BLUtils::MultValues(&defined, coeffDiffuse);
    BLUtils::MultValues(&diffuse, coeffDiffuse);
#endif
    
    ApplyBrillance(&diffuse);
    
#if COMPENSATE_DELAY2 
    // Add delay to avoid black bands in high freqs
    // Add a delay of few samples to compensate filters delay,
    // so defined and diffuse are aligned
    mCompDelays2[0]->ProcessSamples(&loHiFreqs[0]);
    mCompDelays2[1]->ProcessSamples(&loHiFreqs[1]);
    mCompDelays2[2]->ProcessSamples(&defined);
#endif
    
#if TEST_CROSSOVER3
    BLUtils::FillAllZero(&(*ioSamples)[0]);
    
    BLUtils::AddValues(&(*ioSamples)[0], loHiFreqs[0]);
    BLUtils::AddValues(&(*ioSamples)[0], defined);
    BLUtils::AddValues(&(*ioSamples)[0], diffuse);
    
    (*ioSamples)[1] = (*ioSamples)[0];
    
    return;
#endif
    
    // NOTE: transparent
    // Gain
    BoostDiffuse(&diffuse, mGain);
    
    // NOTE: transparent
    // Depth
    WDL_TypedBuf<BL_FLOAT> revSamples[2];
    GenerateEarlyReflect(diffuse, revSamples, mDepth);
    
    // Re-synth
    if (ioSamples->size() != 2)
        return;
    
    (*ioSamples)[0] = defined;
    ////BLUtils::SubstractValues(&(*ioSamples)[0], diffuse);
    BLUtils::AddValues(&(*ioSamples)[0], revSamples[0]);
    BLUtils::AddValues(&(*ioSamples)[0], loHiFreqs[0]);
    BLUtils::AddValues(&(*ioSamples)[0], diffuse);
    
    (*ioSamples)[1] = defined;
    BLUtils::AddValues(&(*ioSamples)[1], revSamples[1]);
    BLUtils::AddValues(&(*ioSamples)[1], loHiFreqs[0]);
    BLUtils::AddValues(&(*ioSamples)[1], diffuse);
    
    // Pan
    vector<WDL_TypedBuf<BL_FLOAT> *> samplesVec;
    for (int i = 0; i < ioSamples->size(); i++)
        samplesVec.push_back(&(*ioSamples)[i]);

    USTProcess::Balance3(&samplesVec, mPan); // Shoud be Balance() (but code is not used)
}

// Simple version, with good depth
void
USTUpmix::ProcessSimple(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples)
{
    if (!mIsEnabled)
        return;
    
    if (ioSamples->empty())
        return;
    
    // First, set to mono if not already
    USTProcess::StereoToMono(ioSamples);
    
    // Split in low and hi freqs
    WDL_TypedBuf<BL_FLOAT> loHiFreqs[2];
    mBassSplitter->Split((*ioSamples)[0], loHiFreqs);
    
    ApplyBrillance(&loHiFreqs[1]);
    
    // Gain
    BoostDiffuse(&loHiFreqs[1], mGain);
    
    // Depth
    WDL_TypedBuf<BL_FLOAT> revSamples[2];
    //GenerateEarlyReflect(loHiFreqs[1], revSamples, mDepth);
    GenerateEarlyReflect2(loHiFreqs[1], revSamples, mDepth);
    
    // Re-synth
    if (ioSamples->size() != 2)
        return;
    
#if 1 // Pre-reverb, then balance (more logical)
    (*ioSamples)[0] = loHiFreqs[0];
    BLUtils::AddValues(&(*ioSamples)[0], loHiFreqs[1]);
    BLUtils::AddValues(&(*ioSamples)[0], revSamples[0]);
    
    (*ioSamples)[1] = loHiFreqs[0];
    BLUtils::AddValues(&(*ioSamples)[1], loHiFreqs[1]);
    BLUtils::AddValues(&(*ioSamples)[1], revSamples[1]);
    
    // Pan
    vector<WDL_TypedBuf<BL_FLOAT> *> samplesVec;
    for (int i = 0; i < ioSamples->size(); i++)
        samplesVec.push_back(&(*ioSamples)[i]);
    
    USTProcess::Balance(&samplesVec, mPan);
#endif
    
#if 0 // Balance, then reverb (strange when panned to extreme side)
    (*ioSamples)[0] = loHiFreqs[0];
    BLUtils::AddValues(&(*ioSamples)[0], loHiFreqs[1]);
    
    (*ioSamples)[1] = loHiFreqs[0];
    BLUtils::AddValues(&(*ioSamples)[1], loHiFreqs[1]);
    
    // Pan
    vector<WDL_TypedBuf<BL_FLOAT> *> samplesVec;
    for (int i = 0; i < ioSamples->size(); i++)
        samplesVec.push_back(&(*ioSamples)[i]);
    
    USTProcess::Balance(&samplesVec, mPan);
    
    BLUtils::AddValues(samplesVec[0], revSamples[0]);
    BLUtils::AddValues(samplesVec[1], revSamples[1]);
#endif
}

void
USTUpmix::ProcessSimpleStereo(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples)
{
    if (!mIsEnabled)
        return;
    
    if (ioSamples->size() < 2)
        return;
    
    // Split in low and hi freqs
    WDL_TypedBuf<BL_FLOAT> loHiFreqs[2][2];
    for (int i = 0; i < 2; i++)
    {
        mBassSplitters[i]->Split((*ioSamples)[i], loHiFreqs[i]);
    
#if !BRILLANCE_FIX
        ApplyBrillanceStereo(i, &loHiFreqs[i][1]);
#endif
        
        // Gain
        BoostDiffuse(&loHiFreqs[i][1], mGain);
    }
    
    // Get mono signal
    WDL_TypedBuf<BL_FLOAT> monoHi;
    BLUtils::StereoToMono(&monoHi, loHiFreqs[0][1], loHiFreqs[1][1]);
    
    // Depth
    WDL_TypedBuf<BL_FLOAT> revSamples[2];
    GenerateEarlyReflect2(monoHi, revSamples, mDepth);
    
#if BRILLANCE_FIX
    for (int i = 0; i < 2; i++)
        ApplyBrillanceStereo(i, &revSamples[i]);
#endif
    
    // Re-synth
    
    // Pre-reverb, then balance (more logical)
    for (int i = 0; i < 2; i++)
    {
        (*ioSamples)[i] = loHiFreqs[i][0];
        BLUtils::AddValues(&(*ioSamples)[i], loHiFreqs[i][1]);
        BLUtils::AddValues(&(*ioSamples)[i], revSamples[i]);
    }
    
    // Pan
    vector<WDL_TypedBuf<BL_FLOAT> *> samplesVec;
    for (int i = 0; i < ioSamples->size(); i++)
        samplesVec.push_back(&(*ioSamples)[i]);
    
    USTProcess::Balance(&samplesVec, mPan);
}

#if 0 // Origin version (v1): duplicate IR
void
USTUpmix::SplitDiffuse(const WDL_TypedBuf<BL_FLOAT> &samples,
                       WDL_TypedBuf<BL_FLOAT> *defined,
                       WDL_TypedBuf<BL_FLOAT> *diffuse)
{
    defined->Resize(samples.GetSize());
    diffuse->Resize(samples.GetSize());
    
    WDL_TypedBuf<BL_FLOAT> delaySamples = samples;
    mSplitSDelay->ProcessSamples(&delaySamples);
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT s0 = samples.Get()[i];
        BL_FLOAT s1 = delaySamples.Get()[i];
        
        BL_FLOAT mid = (s0 + s1)*0.5;
        BL_FLOAT side = s1 - s0;
        
        defined->Get()[i] = mid;
        diffuse->Get()[i] = side;
    }
}
#endif

#if 1
// New version (v2): use complementary comb filters
void
USTUpmix::SplitDiffuse(const WDL_TypedBuf<BL_FLOAT> &samples,
                       WDL_TypedBuf<BL_FLOAT> *defined,
                       WDL_TypedBuf<BL_FLOAT> *diffuse)
{
    defined->Resize(samples.GetSize());
    diffuse->Resize(samples.GetSize());
    
    vector<WDL_TypedBuf<BL_FLOAT> > samplesVec;
    samplesVec.push_back(samples);
    samplesVec.push_back(samples);
    
    // #bl-iplug2
    //USTProcess::MonoToStereo(&samplesVec, mSplitSDelay);
    StereoWidenProcess::MonoToStereo(&samplesVec, mSplitSDelay);
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT s0 = samplesVec[0].Get()[i];
        BL_FLOAT s1 = samplesVec[1].Get()[i];
        
        BL_FLOAT mid = (s0 + s1)*0.5;
        BL_FLOAT side = s1 - s0;
        
        defined->Get()[i] = mid;
        diffuse->Get()[i] = side;
    }
}
#endif

void
USTUpmix::BoostDiffuse(WDL_TypedBuf<BL_FLOAT> *diffuse,
                       BL_FLOAT normGain)
{
    //BL_FLOAT gainDB = normGain*BOOST_DIFFUSE_MAX_GAIN;
    //BL_FLOAT gain = DBToAmp(gainDB);
    
#if 0 // Not transparent
    BL_FLOAT gain = BOOST_DIFFUSE_MIN_GAIN +
                normGain*(BOOST_DIFFUSE_MAX_GAIN - BOOST_DIFFUSE_MIN_GAIN);
#endif
    
#if 1 // Transparent
    BL_FLOAT gainDb = BOOST_DIFFUSE_MIN_GAIN_DB +
                    normGain*(BOOST_DIFFUSE_MAX_GAIN_DB - BOOST_DIFFUSE_MIN_GAIN_DB);
    BL_FLOAT gain = DBToAmp(gainDb);
#endif
    
    BLUtils::MultValues(diffuse, gain);
}

void
USTUpmix::GenerateEarlyReflect(const WDL_TypedBuf<BL_FLOAT> &samples,
                               WDL_TypedBuf<BL_FLOAT> outRevSamples[2],
                               BL_FLOAT normDepth)

{
    // Source coordinate
    BL_FLOAT S[2] = { REV_ROOM_SIZE/2.0, REV_ROOM_SIZE/2.0 };
    
#if 0
    BL_FLOAT normPan = (mPan + 1.0)/2.0;
    S[0] = normPan*REV_ROOM_SIZE;
#endif
    
    // Mics coordinates
    BL_FLOAT L[2] = { REV_ROOM_SIZE/2.0 - REV_INTERMIC_DIST/2.0, 0.0 };
    BL_FLOAT R[2] = { REV_ROOM_SIZE/2.0 + REV_INTERMIC_DIST/2.0, 0.0 };
    
    // Reflection points
    BL_FLOAT Lp0[2] = { (L[0] + S[0])/2.0, REV_ROOM_SIZE };
    BL_FLOAT Rp0[2] = { (R[0] + S[0])/2.0, REV_ROOM_SIZE };
    
    BL_FLOAT Lp1[2] = { 0.0, (L[1] + S[1])/2.0 };
    BL_FLOAT Rp1[2] = { 0.0, (R[1] + S[1])/2.0 };
    
    // Distances
    BL_FLOAT L0 = BLUtils::ComputeDist(S, Lp0) + BLUtils::ComputeDist(Lp0, L);
    BL_FLOAT L1 = BLUtils::ComputeDist(S, Lp1) + BLUtils::ComputeDist(Lp1, L);
    
    BL_FLOAT R0 = BLUtils::ComputeDist(S, Rp0) + BLUtils::ComputeDist(Rp0, R);
    BL_FLOAT R1 = BLUtils::ComputeDist(S, Rp1) + BLUtils::ComputeDist(Rp1, R);
    
    // Times
    BL_FLOAT L0s = L0/REV_SOUND_SPEED;
    BL_FLOAT L1s = L1/REV_SOUND_SPEED;
    
    BL_FLOAT R0s = R0/REV_SOUND_SPEED;
    BL_FLOAT R1s = R1/REV_SOUND_SPEED;
    
    // Delays
    BL_FLOAT Ld0 = L0s*mSampleRate;
    BL_FLOAT Ld1 = L1s*mSampleRate;
    
    BL_FLOAT Rd0 = R0s*mSampleRate;
    BL_FLOAT Rd1 = R1s*mSampleRate;
    
    // Set delays
    mEarlyReflectDelays[0]->SetDelay(Ld0);
    mEarlyReflectDelays[1]->SetDelay(Ld1);
    mEarlyReflectDelays[2]->SetDelay(Rd0);
    mEarlyReflectDelays[3]->SetDelay(Rd1);
    
    // Compute delays
    WDL_TypedBuf<BL_FLOAT> samplesL0 = samples;
    mEarlyReflectDelays[0]->ProcessSamples(&samplesL0);
    
    WDL_TypedBuf<BL_FLOAT> samplesL1 = samples;
    mEarlyReflectDelays[1]->ProcessSamples(&samplesL1);
    
    WDL_TypedBuf<BL_FLOAT> samplesR0 = samples;
    mEarlyReflectDelays[2]->ProcessSamples(&samplesR0);
    
    WDL_TypedBuf<BL_FLOAT> samplesR1 = samples;
    mEarlyReflectDelays[3]->ProcessSamples(&samplesR1);
    
    // Result
    outRevSamples[0].Resize(samples.GetSize());
    outRevSamples[1].Resize(samples.GetSize());
    
    BLUtils::FillAllZero(&outRevSamples[0]);
    BLUtils::AddValues(&outRevSamples[0], samplesL0);
    BLUtils::AddValues(&outRevSamples[0], samplesL1);
    
    BLUtils::FillAllZero(&outRevSamples[1]);
    BLUtils::AddValues(&outRevSamples[1], samplesR0);
    BLUtils::AddValues(&outRevSamples[1], samplesR1);
    
    // Ponderate with depth
    BLUtils::MultValues(&outRevSamples[0], normDepth*REV_DEPTH_COEFF);
    BLUtils::MultValues(&outRevSamples[1], normDepth*REV_DEPTH_COEFF);
}

void
USTUpmix::GenerateEarlyReflect2(const WDL_TypedBuf<BL_FLOAT> &samples,
                                WDL_TypedBuf<BL_FLOAT> outRevSamples[2],
                                BL_FLOAT normDepth)
{
#if USE_FREEVERB
    outRevSamples[0].Resize(samples.GetSize());
    outRevSamples[1].Resize(samples.GetSize());
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT input = samples.Get()[i];
        BL_FLOAT outputL;
        BL_FLOAT outputR;
        
        mRevModel->process(input, outputL, outputR);
        
        outRevSamples[0].Get()[i] = outputL*normDepth;
        outRevSamples[1].Get()[i] = outputR*normDepth;
    }
#endif
}

// PROBLEM: CrossoverFilter3Bands not transparent
// (and 5 bands not exactly transparent)
void
USTUpmix::ApplyBrillance(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    // Split
    WDL_TypedBuf<BL_FLOAT> bands[3];
    mBrillanceSplitter->Split(*ioSamples, bands);
    
    // Increase center band
    BL_FLOAT brillanceDb = mBrillance*BRILLANCE_MAX_DB;
    BL_FLOAT brillanceAmp = DBToAmp(brillanceDb);
    BLUtils::MultValues(&bands[1], brillanceAmp);
    
    // Resynth
    BLUtils::FillAllZero(ioSamples);
    for (int i = 0; i < 3; i++)
    {
        BLUtils::AddValues(ioSamples, bands[i]);
    }
}

// Use two filters, one by channel
// (each channel needs it own filter !)
void
USTUpmix::ApplyBrillanceStereo(int chan, WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    // Split
    WDL_TypedBuf<BL_FLOAT> bands[3];
    mBrillanceSplitters[chan]->Split(*ioSamples, bands);
    
    // Increase center band
    BL_FLOAT brillanceDb = mBrillance*BRILLANCE_MAX_DB;
    BL_FLOAT brillanceAmp = DBToAmp(brillanceDb);
    BLUtils::MultValues(&bands[1], brillanceAmp);
    
    // Resynth
    BLUtils::FillAllZero(ioSamples);
    for (int i = 0; i < 3; i++)
    {
        BLUtils::AddValues(ioSamples, bands[i]);
    }
}

void
USTUpmix::InitRevModel()
{
#if USE_FREEVERB
    //mRevModel->setroomsize(0.5);
    mRevModel->setroomsize(0.1);
    //mRevModel->setdamp(0.5);
    mRevModel->setdamp(2.0);
    mRevModel->setwet(0.3);
    mRevModel->setdry(0.0);
    mRevModel->setwidth(1.0);
    mRevModel->setmode(0.0);
#endif
}

