//
//  StereoVizProcess3.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include <DebugGraph.h>

#include <SASViewerRender.h>

#include <PartialTracker3.h>
#include <SASFrame2.h>

#include "SASViewerProcess.h"


// Scales (x and y)
#define FREQ_MEL_SCALE 1 //0
#define FREQ_LOG_SCALE 0 //1

#define MAGNS_TO_DB    1 //0
#define EPS_DB 1e-15
// With -60, avoid taking background noise
// With -80, takes more partials (but some noise)
#define MIN_DB -60.0 //-80.0

#define MIN_AMP_DB -120.0

#define MEL_SCALE_COEFF 8.0 //4.0
#define LOG_SCALE_COEFF 0.02 //0.01

// Display magns or SAS param while debugging ?
#define DEBUG_DISPLAY_MAGNS 1  //0 //1

#define DEBUG_MUTE_NOISE 0 //1
#define DEBUG_MUTE_PARTIALS 0 //0 //1

#define SHOW_ONLY_ALIVE 0 //1
#define MIN_AGE_DISPLAY 0 //10 // 4


SASViewerProcess::SASViewerProcess(int bufferSize,
                                   BL_FLOAT overlapping, BL_FLOAT oversampling,
                                   BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    //mBufferSize = bufferSize;
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    mSampleRate = sampleRate;
    
    mSASViewerRender = NULL;
    
    mPartialTracker = new PartialTracker3(bufferSize, sampleRate, overlapping);
    mScPartialTracker = new PartialTracker3(bufferSize, sampleRate, overlapping);
    
    mSASFrame = new SASFrame2(bufferSize, sampleRate, overlapping);
    mScSASFrame = new SASFrame2(bufferSize, sampleRate, overlapping);
    mMixSASFrame = new SASFrame2(bufferSize, sampleRate, overlapping);
    
    mThreshold = -60.0;
    mHarmonicFlag = false;
    
    mMode = TRACKING; //AMPLITUDE;
    
    mNoiseMix = 0.0;
    
    // SideChain
    mUseSideChain = false;
    mScThreshold = -60.0;
    mScHarmonicFlag = false;
    
    mSideChainProvided = false;
    
    mMix = 0.0;
    
    mMixFreqFlag = true;
    mMixNoiseFlag = true;
    
    // For additional lines
    mAddNum = 0;
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::Reset(&mTimer0, &mTimerCount0);
    BlaTimer::Reset(&mTimer1, &mTimerCount1);
    BlaTimer::Reset(&mTimer2, &mTimerCount2);
    BlaTimer::Reset(&mTimer3, &mTimerCount3);
#endif
}

SASViewerProcess::~SASViewerProcess()
{
    delete mPartialTracker;
    delete mScPartialTracker;
    
    delete mSASFrame;
    delete mScSASFrame;
    delete mMixSASFrame;
}

void
SASViewerProcess::Reset()
{
    Reset(mOverlapping, mFreqRes/*mOversampling*/, mSampleRate);
    
    mSASFrame->Reset(mSampleRate);
    mScSASFrame->Reset(mSampleRate);
    mMixSASFrame->Reset(mSampleRate);
}

void
SASViewerProcess::Reset(int overlapping, int oversampling,
                        BL_FLOAT sampleRate)
{
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    
    mSampleRate = sampleRate;
    
    //mValues.Resize(0);
    
    mSASFrame->Reset(sampleRate);
    mScSASFrame->Reset(sampleRate);
    mMixSASFrame->Reset(sampleRate);
}

#if 0 // origin, no sidechain (WIP)
void
SASViewerProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    
    if (!mUseSideChain)
        fftSamples = *ioBuffer;
    else
        fftSamples = *scBuffer;
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    mCurrentMagns = magns;
    
#if DEBUG_PARTIAL_TRACKING
    if (mSASViewerRender != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> magnsScale = magns;
        ScaleFreqs(&magnsScale);
        
#if DEBUG_DISPLAY_MAGNS // Display magns
        // Scale to dB for display
        AmpsToDBNorm(&magnsScale);
        
        mSASViewerRender->AddMagns(magnsScale);
#else
        DisplayFrequency();
#endif
    }
#endif
    
#if 0 // Discplay cepstrum
#define MAGNS_COEFF 50.0
    WDL_TypedBuf<BL_FLOAT> magnsScale = magns;
    
    WDL_TypedBuf<BL_FLOAT> cepstrum;
    //FftProcessObj15::MagnsToCepstrum(magnsScale, &cepstrum);
    
    mSASFrame->DBG_GetCepstrum(&cepstrum);
    
    BLUtils::MultValues(&cepstrum, 1.0/200000.0);
    BLUtils::MultValues(&cepstrum, 0.004);
    magnsScale = cepstrum;
    
    BLUtils::MultValues(&magnsScale, MAGNS_COEFF);
    
    ScaleFreqs(&magnsScale);
    
    mSASViewerRender->AddMagns(magnsScale);
#endif
    
    DetectPartials(magns, phases);
    
    if (mPartialTracker != NULL)
    {
        vector<PartialTracker3::Partial> partials;
        mPartialTracker->GetPartials(&partials);
        
         // Debug
        vector<PartialTracker3::Partial> partials0;
        for (int i = 0; i < partials.size(); i++)
        {
            const PartialTracker3::Partial &partial = partials[i];
            if (partial.mFreq < DEBUG_MAX_PARTIAL_FREQ)
                partials0.push_back(partial);
        }
        partials = partials0;
        
#if DEBUG_PARTIAL_TRACKING
        BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
        
        // Display points corresponding to the peaks of the partials
        vector<LinesRender2::Point> points;
        for (int i = 0; i < partials.size(); i++)
        {
            const PartialTracker3::Partial &partial = partials[i];
            
            LinesRender2::Point p;
            
#if !PREDICTIVE
            BL_FLOAT partialX = partial.mFreq/hzPerBin;
#else
            BL_FLOAT partialX = partial.mPredictedFreq/hzPerBin; // TEST
#endif
            partialX = bl_round(partialX);
            
            partialX = ScaleFreq((int)partialX);
            p.mX = partialX/magns.GetSize() - 0.5;
            
            // dB for display
            //p.mY = (partial.mAmpDB - MIN_AMP_DB)/(-MIN_AMP_DB);
            p.mY = DBToAmp(partial.mAmpDB);
            p.mY = AmpToDBNorm(p.mY);
            
            p.mZ = 0.0;
            
            unsigned char color[4];
            PartialToColor(partial, color);
            
            p.mR = color[0];
            p.mG = color[1];
            p.mB = color[2];
            p.mA = color[3];
            
            p.mSize = 6.0;
            if (partial.mId == -1)
                p.mSize = 3.0;
            if (partial.mState == PartialTracker3::Partial::ZOMBIE)
                p.mSize = 3.0;
            if (partial.mState == PartialTracker3::Partial::DEAD)
                p.mSize = 3.0;
            
            points.push_back(p);
        }
        
        if (mSASViewerRender != NULL)
            mSASViewerRender->AddPoints(points);
#endif
        
#if SAS_VIEWER_PROCESS_PROFILE
        BlaTimer::Start(&mTimer1);
#endif
        
        // Avoid sending garbage partials to the SASFrame
        // (would slow down a lot when many garbage partial
        // are used to compute the SASFrame)
        //PartialTracker3::RemoveRealDeadPartials(&partials);
        
        if (!mUseSideChain)
            mSASFrame->SetPartials(partials);
        else
        {
            mScSASFrame->SetPartials(partials);
        }
        
#if SAS_VIEWER_PROCESS_PROFILE
        BlaTimer::StopAndDump(&mTimer1, &mTimerCount1, "profile.txt", "sas compute: %ld"); //
#endif
        // Display the current data
        Display();
        
#if !DEBUG_MUTE_NOISE
      // Normal behavior
      // Noise envelope
        mPartialTracker->GetNoiseEnvelope(&magns);
#endif
        
#if 0 // Test
        mPartialTracker->GetHarmonicEnvelope(&magns);
#endif
    }
    
    // For noise envelope
    BLUtils::MagnPhaseToComplex(ioBuffer, magns, phases);
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
}
#endif

void
SASViewerProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    
    mSideChainProvided = false;
    WDL_TypedBuf<WDL_FFT_COMPLEX> scFftSamples;
    if (scBuffer != NULL)
    {
        scFftSamples = *scBuffer;
        
        mSideChainProvided = true;
    }
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    // Sc
    BLUtils::TakeHalf(&scFftSamples);
    
    WDL_TypedBuf<BL_FLOAT> scMagns;
    WDL_TypedBuf<BL_FLOAT> scPhases;
    BLUtilsComp::ComplexToMagnPhase(&scMagns, &scPhases, scFftSamples);
    
    if (!mUseSideChain || !mSideChainProvided)
        mCurrentMagns = magns;
    else
        mCurrentMagns = scMagns;
    
    DetectPartials(magns, phases);
    
#if 1 // QUICK TEST (for freeze)
    if (mUseSideChain)
#endif
        DetectScPartials(scMagns, scPhases);
    
    if (mPartialTracker != NULL)
    {
        vector<PartialTracker3::Partial> partials;
        mPartialTracker->GetPartials(&partials);
        
        vector<PartialTracker3::Partial> scPartials;
        mScPartialTracker->GetPartials(&scPartials);
        
        if (!mUseSideChain || !mSideChainProvided)
            mCurrentPartials = partials;
        else
            mCurrentPartials = scPartials;
        
#if SAS_VIEWER_PROCESS_PROFILE
        BlaTimer::Start(&mTimer1);
#endif
        
        // Avoid sending garbage partials to the SASFrame
        // (would slow down a lot when many garbage partial
        // are used to compute the SASFrame)
        //PartialTracker3::RemoveRealDeadPartials(&partials);
        
        mSASFrame->SetPartials(partials);
        mScSASFrame->SetPartials(scPartials);
        
        MixFrames(mMixSASFrame, *mSASFrame, *mScSASFrame, mMix);
        
#if SAS_VIEWER_PROCESS_PROFILE
        BlaTimer::StopAndDump(&mTimer1, &mTimerCount1, "profile.txt", "sas compute: %ld"); //
#endif
        // Display the current data
        Display();
        
#if !DEBUG_MUTE_NOISE
        // Normal behavior
        // Noise envelope
        if (mMixNoiseFlag)
        {
            mPartialTracker->GetNoiseEnvelope(&magns);
            mScPartialTracker->GetNoiseEnvelope(&scMagns);
        
            WDL_TypedBuf<BL_FLOAT> mixMagns = magns;
            if (scMagns.GetSize() == magns.GetSize())
                BLUtils::Interp(&mixMagns, &magns, &scMagns, mMix);
            magns = mixMagns;
        }
        else
        {
            mPartialTracker->GetNoiseEnvelope(&magns);
        }
#endif
        
#if 0 // Test
        mPartialTracker->GetHarmonicEnvelope(&magns);
#endif
    }
    
    // For noise envelope
    BLUtilsComp::MagnPhaseToComplex(ioBuffer, magns, phases);
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

void
SASViewerProcess::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                       WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    //if (!mSASFrame->ComputeSamplesFlag())
    //    return;
    if (!mMixSASFrame->ComputeSamplesFlag())
        return;
    
#if DEBUG_MUTE_NOISE
    // For the moment, empty the io buffer
    BLUtils::FillAllZero(ioBuffer);
#endif
    
#if !DEBUG_MUTE_PARTIALS
    // Create a separate buffer for samples synthesis from partials
    // (because it doesn't use overlap)
    WDL_TypedBuf<BL_FLOAT> samplesBuffer;
    BLUtils::ResizeFillZeros(&samplesBuffer, ioBuffer->GetSize());
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::Start(&mTimer2);
#endif
    
    // Compute the samples from partials
    //mSASFrame->ComputeSamples(&samplesBuffer);
    mMixSASFrame->ComputeSamples(&samplesBuffer);
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer2, &mTimerCount2, "profile.txt", "sas gen: %ld"); //
#endif

    // Mix
    // (the current io buffer may contain shaped noise)
    //BLUtils::AddValues(ioBuffer, samplesBuffer);
    MixHarmoNoise(ioBuffer, samplesBuffer);
#endif
}

// Use this to synthetize directly 1/4 of the samples from partials
// (without overlap in internal)
void
SASViewerProcess::ProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                          const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    //if (!mSASFrame->ComputeSamplesWinFlag())
    //    return;
    if (!mMixSASFrame->ComputeSamplesWinFlag())
        return;
    
#if DEBUG_MUTE_NOISE
    // For the moment, empty the io buffer
    BLUtils::FillAllZero(ioBuffer);
#endif
    
#if !DEBUG_MUTE_PARTIALS
    // Create a separate buffer for samples synthesis from partials
    // (because it doesn't use overlap)
    WDL_TypedBuf<BL_FLOAT> samplesBuffer;
    BLUtils::ResizeFillZeros(&samplesBuffer, ioBuffer->GetSize());
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::Start(&mTimer2);
#endif
    
    // Compute the samples from partials
    //mSASFrame->ComputeSamplesWin(&samplesBuffer);
    mMixSASFrame->ComputeSamplesWin(&samplesBuffer);
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer2, &mTimerCount2, "profile.txt", "sas gen: %ld"); //
#endif
    
    // Mix
    // (the current io buffer may contain shaped noise)
    //BLUtils::AddValues(ioBuffer, samplesBuffer);
    
    MixHarmoNoise(ioBuffer, samplesBuffer);
#endif
}

void
SASViewerProcess::SetSASViewerRender(SASViewerRender *sasViewerRender)
{
    mSASViewerRender = sasViewerRender;
}

void
SASViewerProcess::SetMode(Mode mode)
{
    if (mode != mMode)
    {
        mMode = mode;
    
        // Clear the previous data
        if (mSASViewerRender != NULL)
            mSASViewerRender->Clear();
    
        mPartialsPoints.clear();
        
        mPartialLines.clear();
        
        // Try to avoid remaining blue lines when quitting
        // tracking mode and returning to it
        mPartialTracker->ClearResult();
        
        if (mSASViewerRender != NULL)
            mSASViewerRender->ClearAdditionalLines();
        
        mCurrentMagns.Resize(0);
        
        Display();
    }
}

void
SASViewerProcess::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
    mPartialTracker->SetThreshold(threshold);
}

void
SASViewerProcess::SetPitch(BL_FLOAT pitch)
{
    //mSASFrame->SetPitch(pitch);
    mMixSASFrame->SetPitch(pitch);
}

void
SASViewerProcess::SetNoiseMix(BL_FLOAT mix)
{
    mNoiseMix = mix;
}

void
SASViewerProcess::SetHarmonicSoundFlag(bool flag)
{
    mHarmonicFlag = flag;
    mSASFrame->SetHarmonicSoundFlag(flag);
}

void
SASViewerProcess::SetSynthMode(SASFrame2::SynthMode mode)
{
    //mSASFrame->SetSynthMode(mode);
    mMixSASFrame->SetSynthMode(mode);
}

// SideChain
//

void
SASViewerProcess::SetUseSideChainFlag(bool flag)
{
    mUseSideChain = flag;
    
    if (!mUseSideChain)
    {
        mPartialTracker->SetThreshold(mThreshold);
    }
    else
    {
        mScPartialTracker->SetThreshold(mScThreshold);
    }
}

void
SASViewerProcess::SetScThreshold(BL_FLOAT threshold)
{
    mScThreshold = threshold;
    
    mScPartialTracker->SetThreshold(mScThreshold);
}

void
SASViewerProcess::SetScHarmonicSoundFlag(bool flag)
{
    mScHarmonicFlag = flag;
    
    mScSASFrame->SetHarmonicSoundFlag(flag);
}


void
SASViewerProcess::SetMix(BL_FLOAT mix)
{
    mMix = mix;
}

void
SASViewerProcess::SetMixFreqFlag(bool flag)
{
    mMixFreqFlag = flag;
}

void
SASViewerProcess::SetMixNoiseFlag(bool flag)
{
    mMixNoiseFlag = flag;
}

#if 0
BL_FLOAT
SASViewerProcess::IdToFreq(int idx, BL_FLOAT sampleRate, int bufferSize)
{
    BL_FLOAT hzPerBin = sampleRate/bufferSize;
    
#if FREQ_MEL_SCALE
    // Convert from Mel
    hzPerBin *= MEL_SCALE_COEFF;
    
    BL_FLOAT freq = BLUtils::MelNormToFreq(idx, hzPerBin, bufferSize);
#endif
    
#if FREQ_LOG_SCALE
    // Convert from log
    BL_FLOAT freq = BLUtils::LogNormToFreq(idx, hzPerBin, bufferSize);
#endif
    
    return freq;
}

int
SASViewerProcess::FreqToId(BL_FLOAT freq, BL_FLOAT sampleRate, int bufferSize)
{
#if FREQ_MEL_SCALE
    // Convert from Mel
    
    BL_FLOAT hzPerBin = sampleRate/bufferSize;
    
    int idx = freq / hzPerBin;
    
    return idx;
#endif
    
#if FREQ_LOG_SCALE
    // Convert from log
    
    BL_FLOAT hzPerBin = sampleRate/bufferSize;
    
    int idx = freq / hzPerBin;
    
    return idx;
#endif
}
#endif

BL_FLOAT
SASViewerProcess::AmpToDBNorm(BL_FLOAT val)
{
    BL_FLOAT result = 0.0;
    
#if MAGNS_TO_DB
    result = BLUtils::AmpToDBNorm(val, (BL_FLOAT)EPS_DB, (BL_FLOAT)MIN_DB);
#endif
    
    return result;
}

BL_FLOAT
SASViewerProcess::DBToAmpNorm(BL_FLOAT val)
{
    BL_FLOAT result = 0.0;
    
#if MAGNS_TO_DB
    result = BLUtils::DBToAmpNorm(val, (BL_FLOAT)EPS_DB, (BL_FLOAT)MIN_DB);
#endif
    
    return result;
}

void
SASViewerProcess::AmpsToDBNorm(WDL_TypedBuf<BL_FLOAT> *amps)
{
    for (int i = 0; i < amps->GetSize(); i++)
    {
        BL_FLOAT amp = amps->Get()[i];
        amp = AmpToDBNorm(amp);
        
        amps->Get()[i] = amp;
    }
}

void
SASViewerProcess::Display()
{
#if !DEBUG_PARTIAL_TRACKING
#if !DEBUG_DISPLAY_SCEPSTRUM
    if (mMode == TRACKING)
    {
        DisplayTracking();
    }
    
    if (mMode == AMPLITUDE)
    {
        DisplayAmplitude();
    }
    
    if (mMode == FREQUENCY)
    {
        DisplayFrequency();
    }
    
    if (mMode == COLOR)
    {
        DisplayColor();
    }
    
    if (mMode == WARPING)
    {
        DisplayWarping();
    }
#endif
#endif
}

void
SASViewerProcess::ScaleFreqs(WDL_TypedBuf<BL_FLOAT> *values)
{
    WDL_TypedBuf<BL_FLOAT> valuesRescale = *values;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
#if FREQ_MEL_SCALE
    // Convert to Mel
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= MEL_SCALE_COEFF;
    
    BLUtils::FreqsToMelNorm(&valuesRescale, *values, hzPerBin);
#endif
    
#if FREQ_LOG_SCALE
    // Convert to log
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= LOG_SCALE_COEFF;
    
    BLUtils::FreqsToLogNorm(&valuesRescale, *values, hzPerBin);
#endif
    
    *values = valuesRescale;
}

int
SASViewerProcess::ScaleFreq(int idx)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
#if FREQ_MEL_SCALE
    // Convert to Mel
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= MEL_SCALE_COEFF;
    
    int result = BLUtils::FreqIdToMelNormId(idx, hzPerBin, mBufferSize);
#endif
    
#if FREQ_LOG_SCALE
    // Convert to log
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= LOG_SCALE_COEFF;
    
    int result = BLUtils::FreqIdToLogNormId(idx, hzPerBin, mBufferSize);
#endif
    
    return result;
}

void
SASViewerProcess::AmpsToDb(WDL_TypedBuf<BL_FLOAT> *magns)
{
#if MAGNS_TO_DB
    WDL_TypedBuf<BL_FLOAT> magnsDB;
    BLUtils::AmpToDBNorm(&magnsDB, *magns, (BL_FLOAT)EPS_DB, (BL_FLOAT)MIN_DB);
    
    *magns = magnsDB;
#endif
}

void
SASViewerProcess::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                 const WDL_TypedBuf<BL_FLOAT> &phases)
{
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::Start(&mTimer0);
#endif

    mPartialTracker->SetData(magns, phases);
    mPartialTracker->DetectPartials();
    
#if !DEBUG_MUTE_NOISE
    mPartialTracker->ExtractNoiseEnvelope();
#endif
    
    mPartialTracker->FilterPartials();
    
#if 0
    // Debug
    vector<PartialTracker3::Partial> partials;
    mPartialTracker->GetPartials(&partials);
    mPartialTracker->DBG_DumpPartials2(partials);
#endif
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer0, &mTimerCount0, "profile.txt", "tracker: %ld"); //
#endif
}

void
SASViewerProcess::DetectScPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                 const WDL_TypedBuf<BL_FLOAT> &phases)
{
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::Start(&mTimer0);
#endif
    
    mScPartialTracker->SetData(magns, phases);
    mScPartialTracker->DetectPartials();
    
#if !DEBUG_MUTE_NOISE
    mScPartialTracker->ExtractNoiseEnvelope();
#endif
    
    mScPartialTracker->FilterPartials();
    
#if 0
    // Debug
    vector<PartialTracker3::Partial> partials;
    mPartialTracker->GetPartials(&partials);
    mPartialTracker->DBG_DumpPartials2(partials);
#endif
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer0, &mTimerCount0, "profile.txt", "tracker: %ld"); //
#endif
}

void
SASViewerProcess::IdToColor(int idx, unsigned char color[3])
{
    if (idx == -1)
    {
        color[0] = 255;
        color[1] = 0;
        color[2] = 0;
        
        return;
    }
    
    int r = (678678 + idx*12345)%255;
    int g = (3434345 + idx*123345435345)%255;
    int b = (997867 + idx*12345114222)%255;
    
    color[0] = r;
    color[1] = g;
    color[2] = b;
}

void
SASViewerProcess::PartialToColor(const PartialTracker3::Partial &partial,
                                 unsigned char color[4])
{
    if (partial.mId == -1)
    {
        // Green
        color[0] = 0;
        color[1] = 255;
        color[2] = 0;
        color[3] = 255;
        
        return;
    }
    
    int deadAlpha = 255; //128;
    
#if SHOW_ONLY_ALIVE
    deadAlpha = 0;
#endif
    
    if (partial.mState == PartialTracker3::Partial::ZOMBIE)
    {
        // Green
        color[0] = 255;
        color[1] = 0;
        color[2] = 255;
        color[3] = deadAlpha;
        
        return;
    }
    
    if (partial.mState == PartialTracker3::Partial::DEAD)
    {
        // Green
        color[0] = 255;
        color[1] = 0;
        color[2] = 0;
        color[3] = deadAlpha;
        
        return;
    }
    
    int alpha = 255;
    if (partial.mAge < MIN_AGE_DISPLAY)
    {
        alpha = 0;
    }
    
    IdToColor(partial.mId, color);
    color[3] = alpha; //255;
}

void
SASViewerProcess::DisplayTracking()
{
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::Start(&mTimer3);
#endif
    
    if (mSASViewerRender != NULL)
    {
        // Add the magnitudes
        //
        WDL_TypedBuf<BL_FLOAT> magnsScale = mCurrentMagns;
        ScaleFreqs(&magnsScale);
        
        // Scale to dB for display
        AmpsToDBNorm(&magnsScale);
        
        mSASViewerRender->AddMagns(magnsScale);
        
        mSASViewerRender->SetLineMode(LinesRender2::LINES_FREQ);
        
        // Add lines corresponding to the well tracked partials
        //
        BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
        
        //
        //vector<PartialTracker3::Partial> partials;
        //mPartialTracker->GetPartials(&partials);
        vector<PartialTracker3::Partial> partials = mCurrentPartials;
        
        // Create blue lines from trackers
        vector<LinesRender2::Point> line;
        for (int i = 0; i < partials.size(); i++)
        {
            const PartialTracker3::Partial &partial = partials[i];
            
            LinesRender2::Point p;
            
            BL_FLOAT partialX = partial.mFreq/hzPerBin;
            partialX = bl_round(partialX);
            
            partialX = ScaleFreq((int)partialX);
            if (mCurrentMagns.GetSize() <= 1)
                p.mX = partialX/mCurrentMagns.GetSize() - 0.5;
            else
                p.mX = partialX/(mCurrentMagns.GetSize() - 1) - 0.5;
            
            // dB for display
            //p.mY = (partial.mAmpDB - MIN_AMP_DB)/(-MIN_AMP_DB);
            p.mY = DBToAmp(partial.mAmpDB);
            p.mY = AmpToDBNorm(p.mY);
            
            p.mZ = 0.0;
            
            p.mId = partial.mId;
            
            line.push_back(p);
        }
        
        int numSlices = mSASViewerRender->GetNumSlices();
        int speed = mSASViewerRender->GetSpeed();
        
        bool skipAdd = (mAddNum++ % speed != 0);
        if (!skipAdd)
        {
            // Keep track of the points we pop
            vector<LinesRender2::Point> prevPoints;
            
            mPartialsPoints.push_back(line);
            while(mPartialsPoints.size() > numSlices)
            {
                prevPoints = mPartialsPoints[0];
                
                mPartialsPoints.pop_front();
            }
        
#if 0
            //vector<vector<LinesRender2::Point> > partialLines;
            //CreateLines(&partialLines);
#endif
            
#if 1 // Optim
            CreateLines2(prevPoints);
#endif
            
            //unsigned char color[4] = { 0, 0, 255, 255 };
            //unsigned char color[4] = { 128, 128, 255, 255 };
            //unsigned char color[4] = { 64, 64, 255, 255 };
            
            // It is cool like that: lite blue with alpha
            //unsigned char color[4] = { 64, 64, 255, 128 };
            unsigned char color[4] = { 64, 64, 255, 255 };
            
            // Set color
            for (int j = 0; j < mPartialLines.size(); j++)
            {
                LinesRender2::Line &line2 = mPartialLines[j];
                
                if (!line2.mPoints.empty())
                {
                    line2.mColor[0] = color[0];
                    line2.mColor[1] = color[1];
                    line2.mColor[2] = color[2];
                    
                    //IdToColor(line.mPoints[0].mId, line.mColor);
                    line2.mColor[3] = 255; // alpha
                }
            }
            
            //BL_FLOAT lineWidth = 4.0;
            BL_FLOAT lineWidth = 1.5;
            mSASViewerRender->SetAdditionalLines(mPartialLines, lineWidth);
        }
        
        //mSASViewerRender->ShowAdditionalLines(true);
    }
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer3, &mTimerCount3, "profile.txt", "disp tracking: %ld"); //
#endif
}

void
SASViewerProcess::DisplayAmplitude()
{
    BL_FLOAT ampDB;
    if (!mUseSideChain)
        //ampDB = mSASFrame->GetAmplitudeDB();
        ampDB = mMixSASFrame->GetAmplitudeDB();
    else
        ampDB = mScSASFrame->GetAmplitudeDB();
    
#define Y_COEFF_AMP 20.0
#define Y_OFFSET_AMP 0.0
    
    BL_FLOAT amp = DBToAmp(ampDB);
    
    WDL_TypedBuf<BL_FLOAT> amps;
    amps.Resize(mBufferSize/2);
    BLUtils::FillAllValue(&amps, amp);
    
    BLUtils::MultValues(&amps, (BL_FLOAT)Y_COEFF_AMP);
    BLUtils::AddValues(&amps, (BL_FLOAT)Y_OFFSET_AMP);
    
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->AddMagns(amps);
        
        // WARNING: Won't benefit from straight lines optim
        mSASViewerRender->SetLineMode(LinesRender2::LINES_TIME);
        
        mSASViewerRender->ShowAdditionalLines(false);
    }
}

void
SASViewerProcess::DisplayFrequency()
{
    BL_FLOAT freq;
    if (!mUseSideChain)
        //freq = mSASFrame->GetFrequency();
        freq = mMixSASFrame->GetFrequency();
    else
        freq = mScSASFrame->GetFrequency();
    
    BL_FLOAT factor = 3.0;
    freq = BLUtils::LogScaleNormInv(freq, (BL_FLOAT)44100.0, factor);
    
//#define Y_COEFF 0.004
//#define Y_OFFSET -8.0
  
#define Y_COEFF_FREQ 0.002
#define Y_OFFSET_FREQ -4.0

    WDL_TypedBuf<BL_FLOAT> freqs;
    freqs.Resize(mBufferSize/2);
    BLUtils::FillAllValue(&freqs, freq);
    
    BLUtils::MultValues(&freqs, (BL_FLOAT)Y_COEFF_FREQ);
    BLUtils::AddValues(&freqs, (BL_FLOAT)Y_OFFSET_FREQ);
    
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->AddMagns(freqs);
        
        // WARNING: Won't benefit from straight lines optim
        mSASViewerRender->SetLineMode(LinesRender2::LINES_TIME);
        
        mSASViewerRender->ShowAdditionalLines(false);
    }
}

void
SASViewerProcess::DisplayColor()
{
    WDL_TypedBuf<BL_FLOAT> color;
    if (!mUseSideChain)
        //mSASFrame->GetColor(&color);
        mMixSASFrame->GetColor(&color);
    else
        mScSASFrame->GetColor(&color);
    
    ScaleFreqs(&color);
    
    BL_FLOAT amplitudeDB;
    if (!mUseSideChain)
        //amplitudeDB = mSASFrame->GetAmplitudeDB();
        amplitudeDB = mMixSASFrame->GetAmplitudeDB();
    else
        amplitudeDB = mScSASFrame->GetAmplitudeDB();
    
    BL_FLOAT amplitude = DBToAmp(amplitudeDB);
    
    BLUtils::MultValues(&color, amplitude);
    
    // Scale to dB for display
    AmpsToDBNorm(&color); //
    
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->AddMagns(color);
        mSASViewerRender->SetLineMode(LinesRender2::LINES_FREQ);
        mSASViewerRender->ShowAdditionalLines(false);
    }
}

void
SASViewerProcess::DisplayWarping()
{
    WDL_TypedBuf<BL_FLOAT> warping;
    
    if (!mUseSideChain)
        //mSASFrame->GetNormWarping(&warping);
        mMixSASFrame->GetNormWarping(&warping);
    else
        mScSASFrame->GetNormWarping(&warping);
    
    ScaleFreqs(&warping);
    
    BLUtils::AddValues(&warping, (BL_FLOAT)-1.0);
    
#define COEFF 4.0
    BLUtils::MultValues(&warping, (BL_FLOAT)COEFF);
    
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->AddMagns(warping);
        mSASViewerRender->SetLineMode(LinesRender2::LINES_FREQ);
        mSASViewerRender->ShowAdditionalLines(false);
    }
}

int
SASViewerProcess::FindIndex(const vector<int> &ids, int idx)
{
    if (idx == -1)
        return -1;
    
    for (int i = 0; i < ids.size(); i++)
    {
        if (ids[i] == idx)
            return i;
    }
    
    return -1;
}

int
SASViewerProcess::FindIndex(const vector<LinesRender2::Point> &points, int idx)
{
    if (idx == -1)
        return -1;
    
    for (int i = 0; i < points.size(); i++)
    {
        if (points[i].mId == idx)
            return i;
    }
    
    return -1;
}

void
SASViewerProcess::CreateLines(vector<vector<LinesRender2::Point> > *partialLines)
{
    if (mSASViewerRender == NULL)
        return;
    
    vector<int> idsProcessed;
    for (int i = 0; i < mPartialsPoints.size(); i++)
    {
        const vector<LinesRender2::Point> &line = mPartialsPoints[i];
        for (int j = 0; j < line.size(); j++)
        {
            LinesRender2::Point p = line[j];
            
            if (p.mId == -1)
                continue;
            
            int idx = FindIndex(idsProcessed, p.mId);
            if (idx != -1)
                // Already done
                //break; // Buggy
                continue; // Good
            
            // New partial line
            vector<LinesRender2::Point> partialLine;
     
#if 1
            int divisor = mSASViewerRender->GetNumSlices() - 1;
            if (divisor <= 0)
                divisor = 1;
            
            BL_FLOAT z = ((BL_FLOAT)i)/divisor;
            
            // Hack to display well blue tracking lines
            // even when mPartialsPoints is not full
            if (mPartialsPoints.size() < mSASViewerRender->GetNumSlices())
            // Not fully filled
            {
                BL_FLOAT coeff = ((BL_FLOAT)mSASViewerRender->GetNumSlices())/mPartialsPoints.size();
                z = 1.0 -1.0/coeff + z;
            }
#endif
            
#if 0
            BL_FLOAT z = 0.0;
            if (mPartialsPoints.size() > 1)
                z = ((BL_FLOAT)i)/(mPartialsPoints.size() - 1);
#endif
            p.mZ = z;
            
            // Add the current point
            partialLine.push_back(p);
            
            for (int k = i + 1; k < mPartialsPoints.size(); k++)
            {
                const vector<LinesRender2::Point> &line0 = mPartialsPoints[k];
                
                int idx0 = FindIndex(line0, p.mId);
                if (idx0 != -1)
                {
                    LinesRender2::Point p0 = line0[idx0];
                    
#if 1
                    int divisor2 = mSASViewerRender->GetNumSlices() - 1;
                    if (divisor2 <= 0)
                        divisor2 = 1;
                    
                    BL_FLOAT z2 = ((BL_FLOAT)k)/divisor2;
                    
                    // Hack to display well blue tracking lines
                    // even when mPartialsPoints is not full
                    if (mPartialsPoints.size() < mSASViewerRender->GetNumSlices())
                    // Not fully filled
                    {
                        BL_FLOAT coeff =
                            ((BL_FLOAT)mSASViewerRender->GetNumSlices())/
                            mPartialsPoints.size();
                        z2 = 1.0 - 1.0/coeff + z2;
                    }
#endif
                    
#if 0
                    BL_FLOAT z2 = 0.0;
                    if (mPartialsPoints.size() > 1)
                        z2 = ((BL_FLOAT)k)/(mPartialsPoints.size() - 1);
#endif
                    
                    p0.mZ = z2;
                    
                    partialLine.push_back(p0);
                }
            }
            
            if (!partialLine.empty())
            {
                partialLines->push_back(partialLine);
                idsProcessed.push_back(p.mId);
            }
        }
    }
}

// Try to optim
void
SASViewerProcess::CreateLines2(const vector<LinesRender2::Point> &prevPoints)
{
    if (mSASViewerRender == NULL)
        return;
    
    if (mPartialsPoints.empty())
        return;
    
    // Update z for the current line points
    //
    int divisor = mSASViewerRender->GetNumSlices() - 1;
    if (divisor <= 0)
        divisor = 1;
    BL_FLOAT incrZ = 1.0/divisor;
    
#if 0
    BL_FLOAT coeff = ((BL_FLOAT)mSASViewerRender->GetNumSlices())/mPartialsPoints.size();
    BL_FLOAT coeffInv = 0.0;
    if (coeff > 0.0)
        coeffInv = 1.0/coeff;
#endif
    
    for (int i = 0; i < mPartialLines.size(); i++)
    {
        LinesRender2::Line &line = mPartialLines[i];
        for (int j = 0; j < line.mPoints.size(); j++)
        {
            LinesRender2::Point &p = line.mPoints[j];
            
            p.mZ -= incrZ;
            
#if 0 // No need, z value will manage it well
            // Hack to display well blue tracking lines
            // even when mPartialsPoints is not full
            if (mPartialsPoints.size() < mSASViewerRender->GetNumSlices())
                // Not fully filled
            {
                p.mZ = 1.0 -coeffInv + p.mZ;
            }
#endif
        }
    }
    
    // Shorten the lines if they are too long
    //
    vector<LinesRender2::Line> newLines;
    for (int i = 0; i < mPartialLines.size(); i++)
    {
        const LinesRender2::Line &line = mPartialLines[i];
        
        LinesRender2::Line newLine;
        for (int j = 0; j < line.mPoints.size(); j++)
        {
            const LinesRender2::Point &p = line.mPoints[j];
            if (p.mZ > 0.0)
                newLine.mPoints.push_back(p);
        }
        
        if (!newLine.mPoints.empty())
            newLines.push_back(newLine);
    }
    
    // Update the current partial lines
    mPartialLines = newLines;
    newLines.clear();
    
#if 0 // No need, will suppress using z value
    
    // Suppress the lines that disappeared
    //
    
    // Find the ids to suppress
    vector<int> idsToSuppress;
    const vector<LinesRender2::Point> &lastPoints = mPartialsPoints[0];
    for (int i = 0; i < prevPoints.size(); i++)
    {
        const LinesRender2::Point &p = prevPoints[i];
        
        int idx = FindIndex(lastPoints, p.mId);
        if (idx == -1)
        {
            idsToSuppress.push_back(p.mId);
        }
    }
        
    // Suppress the points from the current lines
    for (int i = 0; i < mPartialLines.size(); i++)
    {
        const vector<LinesRender2::Point> &line = mPartialLines[i];
        if (!line.empty())
        {
            int idx = FindIndex(idsToSuppress, line[0].mId);
            if (idx == -1)
            // Not found in the suppress list
            {
                newLines.push_back(line);
            }
        }
    }
    
    // Update the current partial lines
    mPartialLines = newLines;
#endif
    
    // Create the new lines
    //
    const vector<LinesRender2::Point> &newPoints = mPartialsPoints[mPartialsPoints.size() - 1];
    for (int i = 0; i < newPoints.size(); i++)
    {
        LinesRender2::Point newPoint = newPoints[i];
        newPoint.mZ = 1.0;
        
        bool pointAdded = false;
        
        // Search for previous lines to be continued
        for (int j = 0; j < mPartialLines.size(); j++)
        {
            LinesRender2::Line &prevLine = mPartialLines[j];
        
            if (!prevLine.mPoints.empty())
            {
                int lineIdx = prevLine.mPoints[0].mId;
                
                if (lineIdx == newPoint.mId)
                {
                    // Add the point to prev line
                    prevLine.mPoints.push_back(newPoint);
                    
                    // We are done
                    pointAdded = true;
                    
                    break;
                }
            }
        }
        
        // Create a new line ?
        if (!pointAdded)
        {
            LinesRender2::Line newLine;
            newLine.mPoints.push_back(newPoint);
            
            mPartialLines.push_back(newLine);
        }
    }
}

// ioBuffer contains noise in input
// and the result as output
void
SASViewerProcess::MixHarmoNoise(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                const WDL_TypedBuf<BL_FLOAT> &harmoBuffer)
{
    BL_FLOAT noiseCoeff;
    BL_FLOAT harmoCoeff;
    BLUtils::MixParamToCoeffs(mNoiseMix, &noiseCoeff, &harmoCoeff);
    
    WDL_TypedBuf<BL_FLOAT> newBuf;
    newBuf.Resize(ioBuffer->GetSize());
    for (int i = 0; i < newBuf.GetSize(); i++)
    {
        BL_FLOAT n = ioBuffer->Get()[i];
        BL_FLOAT h = harmoBuffer.Get()[i];
        
        BL_FLOAT val = n*noiseCoeff + h*harmoCoeff;
        newBuf.Get()[i] = val;
    }
    
    *ioBuffer = newBuf;
}

void
SASViewerProcess::MixFrames(SASFrame2 *result,
                            const SASFrame2 &frame0,
                            const SASFrame2 &frame1,
                            BL_FLOAT t)
{
    // Amp
    BL_FLOAT amp0 = frame0.GetAmplitudeDB();
    BL_FLOAT amp1 = frame1.GetAmplitudeDB();
    BL_FLOAT resultAmp = (1.0 - t)*amp0 + t*amp1;
    result->SetAmplitudeDB(resultAmp);
    
    // Freq
    if (mMixFreqFlag)
    {
        BL_FLOAT freq0 = frame0.GetFrequency();
        BL_FLOAT freq1 = frame1.GetFrequency();
        BL_FLOAT resultFreq = (1.0 - t)*freq0 + t*freq1;

        result->SetFrequency(resultFreq);
    }
    else
    {
        BL_FLOAT freq0 = frame0.GetFrequency();
        result->SetFrequency(freq0);
    }
    
    // Color
    WDL_TypedBuf<BL_FLOAT> color0;
    frame0.GetColor(&color0);
    
    WDL_TypedBuf<BL_FLOAT> color1;
    frame1.GetColor(&color1);
    
    WDL_TypedBuf<BL_FLOAT> resultColor;
    BLUtils::Interp(&resultColor, &color0, &color1, t);
    
    result->SetColor(resultColor);
    
    // Warping
    WDL_TypedBuf<BL_FLOAT> warp0;
    frame0.GetNormWarping(&warp0);
    
    WDL_TypedBuf<BL_FLOAT> warp1;
    frame1.GetNormWarping(&warp1);
    
    WDL_TypedBuf<BL_FLOAT> resultWarping;
    BLUtils::Interp(&resultWarping, &warp0, &warp1, t);
    
    result->SetNormWarping(resultWarping);
}

#endif // IGRAPHICS_NANOVG
