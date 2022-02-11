//
//  FftProcessObj.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <Utils.h>
#include <Debug.h>

#include "FftProcessObj6.h"


// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0
#define ZERO_PAD_WINDOW_REORDER 0

// Useful for debuging
#define DEBUG_DISABLE_OVERSAMPLING 0
#define DEBUG_DISABLE_PROCESS 0


// NOTE: With freqRes == 2, and oversampling = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj6::FftProcessObj6(int bufferSize, int oversampling, int freqRes,
                               bool normalize,
                               enum AnalysisMethod aMethod,
                               enum SynthesisMethod sMethod, bool skipFFT)
{
    mBufSize = bufferSize;
    mOversampling = oversampling;
    mFreqRes = freqRes;
    
#if DEBUG_DISABLE_OVERSAMPLING
    mOversampling = 1;
#endif

    mShift = mBufSize/mOversampling;
    mOverlap = mBufSize - mShift;
    
    mNormalize = normalize;
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    mSkipFFT = skipFFT;
    
    // Analysis and synthesis Hann windows
    // We take the root as explained above, otherwise it will make low oscillations
    // To check oscillations:
    // - pass a white noise inside the algorithm, without any additional processing
    // - visualize the result signal from 1 to 5000
    //
    // Explanation of thaking the root, both for analysis and synthesis:
    // https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    // Section: "Weighted Overlap Add"
    //Window::MakeRootHanning2(mBufSize, &mAnalysisWindow);
    //Window::MakeRootHanning2(mBufSize, &mSynthesisWindow);
    
    // Better with Hanning
    // With Hanning, the gain is not increased when we increase the oversampling
#if !ZERO_PAD_WINDOW_REORDER
    Window::MakeHanning(mBufSize, &mAnalysisWindow);
    Window::MakeHanning(mBufSize, &mSynthesisWindow);
    
    // With factor = 2, overlap must be >= 4
    // With factor = 4, overlap must be >= 8
    //double factor = 4.0;
    //Window::MakeHanningPow(mBufSize, factor, &mAnalysisWindow);
    //Window::MakeHanningPow(mBufSize, factor, &mSynthesisWindow);
    
    //double cola = Window::CheckCOLA(&mAnalysisWindow, mOversampling);
    //fprintf(stderr, "win-size: %d oversamp: %d COLA sum: %g\n", mBufSize, mOversampling, cola);
    
#else
    Window::MakeHanning(mBufSize*mFreqRes, &mAnalysisWindow);
    Window::MakeHanning(mBufSize*mFreqRes, &mSynthesisWindow);
#endif
    
    // Comppute energy factor
    // Useful to keep the same volume even if we have oversampling > 2 with Hanning
    mAnalysisWinFactor = ComputeHanningWinFactor(mAnalysisWindow);
    mSynthesisWinFactor = ComputeHanningWinFactor(mSynthesisWindow);
    
    mOversamplingFactor = ComputeOversamplingFactor();
    
    Reset();
}

FftProcessObj6::~FftProcessObj6() {}

void
FftProcessObj6::Reset()
{
    // Set the buffers.
    int resultSize = 2*mBufSize;
    resultSize *= mFreqRes;
    
    double *zeros = (double *)malloc(resultSize*sizeof(double));
    for (int i = 0; i < resultSize; i++)
        zeros[i] = 0.0;
    
    mBufFftChunk.Resize(0);
    mBufFftChunk.Add(zeros, mBufSize);
    
    mBufIfftChunk.Resize(0);
    mBufIfftChunk.Add(zeros, mBufSize);
    
    mResultChunk.Resize(0);
    mResultChunk.Add(zeros, resultSize);
    
    free(zeros);
    
    mBufOffset = 0;
    
    int bufSize = mBufSize*mFreqRes;
    mFftBuf.Resize(bufSize);
    mSortedFftBuf.Resize(bufSize);
    mResultTmpChunk.Resize(bufSize);
}

void
FftProcessObj6::AddSamples(double *samples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
}

void
FftProcessObj6::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj6::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer) {}

void
FftProcessObj6::ProcessOneBuffer()
{
    WDL_TypedBuf<double> copySamplesBuf;
    copySamplesBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        copySamplesBuf.Get()[i] = mSamplesChunk.Get()[i + mBufOffset];
    
#if ZERO_PAD_WINDOW_REORDER
    // Zero padding before windowing
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last liens of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        Utils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
    }
#endif
    
    double *samples = copySamplesBuf.Get();
    
    //if (!mSkipFFT)
    // If we skip the fft, we multiply only once at the end by the overlap window
    {
        if (mAnalysisMethod == AnalysisMethodWindow)
        {
            // Windowing
            if (mOverlap > 0) // then should be half the buffer size
            {
                // Multiply the samples by Hanning window
                for (int i = 0; i < copySamplesBuf.GetSize(); i++)
                {
                    samples[i] *= mAnalysisWindow.Get()[i];
                
                    samples[i] /= mAnalysisWinFactor;
                }
            }
        }
    }
    
    if (mSkipFFT)
    {
        // By default, do nothing with the fft
        mResultTmpChunk.Resize(0);
        
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).

        //mResultTmpChunk.Add(&samples[mBufOffset], mBufSize);
        mResultTmpChunk.Add(samples, mBufSize);
    }
    else
    {
#if !ZERO_PAD_WINDOW_REORDER
        if (mFreqRes > 1)
        {
            // Non-cyclic technique, to avoid aliasing
            Utils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
        }
#endif
        
#if 0
        static int count = 0;
        int maxCount = 50;
        if (count == maxCount)
        {
            Debug::DumpData("insamples.txt", copySamplesBuf.Get(), copySamplesBuf.GetSize());
        }
#endif
        
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        
        ComputeFft(&copySamplesBuf, &mSortedFftBuf);
        
#if !DEBUG_DISABLE_PROCESS
        // Apply modifications of the buffer
        ProcessFftBuffer(&mSortedFftBuf);
#endif
        
        ComputeInverseFft(&mSortedFftBuf, &mResultTmpChunk);
      
#if 0
        if (count == maxCount)
        {
            Debug::DumpData("outsamples.txt", mResultTmpChunk.Get(), mResultTmpChunk.GetSize());
        }
        
        count++;
#endif
    }
    
#if !DEBUG_DISABLE_PROCESS
    ProcessSamplesBuffer(&mResultTmpChunk);
#endif
    
    // Fill the result
    double *result = mResultChunk.Get();

    // Standard loop count
    int loopCount = mBufSize;
    
#if ADD_TAIL
    if (mFreqRes > 1)
        // Grater size
        loopCount = copySamplesBuf.GetSize();
#endif
    
    for (int i = 0; i < loopCount; i++)
        // Finally, no need to multiply by Hanning, because the data is already multiplied (at the beginning)
        // So simply overlap.
    {
        // Default value, for no window (SynthesisMethodNone)
        double winCoeff = 1.0;
        
        if (!mSkipFFT)
        {
            if (mOverlap > 0)
            {
                int winIndex = i;
                
                // In the case of adding tail...
                
                // We use a standard synthesis window size
                // For the remaining samples of the tail, do as
                // if it is the next step, and use the same window, but shifted
                winIndex = winIndex % mSynthesisWindow.GetSize();

                if (mSynthesisMethod == SynthesisMethodWindow)
                    winCoeff = mSynthesisWindow.Get()[winIndex];
                else
                    if (mSynthesisMethod == SynthesisMethodInverseWindow)
                    {
                        winCoeff = mAnalysisWindow.Get()[winIndex];
                            
                        winCoeff = 1.0 - winCoeff;
                
                        // Just in case, but should not happen with Hanning
                        if (winCoeff < 0.0)
                            winCoeff = 0.0;
                    }
            
                    winCoeff /= mSynthesisWinFactor;
            }
        }
                
        // Hack
        double skipCoeff = 1.0;
        if (mSkipFFT)
            // We haven't done analysis...
            skipCoeff = 1.0/4.0;
        
        result[i + mBufOffset] += skipCoeff*mOversamplingFactor*winCoeff*mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj6::NextSamplesBuffer()
{
    // Reduce the size by mOverlap
    int size = mSamplesChunk.GetSize();
    if (size == mBufSize)
        mSamplesChunk.Resize(0);
    else
        if (size > mBufSize)
        {
            int newSize = size - mBufSize;
            
            // Resize down, skipping left
            WDL_TypedBuf<double> tmpChunk;
            tmpChunk.Add(&mSamplesChunk.Get()[mBufSize], newSize);
            mSamplesChunk = tmpChunk;
        }
}

void
FftProcessObj6::NextOutBuffer()
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
    else
        if (size > mBufSize)
        {
            int newSize = size - mBufSize;
            
            // Resize down, skipping left
            WDL_TypedBuf<double> tmpChunk;
            tmpChunk.Add(&mResultChunk.Get()[mBufSize], newSize);
            mResultChunk = tmpChunk;
        }
    
    // Grow the output with zeros
    double *zeros = (double *)malloc(mBufSize*sizeof(double));
    for (int i = 0; i < mBufSize; i++)
        zeros[i] = 0.0;
    mResultChunk.Add(zeros, mBufSize);
    free(zeros);
}

void
FftProcessObj6::GetResultOutBuffer(double *output, int nFrames)
{
    // Should not happen
    int size = mResultOutChunk.GetSize()/mFreqRes;
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
FftProcessObj6::Shift()
{
    mBufOffset += mShift;
    
    if (mBufOffset >= mBufSize)
    {
        mBufOffset = 0;
        
        return true;
    }
    
    return false;
}

// NOTE: tested with combinations of BUFFER_SIZE, OVERSAMPLING and FREA_RES,
// with the the ProcessFft() method disabled
// => the intensity of the result is constant !
double
FftProcessObj6::ComputeHanningWinFactor(const WDL_TypedBuf<double> &window)
{
    if (mOversampling == 1)
        return 1.0;
    
    return 0.5;
    
    // Old version, doesn't work anymore since correction of the overlap method
#if 0
    // Hard coded values
    // (but they work !)
    switch(mOversampling)
    {
        case 1:
            // No windowing
            return 1.0;
            break;
        
        case 2:
            return 0.5; // 0.25 too high 0.5 ok
            break;
            
        case 4:
            return 0.5;//0.5ok
            break;
            
        case 8:
            return 0.7; // 1: too low 2 too low ++ 0.5 too high 0.75 a bit too low 0.7: ok
            break;
        
        case 16:
            return 1.0; // 0.7 too high, 1: ok
            break;
        
        case 32:
            return 1.5; //2.0 too low 1.5 ok
            break;
        
        case 64:
            return 2.0; // 1.8 too high
            break;
            
        default:
            break;
    }
    
    return 1.0;
#endif
    
#if 0
    // Sum the values of the synthesis windows
    // For Hanning, this should be 0.5
    for (int i = 0; i < window.GetSize(); i++)
    {
        double winCoeff = window.Get()[i];
        
        factor += winCoeff;
    }
    
    if (window.GetSize() > 0)
        factor /= window.GetSize();
#endif
}

// NOTE: tested with combinations of BUFFER_SIZE, OVERSAMPLING and FREA_RES,
// with the the ProcessFft() method disabled
// => the intensity of the result is constant !
double
FftProcessObj6::ComputeOversamplingFactor()
{
    if (mOversampling == 1)
        return 1.0;
    
    return 1.0/(2.0*mOversampling);    
}


// NOTE: this code works well for 512, 1024, and 2048
bool
FftProcessObj6::Process(double *input, double *output, int nFrames)
{
    // Add the new input data
    AddSamples(input, nFrames);
    
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
        
        NextSamplesBuffer();
        NextOutBuffer();
        
        numSamples = mSamplesChunk.GetSize();
    }
    
    // Get the computed result
    int numOutSamples = mResultOutChunk.GetSize();
    if (nFrames <= numOutSamples)
    {
        if (output != NULL)
        {
            GetResultOutBuffer(output, nFrames);
        }
        
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
FftProcessObj6::NormalizeFftValues(WDL_TypedBuf<double> *magns)
{
    double sum = 0.0f;
    
    for (int i = 1; i < magns->GetSize()/2; i++)
    {
        double magn = magns->Get()[i];
        
        sum += magn;
    }
    
    sum /= magns->GetSize()/2 - 1;
    
    magns->Get()[0] = sum;
    magns->Get()[magns->GetSize() - 1] = sum;
}

void
FftProcessObj6::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    //for (int i = 0; i < ioBuffer->GetSize()/2; i++)
    for (int i = 1; i < ioBuffer->GetSize()/2; i++) // Zarlino
    {
        int id0 = i + ioBuffer->GetSize()/2;
        
        // Zarlino modif
        //int id1 = ioBuffer->GetSize()/2 - i - 1;
        int id1 = ioBuffer->GetSize()/2 - i;
        
        ioBuffer->Get()[id0].re = ioBuffer->Get()[id1].re;
        
        // Complex conjugate
        ioBuffer->Get()[id0].im = -ioBuffer->Get()[id1].im;
    }
}

void
FftProcessObj6::ComputeFft(const WDL_TypedBuf<double> *samples,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples)
{
    int bufSize = samples->GetSize();
    
    // fft
    double coeff = 1.0;
    if (mNormalize)
    {
        coeff = 1.0/bufSize;
    }
    
    if (mTmpFftBuf.GetSize() != bufSize)
        mTmpFftBuf.Resize(bufSize);

    // Fill the fft buf
    for (int i = 0; i < bufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        mTmpFftBuf.Get()[i].re = samples->Get()[i]*coeff;
        mTmpFftBuf.Get()[i].im = 0.0;
    }
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    for (int i = 0; i < bufSize; i++)
        mTmpFftBuf.Get()[i].re *= mFreqRes;
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(mTmpFftBuf.Get(), bufSize, false);
    
    // Sort the fft buffer
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);

        fftSamples->Get()[i].re = mTmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = mTmpFftBuf.Get()[k].im;
    }
}

void
FftProcessObj6::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                  WDL_TypedBuf<double> *samples)
{
    int bufSize = fftSamples->GetSize();
    
    if (mTmpFftBuf.GetSize() != bufSize)
        mTmpFftBuf.Resize(bufSize);

    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);
        
        mTmpFftBuf.Get()[k].re = fftSamples->Get()[i].re;
        mTmpFftBuf.Get()[k].im = fftSamples->Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(mTmpFftBuf.Get(), bufSize, true);

    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] = mTmpFftBuf.Get()[i].re;
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    double coeff = 1.0/mFreqRes;
    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] *= coeff;
}
