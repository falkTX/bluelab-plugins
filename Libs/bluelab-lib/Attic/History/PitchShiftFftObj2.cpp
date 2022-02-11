//
//  PitchShiftFftObj2.cpp
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#include "BLUtils.h"

#include "PitchShiftFftObj2.h"

// To avoid transforming the first and the center values
// When transforming with tempered, the first and center values changes !
#define KEEP_BOUND_VALUES 0

// To ignore the new computed phases
#define IGNORE_NEW_PHASES 0

// BEST (?): USE_SMB_METHOD=1, FILL_MISSING_FREQS=0
// VERY GOOD (a bit different): USE_SMB_METHOD=0, FILL_MISSING_FREQS=1
#define USE_SMB_METHOD 1
#define FILL_MISSING_FREQS 0

#define KEEP_SYNTHESIS_ENERGY 0
#define VARIABLE_HANNING 0


PitchShiftFftObj2::PitchShiftFftObj2(int bufferSize, int oversampling, int freqRes,
                                     BL_FLOAT sampleRate)
: FftObj(bufferSize, oversampling, freqRes,
         AnalysisMethodWindow, SynthesisMethodWindow,
         KEEP_SYNTHESIS_ENERGY,
         VARIABLE_HANNING,
         false),
    mFactor(1.0),
    mSampleRate(sampleRate),
    mFreqObj(bufferSize, oversampling, freqRes, sampleRate)
{
    // "uninitialized"
    mFactors[0] = -1.0;
    mFactors[1] = -1.0;
}

PitchShiftFftObj2::~PitchShiftFftObj2() {}

void
PitchShiftFftObj2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
#if 1
    // Hack to smooth factor changes
    BL_FLOAT t = 1.0;
    if (mOverlapping > 1)
        t = ((BL_FLOAT)mBufferNum)/(mOverlapping - 1);
    
    // Test just in case, to avoid having the pitch flying to infinity
    if (t > 1.0)
        t = 1.0;
    
    mFactor = (1.0 - t)*mFactors[0] + t*mFactors[1];
#endif
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
#if KEEP_BOUND_VALUES
    // Problem with first magn and first phase
    BL_FLOAT magn0 = magns.Get()[0];
    BL_FLOAT phase0 = phases.Get()[0];
    
    // Must save the second value too, (problem with second phase)
    BL_FLOAT magn1 = magns.Get()[1];
    BL_FLOAT phase1 = phases.Get()[1];
    
    // Must save middle value too, (problem with middle phase)
    BL_FLOAT magnEnd = magns.Get()[magns.GetSize() - 1];
    BL_FLOAT phaseEnd = phases.Get()[magns.GetSize() - 1];
#endif
    
    //
    // Convert
    //
    Convert(&magns, &phases);
    
#if KEEP_BOUND_VALUES
    magns.Get()[0] = magn0;
    phases.Get()[0] = phase0;
    
    magns.Get()[1] = magn1;
    phases.Get()[1] = phase1;
    
    magns.Get()[magns.GetSize() - 1] = magnEnd;
    phases.Get()[magns.GetSize() - 1] = phaseEnd;
#endif
    
    //NormalizeFftValues(&magns);
    
    BLUtils::MagnPhaseToComplex(ioBuffer, magns, phases);
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
}

void
PitchShiftFftObj2::SetPitchFactor(BL_FLOAT factor)
{
    if (mFactors[1] < 0.0)
        mFactors[1] = factor;
    
    mFactors[0] = mFactors[1];
    mFactors[1] = factor;
    
    mFactor = mFactors[0];
    
    // ??
    //mBufferNum = 0;
}

void
PitchShiftFftObj2::Reset(int oversampling, int freqRes)
{
    FftObj::Reset(oversampling, freqRes);
    
    if ((oversampling > 0) && (freqRes > 0))
    {
        mFreqObj.Reset(mBufSize, oversampling, freqRes, mSampleRate);
    }
}

void
PitchShiftFftObj2::Reset(int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    FftObj::Reset(oversampling, freqRes);
    
    if ((oversampling > 0) && (freqRes > 0) && (sampleRate > 0.0))
    {
        mFreqObj.Reset(mBufSize, oversampling, freqRes, sampleRate);
        
        mSampleRate = sampleRate;
    }
    else if (sampleRate > 0.0)
    {
        mFreqObj.Reset(mBufSize, mOverlapping, mFreqRes, sampleRate);
                 
        mSampleRate = sampleRate;
    }
}

#if 0 // EXPE
void
PitchShiftFftObj2::SavePhasesState()
{
    mSaveStateFreqObj = mFreqObj;
}

void
PitchShiftFftObj2::RestorePhasesState()
{
    mFreqObj = mSaveStateFreqObj;
}
#endif

void
PitchShiftFftObj2::Convert(WDL_TypedBuf<BL_FLOAT> *ioMagns, WDL_TypedBuf<BL_FLOAT> *ioPhases)
{    
    if (ioMagns->GetSize() != ioPhases->GetSize())
        // Error
        return;
    
    const WDL_TypedBuf<BL_FLOAT> originMagns = *ioMagns;
    const WDL_TypedBuf<BL_FLOAT> originPhases = *ioPhases;
    
    // See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
    WDL_TypedBuf<BL_FLOAT> realFreqs;
    mFreqObj.ComputeRealFrequencies(*ioPhases, &realFreqs);
    
    // Reset all the values
    for (int i = 0; i < ioMagns->GetSize(); i++)
        ioMagns->Get()[i] = 0.0;
    
#if 1
    // The following line changes nothing !
    for (int i = 0; i < ioPhases->GetSize(); i++)
        ioPhases->Get()[i] = 0.0;
#endif
    
    // Do not reset the phases, we will focus on the magnitudes to detected unset values
    
    WDL_TypedBuf<BL_FLOAT> freqs;
    freqs.Resize(originMagns.GetSize());
    
    // -1.0 means undefined when filling holes after
    for (int i = 0; i < freqs.GetSize(); i++)
        freqs.Get()[i] = -1.0;
    
    freqs.Get()[0] = 0.0;
    
    for (int i = 0; i < originMagns.GetSize(); i++)
    {
        // Each of the two following methods has advantages and drawbacks
        
#if !USE_SMB_METHOD
        int srcBin = i;
        
        // Value of the frequency for the source scale
        BL_FLOAT srcBinFreq = BLUtils::FftBinToFreq(srcBin, originMagns.GetSize(), mSampleRate);
        
        // Make two cases, in order to avoid holes
        // The two cases are foward and reverse
        if (mFactor > 1.0)
        {
            // Get the current magnitude
            BL_FLOAT srcMagn = originMagns.Get()[srcBin];
            
            // Compute the value of the destination frequency
            BL_FLOAT dstFreq = srcBinFreq*mFactor;
            int dstBin = BLUtils::FreqToFftBin(dstFreq, originMagns.GetSize(), mSampleRate);
            if (dstBin >= originMagns.GetSize())
                // Too high frequency
                continue;
            
            ioMagns->Get()[dstBin] += srcMagn;
            
            BL_FLOAT freq = realFreqs.Get()[srcBin]*mFactor;
            freqs.Get()[dstBin] = freq;
        }
        else
        {
            // STRANGE...
            BL_FLOAT factor = (mFactor > 0.0) ? mFactor : 1.0;
            
            BL_FLOAT dstFreq = srcBinFreq/factor;
            
            int dstBin = BLUtils::FreqToFftBin(dstFreq, originMagns.GetSize(), mSampleRate);
            if (dstBin >= originMagns.GetSize())
                // Too high frequency
                continue;
            
            BL_FLOAT dstMagn = originMagns.Get()[dstBin];
            ioMagns->Get()[srcBin] = dstMagn;
            
            BL_FLOAT freq = realFreqs.Get()[dstBin]*factor;
            freqs.Get()[srcBin] = freq;
        }
#endif
        
#if USE_SMB_METHOD // Better...
        // Do as smdPitchShift
        int index = i*mFactor;
        if (index < originMagns.GetSize())
        {
            ioMagns->Get()[index] += originMagns.Get()[i];
            freqs.Get()[index] = realFreqs.Get()[i] * mFactor;
        }
#endif
        
    }
    
#if FILL_MISSING_FREQS
    // BUG correction: the Protools audio engine stopped working
    // That was due to NaN in the output
    // which was due to NaN phases after mFreqObj.ComputePhases()
    // which was due to unset freqs above (because we itare over the src
    // and plot to the dst, so there are holes...
    //
    // So fill holes by interpolation
    //
    // This was due to buffer overruns and undefined values (replaced some "<" by "<=")
#if 0
    FillMissingFreqs(&freqs);
#endif
    
    // Better version, better lerp
    //
    // With FillMissingFreqs(), we had 10Hz at bin 10 (i.e 200Hz)
    // That was not accurate at all !
    //
    BLUtils::FillMissingValues(&freqs, false);
#endif
    
    mFreqObj.ComputePhases(ioPhases, freqs);
}

#if 0 // Prefer using the version in Utils 
// (which was done for envelopes at the beginnning)
// It is more accurate (and maybe less buggy )!
void
PitchShiftFftObj2::FillMissingFreqs(WDL_TypedBuf<BL_FLOAT> *ioFreqs)
{
    // Assume that the first value is defined
    int startIndex = 0;
    BL_FLOAT startFreq = 0.0;
    
    while(startIndex < ioFreqs->GetSize())
    {
        BL_FLOAT freq = ioFreqs->Get()[startIndex];
        if (freq > 0.0)
            // Defined
        {
            startFreq = freq;
            startIndex++;
        }
        else
            // Undefined
        {
            // Find how many missing values we have
            int endIndex = startIndex;
            BL_FLOAT endFreq = 0.0;
            bool defined = false;
            
            while(!defined && (endIndex < ioFreqs->GetSize()))
            {
                endIndex++;
                endFreq = ioFreqs->Get()[endIndex];
                
                defined = (endFreq >= 0.0);
            }
            
            if (!defined)
                // Not found at the end
                endFreq = startFreq;
            
            // Fill the missing values
            for (int i = startIndex; i < endIndex; i++)
            {
                BL_FLOAT t = (i - startIndex)/(endIndex - startIndex);
                
                BL_FLOAT newFreq = (1.0 - t)*startFreq + t*endFreq;
                ioFreqs->Get()[i] = newFreq;
            }
            
            startIndex = endIndex;
        }
    }
    
    // Process the last value appart
    // (since we can't interpolate)
    int endIndex = ioFreqs->GetSize() - 1;
    if (ioFreqs->Get()[endIndex] < 0.0)
        // Symply copy the last last value
        // (this should suit...)
        ioFreqs->Get()[endIndex] = ioFreqs->Get()[endIndex - 1];
}
#endif
