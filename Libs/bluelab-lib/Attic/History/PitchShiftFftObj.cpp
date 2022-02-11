//
//  PitchShiftFftObj.cpp
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#include "BLUtils.h"
#include "SoftFft.h"

#include "PitchShiftFftObj.h"

// To avoid transforming the first and the center values
// When transforming with tempered, the first and center values changes !
#define KEEP_BOUND_VALUES 0

// To ignore the new computed phases
#define IGNORE_NEW_PHASES 0


#define KEEP_SYNTHESIS_ENERGY 0
#define USE_VARIABLE_HANNING 0


PitchShiftFftObj::PitchShiftFftObj(int bufferSize, int oversampling, int freqRes,
                                   BL_FLOAT sampleRate,
                                   int maxNumPoints, BL_FLOAT decimFactor)
: FftProcessObj16(bufferSize, oversampling, freqRes,
                  AnalysisMethodWindow, SynthesisMethodWindow,
                  KEEP_SYNTHESIS_ENERGY /*true*/ /*energy*/,
                  false,
                  USE_VARIABLE_HANNING /*true*/ /* variable hanning */),
mFactor(1.0),
mPhase(0.0),
mShift(0.0),
mSampleRate(sampleRate),
mFreqObj(bufferSize, oversampling, freqRes, sampleRate),
mInput(maxNumPoints, decimFactor, true),
mOutput(maxNumPoints, decimFactor, true)
{
    // "uninitialized"
    mFactors[0] = -1.0;
    mFactors[1] = -1.0;
    
    mFftBufferNum = 0;
    
    mPhaseSum = 0.0;
    mPhaseSums.Resize(bufferSize/2);
    BLUtils::FillAllZero(&mPhaseSums);
    
    mShiftSum = 0.0;
    
    // DEBUG
    mMagnsUseLinerp = false;
    mMagnsFillHoles = false;
    
    mUseFreqAdjust = false;
    mUseSoftFft1 = false;
    mUseSoftFft2 = false;
    
    // Used to display the waveform without problem with overlap
    // (otherwise, it would made "packets" at each step)
    mBufferCount = 0;
}

PitchShiftFftObj::~PitchShiftFftObj() {}

#if 0
// WARNING, post process moved in fft12
void
PitchShiftFftObj::PostProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    mShiftSum += mShift;
    
    int shift = mShiftSum*ioBuffer->GetSize();
    
    BLUtils::ShiftSamples(ioBuffer, shift);
}
#endif

void
PitchShiftFftObj::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    WDL_TypedBuf<BL_FLOAT> input = *ioBuffer;
    
    if (mBufferCount % mOverlapping == 0)
        mInput.AddValues(input);
}

void
PitchShiftFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
#if 1
    // Hack to smooth factor changes
    BL_FLOAT t = 1.0;
    if (mOverlapping > 1)
        t = ((BL_FLOAT)mFftBufferNum)/(mOverlapping - 1);
    
    // Test just in case, to avoid having the pitch flying to infinity
    if (t > 1.0)
        t = 1.0;
    
    mFactor = (1.0 - t)*mFactors[0] + t*mFactors[1];
    mFftBufferNum++;
#endif
    
    // Store the input fft buffer
    BLUtils::ComplexToMagn(&mCurrentBuf, *ioBuffer);
    
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
    
    
#if IGNORE_NEW_PHASES
    WDL_TypedBuf<BL_FLOAT> savePhases = phases;
#endif
    
    if (!mUseFreqAdjust)
        Convert(&magns, &phases);
    else
        SmbConvert(&magns, &phases);
    
#if IGNORE_NEW_PHASES
    phases = savePhases;
#endif
    
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
PitchShiftFftObj::PostProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    // UnWindow ?
    
    if (mBufferCount % mOverlapping == 0)
        mOutput.AddValues(*ioBuffer);
    
    mBufferCount++;
    
    // DEBUG
    mShiftSum += mShift;
    int shift = mShiftSum*ioBuffer->GetSize();
    BLUtils::ShiftSamples(ioBuffer, shift);
    //////
}

void
PitchShiftFftObj::GetInput(WDL_TypedBuf<BL_FLOAT> *outInput)
{
    mInput.GetValues(outInput);
}

void
PitchShiftFftObj::GetOutput(WDL_TypedBuf<BL_FLOAT> *outOutput)
{
    mOutput.GetValues(outOutput);
}

void
PitchShiftFftObj::SetPitchFactor(BL_FLOAT factor)
{
    if (mFactors[1] < 0.0)
        mFactors[1] = factor;
    
    mFactors[0] = mFactors[1];
    mFactors[1] = factor;
    
    mFactor = mFactors[0];
    
    mFftBufferNum = 0;
}

void
PitchShiftFftObj::SetPhase(BL_FLOAT phase)
{
    mPhase = phase;
}

void
PitchShiftFftObj::SetShift(BL_FLOAT shift)
{
    mShift = shift;
}

void
PitchShiftFftObj::SetCorrectEnvelope1(bool flag)
{
    FftProcessObj16::DBG_SetCorrectEnvelope1(flag);
}

void
PitchShiftFftObj::SetEnvelopeAutoAlign(bool flag)
{
    FftProcessObj16::DBG_SetEnvelopeAutoAlign(flag);
}

void
PitchShiftFftObj::SetCorrectEnvelope2(bool flag)
{
    FftProcessObj16::DBG_SetCorrectEnvelope2(flag);
}


void
PitchShiftFftObj::SetMagnsUseLinerp(bool flag)
{
    mMagnsUseLinerp = flag;
}

void
PitchShiftFftObj::SetMagnsFillHoles(bool flag)
{
    mMagnsFillHoles = flag;
}

void
PitchShiftFftObj::SetUseFreqAdjust(bool flag)
{
    mUseFreqAdjust = flag;
}

void
PitchShiftFftObj::SetUseSoftFft1(bool flag)
{
    mUseSoftFft1 = flag;
}

void
PitchShiftFftObj::SetUseSoftFft2(bool flag)
{
    mUseSoftFft2 = flag;
}

void
PitchShiftFftObj::Reset(int oversampling, int freqRes)
{
    FftProcessObj16::Reset(oversampling, freqRes);
    
    mFftBufferNum = 0;
    
    if ((oversampling > 0) && (freqRes > 0))
        mFreqObj.Reset(mBufSize, oversampling, freqRes);
    
    // Used to display the waveform without problem with overlap
    // (otherwise, it would made "packets" at each step)
    mBufferCount = 0;
}

//void
//PitchShiftFftObj::ResetPhases()
//{
//    mFreqObj.ResetPhases();
//}

//void
//PitchShiftFftObj::SetPrevPhases(const WDL_TypedBuf<BL_FLOAT> &phases)
//{
//    mFreqObj.SetPrevPhases(phases);
//}

void
PitchShiftFftObj::SavePhasesState()
{
    mSaveStateFreqObj = mFreqObj;
}

void
PitchShiftFftObj::RestorePhasesState()
{
    mFreqObj = mSaveStateFreqObj;
}

void
PitchShiftFftObj::Convert(WDL_TypedBuf<BL_FLOAT> *ioMagns, WDL_TypedBuf<BL_FLOAT> *ioPhases)
{
    if (ioMagns->GetSize() != ioPhases->GetSize())
        // Error
        return;
    
    WDL_TypedBuf<BL_FLOAT> realFreqs;
    // Need it for Soft2 too
    //if (mUseFreqAdjust)
    {
        mFreqObj.ComputeRealFrequencies(*ioPhases, &realFreqs);
    }
    
#if EXTRACT_MAX
    int maxIndex = BLUtils::FindMaxIndex(*ioMagns);
    BL_FLOAT maxValue = ioMagns->Get()[maxIndex];
    BLUtils::FillAllZero(ioMagns);
    ioMagns->Get()[maxIndex] = maxValue;
#endif
    
    const WDL_TypedBuf<BL_FLOAT> originMagns = *ioMagns;
    const WDL_TypedBuf<BL_FLOAT> originPhases = *ioPhases;
    
#if DEBUG_SAVE_SPECTROGRAM
    static BLSpectrogram spect0(ioMagns->GetSize());
    static int saveCount0 = 0;
    spect0.SetMultipliers(2.0, 0.0002/*0.002*/);
    spect0.AddLine(*ioMagns, *ioPhases);
    
    if (saveCount0++ == DEBUG_SAVE_SPECTROGRAM_COUNT) // Q1
        //if (saveCount++ == 5500) // Q4
    {
        spect0.SavePPM16("BL-PitchShift-inside0.ppm");
    }
#endif
    
    // GARBAGE
#if 0 // DEBUG LOAD SPECTROGRAM
    static BLSpectrogram *testSpec = NULL;
    static int lineCount = 0;
    if (testSpec == NULL)
        testSpec = BLSpectrogram::LoadPPM16("spectro-16-pitch-unsharp.ppm", 2.0, 0.0002);
    
    WDL_TypedBuf<BL_FLOAT> dummy;
    
    bool lineOk = testSpec->GetLine(lineCount, ioMagns, &dummy);
    
    lineCount++;
    
    if (lineOk)
        return;
#endif
    
    // By default, keep the original values
    // Will be usefull when overtaking Nyquist or synthesis window frequency
    
#if 1
    // Reset all the values
    for (int i = 0; i < ioMagns->GetSize(); i++)
        ioMagns->Get()[i] = 0.0;
#endif
    
    for (int i = 0; i < originMagns.GetSize(); i++)
    {
        int srcBin = i;
        
        // Value of the frequency for the source scale
        BL_FLOAT srcFreq = BLUtils::FftBinToFreq(srcBin, originMagns.GetSize()*2, mSampleRate);
        
        // Make two cases, in order to avoid holes
        // The two cases are foward and reverse
        if (mFactor > 1.0)
        {
            BL_FLOAT factor = (mFactor > 0.0) ? mFactor : 1.0;
            
            BL_FLOAT dstFreq;
            if (mMagnsFillHoles || mMagnsUseLinerp)
                // avoid holes
            {
                dstFreq = srcFreq/factor;
            }
            else //avoid leakage
            {
                dstFreq = srcFreq*factor;
            }
            
#if BOUND_NYQUIST
            if ((srcFreq >= mSampleRate/2.0) ||
                (dstFreq >= mSampleRate/2.0))
                // Nyquist
                continue;
#endif
            
#if BOUND_SYNWIN_FREQ
            if ((srcFreq <= SYNWIN_FREQ) ||
                (dstFreq <= SYNWIN_FREQ))
                continue;
#endif
            
            // Compute the value of the destination frequency
            BL_FLOAT t;
            int dstBin = BLUtils::FreqToFftBin(dstFreq, originMagns.GetSize()*2, mSampleRate, &t);
            
            if (!mMagnsUseLinerp)
                // No interpolation
            {
                if (mMagnsFillHoles)
                    // avoid holes
                {
                    BL_FLOAT dstMagn = originMagns.Get()[dstBin];
                    ioMagns->Get()[srcBin] = dstMagn;
                }
                else
                    // avoid leakage
                {
                    BL_FLOAT srcMagn = originMagns.Get()[srcBin];
                    ioMagns->Get()[dstBin] += srcMagn;
                }
            }
            else
                // Linear interpolation
            {
                BL_FLOAT dstMagn;
                if (dstBin >= originMagns.GetSize()-1)
                    dstMagn = originMagns.Get()[dstBin];
                else
                {
                    BL_FLOAT dstMagn0 = originMagns.Get()[dstBin];
                    BL_FLOAT dstMagn1 = originMagns.Get()[dstBin + 1];
                    
                    dstMagn = (1.0 - t)*dstMagn0 + t*dstMagn1;
                }
                
                ioMagns->Get()[srcBin] = dstMagn;
            }
        }
#if 1
        else
        {
            // Bypass (instead of lower the sound, for the moment)
            *ioMagns = originMagns;
            *ioPhases = originPhases;
        }
#else  // Lower the sound
        else
            // NOT TESTED !!!!!!!!
        {
            // Compute the value of the destination frequency
            BL_FLOAT t;
            BL_FLOAT dstFreq = srcFreq*mFactor;
            int dstBin = BLUtils::FreqToFftBin(dstFreq, originMagns.GetSize()*2, mSampleRate, &t);
            
            //if (dstBin >= originMagns.GetSize()/*/2*/)
            //  // Too high frequency
            //  continue;
            
#if BOUND_NYQUIST
            if ((srcFreq >= mSampleRate/2.0) ||
                (dstFreq >= mSampleRate/2.0))
                // Nyquist
                continue;
#endif
            
#if BOUND_SYNWIN_FREQ
            if ((srcFreq <= SYNWIN_FREQ) ||
                (dstFreq <= SYNWIN_FREQ))
                continue;
#endif
            
#if !USE_LINERP // No interploation
            BL_FLOAT srcMagn = originMagns.Get()[srcBin];
            ioMagns->Get()[dstBin] /*+*/= srcMagn;
#else
            // Linear interpolation
            BL_FLOAT srcMagn;
            
            if (srcBin >= originMagns.GetSize()-1)
                srcMagn = originMagns.Get()[srcBin];
            else
            {
                BL_FLOAT srcMagn0 = originMagns.Get()[srcBin];
                BL_FLOAT srcMagn1 = originMagns.Get()[srcBin + 1];
                
                srcMagn = (1.0 - t)*srcMagn0 + t*srcMagn1;
            }
            
            ioMagns->Get()[dstBin] = srcMagn;
#endif
        }
#endif // Lower the sound
    }
    
    //if (mUseFreqAdjust) // NOT USED
    //{
    //  BLUtils::MultValues(&realFreqs, 1.0/mFactor);
    //  mFreqObj.ComputePhases(ioPhases, realFreqs);
    //}
    
    // Phase parameter
    BL_FLOAT phaseParam = mPhase*2.0*M_PI;
    mPhaseSum += phaseParam;
    BLUtils::AddValues(ioPhases, mPhaseSum);
    
    if (mUseSoftFft1 || mUseSoftFft2)
    {
        WDL_TypedBuf<BL_FLOAT> freqs;
        WDL_TypedBuf<BL_FLOAT> phases;
        
        // Freqs
        if (mUseSoftFft1)
        {
            WDL_TypedBuf<BL_FLOAT> origFreqs;
            origFreqs.Resize(originMagns.GetSize());
            
            freqs.Resize(originMagns.GetSize());
            for (int i = 0; i < freqs.GetSize(); i++)
            {
                BL_FLOAT freq = BLUtils::FftBinToFreq(i, originMagns.GetSize()*2, mSampleRate);
                
                origFreqs.Get()[i] = freq;
                
                BL_FLOAT newFreq = freq*mFactor;
                
                if (newFreq < mSampleRate/2)
                    freqs.Get()[i] = newFreq;
                else
                    freqs.Get()[i] = freq;
            }
            
            phases.Resize(freqs.GetSize());
            for (int i = 0; i < phases.GetSize(); i++)
            {
                BL_FLOAT freq = freqs.Get()[i];
                BL_FLOAT t;
                int idx = BLUtils::FindValueIndex(freq, origFreqs, &t);
                
                if ((idx >= 0) && (idx < ioPhases->GetSize() - 1))
                {
                    BL_FLOAT phase0 = originPhases.Get()[idx];
                    BL_FLOAT phase1 = ioPhases->Get()[idx+1];
                    
                    BL_FLOAT newPhase = (1.0 - t)*phase0 + t*phase1;
                    phases.Get()[i] = newPhase;
                }
            }
        }
        else if (mUseSoftFft2)
        {
            phases = originPhases;
            
#if 1
            freqs = realFreqs;
            
#else
            WDL_TypedBuf<BL_FLOAT> origFreqs;
            origFreqs.Resize(originMagns.GetSize());
            
            freqs.Resize(originMagns.GetSize());
            for (int i = 0; i < freqs.GetSize(); i++)
            {
                BL_FLOAT freq = realFreqs.Get()[i];
                
                origFreqs.Get()[i] = freq;
                
                BL_FLOAT newFreq = freq*mFactor;
                
                if (newFreq < mSampleRate/2)
                    freqs.Get()[i] = newFreq;
                else
                    freqs.Get()[i] = freq;
            }
#endif
        }
        
        WDL_TypedBuf<BL_FLOAT> samples;
        samples.Resize(ioMagns->GetSize()*2);
        SoftFft::Ifft(*ioMagns, freqs, phases, mSampleRate, &samples);
        
        WDL_TypedBuf<BL_FLOAT> resultMagns;
        WDL_TypedBuf<BL_FLOAT> resultPhases;
        FftProcessObj16::SamplesToFft(samples, &resultMagns, &resultPhases);
        
        BLUtils::TakeHalf(&resultMagns);
        BLUtils::TakeHalf(&resultPhases);
        
        *ioMagns = resultMagns;
        *ioPhases = resultPhases;
    }
    
#if DEBUG_SAVE_SPECTROGRAM
    static BLSpectrogram spect1(ioMagns->GetSize());
    static int saveCount1 = 0;
    spect1.SetMultipliers(2.0, 0.0002/*0.002*/);
    spect1.AddLine(*ioMagns, *ioPhases);
    
    if (saveCount1++ == DEBUG_SAVE_SPECTROGRAM_COUNT)
    {
        spect1.SavePPM16("BL-PitchShift-inside1.ppm");
    }
#endif
}

void
PitchShiftFftObj::SmbConvert(WDL_TypedBuf<BL_FLOAT> *ioMagns, WDL_TypedBuf<BL_FLOAT> *ioPhases)
{
#define USE_SMB_METHOD 1
#define FILL_MISSING_FREQS 1
    
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
    freqs.Resize(originMagns.GetSize()/*/2*/ /*+ 1*/); ///
    
    // -1.0 means undefined when filling holes after
    for (int i = 0; i < freqs.GetSize(); i++)
        freqs.Get()[i] = -1.0;
    
    freqs.Get()[0] = 0.0;
    
    for (int i = 0; i </*=*/ originMagns.GetSize()/*/2*/; i++) ///
    {
        int srcBin = i;
        
        // Each of the two following methods has advantages and drawbacks
        
#if !USE_SMB_METHOD
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
            if (dstBin >= originMagns.GetSize()/*/2*/)
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
            if (dstBin >= originMagns.GetSize()/*/2*/)
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
        if (index < originMagns.GetSize()/*/2*/)
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
    SmbFillMissingFreqs(&freqs);
#endif
    
    mFreqObj.ComputePhases(ioPhases, freqs);
}

void
PitchShiftFftObj::SmbFillMissingFreqs(WDL_TypedBuf<BL_FLOAT> *ioFreqs)
{
    // Assume that the first value is defined
    int startIndex = 0;
    BL_FLOAT startFreq = 0.0;
    
    while(startIndex < ioFreqs->GetSize()/*/2*/)
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
            
            while(!defined && (endIndex < ioFreqs->GetSize()/*/2*/))
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
