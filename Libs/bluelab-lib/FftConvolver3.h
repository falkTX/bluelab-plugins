//
//  FftConvolver.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolver3__
#define __Spatializer__FftConvolver3__

#include <BLTypes.h>

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


// We must add zeros ath the end of both buffers, to avoid aliasing problems
// In our case, we must double the size and fill with zeros
// See: http://eeweb.poly.edu/iselesni/EL713/zoom/overlap.pdf

// From FftConvolver (and not 2...)
class FftConvolver3
{
public:
    FftConvolver3(int bufferSize, bool normalize = true);
        
    virtual ~FftConvolver3();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<BL_FLOAT> *response);
    
    // Retrive the last set response
    void GetResponse(WDL_TypedBuf<BL_FLOAT> *response);
    
    // Return true if nFrames were provided
    // output can be NULL. In this case, we will only feed the convolver with new data.
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> &response);
    
    void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &sampleBuf,
                          const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
                          int offsetSamples, int offsetResult);
        
    void ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                    bool noemalize);
        
    void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                       WDL_TypedBuf<BL_FLOAT> *samples);
    
        
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
};

#endif /* defined(__Spatializer__FftConvolver3__) */
