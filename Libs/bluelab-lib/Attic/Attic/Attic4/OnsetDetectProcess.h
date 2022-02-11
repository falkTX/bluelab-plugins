//
//  OnsetDetectProcess.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Air__OnsetDetectProcess__
#define __BL_Air__OnsetDetectProcess__


#include "FftProcessObj16.h"

class OnsetDetector;
class OnsetDetectProcess : public ProcessObj
{
public:
    OnsetDetectProcess(int bufferSize,
                BL_FLOAT overlapping, BL_FLOAT oversampling,
                BL_FLOAT sampleRate);
    
    virtual ~OnsetDetectProcess();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void SetThreshold(BL_FLOAT threshold);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
protected:
    
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    OnsetDetector *mOnsetDetector;
};

#endif /* defined(__BL_Air__OnsetDetectProcess__) */
