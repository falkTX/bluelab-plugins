//
//  FftProcessObj12.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <Utils.h>
#include <Debug.h>
#include <DebugGraph.h>

#include "FftProcessObj12.h"

// Weird sidechain use. Not sure it is used by any plugin
#define EXP_SIDECHAIN_USE 0

#define NORMALIZE_FFT 1

// WARNING: this is buggy and false, need some fix and reconsidered if want to use it
// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0

// Useful for debuging
#define DEBUG_DISABLE_OVERLAPPING 0
#define DEBUG_DISABLE_PROCESS 0
#define DEBUG_WINDOWS 0

// NOTE: With freqRes == 2, and overlapping = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj12::FftProcessObj12(int bufferSize, int overlapping, int freqRes,
                                 enum AnalysisMethod aMethod,
                                 enum SynthesisMethod sMethod,
                                 bool keepSynthesisEnergy, bool skipFFT,
                                 bool variableHanning)
{
    mBufSize = bufferSize;
    
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    mKeepSynthesisEnergy = keepSynthesisEnergy;
    
    mSkipFFT = skipFFT;
    
    mVariableHanning = variableHanning;
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
    
    Reset(overlapping, freqRes);
}

FftProcessObj12::~FftProcessObj12() {}

void
FftProcessObj12::Reset(int overlapping, int freqRes)
{
    if (overlapping > 0)
        mOverlapping = overlapping;
    
    if (freqRes > 0)
        mFreqRes = freqRes;
    
#if DEBUG_DISABLE_OVERLAPPING
    mOverlapping = 1;
#endif
    
    mShift = mBufSize/mOverlapping;
    
    MakeWindows();
    
    // Set the buffers.
    int resultSize = 2*mBufSize*mFreqRes;
    Utils::ResizeFillZeros(&mResultChunk, resultSize);
    
    mBufOffset = 0;
    
    int bufSize = mBufSize*mFreqRes;
    Utils::ResizeFillZeros(&mSortedFftBuf, bufSize);
    Utils::ResizeFillZeros(&mResultTmpChunk, bufSize);
   
    Utils::ResizeFillZeros(&mSortedFftBufSc, bufSize);
    
    mSamplesChunk.Resize(0);
    mScChunk.Resize(0);
    
    mResultOutChunk.Resize(0);
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
}

void
FftProcessObj12::AddSamples(double *samples, double *scSamples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
    
    if (scSamples != NULL)
        mScChunk.Add(scSamples, numSamples);
}

void
FftProcessObj12::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) {}

void
FftProcessObj12::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                     WDL_TypedBuf<double> *scBuffer) {}

void
FftProcessObj12::SamplesToFft(const WDL_TypedBuf<double> &inSamples,
                              WDL_TypedBuf<double> *outFftMagns,
                              WDL_TypedBuf<double> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = Utils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<double> paddedSamples = inSamples;
    Utils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    Utils::ComplexToMagnPhase(outFftMagns, outFftPhases, fftSamples);
}

void
FftProcessObj12::SamplesToFftMagns(const WDL_TypedBuf<double> &inSamples,
                                   WDL_TypedBuf<double> *outFftMagns)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = Utils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<double> paddedSamples = inSamples;
    Utils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    Utils::ComplexToMagn(outFftMagns, fftSamples);
}

void
FftProcessObj12::FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                              WDL_TypedBuf<double> *outSamples)
{
    outSamples->Resize(fftBuffer.GetSize());
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

#define DEBUG_PHASE 0

#define USE_DEBUG_GRAPH 1

void
FftProcessObj12::ProcessOneBuffer()
{
    WDL_TypedBuf<double> copySamplesBuf;
    copySamplesBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        copySamplesBuf.Get()[i] = mSamplesChunk.Get()[i + mBufOffset];
    WDL_TypedBuf<double> copyScBuf;
    if (mScChunk.GetSize() > 0)
    {
        copyScBuf.Resize(mBufSize);
        for (int i = 0; i < mBufSize; i++)
            copyScBuf.Get()[i] = mScChunk.Get()[i + mBufOffset];
    }
 
    /*
#if DEBUG_PHASE
    WDL_TypedBuf<double> samples00 = copySamplesBuf;
    samples00.Get()[0] = 1.0;
    Debug::AppendData("samples00.txt", samples00);
#endif
    */
    
    WDL_TypedBuf<double> saveSamples = copySamplesBuf;
    
    // ORIG
    
    // Apply analysis windows before resizing !
    ApplyAnalysisWindow(&copySamplesBuf);

    //ApplyAnalysisWindowInv(&copySamplesBuf);
    
    
//#if DEBUG_PHASE
//    WDL_TypedBuf<double> samples00Win = copySamplesBuf;
//    samples00Win.Get()[0] = 1.0;
//    Debug::AppendData("samples00-win.txt", samples00Win);
//#endif
    
    /*
    copySamplesBuf = saveSamples;
    
    // Apply analysis windows before resizing !
    ApplyAnalysisWindowInv(&copySamplesBuf);
    
#if DEBUG_PHASE
    WDL_TypedBuf<double> samples00WinInv = copySamplesBuf;
    samples00WinInv.Get()[0] = 1.0;
    Debug::AppendData("samples00-win-inv.txt", samples00WinInv);
#endif
    */
    
    // Compute "energy" just after analysis windowing
    // then just after synthesis windowing
    // Compute and apply a coefficient to avoid loosing energy
    double energy0 = 0.0;
    if (mKeepSynthesisEnergy)
        energy0 = Utils::ComputeAbsAvg(copySamplesBuf);
    
    // windowing before zero padding
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last links of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        Utils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
        
        if (mScChunk.GetSize() > 0)
            Utils::ResizeFillZeros(&copyScBuf, copyScBuf.GetSize()*mFreqRes);
    }
    
    if (mScChunk.GetSize() > 0)
        ApplyAnalysisWindow(&copyScBuf);
    
    if (mSkipFFT)
    {
#if EXP_SIDECHAIN_USE
        if (mScChunk.GetSize() == 0)
            // No side chain, take the input
        {
            // By default, do nothing with the fft
            mResultTmpChunk.Resize(0);
        
            // Be careful to not add the beginning of the buffer
            // (otherwise the windows will be applied twice sometimes).
            mResultTmpChunk.Add(copySamplesBuf.Get(), mBufSize);
        }
        else
            // Side chain: take the side chain buffer
        {
            mResultTmpChunk.Resize(0);
            mResultTmpChunk.Add(copyScBuf.Get(), mBufSize);
        }
#else
        // By default, do nothing with the fft
        mResultTmpChunk.Resize(0);
        
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).
        mResultTmpChunk.Add(copySamplesBuf.Get(), mBufSize);
#endif
    }
    else
    {
#if EXP_SIDECHAIN_USE
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        if (mScChunk.GetSize() == 0)
            // No side chain, take the input
            ComputeFft(copySamplesBuf, &mSortedFftBuf, mFreqRes);
        else
            // Side chain: process the side chain
            ComputeFft(copyScBuf, &mSortedFftBuf, mFreqRes);
#else
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        ComputeFft(copySamplesBuf, &mSortedFftBuf, mFreqRes);
        
        if (mScChunk.GetSize() > 0)
            // Side chain: process the side chain
            ComputeFft(copyScBuf, &mSortedFftBufSc, mFreqRes);
#endif
                
#if USE_DEBUG_GRAPH
        WDL_TypedBuf<double> samples0;
        FftProcessObj12::FftToSamples(mSortedFftBuf, &samples0);
        DebugGraph::SetCurveValues(samples0,
                                   0,
                                   -1.0, 1.0,
                                   1.0,
                                   0, 0, 255,
                                   false, 0.5);
        
        WDL_TypedBuf<double> magns0;
        WDL_TypedBuf<double> phases0;
        Utils::ComplexToMagnPhase(&magns0, &phases0, mSortedFftBuf);
      
        WDL_TypedBuf<double> envelope0;
        Utils::ComputeEnvelppe(samples0, &envelope0);
        DebugGraph::SetCurveValues(envelope0,
                                   1,
                                   -1.0, 1.0,
                                   2.0,
                                   0, 0, 255,
                                   false, 0.5);
        
#define COEFF 3.0
        Utils::MultValues(&magns0, COEFF);
        DebugGraph::SetCurveValues(magns0,
                                   2,
                                   -1.0, 1.0,
                                   1.0,
                                   128, 128, 255,
                                   false, 0.5);
#endif
        
#if DEBUG_PHASE
        //WDL_TypedBuf<double> magns0;
        //WDL_TypedBuf<double> phases0;
        //Utils::ComplexToMagnPhase(&magns0, &phases0, mSortedFftBuf);
        //Debug::DumpData("magns0.txt", magns0);
        //Debug::DumpData("phases0.txt", phases0);
#endif
        
#if DEBUG_PHASE
        //WDL_TypedBuf<double> samples0;
        //FftProcessObj12::FftToSamples(mSortedFftBuf, &samples0);
        samples0.Get()[0] = 1.0;
        Debug::AppendData("samples0.txt", samples0);
#endif

        
#if !DEBUG_DISABLE_PROCESS
        // Apply modifications of the buffer
        if (mScChunk.GetSize() == 0)
            ProcessFftBuffer(&mSortedFftBuf);
        else
            // It is a good idea to pass the sidechain like that
            // Sidechain is managed as samples buffers
            // So it is buffered exactly the same way (e.g in the case of overlapping)
            // ... which is good !
            ProcessFftBuffer(&mSortedFftBuf, &mSortedFftBufSc);
            
#endif
             
#if USE_DEBUG_GRAPH
        WDL_TypedBuf<double> samples1;
        FftProcessObj12::FftToSamples(mSortedFftBuf, &samples1);
        DebugGraph::SetCurveValues(samples1,
                                   3,
                                   -1.0, 1.0,
                                   1.0,
                                   0, 255, 0,
                                   false, 0.5);
        
        WDL_TypedBuf<double> envelope1;
        Utils::ComputeEnvelppe(samples1, &envelope1);
        
#if 0
#if 1 // LOAD
        Debug::LoadData("samples1-orig.txt", &samples1);
        Utils::ComputeEnvelppe(samples1, &envelope1);
#endif
        
#if 1 // SAVE
        Debug::DumpData("samples1.txt", samples1);
        Debug::DumpData("envelope1.txt", envelope1);
        
        exit(0);
#endif
#endif
        
        DebugGraph::SetCurveValues(envelope1,
                                   4,
                                   -1.0, 1.0,
                                   2.0,
                                   0, 255, 0,
                                   false, 0.5);
        
        WDL_TypedBuf<double> magns1;
        WDL_TypedBuf<double> phases1;
        Utils::ComplexToMagnPhase(&magns1, &phases1, mSortedFftBuf);
        Utils::MultValues(&magns1, COEFF);
        DebugGraph::SetCurveValues(magns1,
                                   5,
                                   -1.0, 1.0,
                                   1.0,
                                   128, 255, 128,
                                   false, 0.5);
#endif
        
        CorrectEnveloppe(&samples1, envelope0, envelope1);
        
        DebugGraph::SetCurveValues(samples1,
                                   6,
                                   -1.0, 1.0,
                                   1.0,
                                   255, 255, 255,
                                   false, 0.5);
        
//#if DEBUG_PHASE
//        WDL_TypedBuf<double> magns1;
//        WDL_TypedBuf<double> phases1;
//        Utils::ComplexToMagnPhase(&magns1, &phases1, mSortedFftBuf);
//        Debug::DumpData("magns1.txt", magns1);
//        Debug::DumpData("phases1.txt", phases1);
//#endif
        
      
        // TMP TODO
        mResultTmpChunk = samples1;
        
#if DEBUG_PHASE
        //WDL_TypedBuf<double> samples1;
        //FftProcessObj12::FftToSamples(mSortedFftBuf, &samples1);
        samples1.Get()[0] = 1.0;
        Debug::AppendData("samples1.txt", samples1);
#endif
        
        // TODO: freq res...
        //ComputeInverseFft(mSortedFftBuf, &mResultTmpChunk, mFreqRes);
        
        // TEST
#if 0
        Utils::ShiftSamples(&mResultTmpChunk, mResultTmpChunk.GetSize()/2);
        
#if DEBUG_PHASE
        WDL_TypedBuf<double> shiftSamples1 = mResultTmpChunk;
        shiftSamples1.Get()[0] = 1.0;
        Debug::AppendData("samples1-shift.txt", shiftSamples1);
#endif
#endif
    }
    
#if !DEBUG_DISABLE_PROCESS
    if (mScChunk.GetSize() == 0)
        // No side chain
        ProcessSamplesBuffer(&mResultTmpChunk, NULL);
    else
        ProcessSamplesBuffer(&copySamplesBuf, &mResultTmpChunk);
#endif
    
    // Fill the result
    double *result = mResultChunk.Get();
    
    // Apply synthesis window
    ApplySynthesisWindow(&mResultTmpChunk);
    
    if (mKeepSynthesisEnergy)
    {
        double energy1 = Utils::ComputeAbsAvg(mResultTmpChunk);
    
        double energyCoeff = 1.0;
        if (energy1 > 0)
        energyCoeff = energy0/energy1;

        // If mOverlapping is 1, thn div is 0
        if (mOverlapping > 1)
        {
            double div = log(mOverlapping)/log(2.0);
            energyCoeff /= div;
    
            Utils::MultValues(&mResultTmpChunk, energyCoeff);
        }
    }
    
#if !ADD_TAIL
    // Standard size
    int loopCount = mBufSize;
#else
    // Size to add tail
    int loopCount = copySamplesBuf.GetSize();
#endif
    
#if DEBUG_PHASE
    WDL_TypedBuf<double> result0 = mResultChunk;
    result0.Get()[0] = 1.0;
    Debug::AppendData("result0.txt", result0);
#endif
    
    // Make the sum (overlap)
    for (int i = 0; i < loopCount; i++)
    {
        result[i + mBufOffset] += mResultTmpChunk.Get()[i];
    }
    
#if DEBUG_PHASE
    WDL_TypedBuf<double> result1 = mResultChunk;
    result1.Get()[0] = 1.0;
    Debug::AppendData("result1.txt", result1);
    
    static int testCount = 0;
    if (testCount++ == 20)
        exit(0);
#endif
}

void
FftProcessObj12::NextSamplesBuffer(WDL_TypedBuf<double> *samples)
{
    // Reduce the size by mOverlap
    int size = samples->GetSize();
    if (size == mBufSize)
        samples->Resize(0);
    else if (size > mBufSize)
    {
        int newSize = size - mBufSize;
            
        // Resize down, skipping left
        WDL_TypedBuf<double> tmpChunk;
        tmpChunk.Add(&samples->Get()[mBufSize], newSize);
        *samples = tmpChunk;
    }
}

void
FftProcessObj12::NextOutBuffer()
{
    // Reduce the size by mOverlap
    int size = mResultChunk.GetSize();
    
    int minSize = mBufSize*2*mFreqRes;

    if (size < minSize)
        return;
    
    double *resultBuf = mResultChunk.Get();
    
    WDL_TypedBuf<double> newBuffer;
    for (int i = 0; i < mBufSize; i++)
    {
        newBuffer.Add(resultBuf[i]);
    }
    
    for (int i = 0; i < mBufSize; i++)
        mResultOutChunk.Add(newBuffer.Get()[i]);
    
    if (size == mBufSize)
        mResultChunk.Resize(0);
    else if (size > mBufSize)
    {
        int newSize = size - mBufSize;
            
        // Resize down, skipping left
        WDL_TypedBuf<double> tmpChunk;
        tmpChunk.Add(&mResultChunk.Get()[mBufSize], newSize);
        mResultChunk = tmpChunk;
    }
    
    // Grow the output with zeros
    Utils::ResizeFillZeros(&mResultChunk,
                           mResultChunk.GetSize() + mBufSize);
}

void
FftProcessObj12::GetResultOutBuffer(double *output, int nFrames)
{
    int size = mResultOutChunk.GetSize();
 
    // Should not happen
    if (size < nFrames)
        return;
    
    // Copy the result
    memcpy(output, mResultOutChunk.Get(), nFrames*sizeof(double));
    
    // Consume the returned samples
    int newSize = mResultOutChunk.GetSize() - nFrames;
    
    // Resize down, skipping left
    WDL_TypedBuf<double> tmpChunk;
    tmpChunk.Add(&mResultOutChunk.Get()[nFrames], newSize);
    mResultOutChunk = tmpChunk;
}

bool
FftProcessObj12::Shift()
{
    mBufOffset += mShift;
    
    if (mBufOffset >= mBufSize)
    {
        mBufOffset = 0;
        
        return true;
    }
    
    return false;
}

bool
FftProcessObj12::Process(double *input, double *output,
                         double *inSc, int nFrames)
{
#if DEBUG_WINDOWS
    for (int i = 0; i < nFrames; i++)
        input[i] = 1.0;
#endif
    
    // Add the new input data
    AddSamples(input, inSc, nFrames);
    
    if (mMustPadSamples)
    {
        Utils::PadZerosLeft(&mSamplesChunk, mAnalysisWindow.GetSize());
        
        if (mScChunk.GetSize() > 0)
            Utils::PadZerosLeft(&mScChunk, mAnalysisWindow.GetSize());
        
        mMustPadSamples = false;
        mMustUnpadResult = true;
    }
    
    // Compute from the available input data
    int numSamples = mSamplesChunk.GetSize();
    while (numSamples >= 2*mBufSize - mShift)
    {
        bool stopFlag = false;
        while(!stopFlag)
        {
            ProcessOneBuffer();
            stopFlag = Shift();
        }
        
        NextSamplesBuffer(&mSamplesChunk);
        NextSamplesBuffer(&mScChunk);
        
        NextOutBuffer();
        
        numSamples = mSamplesChunk.GetSize();
    }
    
    // With that (x1.5), this was impossible to setup the latency correctly
    //if (mResultOutChunk.GetSize() >= mAnalysisWindow.GetSize()*1.5)
    
    // This one (x2) is correct for setting the latency correctly
    if (mResultOutChunk.GetSize() >= mAnalysisWindow.GetSize()*2)
    {
        if (mMustUnpadResult)
        {
            Utils::ConsumeLeft(&mResultOutChunk, mAnalysisWindow.GetSize());
            
            mMustUnpadResult = false;
        }
    }
    
    // Get the computed result
    int numOutSamples = mResultOutChunk.GetSize();
    if ((nFrames <= numOutSamples) && !mMustUnpadResult)
    {
        if (output != NULL)
        {
            GetResultOutBuffer(output, nFrames);
        }
        
#if DEBUG_WINDOWS
        Debug::AppendData("output.txt", output, nFrames);
#endif

        return true;
    }
    else
        // Fill with zeros
        // (as required when there is latency and data is not yet available)
    {
        if (output != NULL)
        {
            for (int i = 0; i < nFrames; i++)
                output[i] = 0.0;
        }

        return false;
    }
}

void
FftProcessObj12::NormalizeFftValues(WDL_TypedBuf<double> *magns)
{
    double sum = 0.0f;
    
    // Not test "/2"
    for (int i = 1; i < magns->GetSize()/*/2*/; i++)
    {
        double magn = magns->Get()[i];
        
        sum += magn;
    }
    
    sum /= magns->GetSize()/*/2*/ - 1;
    
    magns->Get()[0] = sum;
    magns->Get()[magns->GetSize() - 1] = sum;
}

void
FftProcessObj12::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    if (ioBuffer->GetSize() < 2)
        return;
    
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    for (int i = 1; i < ioBuffer->GetSize()/2; i++)
    {
        int id0 = i + ioBuffer->GetSize()/2;
        
#if 1 // ORIG
        // Orig, bug...
        // doesn't fill exactly the symetry (the last value differs)
        // but WDL generates ffts like that
        int id1 = ioBuffer->GetSize()/2 - i;
#endif
        
#if 0 // Bug fix (but strange WDL behaviour)
        // Really symetric version
        // with correct last value
        // But if we apply to just generate WDL fft, the behaviour becomes different
        int id1 = ioBuffer->GetSize()/2 - i - 1;
#endif
        
        ioBuffer->Get()[id0].re = ioBuffer->Get()[id1].re;
        
        // Complex conjugate
        ioBuffer->Get()[id0].im = -ioBuffer->Get()[id1].im;
    }
}

void
FftProcessObj12::ApplyAnalysisWindow(WDL_TypedBuf<double> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        double winCoeff = mAnalysisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj12::ApplyAnalysisWindowInv(WDL_TypedBuf<double> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        double winCoeff = mAnalysisWindow.Get()[i];
        
        winCoeff = 1.0 - winCoeff;
        
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj12::ApplySynthesisWindow(WDL_TypedBuf<double> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary
    // (no synthesis window, or no overlap)
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        double winCoeff = mSynthesisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj12::ApplySynthesisWindowNorm(WDL_TypedBuf<double> *samples)
{
    double lostSum = 0.0;
    double keepSum = 0.0;
    
    // Apply window and compute keep and lost sums
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        double winCoeff = mSynthesisWindow.Get()[i];
        double sample = samples->Get()[i];
        
        keepSum += winCoeff*fabs(sample);
        lostSum += (1.0 - winCoeff)*fabs(sample);
        
        sample *= winCoeff;
        
        samples->Get()[i] = sample;
    }
    
    double coeff = 1.0;
    if (keepSum > 0.0)
        coeff = (keepSum + lostSum)/keepSum;
    
    // WHY ?
    coeff *= 0.5;
    
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        double sample = samples->Get()[i];
        
        sample *= coeff;
        
        samples->Get()[i] = sample;
    }
}

void
FftProcessObj12::CorrectEnveloppe(WDL_TypedBuf<double> *samples,
                                  const WDL_TypedBuf<double> &envelope0,
                                  const WDL_TypedBuf<double> &envelope1)
{
#define EPS 1e-15
    
    for (int i = 0; i < samples->GetSize(); i++)
    {
        double sample = samples->Get()[i];
        
        double env0 = envelope0.Get()[i];
        double env1 = envelope1.Get()[i];
        
        double coeff = 1.0;
        
        if ((env0 > EPS) && (env1 > EPS))
            coeff = env0/env1;
        
        sample *= coeff;
        
        samples->Get()[i] = sample;
    }
}

void
FftProcessObj12::MakeWindows()
{
    // Analysis and synthesis Hann windows
    // With Hanning, the gain is not increased when we increase the overlapping
    
    int anaWindowSize = mBufSize;
    
    int synWindowSize = mBufSize;
    
#if ADD_TAIL
    synWindowSize *= mFreqRes;
#endif
    
    //
    // Hanning pow is used to narrow the analysis and synthesis windows
    // as much as possible, to have more "horizontal blur" in the spectrograms
    // (this reduces the attachs otherwise)
    //
    // Overlapping must be Hanning pow x 2 at the minimum
    // otherwise, we loose COLA condition
    //
    // Be careful, when using both analysis and synthesis windows,
    // it is as the Hanning pow was twice
    // (we have ana*syn equivalent to han*han).
    //
    //
    // NOTE: in the case of overlapping = 2, and two windows,
    // we will apply automatically the root Hanning, which is logical
    //
    // See: https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    // and
    //  https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    // Section: "Weighted Overlap Add"
    //
    // NOTE: here, we choose the maximum narrow value. Maybe we can narrow less by default
    //
    double hanningFactor = 1.0;
    if (mVariableHanning && (mOverlapping > 1))
    {
        if ((mAnalysisMethod == AnalysisMethodWindow) ||
            (mSynthesisMethod == SynthesisMethodWindow))
            // We happly windows twice (before, and after)
            hanningFactor = mOverlapping/4.0;
        else
            // We apply only one window, so we can narrow more
            hanningFactor = mOverlapping/2.0;
    }
    
    // Analysis
    if ((mAnalysisMethod == AnalysisMethodWindow) && (mOverlapping > 1))
        Window::MakeHanningPow(anaWindowSize, hanningFactor, &mAnalysisWindow);
    else
        Window::MakeSquare(anaWindowSize, 1.0, &mAnalysisWindow);
    
#if DEBUG_WINDOWS
    double cola0 = Window::CheckCOLA(&mAnalysisWindow, mOverlapping);
   fprintf(stderr, "ANALYSIS - over: %d cola: %g\n", mOverlapping, cola0);
#endif
    
    // Synthesis
    if ((mSynthesisMethod == SynthesisMethodWindow) && (mOverlapping > 1))
        Window::MakeHanningPow(synWindowSize, hanningFactor, &mSynthesisWindow);
    else
        Window::MakeSquare(synWindowSize, 1.0, &mSynthesisWindow);
    
#if DEBUG_WINDOWS
    double cola1 = Window::CheckCOLA(&mSynthesisWindow, mOverlapping);
    fprintf(stderr, "SYNTHESIS - over: %d cola: %g\n", mOverlapping, cola1);
    fprintf(stderr, "\n");
#endif
    
    // Normalize
    if ((mAnalysisMethod == AnalysisMethodWindow) &&
        (mSynthesisMethod == SynthesisMethodNone))
    {
        Window::NormalizeWindow(&mAnalysisWindow, mOverlapping);
    }
    else if((mAnalysisMethod == AnalysisMethodNone) &&
            (mSynthesisMethod == SynthesisMethodWindow))
    {
        Window::NormalizeWindow(&mSynthesisWindow, mOverlapping);
    }
    else if((mAnalysisMethod == AnalysisMethodWindow) &&
            (mSynthesisMethod == SynthesisMethodWindow))
    {
        // Must make the test
        // Otherwise, when overlapping is 1 (no overlapping),
        // we have two rectangular windows, and cola returns -1
        if (mOverlapping > 1)
        {
            // We have both windows
            // We must compute the cola of the next hanning factor,
            // then normalize with it
            //
        
            WDL_TypedBuf<double> win;
            Window::MakeHanningPow(mAnalysisWindow.GetSize(), hanningFactor*2, &win);
        
            double cola = Window::CheckCOLA(&win, mOverlapping);
        
            // Normalize only the synthesis window
            Window::NormalizeWindow(&mSynthesisWindow, cola);
        }
    }
    else if ((mAnalysisMethod == AnalysisMethodNone) &&
             (mSynthesisMethod == SynthesisMethodNone))
    {
        // Normalize only the synthesis window...
        Window::NormalizeWindow(&mSynthesisWindow, mOverlapping);
    }
}

void
FftProcessObj12::ComputeFft(const WDL_TypedBuf<double> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            int freqRes)
{
    int bufSize = samples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    fftSamples->Resize(bufSize);
    
    double coeff = 1.0;
#if NORMALIZE_FFT
    coeff = 1.0/bufSize;
#endif
    
    // Fill the fft buf
    for (int i = 0; i < bufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        tmpFftBuf.Get()[i].re = samples.Get()[i]*coeff;
        tmpFftBuf.Get()[i].im = 0.0;
    }
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    for (int i = 0; i < bufSize; i++)
        tmpFftBuf.Get()[i].re *= freqRes;
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(tmpFftBuf.Get(), bufSize, false);
    
    // Sort the fft buffer
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);

        fftSamples->Get()[i].re = tmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = tmpFftBuf.Get()[k].im;
    }
}

void
FftProcessObj12::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                   WDL_TypedBuf<double> *samples,
                                   int freqRes)
{
    int bufSize = fftSamples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    //
    samples->Resize(fftSamples.GetSize());
    
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);
        
        tmpFftBuf.Get()[k].re = fftSamples.Get()[i].re;
        tmpFftBuf.Get()[k].im = fftSamples.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(tmpFftBuf.Get(), bufSize, true);

    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] = tmpFftBuf.Get()[i].re;
    
    // fft
    double coeff = 1.0;
#if !NORMALIZE_FFT
    // Not sure about that...
    //coeff = bufSize;
#endif
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    coeff *= 1.0/freqRes;
    
    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] *= coeff;
}
