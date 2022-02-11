//
//  PitchShiftTransientFftObj.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__PostTransientFftObj__
#define __BL_PitchShift__PostTransientFftObj__

#include <FftObj.h>
//#include <FftProcessObj13.h>

class TransientShaperFftObj2;


// Composite ;)
class PostTransientFftObj : public FftObj //FftProcessObj13
{
public:
    // Set skipFFT to true to skip fft and use only overlaping
    PostTransientFftObj(int bufferSize, int overlapping, int freqRes,
                        bool keepSynthesisEnergy, bool variableHanning,
                        FftObj *obj);
    
    virtual ~PostTransientFftObj();
    
    virtual void Reset(int overlapping, int freqRes);
    
    void SetTransBoost(BL_FLOAT factor);
    
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                      WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
protected:
    void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &inBuffer,
                          const WDL_TypedBuf<BL_FLOAT> &inScBuffer,
                          WDL_TypedBuf<BL_FLOAT> *outBuffer);
    
    TransientShaperFftObj2 *mTransObj;
    FftObj *mFftObj;
    
    BL_FLOAT mTransBoost;
};

#endif /* defined(__BL_PitchShift__PostTransientFftObj__) */
