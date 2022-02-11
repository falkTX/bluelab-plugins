//
//  FftConvolver.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolver6__
#define __Spatializer__FftConvolver6__

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
class FftConvolver6
{
public:
    FftConvolver6(int bufferSize, bool normalize = true,
                  bool usePadFactor = false, bool normalizeResponses = true);
        
    virtual ~FftConvolver6();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<double> *response);
    
    // Retrive the last set response
    void GetResponse(WDL_TypedBuf<double> *response);
    
    // Return true if nFrames were provided
    // output can be NULL. In this case, we will only feed the convolver with new data.
    bool Process(double *input, double *output, int nFrames);
    
    static void ResampleImpulse(WDL_TypedBuf<double> *impulseResponse,
                                double sampleRate, double respSampleRate);
    
    static void ResizeImpulse(WDL_TypedBuf<double> *impulseResponse);
    
protected:
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &response);
    
    void ProcessOneBuffer(const WDL_TypedBuf<double> &sampleBuf,
                          const WDL_TypedBuf<double> *ioResultBuf,
                          int offsetSamples, int offsetResult);
    
    void ComputeFft(const WDL_TypedBuf<double> *samples,
                    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                    bool normalize);

    void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                       WDL_TypedBuf<double> *samples);
    
    
    // Normalize the fft of the impulse response
    void NormalizeResponseFft(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples);
    
    
    int mBufSize;
    int mShift;
   
    bool mInit;
    
    WDL_TypedBuf<double> mSamplesBuf;
    WDL_TypedBuf<double> mResultBuf;
    
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
    // And doubled size and padded with zeros
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPadFftResponse;
    
    // Keep track of the original last set response
    WDL_TypedBuf<double> mPadSampleResponse;
    
    // Used when nFrames < mBufSize
    int mNumResultReady;
    
    WDL_TypedBuf<double> mWindow;
};

#endif /* defined(__Spatializer__FftConvolver6__) */
