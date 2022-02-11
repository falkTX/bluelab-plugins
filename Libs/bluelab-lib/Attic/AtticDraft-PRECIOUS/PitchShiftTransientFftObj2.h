//
//  PitchShiftTransientFftObj2.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__PitchShiftTransientFftObj2__
#define __BL_PitchShift__PitchShiftTransientFftObj2__

#include <FftProcessObj14.h>

class TransientShaperFftObj3;
class PitchShiftFftObj3;


// Composite ;)
class PitchShiftTransientFftObj2 : public ProcessObj
{
public:
    // Set skipFFT to true to skip fft and use only overlaping
    PitchShiftTransientFftObj2()//int bufferSize, int overlapping, int freqRes,
                              //double sampleRate);
    
    virtual ~PitchShiftTransientFftObj2();
    
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
    
    TransientShaperFftObj3 *mTransObj;
    PitchShiftFftObj3 *mPitchObj;
};

#endif /* defined(__BL_PitchShift__PitchShiftTransientFftObj2__) */
