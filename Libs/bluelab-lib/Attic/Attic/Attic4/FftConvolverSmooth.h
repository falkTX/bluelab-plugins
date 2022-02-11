//
//  FftConvolverSmooth.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolverSmooth__
#define __Spatializer__FftConvolverSmooth__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

class FftConvolver3;

// FftConvolver composite
// Uses two FftConvolvers, and make a fade just after the response
// has been changed
// This avoids clicks in the signal when changin the reseponse continuously and rapidly
class FftConvolverSmooth
{
public:
    FftConvolverSmooth(int bufferSize, bool normalize = true);
    
    virtual ~FftConvolverSmooth();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<BL_FLOAT> *response);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    FftConvolver3 *mConvolvers[2];
    int mCurrentConvolverIndex;
    bool mResponseJustChanged;
    bool mHasJustReset;
};

#endif /* defined(__Spatializer__FftConvolverSmooth__) */

