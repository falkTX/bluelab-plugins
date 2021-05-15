//
//  StereoPhasesProcess.h
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_PitchShift__StereoPhasesProcess__
#define __BL_PitchShift__StereoPhasesProcess__

#include "FftProcessObj16.h"

#include "PhasesDiff.h"

class StereoPhasesProcess : public MultichannelProcess
{
public:
    StereoPhasesProcess(int bufferSize);
    
    virtual ~StereoPhasesProcess();
    
    void Reset();
    
    // For PhasesDiff::USE_LINERP_PHASES
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate);
    
    void SetActive(bool flag);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    void ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                          const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
protected:
    PhasesDiff mPhasesDiff;
    
    bool mIsActive;
};

#endif /* defined(__BL_PitchShift__StereoPhasesProcess__) */
