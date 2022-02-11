//
//  PitchShiftTransientFftObj.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__PitchShiftTransientFftObj__
#define __BL_PitchShift__PitchShiftTransientFftObj__

#include <FftObj.h>

class TransientShaperFftObj2;
class PitchShiftFftObj2;


// Composite ;)
class PitchShiftTransientFftObj : public FftObj
{
public:
    // Set skipFFT to true to skip fft and use only overlaping
    PitchShiftTransientFftObj(int bufferSize, int overlapping, int freqRes,
                              double sampleRate);
    
    virtual ~PitchShiftTransientFftObj();
    
    virtual void Reset(int overlapping, int freqRes, double sampleRate);
    
    void SetPitchFactor(double factor);
    
    void SetTransBoost(double factor);
    
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                      WDL_TypedBuf<double> *scBuffer);
    
protected:
    void ProcessOneBuffer(const WDL_TypedBuf<double> &inBuffer,
                          const WDL_TypedBuf<double> &inScBuffer,
                          WDL_TypedBuf<double> *outBuffer);
    
    TransientShaperFftObj2 *mTransObj;
    PitchShiftFftObj2 *mPitchObj;
};

#endif /* defined(__BL_PitchShift__PitchShiftTransientFftObj__) */
