//
//  FilterFftLowPass.h
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#ifndef __UST__FilterFftLowPass__
#define __UST__FilterFftLowPass__

#define DEFAULT_FFT_SIZE 2048

class FftProcessObj16;
class FftLowPassProcess;

class FilterFftLowPass
{
public:
    FilterFftLowPass();
    
    virtual ~FilterFftLowPass();
    
    void Init(BL_FLOAT fc, BL_FLOAT sampleRate, int fftSize = DEFAULT_FFT_SIZE);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    BL_FLOAT mFC;
    int mFftSize;
    int mBlockSize;
    
    FftProcessObj16 *mFftObj;
    FftLowPassProcess *mProcessObj;
    
    BL_FLOAT mPrevSampleRate;
};

#endif /* defined(__UST__FilterFftLowPass__) */
