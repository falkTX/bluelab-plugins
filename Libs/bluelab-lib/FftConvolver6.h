/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  FftConvolver.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolver6__
#define __Spatializer__FftConvolver6__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"
#include "../../WDL/fastqueue.h"

//#include "../../WDL/IPlug/Containers.h"

// Do not use windowing (which is useless for convolution and
// gives artifacts.
// We don't need oversample neither.
//
// We take care to avoid cyclic convolution, by growing the
// buffers by 2 and padding with zeros
// And we sum also the second part of the buffers, which contains
// "future" sample contributions
// See: See: http://eeweb.poly.edu/iselesni/EL713/zoom/overlap.pdf


// We must add zeros ath the end of both buffers, to avoid aliasing problems (padding)
// In our case, we must double the size and fill with zeros
// See: http://eeweb.poly.edu/iselesni/EL713/zoom/overlap.pdf

// FftConvolver4: Mix between FftConvolver2 (bigger buffers)
// and FftConvolver3 (no windowing)
//
// FftConvolver5: manages impulse responses with big or non-fixed size
//
// - compute only half of the fft
// - normalize impulse response
//
// [NOT USED]
// FftConvolver6: try to reduce ringing artifacts ("echo ghost")
// See: https://dsp.stackexchange.com/questions/29632/convolution-and-windowing-using-a-buffer-how-do-i-do-overlap-add/29645#29645
// and : http://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch18.pdf
// USE_WINDOWING & USE_WINDOWING_FFT
//
// FIX varying latency whith host buffer size < FftConvolver6::bufferSize

// Fix varying latency depending on the host buffer size (447, 1024)
// NOTE: with host buffer size > 1024, latency will be unavoidable
#define FIX_LATENCY 1

class FftConvolver6
{
public:
    FftConvolver6(int bufferSize, bool normalize = true,
                  bool usePadFactor = false, bool normalizeResponses = true);
        
    virtual ~FftConvolver6();
    
    static void Init();
    
#if FIX_LATENCY
    void SetLatency(int latency);
#endif
    
    void Reset();
    
    // NEW
    void Reset(int bufferSize);
    
    void Flush();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<BL_FLOAT> *response);
    
    // Retrive the last set response
    void GetResponse(WDL_TypedBuf<BL_FLOAT> *response);
    
    // Return true if nFrames were provided
    // output can be NULL. In this case, we will only feed the convolver with new data.
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
    bool Process(const WDL_TypedBuf<BL_FLOAT> &inMagns,
                 const WDL_TypedBuf<BL_FLOAT> &inPhases,
                 WDL_TypedBuf<BL_FLOAT> *resultMagns,
                 WDL_TypedBuf<BL_FLOAT> *resultPhases);

    static void ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                BL_FLOAT sampleRate, BL_FLOAT respSampleRate);
    
    // For Spatializer
    static void ResampleImpulse2(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                 BL_FLOAT sampleRate, BL_FLOAT respSampleRate,
                                 bool resizeToNextPowerOfTwo = true);
    
    static void ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
    
protected:
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &response);

    // For using with already frequential signal
    void ProcessFftBuffer2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                           const WDL_TypedBuf<WDL_FFT_COMPLEX> &response);

#if !FIX_LATENCY
    //void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &sampleBuf,
    //                      const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
    //                      int offsetSamples, int offsetResult);
    void ProcessOneBuffer(const WDL_TypedFastQueue<BL_FLOAT> &sampleBuf,
                          const WDL_TypedFastQueue<BL_FLOAT> *ioResultBuf,
                          int offsetSamples, int offsetResult);
#endif
    
#if FIX_LATENCY
    // New version, without offsets (more clear)
    //void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &sampleBuf,
    //                       WDL_TypedBuf<BL_FLOAT> *ioResultBuf);
    void ProcessOneBuffer(const WDL_TypedFastQueue<BL_FLOAT> &sampleBuf,
                          WDL_TypedFastQueue<BL_FLOAT> *ioResultBuf);
    
    // Get partial result if necessary
    bool GetResult(WDL_TypedBuf<BL_FLOAT> *output, int numRequested);
    
    void GetResultOutBuffer(WDL_TypedBuf<BL_FLOAT> *output,
                            int numRequested);

#endif
    
    void ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                    bool normalize);

    void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                       WDL_TypedBuf<BL_FLOAT> *samples);
    
    
    // Normalize the fft of the impulse response
    void NormalizeResponseFft(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples);
    
    
    int mBufferSize;
    
    //bool mInit;
    
    //WDL_TypedBuf<BL_FLOAT> mSamplesBuf;
    WDL_TypedFastQueue<BL_FLOAT> mSamplesBuf;
    
    //WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedFastQueue<BL_FLOAT> mResultBuf;
    
#if !FIX_LATENCY
    // NOTE: these variables made the code unclear
    // So when implementing FIX_LATENCY, they were
    // removed and the code was made simpler
    
    int mShift;
    
    // Need a member offset for the result
    // because it can remain processed results,
    // if we have just less than nFrames results
    // from one host processing loop to another
    int mOffsetResult;
    
    // Used when nFrames < mBufSize
    int mNumResultReady;
#endif
    
    // Tweak parameters
    bool mNormalize;
    bool mUsePadFactor;
    bool mNormalizeResponses;
    
    // Response transformed by fft
    // And BL_FLOATd size and padded with zeros
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPadFftResponse;
    
    // Keep track of the original last set response
    WDL_TypedBuf<BL_FLOAT> mPadSampleResponse;
    
    WDL_TypedBuf<BL_FLOAT> mWindow;
    
#if 0 //FIX_LATENCY
    // For latency fix
    // Largely inspired by FftProcessObj15
    bool mBufWasFull;
    unsigned long long mTotalInSamples;
    
    WDL_TypedBuf<BL_FLOAT> mResultOut;
#endif
    
#if FIX_LATENCY
    // For latency fix 2
    int mLatency;
    int mCurrentLatency;
    
    //WDL_TypedBuf<BL_FLOAT> mResultOut;
    WDL_TypedFastQueue<BL_FLOAT> mResultOut;
#endif

private:
    // Tmp buffer
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf12;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf15;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf16;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf17;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf18;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf19;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf20;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf21;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf22;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf23;
};

#endif /* defined(__Spatializer__FftConvolver6__) */
