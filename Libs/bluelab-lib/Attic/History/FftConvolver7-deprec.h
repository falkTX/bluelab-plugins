//
//  FftConvolver.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolver7__
#define __Spatializer__FftConvolver7__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

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
// In our case, we must BL_FLOAT the size and fill with zeros
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
// FftConvolver7: possibility to convolve directly in Fft domain (not only sample domain)

SHOULD NOT BE USED
// There was a mess in FftConvolver versions
// the correct version is FftConvolver6
// The modifications of FftConvolver7 have been reported to FftConvolver6

class FftConvolver7
{
public:
    FftConvolver7(int bufferSize, bool normalize = true,
                  bool usePadFactor = false, bool normalizeResponses = true);
        
    virtual ~FftConvolver7();
    
    void Reset();
    
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
    
    static void ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
    
protected:
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &response);
    
    // For using with already frequential signal
    void ProcessFftBuffer2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                           const WDL_TypedBuf<WDL_FFT_COMPLEX> &response);

    
    void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &sampleBuf,
                          const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
                          int offsetSamples, int offsetResult);
    
    void ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                    bool normalize);

    void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                       WDL_TypedBuf<BL_FLOAT> *samples);
    
    
    // Normalize the fft of the impulse response
    void NormalizeResponseFft(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples);
    
    
    int mBufSize;
    int mShift;
   
    bool mInit;
    
    WDL_TypedBuf<BL_FLOAT> mSamplesBuf;
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    
    // Need a member offset for the result
    // because it can remain processed results,
    // if we have just less than nFrames results
    // from one host processing loop to another
    int mOffsetResult;
    
    // Tweak parameters
    bool mNormalize;
    bool mUsePadFactor;
    bool mNormalizeResponses;
    
    // Response transformed by fft
    // And BL_FLOATd size and padded with zeros
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPadFftResponse;
    
    // Keep track of the original last set response
    WDL_TypedBuf<BL_FLOAT> mPadSampleResponse;
    
    // Used when nFrames < mBufSize
    int mNumResultReady;
    
    WDL_TypedBuf<BL_FLOAT> mWindow;
};

#endif /* defined(__Spatializer__FftConvolver7__) */
