//
//  StereoWidthProcess.h
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_PitchShift__StereoWidthProcess__
#define __BL_PitchShift__StereoWidthProcess__

#include "FftProcessObj14.h"

class StereoWidthProcess : public MultichannelProcess
{
public:
    //enum Mode
    //{
    //    POLAR_LEVEL = 0,
    //    POLAR_SAMPLES
    //};
    
    StereoWidthProcess(int bufferSize);
    
    virtual ~StereoWidthProcess();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, double sampleRate);
    
    void SetWidthChange(double widthChange);
    
    //void SetMode(enum Mode mode);
    
    void ProcessInputSamples(vector<WDL_TypedBuf<double> * > *ioSamples);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples);
    
    // Get the result
    void GetWidthValues(WDL_TypedBuf<double> *xValues,
                        WDL_TypedBuf<double> *yValues);
    
protected:
    void ApplyWidthChange(WDL_TypedBuf<double> *ioPhasesL,
                          WDL_TypedBuf<double> *ioPhasesR);
    
    void Stereoize(WDL_TypedBuf<double> *ioPhasesL,
                   WDL_TypedBuf<double> *ioPhasesR);
    
    double mWidthChange;
    
    //enum Mode mMode;
    
    WDL_TypedBuf<double> mWidthValuesX;
    WDL_TypedBuf<double> mWidthValuesY;
    
    WDL_TypedBuf<double> mCurrentSamples;
};

#endif /* defined(__BL_PitchShift__StereoPhasesProcess__) */
