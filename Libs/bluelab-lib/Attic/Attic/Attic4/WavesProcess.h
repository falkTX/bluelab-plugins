//
//  WavesProcess.h
//  BL-Waves
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Waves__WavesProcess__
#define __BL_Waves__WavesProcess__


#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include "FftProcessObj16.h"


class WavesRender;
class WavesProcess : public ProcessObj
{
public:
    WavesProcess(int bufferSize,
                 BL_FLOAT overlapping, BL_FLOAT oversampling,
                 BL_FLOAT sampleRate);
    
    virtual ~WavesProcess();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void SetWavesRender(WavesRender *wavesRender);
    
protected:
    int GetDisplayRefreshRate();
    
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_FLOAT> mValues;
    
    // Vol Render
    WavesRender *mWavesRender;
};

#endif /* defined(__BL_Waves__WavesProcess__) */
