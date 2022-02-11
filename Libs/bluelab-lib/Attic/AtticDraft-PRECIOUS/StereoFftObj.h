//
//  StereoFftObj.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__StereoFftObj__
#define __BL_PitchShift__StereoFftObj__

#include "FftObj.h"

class StereoFftObj
{
public:
    StereoFftObj(FftObj *objs[2]);
    
    virtual ~StereoFftObj();
    
    virtual void Reset(int oversampling, int freqRes);
    
    virtual bool Process(double *input, double *output, double *inSc, int nFrames);
    
    virtual void PreProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer);
    
    virtual void PostProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer);
    
    
    virtual void SetTrackIO(int maxNumPoints, double decimFactor,
                            bool trackInput, bool trackOutput);
    
    virtual void GetCurrentInput(WDL_TypedBuf<double> *outInput);
    
    virtual void GetCurrentOutput(WDL_TypedBuf<double> *outOutput);
    
protected:
    FftObj *mObjs[2];
};

#endif /* defined(__BL_PitchShift__StereoFftObj__) */
