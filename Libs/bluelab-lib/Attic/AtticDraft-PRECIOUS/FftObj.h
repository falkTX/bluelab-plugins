//
//  FftObj.h
//  BL-PitchShift
//
//  Created by Pan on 17/04/18.
//
//

#ifndef __BL_PitchShift__FftObj__
#define __BL_PitchShift__FftObj__

#include <FftProcessObj13.h>
#include <FifoDecimator.h>

// Generic FftObj for processing
class FftObj : public FftProcessObj13
{
public:
    FftObj(int bufferSize, int oversampling, int freqRes,
           enum AnalysisMethod aMethod,
           enum SynthesisMethod sMethod,
           bool keepSynthesisEnergy,
           bool variableHanning,
           bool skipFFT);
    
    virtual ~FftObj();
    
    
    virtual void Reset(int oversampling, int freqRes);
    
    
    virtual void PreProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer);
    
    virtual void PostProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer);
    
    
    virtual void SetTrackIO(int maxNumPoints, double decimFactor,
                            bool trackInput, bool trackOutput);
    
    virtual void GetCurrentInput(WDL_TypedBuf<double> *outInput);
    
    virtual void GetCurrentOutput(WDL_TypedBuf<double> *outOutput);
    
protected:
    FifoDecimator mInput;
    FifoDecimator mOutput;
    
    bool mTrackInput;
    bool mTrackOutput;
    
    // Can be used for display + overlap
    //
    // It is used in some child classes too !
    long mBufferNum;
};

#endif /* defined(__BL_PitchShift__FftObj__) */
