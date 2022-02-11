//
//  SamplesProcessObj.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__SamplesProcessObj__
#define __BL_PitchShift__SamplesProcessObj__

#include "FftProcessObj13.h"

// Same as FftProcessObj, but just nothing on with the Fft
//
// This is used to bufferize samples as done with FftProcessObj,
// to keep the synchronization of necessary

class SamplesProcessObj : public FftProcessObj13
{
public:
    SamplesProcessObj(int bufferSize, int overlapping)
        : FftProcessObj13(bufferSize, overlapping, 1,
                          // Use no window, to get the data exactly similar
                          AnalysisMethodNone, SynthesisMethodNone,
                          // Use Hanning, to have identity after the sum
                          //AnalysisMethodWindow, SynthesisMethodWindow,
                          false, false, false) {}
    
    virtual ~SamplesProcessObj() {}
    
protected:
    // Does nothing by default
    //
    // In this way, we skeeze the all the Fft stuffs
    virtual void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &inBuffer,
                                  const WDL_TypedBuf<BL_FLOAT> &inScBuffer,
                                  WDL_TypedBuf<BL_FLOAT> *outBuffer)
    {
        // Just bypass
        *outBuffer = inBuffer;
    }
};

#endif /* defined(__BL_PitchShift__SamplesProcessObj__) */
