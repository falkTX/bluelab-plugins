//
//  FftConvolverSmooth3.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolverSmooth3__
#define __Spatializer__FftConvolverSmooth3__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

class FftConvolver4;

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
// And skip responses if they arrive too rapidly (not tested)
//
// FftConvolverSmooth3: use FftConvolver4 instead of FftConvolver3
class FftConvolverSmooth3
{
public:
    FftConvolverSmooth3(int bufferSize, bool normalize = true);
    
    virtual ~FftConvolverSmooth3();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<BL_FLOAT> *response);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    FftConvolver4 *mConvolvers[2];
    int mCurrentConvolverIndex;
    
    bool mStableState;
    bool mHasJustReset;
    bool mFirstNewBufferProcessed;
    
    WDL_TypedBuf<BL_FLOAT> mNewResponse;
};

#endif /* defined(__Spatializer__FftConvolverSmooth3__) */

