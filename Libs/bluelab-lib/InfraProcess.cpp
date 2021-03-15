//
//  InfraProcess.cpp
//  BL-Infra
//
//  Created by Pan on 20/04/18.
//
//

#include <algorithm>
using namespace std;

#include <IPlugUtilities.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include <DebugGraph.h>

#include <PartialTracker3.h>
#include <SineSynth.h>

#include <FilterIIRLow12dB.h>

#include "InfraProcess.h"

#define EPS 1e-15

#define FIX_RESET 1

// Phantom
#define PHANTOM_NUM_PARTIALS 10
#define DECREASE_DB 24.0 //12.0
#define PHANTOM_PARTIAL_ID_OFFSET 10000

// PROBLEM: we can't compute the initial freq very exactly
// with partial tracker (error 0.2%)
// => so when we add it to the original sound, there is
// and oscillation in amp (due to beating frequency)
// oscillation: around 0.5-1Hz
#define ADD_INITIAL_FREQ_PARTIAL 0

// PROBLEM
// Makes a small EQ effect (seems to increase some medium freqs)
#define INCREASE_INITIAL_FREQ 0

// Increase all original signal
// PROBLEM: also increase the high frequencies
#define INCREASE_ALL_FREQS 0

// GOOD
// Low pass filter then increase
#define INCREASE_LOW_FREQS 1
// See: https://www.teachmeaudio.com/mixing/techniques/audio-spectrum/
#define LOW_FREQ_CUTOFF 250.0

#define INITIAL_INCREASE_DB 6.0 //24.0 //6.0 //12.0 //6.0 //24.0 //6.0

// Sub
#define SUB_PARTIAL_ID_OFFSET 20000

#define KEEP_ONLY_FIRST_DETECTED_PARTIAL 1

// Do not process phantom fundamental if the first partial frequency is too high
#define MIN_PHANTOM_FREQ_PARTIAL 250.0

// Do not create sub freq if partial freq is greater than this value
#define MIN_SUB_FREQ_PARTIAL 250.0

// With that, we really hear the phantom harmonic effect !
#define PHANTOM_BOOST 1

// Filter high frequencies that appear when generating the sub octave frequency
#define FIX_HIGH_FREQS_SUB 1
#define SUB_CUTOFF 2000.0

// OPTIM PROF Infra
#define MAX_DETECT_FREQ 500.0

// OPTIM PROF Infra
// Do not compute the fft phases to avoid atan2
// (in the current implementation, we don't need fft phases
// => be careful if we change flags here or in PartialTracker3 )
#define SKIP_FFT_PHASES 1


InfraProcess::InfraProcess(int bufferSize,
                           BL_FLOAT overlapping, BL_FLOAT oversampling,
                           BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mPartialTracker = new PartialTracker3(bufferSize, sampleRate, overlapping);
    
    mPartialTracker->SetMaxDetectFreq(MAX_DETECT_FREQ);
    
    mPhantomSynth = new SineSynth(bufferSize, sampleRate, overlapping);
    mSubSynth = new SineSynth(bufferSize, sampleRate, overlapping);
    
    //
    mPhantomFreq = 20.0;
    mPhantomMix = 0.5;
    mSubOrder = 1;
    mSubMix = 0.0;
    
    // For ramps
    mPrevPhantomMix = mPhantomMix;
    mPrevSubMix = mSubMix;
    
    //
    mLowFilter = NULL;
    mSubLowFilter = NULL;
    
#if INCREASE_LOW_FREQS
    mLowFilter = new FilterIIRLow12dB();
    mLowFilter->Init(LOW_FREQ_CUTOFF, sampleRate);
#endif
    
#if FIX_HIGH_FREQS_SUB
    mSubLowFilter = new FilterIIRLow12dB();
    mSubLowFilter->Init(SUB_CUTOFF, sampleRate);
#endif
    
#if INFRA_PROCESS_PROFILE
    BlaTimer::Reset(&mTimer, &mCount);
#endif
}

InfraProcess::~InfraProcess()
{
    delete mPartialTracker;

    delete mPhantomSynth;
    delete mSubSynth;
    
#if INCREASE_LOW_FREQS
    delete mLowFilter;
#endif
    
#if FIX_HIGH_FREQS_SUB
    delete mSubLowFilter;
#endif
}

void
InfraProcess::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
InfraProcess::Reset(int bufferSize, int overlapping, int oversampling,
                   BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if FIX_RESET
    mPartialTracker->Reset();
#endif
    
    mPhantomSynth->Reset(bufferSize, sampleRate, overlapping);
    mSubSynth->Reset(bufferSize, sampleRate, overlapping);
    
#if INCREASE_LOW_FREQS
    mLowFilter->Init(LOW_FREQ_CUTOFF, sampleRate);
#endif
    
#if FIX_HIGH_FREQS_SUB
    mSubLowFilter->Init(SUB_CUTOFF, sampleRate);
#endif
}

void
InfraProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
#if INFRA_PROCESS_PROFILE
    BlaTimer::Start(&mTimer);
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
#if !SKIP_FFT_PHASES
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
#else
    BLUtilsComp::ComplexToMagn(&magns, fftSamples);
    phases.Resize(magns.GetSize());
    BLUtils::FillAllZero(&phases);
#endif
    
    DetectPartials(magns, phases);
    
#if INCREASE_INITIAL_FREQ
    // Get partials
    vector<PartialTracker3::Partial> partials;
    mPartialTracker->GetPartials(&partials);
    sort(partials.begin(), partials.end(), PartialTracker3::Partial::FreqLess);

    // Increase initial freq
    WDL_TypedBuf<BL_FLOAT> increasedInitial;
    IncreaseInitialFreq(&increasedInitial, magns, partials);
    
    // Mix
    BLUtils::MultValues(&increasedInitial, mPhantomMix);
    
    // Add result
    BLUtils::AddValues(&magns, increasedInitial);
#endif
    
#if INCREASE_ALL_FREQS
    IncreaseAllFreqs(&magns, mPhantomMix);
#endif
    
#if (INCREASE_INITIAL_FREQ || INCREASE_ALL_FREQS)
    // Re-synth
    BLUtils::MagnPhaseToComplex(ioBuffer, magns, phases);
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
#endif
    
#if INFRA_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer, &mCount, "InfraProcess-profile.txt", "%ld");
#endif
}

void
InfraProcess::ProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                      const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    vector<PartialTracker3::Partial> partials;
    mPartialTracker->GetPartials(&partials);
    
#if KEEP_ONLY_FIRST_DETECTED_PARTIAL
    sort(partials.begin(), partials.end(), PartialTracker3::Partial::FreqLess);
#endif
 
    // Phantom
    WDL_TypedBuf<BL_FLOAT> phantomSynthBuffer;
    BLUtils::ResizeFillZeros(&phantomSynthBuffer, ioBuffer->GetSize());
    
    vector<PartialTracker3::Partial> phantomPartials;
    GeneratePhantomPartials(partials, &phantomPartials);
    mPhantomSynth->SetPartials(phantomPartials);
    mPhantomSynth->ComputeSamples(&phantomSynthBuffer);

    // Apply shape to have a good progression in dB
    BL_FLOAT phantomMix = BLUtils::ApplyParamShape(mPhantomMix, (BL_FLOAT)0.5);
    BL_FLOAT prevPhantomMix = BLUtils::ApplyParamShape(mPrevPhantomMix, (BL_FLOAT)0.5);
    //BLUtils::MultValues(&phantomSynthBuffer, phantomMix);
    BLUtils::MultValuesRamp(&phantomSynthBuffer, prevPhantomMix, phantomMix);
    
    // Sub
    WDL_TypedBuf<BL_FLOAT> subSynthBuffer;
    BLUtils::ResizeFillZeros(&subSynthBuffer, ioBuffer->GetSize());
    
    vector<PartialTracker3::Partial> subPartials;
    GenerateSubPartials(partials, &subPartials);
    mSubSynth->SetPartials(subPartials);
    mSubSynth->ComputeSamples(&subSynthBuffer);
    
#if FIX_HIGH_FREQS_SUB
    // Only the first 1/4 of the buffer is filled
    // So reduce the buffer, to avoid passing the low pass filter
    // on the zeros too (would have made artifacts)
    subSynthBuffer.Resize(subSynthBuffer.GetSize()/mOverlapping);
    
    WDL_TypedBuf<BL_FLOAT> subLowBuffer;
    mSubLowFilter->Process(&subLowBuffer, subSynthBuffer);
    subSynthBuffer = subLowBuffer;
    
    // Resize back
    BLUtils::ResizeFillZeros(&subSynthBuffer, subSynthBuffer.GetSize()*mOverlapping);
#endif
    
    // Apply shape to have a good progression in dB
    BL_FLOAT subMix = BLUtils::ApplyParamShape(mSubMix, (BL_FLOAT)0.5);
    BL_FLOAT prevSubMix = BLUtils::ApplyParamShape(mPrevSubMix, (BL_FLOAT)0.5);
    //BLUtils::MultValues(&subSynthBuffer, subMix);
    BLUtils::MultValuesRamp(&subSynthBuffer, prevSubMix, subMix);
    
    // Phantom initial freqs
#if INCREASE_LOW_FREQS
    WDL_TypedBuf<BL_FLOAT> lowBuffer;
    mLowFilter->Process(&lowBuffer, *ioBuffer);
    
    BL_FLOAT dbCoeff = mPhantomMix*BLUtils::DBToAmp(INITIAL_INCREASE_DB);
    BL_FLOAT prevDbCoeff = mPrevPhantomMix*BLUtils::DBToAmp(INITIAL_INCREASE_DB);
    //BLUtils::MultValues(&lowBuffer, dbCoeff);
    BLUtils::MultValuesRamp(&lowBuffer, prevDbCoeff, dbCoeff);
    
    BLUtils::AddValues(&phantomSynthBuffer, lowBuffer);
#endif
    
    // Result
    BLUtils::AddValues(ioBuffer, phantomSynthBuffer);
    BLUtils::AddValues(ioBuffer, subSynthBuffer);
    
    // Update the prev values for ramps
    mPrevPhantomMix = mPhantomMix;
    mPrevSubMix = mSubMix;
}

void
InfraProcess::SetThreshold(BL_FLOAT threshold)
{
    mPartialTracker->SetThreshold(threshold);
}

void
InfraProcess::SetPhantomFreq(BL_FLOAT phantomFreq)
{
    mPhantomFreq = phantomFreq;
}

void
InfraProcess::SetPhantomMix(BL_FLOAT phantomMix)
{
    mPhantomMix = phantomMix;
}

void
InfraProcess::SetSubOrder(int subOrder)
{
    mSubOrder = subOrder;
}

void
InfraProcess::SetSubMix(BL_FLOAT subMix)
{
    mSubMix = subMix;
}

void
InfraProcess::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                             const WDL_TypedBuf<BL_FLOAT> &phases)
{    
    mPartialTracker->SetData(magns, phases);
    
    mPartialTracker->DetectPartials();
    
    // Filter or not ?
    //
    // If we filter and we get zombie partials
    // this would make musical noise
    mPartialTracker->FilterPartials();
}

// Generate higher partials
void
InfraProcess::GeneratePhantomPartials(const vector<PartialTracker3::Partial> &partials,
                                      vector<PartialTracker3::Partial> *newPartials)
{
    newPartials->clear();
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial p = partials[i];
     
        if (p.mFreq > MIN_PHANTOM_FREQ_PARTIAL)
            break;
        
        // Compute phantom freq
        BL_FLOAT phantomFreq = p.mFreq;
        while(phantomFreq > mPhantomFreq)
        {
            phantomFreq *= 0.5;
        }
        phantomFreq *= 2.0;
        
#if ADD_INITIAL_FREQ_PARTIAL
        PartialTracker3::Partial initP = p;
        initP.mAmpDB = p.mAmpDB + INITIAL_INCREASE_DB;
        initP.mFreq = p.mFreq;
        initP.mId = i + PHANTOM_PARTIAL_ID_OFFSET;
        
        // Just in case
        initP.mPhase = p.mPhase;
        
        newPartials->push_back(initP);
#endif
        
        int partialNum = 0;
#if !PHANTOM_BOOST
        BL_FLOAT ampDB = p.mAmpDB - DECREASE_DB;
#else
        // If phantom boost, do not decrease the first partial
        // We can really hear the phantom fundamental effect here !
        BL_FLOAT ampDB = p.mAmpDB;
#endif
        BL_FLOAT freq = p.mFreq + phantomFreq;
        
        while(partialNum < PHANTOM_NUM_PARTIALS)
        {
            PartialTracker3::Partial p0 = p;
            p0.mFreq = freq;
            // Prefer choosing "i" to avoid problems when ids are blinking
            p0.mId = i + (partialNum + 2)*PHANTOM_PARTIAL_ID_OFFSET;
            p0.mAmpDB = ampDB;
            
            // Just in case
            p0.mPhase = p.mPhase;
            
            newPartials->push_back(p0);
            
            // DB per octave
            BL_FLOAT numOctave = std::log((freq + phantomFreq)/freq)/std::log(2.0);
            
            ampDB -= numOctave*DECREASE_DB;
            freq += phantomFreq;
            partialNum++;
        }
        
#if KEEP_ONLY_FIRST_DETECTED_PARTIAL
        break;
#endif
        
    }
}

// Generate higher partials
void
InfraProcess::GenerateSubPartials(const vector<PartialTracker3::Partial> &partials,
                                  vector<PartialTracker3::Partial> *newPartials)
{
    newPartials->clear();
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial p = partials[i];
        
        BL_FLOAT subFreq = p.mFreq;
        
        if (subFreq > MIN_SUB_FREQ_PARTIAL)
            break;
        
        for (int j = 0; j < mSubOrder; j++)
            subFreq *= 0.5;
        
        BL_FLOAT ampDB = p.mAmpDB + mSubOrder*DECREASE_DB;
        if (ampDB > 0.0)
            ampDB = 0.0;
        
        PartialTracker3::Partial p0 = p;
        p0.mFreq = subFreq;
        // Prefer choosing "i" to avoid problems when ids are blinking
        p0.mId = i + SUB_PARTIAL_ID_OFFSET;
        p0.mAmpDB = ampDB;
        
        // Just in case
        p0.mPhase = p.mPhase;
        
        newPartials->push_back(p0);
        
#if KEEP_ONLY_FIRST_DETECTED_PARTIAL
        break;
#endif
    }
}

void
InfraProcess::IncreaseInitialFreq(WDL_TypedBuf<BL_FLOAT> *result,
                                  const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const vector<PartialTracker3::Partial> &partials)
{
    result->Resize(magns.GetSize());
    BLUtils::FillAllZero(result);
    
    if (partials.empty())
        return;
    
    BL_FLOAT dbCoeff = BLUtils::DBToAmp(INITIAL_INCREASE_DB);
    
    // Partials must be sorted by frequency
    const PartialTracker3::Partial &p = partials[0];
    
    for (int i = p.mLeftIndex; i <= p.mRightIndex; i++)
    {
        BL_FLOAT coeff = 1.0;
        if (i < p.mPeakIndex)
        {
            if (p.mPeakIndex - p.mLeftIndex > 0)
                coeff = ((BL_FLOAT)(i - p.mLeftIndex))/(p.mPeakIndex - p.mLeftIndex);
            else
                coeff = 0.0;
        }
        
        if (i > p.mPeakIndex)
        {
            if (p.mRightIndex - p.mPeakIndex > 0)
                coeff = ((BL_FLOAT)(p.mRightIndex - i))/(p.mRightIndex - p.mPeakIndex);
            else
                coeff = 0.0;
        }
        
        result->Get()[i] = magns.Get()[i]*coeff*dbCoeff;
    }
}

void
InfraProcess::IncreaseAllFreqs(WDL_TypedBuf<BL_FLOAT> *ioBuffer, BL_FLOAT mix)
{
    BL_FLOAT dbCoeff = BLUtils::DBToAmp(INITIAL_INCREASE_DB*mix);
    // NOTE: should be:
    //
    // BL_FLOAT dbCoeff = mix*DBToAmp(INITIAL_INCREASE_DB);
    //
    
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT val = ioBuffer->Get()[i];
        
        val *= dbCoeff;
        
        ioBuffer->Get()[i] = val;
    }
}
