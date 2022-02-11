//
//  FftConvolverSmooth.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolverSmooth2__
#define __Spatializer__FftConvolverSmooth2__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

class FftConvolver3;

// FftConvolver composite
// Uses two FftConvolvers, and make a fade just after the response
// has been changed
// This avoids clicks in the signal when changin the reseponse continuously and rapidly
//
// FftConvolverSmooth2: from FftConvolverSmooth
// Wait for having processed one buffer before making interpolation
// Because when a new impulse response is set to a convolver, there is a blank at the beginning
// of the first new buffer.
// So we wait, to interpolate without the blank
//
// And skip responses if they arrive too rapidly
class FftConvolverSmooth2
{
public:
    FftConvolverSmooth2(int bufferSize, bool normalize = true);
    
    virtual ~FftConvolverSmooth2();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<double> *response);
    
    // Return true if nFrames were provided
    bool Process(double *input, double *output, int nFrames);
    
protected:
    FftConvolver3 *mConvolvers[2];
    int mCurrentConvolverIndex;
    //bool mResponseJustChanged;
    bool mStableState;
    bool mHasJustReset;
    bool mFirstNewBufferProcessed;
    
    WDL_TypedBuf<double> mNewResponse;
};

#endif /* defined(__Spatializer__FftConvolverSmooth2__) */

