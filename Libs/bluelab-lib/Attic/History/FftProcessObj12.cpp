//
//  FftProcessObj12.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <BLUtils.h>
#include <DebugGraph.h>

#include "FftProcessObj12.h"

// BUG: the transientness curve displays the borders of the window
// that are detected as transients
//
// BUG: performance problem ?


#define USE_DEBUG_GRAPH 0

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

void
FftProcessObj12::Init()
{
    // Init WDL FFT
    WDL_fft_init();
}

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
    
    // DEBUG
    mCorrectEnvelope1 = false;
    mEnvelopeAutoAlign = false;
    mCorrectEnvelope2 = false;
    
    mNoOutput = false;
    
    mDebugMode = false;
    
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
    
    MakeWindows(mBufSize, mOverlapping,
                mAnalysisMethod, mSynthesisMethod, mVariableHanning,
                &mAnalysisWindow, &mSynthesisWindow);
    
    // Set the buffers.
    int resultSize = 2*mBufSize*mFreqRes;
    BLUtils::ResizeFillZeros(&mResultChunk, resultSize);
    
    mBufOffset = 0;
    
    int bufSize = mBufSize*mFreqRes;
    BLUtils::ResizeFillZeros(&mFftBuf, bufSize);
    BLUtils::ResizeFillZeros(&mResultTmpChunk, bufSize);
   
    BLUtils::ResizeFillZeros(&mFftBufSc, bufSize);
    
    mSamplesChunk.Resize(0);
    mScChunk.Resize(0);
    
    mResultOutChunk.Resize(0);
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
}

void
FftProcessObj12::DBG_SetCorrectEnvelope1(bool flag)
{
    mCorrectEnvelope1 = flag;
}

void
FftProcessObj12::DBG_SetEnvelopeAutoAlign(bool flag)
{
    mEnvelopeAutoAlign = flag;
}

void
FftProcessObj12::DBG_SetCorrectEnvelope2(bool flag)
{
    mCorrectEnvelope2 = flag;
}

void
FftProcessObj12::SetDebugMode(bool flag)
{
    mDebugMode = flag;
}

void
FftProcessObj12::AddSamples(BL_FLOAT *samples, BL_FLOAT *scSamples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
    
    if (scSamples != NULL)
        mScChunk.Add(scSamples, numSamples);
}

void
FftProcessObj12::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) {}

void
FftProcessObj12::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                     WDL_TypedBuf<BL_FLOAT> *scBuffer) {}

void
FftProcessObj12::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) {}

void
FftProcessObj12::PostProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) {}

void
FftProcessObj12::UnapplyAnalysisWindowSamples(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    BLUtils::UnapplyWindow(ioBuffer, mAnalysisWindow, 2);
}

void
FftProcessObj12::UnapplyAnalysisWindowFft(WDL_TypedBuf<BL_FLOAT> *magns,
                                          const WDL_TypedBuf<BL_FLOAT> &phases)
{
    BLUtils::UnapplyWindowFft(magns, phases, mAnalysisWindow, 2);
}

void
FftProcessObj12::SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                              WDL_TypedBuf<BL_FLOAT> *outFftMagns,
                              WDL_TypedBuf<BL_FLOAT> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    BLUtils::ComplexToMagnPhase(outFftMagns, outFftPhases, fftSamples);
}

void
FftProcessObj12::SamplesToFftMagns(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                   WDL_TypedBuf<BL_FLOAT> *outFftMagns)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    BLUtils::ComplexToMagn(outFftMagns, fftSamples);
}

void
FftProcessObj12::FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                              WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftBuffer.GetSize());
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj12::SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer)
{
    outFftBuffer->Resize(inSamples.GetSize());
    
    int freqRes = 1;
    ComputeFft(inSamples, outFftBuffer, freqRes);
}

void
FftProcessObj12::FftToSamples(const WDL_TypedBuf<BL_FLOAT> &fftMagns,
                              const  WDL_TypedBuf<BL_FLOAT> &fftPhases,
                              WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftMagns.GetSize());
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer;
    BLUtils::MagnPhaseToComplex(&fftBuffer, fftMagns, fftPhases);
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj12::ProcessOneBuffer()
{
    WDL_TypedBuf<BL_FLOAT> copySamplesBuf;
    copySamplesBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        copySamplesBuf.Get()[i] = mSamplesChunk.Get()[i + mBufOffset];
    WDL_TypedBuf<BL_FLOAT> copyScBuf;
    if (mScChunk.GetSize() > 0)
    {
        copyScBuf.Resize(mBufSize);
        for (int i = 0; i < mBufSize; i++)
            copyScBuf.Get()[i] = mScChunk.Get()[i + mBufOffset];
    }
    
    ProcessOneBuffer(copySamplesBuf, copyScBuf, &mResultTmpChunk);
    
#if !ADD_TAIL
    // Standard size
    int loopCount = mBufSize;
#else
    // Size to add tail
    int loopCount = copySamplesBuf.GetSize();
#endif
    
    // Fill the result
    BL_FLOAT *result = mResultChunk.Get();
    
    // Make the sum (overlap)
    for (int i = 0; i < loopCount; i++)
    {
        result[i + mBufOffset] += mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj12::ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &inBuffer,
                                  const WDL_TypedBuf<BL_FLOAT> &inScBuffer,
                                  WDL_TypedBuf<BL_FLOAT> *outBuffer)
{
    if (outBuffer != NULL)
        outBuffer->Resize(inBuffer.GetSize());
    
    WDL_TypedBuf<BL_FLOAT> inBufferCopy = inBuffer;
    WDL_TypedBuf<BL_FLOAT> inScBufferCopy = inScBuffer;
    
    //PreProcessSamplesBuffer(&copySamplesBuf);
    
    // Apply analysis windows before resizing !
    ApplyAnalysisWindow(&inBufferCopy);
    
    PreProcessSamplesBuffer(&inBufferCopy);
    
    // Compute "energy" just after analysis windowing
    // then just after synthesis windowing
    // Compute and apply a coefficient to avoid loosing energy
    BL_FLOAT energy0 = 0.0;
    if (mKeepSynthesisEnergy)
        energy0 = BLUtils::ComputeAbsAvg(inBufferCopy);
    
    // windowing before zero padding
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last links of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        BLUtils::ResizeFillZeros(&inBufferCopy, inBufferCopy.GetSize()*mFreqRes);
        
        if (mScChunk.GetSize() > 0)
            BLUtils::ResizeFillZeros(&inScBufferCopy, inScBufferCopy.GetSize()*mFreqRes);
    }
    
    if (mScChunk.GetSize() > 0)
        ApplyAnalysisWindow(&inScBufferCopy);
    
    if (mSkipFFT)
    {
#if EXP_SIDECHAIN_USE
        if (mScChunk.GetSize() == 0)
            // No side chain, take the input
        {
            // By default, do nothing with the fft
            outBuffer->Resize(0);
            
            // Be careful to not add the beginning of the buffer
            // (otherwise the windows will be applied twice sometimes).
            outBuffer->Add(inBufferCopy.Get(), mBufSize);
        }
        else
            // Side chain: take the side chain buffer
        {
            outBuffer->Resize(0);
            outBuffer->Add(inScBufferCopy.Get(), mBufSize);
        }
#else
        if (outBuffer != NULL)
            // By default, do nothing with the fft
            outBuffer->Resize(0);
        
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).
        mResultTmpChunk.Add(inBufferCopy.Get(), mBufSize);
#endif
    }
    else
    {
#if EXP_SIDECHAIN_USE
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        if (mScChunk.GetSize() == 0)
            // No side chain, take the input
            ComputeFft(inBufferCopy, &mSortedFftBuf, mFreqRes);
        else
            // Side chain: process the side chain
            ComputeFft(inScBufferCopy, &mSortedFftBuf, mFreqRes);
#else
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        ComputeFft(inBufferCopy, &mFftBuf, mFreqRes);
        
        if (mScChunk.GetSize() > 0)
            // Side chain: process the side chain
            ComputeFft(inScBufferCopy, &mFftBufSc, mFreqRes);
#endif
        
        //////
        WDL_TypedBuf<BL_FLOAT> samples0;
        FftProcessObj12::FftToSamples(mFftBuf, &samples0);
        
#if USE_DEBUG_GRAPH
        
        if (mDebugMode)
        {
#define GRAPH_MAX_VALUE 2.0
            DebugGraph::SetCurveValues(samples0,
                                       0,
                                       -GRAPH_MAX_VALUE, GRAPH_MAX_VALUE,
                                       1.0,
                                       0, 0, 255,
                                       false, 0.5);
        }
#endif
        
        WDL_TypedBuf<BL_FLOAT> envelope0;
        
        WDL_TypedBuf<BL_FLOAT> magns0;
        WDL_TypedBuf<BL_FLOAT> phases0;
        BLUtils::ComplexToMagnPhase(&magns0, &phases0, mFftBuf);
        
#if USE_DEBUG_GRAPH
        
        if (mDebugMode)
        {
            // TODO: factorize that, and make a method
#define MAGNS_COEFF 2.0
            BLUtils::TakeHalf(&magns0);
            BLUtils::TakeHalf(&magns0);
            BLUtils::TakeHalf(&magns0);
            BLUtils::TakeHalf(&magns0);
            BLUtils::TakeHalf(&magns0);
            
            BLUtils::ScaleNearest(&magns0, 32);
            
            BLUtils::MultValues(&magns0, MAGNS_COEFF);
            DebugGraph::SetCurveValues(magns0,
                                       2,
                                       0.0, 1.0,
                                       1.0,
                                       128, 128, 255,
                                       false, 0.5);
        }
#endif
        
        if (mCorrectEnvelope1 || mCorrectEnvelope2)
        {
            //BLUtils::ComputeEnvelope(samples0, &envelope0, false);
            BLUtils::ComputeEnvelopeSmooth2(samples0, &envelope0, 0.125);
            
            // We know that the window is 0 at the edges
            // So fix the envelope, to avoid further artifacts
            //BLUtils::ZeroBoundEnvelope(&envelope0);
            
#if USE_DEBUG_GRAPH
            if (mDebugMode)
            {
                DebugGraph::SetCurveValues(envelope0,
                                           1,
                                           -GRAPH_MAX_VALUE, GRAPH_MAX_VALUE,
                                           2.0,
                                           0, 0, 255,
                                           false, 0.5);
            }
#endif
        }
        /////////
        
        
#if !DEBUG_DISABLE_PROCESS
        // Apply modifications of the buffer
        if (mScChunk.GetSize() == 0)
            ProcessFftBuffer(&mFftBuf);
        else
            // It is a good idea to pass the sidechain like that
            // Sidechain is managed as samples buffers
            // So it is buffered exactly the same way (e.g in the case of overlapping)
            // ... which is good !
            ProcessFftBuffer(&mFftBuf, &mFftBufSc);
        
#endif
        
        ////////
        WDL_TypedBuf<BL_FLOAT> samples1;
        //FftProcessObj12::FftToSamples(mSortedFftBuf, &samples1);
        ComputeInverseFft(mFftBuf, &samples1, mFreqRes); // TEST
        
        //PostProcessSamplesBuffer(&samples1);
        
#if USE_DEBUG_GRAPH
        if (mDebugMode)
        {
            DebugGraph::SetCurveValues(samples1,
                                       3,
                                       -GRAPH_MAX_VALUE, GRAPH_MAX_VALUE,
                                       1.0,
                                       0, 255, 0,
                                       false, 0.5);
        }
#endif
        
        WDL_TypedBuf<BL_FLOAT> magns1;
        WDL_TypedBuf<BL_FLOAT> phases1;
        BLUtils::ComplexToMagnPhase(&magns1, &phases1, mFftBuf);
        
        BLUtils::TakeHalf(&magns1);
        BLUtils::TakeHalf(&magns1);
        BLUtils::TakeHalf(&magns1);
        BLUtils::TakeHalf(&magns1);
        BLUtils::TakeHalf(&magns1);
        
        BLUtils::ScaleNearest(&magns1, 32);
        
#define MAGNS_COEFF2 2.0
        BLUtils::MultValues(&magns1, MAGNS_COEFF2);
        
#if USE_DEBUG_GRAPH
        if (mDebugMode)
        {
            DebugGraph::SetCurveValues(magns1,
                                       5,
                                       0.0, 1.0,
                                       1.0,
                                       128, 255, 128,
                                       false, 0.5);
        }
#endif
        
        if (mCorrectEnvelope1 || mCorrectEnvelope2)
        {
            WDL_TypedBuf<BL_FLOAT> envelope1;
            //BLUtils::ComputeEnvelope(samples1, &envelope1, true);
            BLUtils::ComputeEnvelopeSmooth2(samples1, &envelope1, 0.125);
            
            if (mEnvelopeAutoAlign)
            {
                // TEST
                int precision = 1024;
                //int precision = 256;
                //int precision = 1;
                int shift = BLUtils::GetEnvelopeShift(envelope0, envelope1, precision);
                
                BLUtils::ShiftSamples(&samples1, -shift);
                
#if 0 // Shift envelope
                BLUtils::ShiftSamples(&envelope1, -shift);
#else
                // Recompute the envelope, to avoid a break in the
                // link prev begin / prev end
                //BLUtils::ComputeEnvelope(samples1, &envelope1, true);
                BLUtils::ComputeEnvelopeSmooth2(samples1, &envelope1, 0.125);
#endif
            }
            
            if (mCorrectEnvelope1 || mCorrectEnvelope2)
            {
                if (mCorrectEnvelope1)
                {
                    BLUtils::CorrectEnvelope(&samples1, envelope0, envelope1);
                }
                else if (mCorrectEnvelope2)
                {
                    BL_FLOAT e0 = BLUtils::ComputeAbsAvg(samples1);
                    
                    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples1;
                    ComputeFft(samples1, &fftSamples1, mFreqRes);
                    
                    // Better with envelope0
                    //
                    // (with samples, and without normalization, the bubble makes pulsations)
                    //
                    ApplyInverseWindow(&fftSamples1, mAnalysisWindow, &envelope0);
                    
                    ComputeInverseFft(fftSamples1, &samples1, mFreqRes);
                    
#if 1 // No need ?
                    ApplyAnalysisWindow(&samples1);
#endif
                    
#if 1 // Good
                    BL_FLOAT max0 = BLUtils::ComputeMaxAbs(samples0);
                    BL_FLOAT max1 = BLUtils::ComputeMaxAbs(samples1);
                    if (max1 > 0.0)
                    {
                        BL_FLOAT coeff = max0/max1;
                        BLUtils::MultValues(&samples1, coeff);
                    }
                    
                    // TEST
                    ApplyAnalysisWindow(&samples1);
#endif
                    
#if 0 // Goes a little over envelope0
                    BL_FLOAT e1 = BLUtils::ComputeAbsAvg(samples1);
                    if (e1 > 0.0)
                    {
                        BL_FLOAT coeff = e0/e1;
                        BLUtils::MultValues(&samples1, coeff);
                    }
#endif
                    
                }
            }
        }
        
#if USE_DEBUG_GRAPH
        if (mDebugMode)
        {
            DebugGraph::SetCurveValues(samples1,
                                       6,
                                       -GRAPH_MAX_VALUE, GRAPH_MAX_VALUE,
                                       1.0,
                                       255, 255, 255,
                                       false, 0.5);
        }
#endif
        
        //////////
        
        if (outBuffer != NULL)
            *outBuffer = samples1;
        
        // TODO: freq res...
        //ComputeInverseFft(mSortedFftBuf, &mResultTmpChunk, mFreqRes);
        
        //////////
        
    }
    
    if (outBuffer != NULL)
    {
#if !DEBUG_DISABLE_PROCESS
        if (mScChunk.GetSize() == 0)
            // No side chain
            ProcessSamplesBuffer(outBuffer, NULL);
        else
            ProcessSamplesBuffer(&inBufferCopy, outBuffer);
    }
#endif
    
#if USE_DEBUG_GRAPH
    if (mDebugMode)
    {
        WDL_TypedBuf<BL_FLOAT> resultMagns;
        WDL_TypedBuf<BL_FLOAT> resultPhases;
        FftProcessObj12::SamplesToFft(*outBuffer, &resultMagns, &resultPhases);
        
        BLUtils::TakeHalf(&resultMagns);
        BLUtils::TakeHalf(&resultPhases);
        DebugGraph::AddSpectrogramLine(resultMagns, resultPhases);
    }
#endif
    
    if (outBuffer != NULL)
        PostProcessSamplesBuffer(outBuffer);
    
    if (mNoOutput || (outBuffer == NULL))
        // We have finished
        return;
    
    if (outBuffer != NULL)
        // Apply synthesis window
        ApplySynthesisWindow(outBuffer);
    
    if (mKeepSynthesisEnergy)
    {
        BL_FLOAT energy1 = BLUtils::ComputeAbsAvg(*outBuffer);
        
        BL_FLOAT energyCoeff = 1.0;
        if (energy1 > 0)
            energyCoeff = energy0/energy1;
        
        // If mOverlapping is 1, thn div is 0
        if (mOverlapping > 1)
        {
	  BL_FLOAT div = std::log(mOverlapping)/std::log(2.0);
            energyCoeff /= div;
            
            BLUtils::MultValues(outBuffer, energyCoeff);
        }
    }
}

void
FftProcessObj12::NextSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Reduce the size by mOverlap
    int size = samples->GetSize();
    if (size == mBufSize)
        samples->Resize(0);
    else if (size > mBufSize)
    {
        int newSize = size - mBufSize;
            
        // Resize down, skipping left
        WDL_TypedBuf<BL_FLOAT> tmpChunk;
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
    
    BL_FLOAT *resultBuf = mResultChunk.Get();
    
    WDL_TypedBuf<BL_FLOAT> newBuffer;
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
        WDL_TypedBuf<BL_FLOAT> tmpChunk;
        tmpChunk.Add(&mResultChunk.Get()[mBufSize], newSize);
        mResultChunk = tmpChunk;
    }
    
    // Grow the output with zeros
    BLUtils::ResizeFillZeros(&mResultChunk,
                           mResultChunk.GetSize() + mBufSize);
}

void
FftProcessObj12::GetResultOutBuffer(BL_FLOAT *output, int nFrames)
{
    int size = mResultOutChunk.GetSize();
 
    // Should not happen
    if (size < nFrames)
        return;
    
    // Copy the result
    memcpy(output, mResultOutChunk.Get(), nFrames*sizeof(BL_FLOAT));
    
    // Consume the returned samples
    int newSize = mResultOutChunk.GetSize() - nFrames;
    
    // Resize down, skipping left
    WDL_TypedBuf<BL_FLOAT> tmpChunk;
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
FftProcessObj12::Process(BL_FLOAT *input, BL_FLOAT *output,
                         BL_FLOAT *inSc, int nFrames)
{
#if DEBUG_WINDOWS
    for (int i = 0; i < nFrames; i++)
        input[i] = 1.0;
#endif
    
    mNoOutput = (output == NULL);
    
    // Add the new input data
    AddSamples(input, inSc, nFrames);
    
    if (mMustPadSamples)
    {
        BLUtils::PadZerosLeft(&mSamplesChunk, mAnalysisWindow.GetSize());
        
        if (mScChunk.GetSize() > 0)
            BLUtils::PadZerosLeft(&mScChunk, mAnalysisWindow.GetSize());
        
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
            BLUtils::ConsumeLeft(&mResultOutChunk, mAnalysisWindow.GetSize());
            
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
FftProcessObj12::NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns)
{
    BL_FLOAT sum = 0.0f;
    
    // Not test "/2"
    for (int i = 1; i < magns->GetSize()/*/2*/; i++)
    {
        BL_FLOAT magn = magns->Get()[i];
        
        sum += magn;
    }
    
    sum /= magns->GetSize()/*/2*/ - 1;
    
    magns->Get()[0] = sum;
    magns->Get()[magns->GetSize() - 1] = sum;
}


void
FftProcessObj12::ApplyAnalysisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT winCoeff = mAnalysisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj12::ApplyAnalysisWindowInv(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT winCoeff = mAnalysisWindow.Get()[i];
        
        winCoeff = 1.0 - winCoeff;
        
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj12::ApplySynthesisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary
    // (no synthesis window, or no overlap)
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        BL_FLOAT winCoeff = mSynthesisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj12::ApplySynthesisWindowNorm(WDL_TypedBuf<BL_FLOAT> *samples)
{
    BL_FLOAT lostSum = 0.0;
    BL_FLOAT keepSum = 0.0;
    
    // Apply window and compute keep and lost sums
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        BL_FLOAT winCoeff = mSynthesisWindow.Get()[i];
        BL_FLOAT sample = samples->Get()[i];
        
        keepSum += winCoeff*std::fabs(sample);
        lostSum += (1.0 - winCoeff)*std::fabs(sample);
        
        sample *= winCoeff;
        
        samples->Get()[i] = sample;
    }
    
    BL_FLOAT coeff = 1.0;
    if (keepSum > 0.0)
        coeff = (keepSum + lostSum)/keepSum;
    
    // WHY ?
    coeff *= 0.5;
    
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        BL_FLOAT sample = samples->Get()[i];
        
        sample *= coeff;
        
        samples->Get()[i] = sample;
    }
}

void
FftProcessObj12::ApplyInverseWindow(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                    const WDL_TypedBuf<BL_FLOAT> &window,
                                    const WDL_TypedBuf<BL_FLOAT> *originEnvelope)
{
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, *fftSamples);
    
    ApplyInverseWindow(&magns, phases, window, originEnvelope);
    
    BLUtils::MagnPhaseToComplex(fftSamples, magns, phases);
}

void
FftProcessObj12::ApplyInverseWindow(WDL_TypedBuf<BL_FLOAT> *magns,
                                    const WDL_TypedBuf<BL_FLOAT> &phases,
                                    const WDL_TypedBuf<BL_FLOAT> &window,
                                    const WDL_TypedBuf<BL_FLOAT> *originEnvelope)
{
#define WIN_EPS 1e-3
    
    // Suppresses the amplification of noise at the border of the wndow
#define MAGN_EPS 1e-6
    
    WDL_TypedBuf<int> samplesIds;
    BLUtils::FftIdsToSamplesIds(phases, &samplesIds);
    
    const WDL_TypedBuf<BL_FLOAT> origMagns = *magns;
    
    for (int i = 0; i < magns->GetSize(); i++)
    //for (int i = 1; i < magns->GetSize() - 1; i++)
    {
        int sampleIdx = samplesIds.Get()[i];
        
        BL_FLOAT magn = origMagns.Get()[i];
        BL_FLOAT win1 = window.Get()[sampleIdx];
        
        BL_FLOAT coeff = 0.0;
        
        if (win1 > WIN_EPS)
            coeff = 1.0/win1;
        
        //coeff = win1;
        
        // Better with
        if (originEnvelope != NULL)
        {
            BL_FLOAT originSample = originEnvelope->Get()[sampleIdx];
            coeff *= std::fabs(originSample);
        }
        
        if (magn > MAGN_EPS) // TEST
            magn *= coeff;
        
#if 0
        // Just in case
        if (magn > 1.0)
            magn = 1.0;
        if (magn < -1.0)
            magn = -1.0;
#endif
        
        magns->Get()[i] = magn;
    }
}

void
FftProcessObj12::MakeWindows(int bufSize, int overlapping,
                             enum AnalysisMethod analysisMethod,
                             enum SynthesisMethod synthesisMethod,
                             bool variableHanning,
                             WDL_TypedBuf<BL_FLOAT> *analysisWindow,
                             WDL_TypedBuf<BL_FLOAT> *synthesisWindow)
{
    // Analysis and synthesis Hann windows
    // With Hanning, the gain is not increased when we increase the overlapping
    
    int anaWindowSize = bufSize;
    
    int synWindowSize = bufSize;
    
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
    BL_FLOAT hanningFactor = 1.0;
    if (variableHanning && (overlapping > 1))
    {
        if ((analysisMethod == AnalysisMethodWindow) ||
            (synthesisMethod == SynthesisMethodWindow))
            // We happly windows twice (before, and after)
            hanningFactor = overlapping/4.0;
        else
            // We apply only one window, so we can narrow more
            hanningFactor = overlapping/2.0;
    }
    
    // Analysis
    //
    // NOTE: if no overlapping, it is sometimes better to even have an analysis window !
    //
    if ((analysisMethod == AnalysisMethodWindow) && (overlapping > 1))
        Window::MakeHanningPow(anaWindowSize, hanningFactor, analysisWindow);
    else
        Window::MakeSquare(anaWindowSize, 1.0, analysisWindow);
    
#if DEBUG_WINDOWS
    BL_FLOAT cola0 = Window::CheckCOLA(&mAnalysisWindow, mOverlapping);
   fprintf(stderr, "ANALYSIS - over: %d cola: %g\n", mOverlapping, cola0);
#endif
    
    // Synthesis
    if ((synthesisMethod == SynthesisMethodWindow) && (overlapping > 1))
        Window::MakeHanningPow(synWindowSize, hanningFactor, synthesisWindow);
    else
        Window::MakeSquare(synWindowSize, 1.0, synthesisWindow);
    
#if DEBUG_WINDOWS
    BL_FLOAT cola1 = Window::CheckCOLA(synthesisWindow, overlapping);
    fprintf(stderr, "SYNTHESIS - over: %d cola: %g\n", overlapping, cola1);
    fprintf(stderr, "\n");
#endif
    
    // Normalize
    if ((analysisMethod == AnalysisMethodWindow) &&
        (synthesisMethod == SynthesisMethodNone))
    {
        Window::NormalizeWindow(analysisWindow, overlapping);
    }
    else if((analysisMethod == AnalysisMethodNone) &&
            (synthesisMethod == SynthesisMethodWindow))
    {
        Window::NormalizeWindow(synthesisWindow, overlapping);
    }
    else if((analysisMethod == AnalysisMethodWindow) &&
            (synthesisMethod == SynthesisMethodWindow))
    {
        // Must make the test
        // Otherwise, when overlapping is 1 (no overlapping),
        // we have two rectangular windows, and cola returns -1
        if (overlapping > 1)
        {
            // We have both windows
            // We must compute the cola of the next hanning factor,
            // then normalize with it
            //
        
            WDL_TypedBuf<BL_FLOAT> win;
            Window::MakeHanningPow(analysisWindow->GetSize(), hanningFactor*2, &win);
        
            BL_FLOAT cola = Window::CheckCOLA(&win, overlapping);
        
            // Normalize only the synthesis window
            Window::NormalizeWindow(synthesisWindow, cola);
        }
    }
    else if ((analysisMethod == AnalysisMethodNone) &&
             (synthesisMethod == SynthesisMethodNone))
    {
        // Normalize only the synthesis window...
        Window::NormalizeWindow(synthesisWindow, overlapping);
    }
}

void
FftProcessObj12::ComputeFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            int freqRes)
{
    int bufSize = samples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    fftSamples->Resize(bufSize);
    
    BL_FLOAT coeff = 1.0;
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
                                   WDL_TypedBuf<BL_FLOAT> *samples,
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
    BL_FLOAT coeff = 1.0;
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
