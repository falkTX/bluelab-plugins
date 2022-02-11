//
//  FftConvolver.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolver5__
#define __Spatializer__FftConvolver5__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

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
class FftConvolver5
{
public:
    FftConvolver5(int bufferSize, bool normalize = true);
        
    virtual ~FftConvolver5();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<BL_FLOAT> *response);
    
    // Retrive the last set response
    void GetResponse(WDL_TypedBuf<BL_FLOAT> *response);
    
    // Return true if nFrames were provided
    // output can be NULL. In this case, we will only feed the convolver with new data.
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
    static void ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                BL_FLOAT sampleRate, BL_FLOAT respSampleRate);
    
    static void ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
    
protected:
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
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
        
    // Response transformed by fft
    // And BL_FLOATd size and padded with zeros
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPadFftResponse;
    
    // Keep track of the original last set response
    WDL_TypedBuf<BL_FLOAT> mPadSampleResponse;
    
    // Used when nFrames < mBufSize
    int mNumResultReady;
    
    WDL_TypedBuf<BL_FLOAT> mWindow;
};

#endif /* defined(__Spatializer__FftConvolver5__) */
