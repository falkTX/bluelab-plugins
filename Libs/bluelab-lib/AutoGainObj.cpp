//
//  AutoGainObj.cpp
//  BL-AutoGain-macOS
//
//  Created by applematuer on 11/26/20.
//
//

#include <SmoothAvgHistogram.h>
#include <ParamSmoother.h>

#include <BLUtils.h>
#include <BLUtilsPlug.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "AutoGainObj.h"

#define DB_INF -120.0

// FIX: Flat curve at the end of the graph with high sample rates
// Set a very low value, so if we increase the gain,
// we won't have flat values at the end of the result curve
//
// WARNING: mofify the result compared to all at -120dB
#define DB_INF2 -200.0

#define SMOOTH_HISTO_COEFF 0.8 // 0.63849ms at 44100Hz
#define HISTO_DEFAULT_VALUE -60.0

// Debug: test bigger buffer size for high SR
#define TEST_BUFFERS_SIZE 0

#define USE_AWEIGHTING 0

// With that, we have lag (it seems...)
// With 0, we have jumps in volume
#define GAIN_SMOOTHER_SMOOTH_COEFF_MIN 0.9 // 1.3522ms at 44100Hz
#define GAIN_SMOOTHER_SMOOTH_COEFF_MAX 0.99 // 14.1762ms at 44100Hz

// With 0.9, we have too much lag !
// With 0.5, this looks good
// (and this make a better vumeter display)
#define SAMPLES_SMOOTHER_SMOOTH_COEFF 0.5 // 0.205549ms at 44100Hz

// Problem: do not cover the whole range when changing the sc gain knob
//#define CONSTANT_SC_BASE -80

// GOOD: Covers the whole range when changing the sc gain knob
#define CONSTANT_SC_BASE -60

// FIX: for fix for constant level whatever the sample rate
#define REF_SAMPLE_RATE 44100.0

// In theory, we should compute RMS gain with samples,
// not fft
#define FIX_COMPUTE_IN_GAIN 0 //1


AutoGainObj::AutoGainObj(int bufferSize, int oversampling, int freqRes,
                         BL_FLOAT sampleRate,
                         BL_FLOAT minGain, BL_FLOAT maxGain)
: MultichannelProcess()
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mMinGain = minGain;
    mMaxGain = maxGain;
    
    mMode = BYPASS_WRITE;

#if USE_LEGACY_SILENCE_THRESHOLD
    mThreshold = -120.0;
#endif
    
    mPrecision = 50.0;
    mDryWet = 1.0;
    
    mGain = 0.0;
    
    mScGain = 0.0;
    
#if SKIP_FIRST_FRAME
    mNumSamples = 0;
#endif
    
#if FIX_START_JUMP
    mHasJustReset = true;
#endif
    
    //
    mConstantSc = false;
    mScConstantValue = 0.0;
    
#if !TEST_BUFFERS_SIZE
    //
    mAvgHistoIn = new SmoothAvgHistogram(bufferSize/2,
                                         SMOOTH_HISTO_COEFF, HISTO_DEFAULT_VALUE);
    mAvgHistoScIn = new SmoothAvgHistogram(bufferSize/2,
                                           SMOOTH_HISTO_COEFF, HISTO_DEFAULT_VALUE);
    
    
    mAvgHistoOut = new SmoothAvgHistogram(bufferSize/2,
                                          SMOOTH_HISTO_COEFF, HISTO_DEFAULT_VALUE);
#else
    //
    mAvgHistoIn = new SmoothAvgHistogram(bufferSize/2,
                                         SMOOTH_HISTO_COEFF, HISTO_DEFAULT_VALUE);
    mAvgHistoScIn = new SmoothAvgHistogram(bufferSize/2,
                                           SMOOTH_HISTO_COEFF, HISTO_DEFAULT_VALUE);
    
    
    mAvgHistoOut = new SmoothAvgHistogram(bufferSize/2,
                                          SMOOTH_HISTO_COEFF, HISTO_DEFAULT_VALUE);
#endif
    
    // Smoothers
    BL_FLOAT defaultGain = 0.0;
    mGainSmoother =
        new ParamSmoother(defaultGain, GAIN_SMOOTHER_SMOOTH_COEFF_MIN);
    mScSamplesSmoother =
        new ParamSmoother(defaultGain, SAMPLES_SMOOTHER_SMOOTH_COEFF);
    mInSamplesSmoother =
        new ParamSmoother(defaultGain, SAMPLES_SMOOTHER_SMOOTH_COEFF);
}

AutoGainObj::~AutoGainObj()
{
    delete mAvgHistoIn;
    delete mAvgHistoScIn;
    
    delete mAvgHistoOut;
    
    delete mScSamplesSmoother;
    delete mInSamplesSmoother;
    delete mGainSmoother;
}

void
AutoGainObj::Reset()
{
    MultichannelProcess::Reset();
    
    mAvgHistoIn->Reset();
    mAvgHistoScIn->Reset();
    
    mAvgHistoOut->Reset();
    
#if USE_AWEIGHTING
    // Sample rate may have changed
    BL_FLOAT sampleRate = GetSampleRate();
    AWeighting::ComputeAWeights(&mAWeights, mAWeights.GetSize(), sampleRate);
#endif

    
#if 0 // Makes gain jump when changing mode
    mScSamplesSmoother->Reset();
    mInSamplesSmoother->Reset();
    mGainSmoother->Reset();
#endif

#if 1 // Fixed gain jump when changing mode
    BL_FLOAT defaultGain = 0.0;

    mScSamplesSmoother->Reset(defaultGain);
    mScSamplesSmoother->Update();

    mInSamplesSmoother->Reset(defaultGain);
    mInSamplesSmoother->Update();
    
    mGainSmoother->Reset(defaultGain);
    mGainSmoother->Update();
#endif
    
#if 0 // NOTE: with this at 1, when no sc, the sc gain is always 0 until we turn the
      // sc gain knob again 
    // NEW
    mGain = 0.0;
    mScGain = 0.0;
#endif
    
#if SKIP_FIRST_FRAME
    mNumSamples = 0;
#endif
    
#if FIX_START_JUMP
    mHasJustReset = true;
#endif
    
    // Curves data
    mCurveSignal0.Resize(0);
    mCurveSignal1.Resize(0);
    mCurveResult.Resize(0);
}

void
AutoGainObj::Reset(int bufferSize, int overlapping,
                   int oversampling, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, overlapping, oversampling, sampleRate);
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
#if !TEST_BUFFERS_SIZE
    mAvgHistoIn->Reset();
    mAvgHistoScIn->Reset();
    mAvgHistoOut->Reset();
#else
    mAvgHistoIn->Resize(bufferSize/2);
    mAvgHistoScIn->Resize(bufferSize/2);
    mAvgHistoOut->Resize(bufferSize/2);
#endif
    
#if USE_AWEIGHTING
    // Sample rate may have changed
    BL_FLOAT sampleRate = GetSampleRate();
    AWeighting::ComputeAWeights(&mAWeights, mAWeights.GetSize(), sampleRate);
#endif
    
    mScSamplesSmoother->Reset();
    mInSamplesSmoother->Reset();
    
    mGainSmoother->Reset();

#if 0 // NOTE: with this at 1, when no sc, the sc gain is always 0 until we turn the
      // sc gain knob again 
    // NEW
    mGain = 0.0;
    mScGain = 0.0;
#endif
    
#if SKIP_FIRST_FRAME
    mNumSamples = 0;
#endif
    
#if FIX_START_JUMP
    mHasJustReset = true;
#endif
    
    // Curves data
    mCurveSignal0.Resize(0);
    mCurveSignal1.Resize(0);
    mCurveResult.Resize(0);
}

void
AutoGainObj::SetMode(Mode mode)
{
    mMode = mode;
}

#if USE_LEGACY_SILENCE_THRESHOLD
void
AutoGainObj::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
}
#endif

void
AutoGainObj::SetPrecision(BL_FLOAT precision)
{
    mPrecision = precision;
}

void
AutoGainObj::SetDryWet(BL_FLOAT dryWet)
{
    mDryWet = dryWet;
}

BL_FLOAT
AutoGainObj::GetGain()
{
    return mGain;
}

void
AutoGainObj::SetGain(BL_FLOAT gain)
{
    mGain = gain;
}

void
AutoGainObj::SetScGain(BL_FLOAT gain)
{
    mScGain = gain;

    if (mMode == BYPASS_WRITE)
    {
        mGain = mScGain;
    }   
}

void
AutoGainObj::ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                    const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer)
{
    // If no sidechain, create dummy sidechain buffers,
    // with a constant value
    //
    // So we can use the plug on a single track without sidechain
    mConstantSc = false;
    mScConstantValue = 0.0;

#if FIX_COMPUTE_IN_GAIN
    if (!ioSamples->empty())
    {
        WDL_TypedBuf<BL_FLOAT> &monoIn = mTmpBuf31;
        monoIn = *(*ioSamples)[0];

        if (ioSamples->size() >= 2)
        {
            BLUtils::StereoToMono(&monoIn, *(*ioSamples)[0], *(*ioSamples)[1]);
        }
        
        BL_FLOAT inGain = ComputeInGainSamples(monoIn);

        // TODO: put it in a member variable for later use
    }
#endif
    
    // Hack for VST !
    // For VST, if we create sidechain, even not connect,
    // we will receive zeros by the side chain
    
    // FIX: type mistake fixed for FL Studio graphic freeze
    bool scAllZero = ((scBuffer == NULL) || BLUtilsPlug::ChannelAllZero(*scBuffer));
    
    if ((scBuffer == NULL) || scBuffer->empty() || scAllZero)
    {
        mConstantSc = true;
        mScConstantValue = DBToAmp(CONSTANT_SC_BASE);
    }
}

void
AutoGainObj::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples0,
                             const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
#if SKIP_FIRST_FRAME
    // For Protools (sidechain latency not well managed)
    // Skip the first samples
    //
    if (!ioFftSamples->empty())
    {
        mNumSamples += (*ioFftSamples)[0]->GetSize()/OVERSAMPLING;
    }
    if (mNumSamples <= PLUG_LATENCY)
        return;
#endif
    
    // In
    vector<WDL_TypedBuf<BL_FLOAT> > &in = mTmpBuf0;
    in.resize(ioFftSamples0->size());
    
    vector<WDL_TypedBuf<BL_FLOAT> > &inPhases = mTmpBuf1;
    inPhases.resize(ioFftSamples0->size());

    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &ioFftSamples = mTmpBuf28;
    ioFftSamples.resize(ioFftSamples0->size());
    
    for (int i = 0; i < ioFftSamples0->size(); i++)
    {
        // NOTE: not optimal for memory
        //BLUtils::TakeHalf((*ioFftSamples)[i]);
        BLUtils::TakeHalf(*(*ioFftSamples0)[i], &ioFftSamples[i]);
        
        WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf29;
        WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf30;
        BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioFftSamples[i]);
        
        //in.push_back(magns);
        //inPhases.push_back(phases);
        in[i] = magns;
        inPhases[i] = phases;
    }
    
    // ScIn
    vector<WDL_TypedBuf<BL_FLOAT> > &scIn = mTmpBuf2;
    if (scBuffer != NULL)
    {
        vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &scBufferCopy = mTmpBuf3;
        scBufferCopy = *scBuffer;
        
        scIn.resize(scBufferCopy.size());
        
        for (int i = 0; i < scBufferCopy.size(); i++)
        {
            //BLUtils::TakeHalf(&scBufferCopy[i]);
            BLUtils::TakeHalf((*scBuffer)[i], &scBufferCopy[i]);
            
            WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf4;
            WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf5;
            BLUtilsComp::ComplexToMagnPhase(&magns, &phases, scBufferCopy[i]);
            
            //scIn.push_back(magns);
            scIn[i] = magns;
        }
    }
    
    // Out
    vector<WDL_TypedBuf<BL_FLOAT> > &out = mTmpBuf6;
    out = in;
    
    vector<WDL_TypedBuf<BL_FLOAT> > &outPhases = mTmpBuf7;
    outPhases = inPhases;
    
    if (mMode == BYPASS_WRITE)
    {
        // Apply the side chain gain
        if (!mConstantSc)
            ApplyGain(scIn, &scIn, mScGain);
        else
            ApplyGainConstantSc(&mScConstantValue, mScGain);
        
        // Original method, based on simple mean
        //BL_FLOAT outGain = ComputeOutGainRMS(in, scIn, nFrames);
        
        BL_FLOAT outGain;
        
        if (!mConstantSc)
            outGain = ComputeOutGainSpect(in, scIn);
        else
            outGain = ComputeOutGainSpectConstantSc(in, mScConstantValue);
        
        // Setup the computed gain, for the vumeter and write automation
        mGain = outGain;
        
        // Apply the new gain
        ApplyGain(in, &out, mGain);
        
        // Process dry/wet
        ApplyDryWet(in, &out, mDryWet);
    }
    else // mMode == READ
    {
        // Apply the gain
        ApplyGain(in, &out, mGain);
        
        // Process dry/wet
        ApplyDryWet(in, &out, mDryWet);
        
        UpdateGraphReadMode(in, out);
    }
    
    // Result
    for (int i = 0; i < ioFftSamples0->size(); i++)
    {
        BLUtilsComp::MagnPhaseToComplex(&ioFftSamples[i], out[i], outPhases[i]);

        // NOTE: not optimal for memory
        //BLUtils::ResizeFillZeros((*ioFftSamples)[i], (*ioFftSamples)[i]->GetSize()*2);
        //BLUtils::FillSecondFftHalf((*ioFftSamples)[i]);

        //memcpy((*ioFftSamples0)[i]->Get(), ioFftSamples[i].Get(),
        //       ioFftSamples[i].GetSize()*sizeof(WDL_FFT_COMPLEX));
        BLUtils::SetBuf((*ioFftSamples0)[i], ioFftSamples[i]);
        
        BLUtilsFft::FillSecondFftHalf((*ioFftSamples0)[i]);
    }
}

void
AutoGainObj::GetCurveSignal0(WDL_TypedBuf<BL_FLOAT> *signal0)
{
    *signal0 = mCurveSignal0;
}

void
AutoGainObj::GetCurveSignal1(WDL_TypedBuf<BL_FLOAT> *signal1)
{
    *signal1 = mCurveSignal1;
}
void
AutoGainObj::GetCurveResult(WDL_TypedBuf<BL_FLOAT> *result)
{
    *result = mCurveResult;
}

#if 0 // Origin version
void
AutoGainObj::ApplyGain(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                       vector<WDL_TypedBuf<BL_FLOAT> > *outSamples,
                       BL_FLOAT gainDB)
{
    if (inSamples.empty())
        return;
    
    int nFrames = inSamples[0].GetSize();
    
    BL_FLOAT gain = DBToAmp(gainDB);
    
    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if ((j < inSamples.size()) && (j < (*outSamples).size()))
                (*outSamples)[j].Get()[i] = gain*inSamples[j].Get()[i];
        }
    }
}
#endif
#if 1 // optim version
void
AutoGainObj::ApplyGain(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                       vector<WDL_TypedBuf<BL_FLOAT> > *outSamples,
                       BL_FLOAT gainDB)
{
    if (inSamples.empty())
        return;
    
    int nFrames = inSamples[0].GetSize();
    
    BL_FLOAT gain = DBToAmp(gainDB);

    for (int j = 0; j < 2; j++)
    {
        if ((j < inSamples.size()) && (j < (*outSamples).size()))
        {
            BL_FLOAT *inSamplesData = inSamples[j].Get();
            BL_FLOAT *outSamplesData = (*outSamples)[j].Get();
            for (int i = 0; i < nFrames; i++)
            {   
                outSamplesData[i] = gain*inSamplesData[i];
            }
        }
    }
}
#endif

void
AutoGainObj::ApplyGainConstantSc(BL_FLOAT *scConstantValue,
                                 BL_FLOAT gainDB)
{
    BL_FLOAT gain = DBToAmp(gainDB);
    
    *scConstantValue *= gain;
}

#if 0 // origin version
void
AutoGainObj::ApplyDryWet(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                         vector<WDL_TypedBuf<BL_FLOAT> > *outSamples,
                         BL_FLOAT dryWet)
{
    if (inSamples.empty())
        return;
    
    int nFrames = inSamples[0].GetSize();
    
    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if ((j < inSamples.size()) && (j < (*outSamples).size()))
            {
                BL_FLOAT drySample = inSamples[j].Get()[i];
                BL_FLOAT wetSample = (*outSamples)[j].Get()[i];
                
                (*outSamples)[j].Get()[i] = (1.0 - dryWet)*drySample + dryWet*wetSample;
            }
        }
    }
}
#endif
#if 1 // Optim version
void
AutoGainObj::ApplyDryWet(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                         vector<WDL_TypedBuf<BL_FLOAT> > *outSamples,
                         BL_FLOAT dryWet)
{
    if (inSamples.empty())
        return;
    
    int nFrames = inSamples[0].GetSize();

    for (int j = 0; j < 2; j++)
    {
        if ((j < inSamples.size()) && (j < (*outSamples).size()))
        {
            BL_FLOAT *inSamplesData = inSamples[j].Get();
            BL_FLOAT *outSamplesData = (*outSamples)[j].Get();
            for (int i = 0; i < nFrames; i++)
            {
        
                BL_FLOAT drySample = inSamplesData[i];
                BL_FLOAT wetSample = outSamplesData[i];
                
                outSamplesData[i] = (1.0 - dryWet)*drySample + dryWet*wetSample;
            }
        }
    }
}
#endif

// Stereo to mono with Fft: valid ?
BL_FLOAT
AutoGainObj::ComputeOutGainSpect(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &scIn)
{
    if (scIn.empty())
        return 0.0;

    // We have at least one sidechain channel
    
    // Stereo to mono
    WDL_TypedBuf<BL_FLOAT> &sideChain = mTmpBuf8;
    if (scIn.size() > 0)
    {
        sideChain = scIn[0];
        if (scIn.size() > 1)
            BLUtils::StereoToMono(&sideChain, scIn[0], scIn[1]);
    }
    else
        sideChain.Resize(0);
    
    WDL_TypedBuf<BL_FLOAT> &monoIn = mTmpBuf9;
    if (inSamples.size() > 0)
    {
        monoIn = inSamples[0];
        
        if (inSamples.size() > 1)
            BLUtils::StereoToMono(&monoIn, inSamples[0], inSamples[1]);
    }
    else
        monoIn.Resize(0);

    //Convert to dB
    WDL_TypedBuf<BL_FLOAT> &dbSc = mTmpBuf10;
    BLUtils::AmpToDB(&dbSc, sideChain, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    
    WDL_TypedBuf<BL_FLOAT> &dbIn = mTmpBuf11;
    BLUtils::AmpToDB(&dbIn, monoIn, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);

    // See: FIX_COMPUTE_IN_GAIN
    BL_FLOAT inGain = ComputeInGainFft(monoIn);
    
    BL_FLOAT result = ComputeOutGainSpectAux(dbIn, dbSc, inGain);
    
    return result;
}

BL_FLOAT
AutoGainObj::
ComputeOutGainSpectConstantSc(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                              BL_FLOAT constantScValue)
{
    WDL_TypedBuf<BL_FLOAT> &monoIn = mTmpBuf12;
    if (inSamples.size() > 0)
    {
        monoIn = inSamples[0];
        
        if (inSamples.size() > 1)
            BLUtils::StereoToMono(&monoIn, inSamples[0], inSamples[1]);
    }
    else
        monoIn.Resize(0);

#if 0 // Origin version
    // Dummy Sc magn array
    WDL_TypedBuf<BL_FLOAT> &scMagns = mTmpBuf13;
    BLUtils::ResizeFillValue(&scMagns, monoIn.GetSize(), constantScValue);
    
    //Convert to dB
    WDL_TypedBuf<BL_FLOAT> &dbSc = mTmpBuf14;
    BLUtils::AmpToDB(&dbSc, scMagns, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
#endif
#if 1 // Optim version
    BL_FLOAT constantScValueDB = BLUtils::AmpToDB(constantScValue,
                                                  (BL_FLOAT)BL_EPS,
                                                  (BL_FLOAT)DB_INF);
    // Dummy Db Sc magn array
    WDL_TypedBuf<BL_FLOAT> &dbSc = mTmpBuf14;
    BLUtils::ResizeFillValue(&dbSc, monoIn.GetSize(), constantScValueDB);
#endif
    
    // FIX: fix flat end of result curve, when using sample rate > 44100
    WDL_TypedBuf<BL_FLOAT> &dbIn = mTmpBuf15;
    BLUtils::AmpToDB(&dbIn, monoIn, (BL_FLOAT)BL_EPS, /*DB_INF*/(BL_FLOAT)DB_INF2);
    
    //BL_FLOAT inGain = ComputeInGain(monoIn);
    BL_FLOAT inGain = ComputeInGainFft(monoIn);
    
    BL_FLOAT result = ComputeOutGainSpectAux(dbIn, dbSc, inGain);
    
    // Hack
    // Overwrite the sc curve, if we have constant sc
    // Use a fake db curve, centered around -60dB
    BL_FLOAT constantFcValueDB = BLUtils::AmpToDB(constantScValue);
    mCurveSignal1.Resize(monoIn.GetSize());
    BLUtils::FillAllValue(&mCurveSignal1, constantFcValueDB);
    
    return result;
}

BL_FLOAT
AutoGainObj::ComputeOutGainSpectAux(const WDL_TypedBuf<BL_FLOAT> &dbIn,
                                    const WDL_TypedBuf<BL_FLOAT> &dbSc,
                                    BL_FLOAT inGain)
{
#if USE_AWEIGHTING
    // Lazy evaluation
    // (bacause we don't know nFrames at the beginning)
    if (mAWeights.GetSize() != nFrames/2)
    {
        BL_FLOAT sampleRate = GetSampleRate();
        // We take half of the size of the fft
        AWeighting::ComputeAWeights(&mAWeights, inMagns.GetSize(), sampleRate);
    }
    
    // Apply A-weighting
    Utils::AddValues(&dbSc, mAWeights);
    Utils::AddValues(&dbIn, mAWeights);
#endif
    
#if FIX_START_JUMP
    if (mHasJustReset)
        mAvgHistoScIn->Reset(dbSc);
#endif
    
    mAvgHistoScIn->AddValues(dbSc);
    WDL_TypedBuf<BL_FLOAT> &avgSc = mTmpBuf16;
    mAvgHistoScIn->GetValues(&avgSc);
    
#if FIX_START_JUMP
    if (mHasJustReset)
        mAvgHistoIn->Reset(dbIn);
    
    mHasJustReset = false;
#endif
    
    mAvgHistoIn->AddValues(dbIn);
    WDL_TypedBuf<BL_FLOAT> &avgIn = mTmpBuf17;
    mAvgHistoIn->GetValues(&avgIn);

    BL_FLOAT outGain = 0.0;
    if (!mConstantSc)
    {
#if !SILENCE_THRS_ALGO2
        outGain = ComputeFftGain(avgIn, avgSc);
    
        // Avoid amplifying silences (to avoid amplifying noise)
        if (inGain < mThreshold)
        {
            outGain = 0.0;
        }
#else
#if USE_LEGACY_SILENCE_THRESHOLD
        outGain = ComputeFftGain2(avgIn, avgSc);
#else
        outGain = ComputeFftGain3(avgIn, avgSc);
#endif
#endif
    }
    else
    {
        // NEW: put directly the gain if sc is not set
        outGain = mScGain;
    }
    
    // Bound and smooth gain before applyting it to the curve
    // So the curve will be smoothed too
    
    // Bound...
    if (outGain < mMinGain)
        outGain = mMinGain;
    if (outGain > mMaxGain)
        outGain = mMaxGain;
    
    // Smooth the output gain (may add a lag)
    mGainSmoother->SetNewValue(outGain);
    mGainSmoother->Update();
    outGain = mGainSmoother->GetCurrentValue();
    
    WDL_TypedBuf<BL_FLOAT> &newAvgIn = mTmpBuf18;
    newAvgIn = avgIn;
    BLUtils::AddValues(&newAvgIn, outGain);
    
    // Bound the side chain curve
    // So if we modifiy the side chain input gain
    // we won't search the curve if it is displayed out of the graph
    BoundScCurve(&avgSc, DB_INF, 0.0, mScGain);
    
    // Curves data
    mCurveSignal0 = avgIn;
    mCurveSignal1 = avgSc;
    mCurveResult = newAvgIn;
    
    return outGain;
}

// With samples, compute the RMS
BL_FLOAT
AutoGainObj::ComputeInGainSamples(const WDL_TypedBuf<BL_FLOAT> &monoIn)
{
    BL_FLOAT inAvg = BLUtils::ComputeRMSAvg/*2*/(monoIn.Get(), monoIn.GetSize());
    BL_FLOAT inGain = BLUtils::AmpToDB(inAvg, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    
    mInSamplesSmoother->SetNewValue(inGain);
    mInSamplesSmoother->Update();
    inGain = mInSamplesSmoother->GetCurrentValue();
    
    return inGain;
}

// With FFT, compute the max peak (this should be an acceptable approximation)
// (we can't compute the RMS over frequencies, imagine a single pure sine wave,
// if we compute the RMS, the computed gain will be very small)
BL_FLOAT
AutoGainObj::ComputeInGainFft(const WDL_TypedBuf<BL_FLOAT> &monoIn)
{
    BL_FLOAT inMax = BLUtils::ComputeMax(monoIn.Get(), monoIn.GetSize());
    BL_FLOAT inGain = BLUtils::AmpToDB(inMax, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    
    mInSamplesSmoother->SetNewValue(inGain);
    mInSamplesSmoother->Update();
    inGain = mInSamplesSmoother->GetCurrentValue();
    
    return inGain;
}

#if 0
// Note used anymore!
BL_FLOAT
AutoGainObj::ComputeOutGainRMS(const vector<WDL_TypedBuf<BL_FLOAT> > &inSamples,
                               const vector<WDL_TypedBuf<BL_FLOAT> > &scIn)
{
    if (scIn.empty())
        return 0.0;
    
    // Compute the sidechain gain
    
    // We have at least one sidechain channel
    WDL_TypedBuf<BL_FLOAT> &sideChain = mTmpBuf19;
    if (scIn.size() > 0)
    {
        sideChain = scIn[0];
        
        if (scIn.size() > 1)
        {
            BLUtils::StereoToMono(&sideChain, scIn[0], scIn[1]);
        }
    }
    else
        sideChain.Resize(0);
    
    BL_FLOAT sideChainAvg = BLUtils::ComputeRMSAvg2(sideChain.Get(), sideChain.GetSize());
    BL_FLOAT scGain = BLUtils::AmpToDB(sideChainAvg, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    
    mScSamplesSmoother->SetNewValue(scGain);
    mScSamplesSmoother->Update();
    scGain = mScSamplesSmoother->GetCurrentValue();
    
    WDL_TypedBuf<BL_FLOAT> &monoIn = mTmpBuf20;
    if (inSamples.size() > 0)
    {
        monoIn = inSamples[0];
        
        if (inSamples.size() > 1)
            BLUtils::StereoToMono(&monoIn, inSamples[0], inSamples[1]);
    }
    else
        monoIn.Resize(0);
    
    BL_FLOAT inAvg = BLUtils::ComputeRMSAvg2(monoIn.Get(), monoIn.GetSize());
    BL_FLOAT inGain = BLUtils::AmpToDB(inAvg, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    
    mInSamplesSmoother->SetNewValue(inGain);
    mInSamplesSmoother->Update();
    inGain = mInSamplesSmoother->GetCurrentValue();
    
    BL_FLOAT outGain = 0.0;
    
    // Avoid amplifying silences (to avoid amplifyingnoise)
    if (inGain > mThreshold)
    {
        if (fabs(scGain) > BL_EPS)
        {
            // Here, the ratio is reversed, this is because
            // we converted in dB just above
            //
            // All this is not really correct, but it works !
            //
            outGain = inGain/scGain;
        }
        
        outGain = BLUtils::AmpToDB(outGain, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    }
    
    // Not tested ?
    
    // Bound...
    if (outGain < mMinGain)
        outGain = mMinGain;
    if (outGain > mMaxGain)
        outGain = mMaxGain;
    
    // Smooth the output gain (may add a lag)
    mGainSmoother->SetNewValue(outGain);
    mGainSmoother->Update();
    outGain = mGainSmoother->GetCurrentValue();
    
    return outGain;
}
#endif

BL_FLOAT
AutoGainObj::ComputeFftGain(const WDL_TypedBuf<BL_FLOAT> &avgIn,
                            const WDL_TypedBuf<BL_FLOAT> &avgSc)
{
    WDL_TypedBuf<BL_FLOAT> &diff = mTmpBuf21;
    diff.Resize(avgIn.GetSize());
    
    BLUtils::ComputeDiff(&diff, avgIn, avgSc);
    
#if 1 // FIX (1/2): with constant sc, gain increases while samplerate increases
    BL_FLOAT coeff = REF_SAMPLE_RATE/mSampleRate;
    int numValuesAvg = coeff*mBufferSize/2.0;
    
    // Crop the values from the 22050 bin to the end
    // (in general, this is a series of almost zeros
    diff.Resize(numValuesAvg);
#endif
    
    BL_FLOAT avgDiff = BLUtils::ComputeAvg(diff);
    
#if 1 // FIX (2/2): HACK: compensate amplitude, depending on the sampling rate
    avgDiff *= coeff;
#endif
    
    return avgDiff;
}

#if USE_LEGACY_SILENCE_THRESHOLD
// Compute by difference in dB
BL_FLOAT
AutoGainObj::ComputeFftGain2(const WDL_TypedBuf<BL_FLOAT> &avgIn,
                             const WDL_TypedBuf<BL_FLOAT> &avgSc)
{
    // Add the diff only if both input and sidechain ore below
    // the silence threshold
    BL_FLOAT avgDiff = 0.0;
    int avgNumValues = 0;
    for (int i = 0; i < avgIn.GetSize(); i++)
    {
        BL_FLOAT inVal = avgIn.Get()[i];
        BL_FLOAT scVal = avgSc.Get()[i];
        
        if (inVal < mThreshold)
            continue;
        
        if (scVal < mThreshold)
            continue;
        
        //BL_FLOAT diff = inVal - scVal;
        BL_FLOAT diff = scVal - inVal;
        avgDiff += diff;
        avgNumValues++;
    }
    if (avgNumValues > 0)
        avgDiff /= avgNumValues;
    
#if 1 // FIX: with constant sc, gain increases while samplerate increases
    BL_FLOAT coeff = REF_SAMPLE_RATE/mSampleRate;
    
    avgDiff *= coeff;
#endif
    
    return avgDiff;
}
#endif

// Ponderate diff by dB position
// NOTE: seems to work very well, and no need to use a silence threshold parameter!
BL_FLOAT
AutoGainObj::ComputeFftGain3(const WDL_TypedBuf<BL_FLOAT> &avgIn,
                             const WDL_TypedBuf<BL_FLOAT> &avgSc)
{
    // Add the diff only if both input and sidechain ore below
    // the silence threshold
    BL_FLOAT avgDiff = 0.0;
    BL_FLOAT avgSumWeights = 0;
    
    for (int i = 0; i < avgIn.GetSize(); i++)
    {
        BL_FLOAT inVal = avgIn.Get()[i];
        BL_FLOAT scVal = avgSc.Get()[i];

        BL_FLOAT w = BLUtils::DBToAmp((inVal + scVal)*0.5);
        
        BL_FLOAT diff = scVal - inVal;
        avgDiff += w*diff;
        
        avgSumWeights += w;
    }
    
    if (avgSumWeights > 0.0)
    {
        avgDiff /= avgSumWeights;
    }

#if 1 // FIX: with constant sc, gain increases while samplerate increases
    BL_FLOAT coeff = REF_SAMPLE_RATE/mSampleRate;
    
    avgDiff *= coeff;
#endif
    
    return avgDiff;
}

void
AutoGainObj::SetGainSmooth(BL_FLOAT gainSmooth)
{
#define SHAPE_EXP 0.125
    
    // This was in percent
    //BL_FLOAT coeff = gainSmooth/100.0;
    BL_FLOAT coeff = gainSmooth;
    
    // Set shape
    coeff = pow(coeff, SHAPE_EXP);
    
    BL_FLOAT smooth = (1.0 - coeff)*GAIN_SMOOTHER_SMOOTH_COEFF_MIN +
    coeff*GAIN_SMOOTHER_SMOOTH_COEFF_MAX;
    
    mGainSmoother->SetSmoothCoeff(smooth);
}

void
AutoGainObj::BoundScCurve(WDL_TypedBuf<BL_FLOAT> *curve,
                          BL_FLOAT mindB, BL_FLOAT maxdB,
                          BL_FLOAT scGain)
{
    for (int i = 0; i < curve->GetSize(); i++)
    {
        BL_FLOAT val = curve->Get()[i];
        
        if (fabs(scGain - 1.0) > BL_EPS)
            // We have modified the gain
        {
            if (val < mindB)
                val = mindB;
            if (val > maxdB)
                val = maxdB;
        }
        
        curve->Get()[i] = val;
    }
}

void
AutoGainObj::UpdateGraphReadMode(const vector<WDL_TypedBuf<BL_FLOAT> > &in,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &out)
{
    // In
    WDL_TypedBuf<BL_FLOAT> &monoIn = mTmpBuf22;
    if (in.size() > 0)
    {
        monoIn = in[0];
        
        if (in.size() > 1)
            BLUtils::StereoToMono(&monoIn, in[0], in[1]);
    }
    else
        monoIn.Resize(0);
    
    WDL_TypedBuf<BL_FLOAT> &dbIn = mTmpBuf23;
    BLUtils::AmpToDB(&dbIn, monoIn, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    
#if FIX_START_JUMP
    if (mHasJustReset)
        mAvgHistoIn->Reset(dbIn);
#endif
    
    mAvgHistoIn->AddValues(dbIn);
    WDL_TypedBuf<BL_FLOAT> &avgIn = mTmpBuf24;
    mAvgHistoIn->GetValues(&avgIn);
    
    // Out
    WDL_TypedBuf<BL_FLOAT> &monoOut = mTmpBuf25;
    if (out.size() > 0)
    {
        monoOut = out[0];
        
        if (out.size() > 1)
            BLUtils::StereoToMono(&monoOut, out[0], out[1]);
    }
    else
        monoOut.Resize(0);
    
    WDL_TypedBuf<BL_FLOAT> &dbOut = mTmpBuf26;
    BLUtils::AmpToDB(&dbOut, monoOut, (BL_FLOAT)BL_EPS, (BL_FLOAT)DB_INF);
    
#if FIX_START_JUMP
    if (mHasJustReset)
        mAvgHistoOut->Reset(dbOut);
    
    mHasJustReset = false;
#endif
    
    mAvgHistoOut->AddValues(dbOut);
    WDL_TypedBuf<BL_FLOAT> &avgOut = mTmpBuf27;
    mAvgHistoOut->GetValues(&avgOut);
    
    // Curves data
    mCurveSignal0 = avgIn;
    mCurveSignal1.Resize(0);
    mCurveResult = avgOut;
}
