#ifndef IMPULSE_RESPONSE_EXCTRACTOR_H
#define IMPULSE_RESPONSE_EXCTRACTOR_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class ImpulseResponseExtractor
{
 public:
    ImpulseResponseExtractor(long responseSize, BL_FLOAT sampleRate,
                             BL_FLOAT decimationFactor = 1.0);
  
    virtual ~ImpulseResponseExtractor();
  
    void Reset(long responseSize, BL_FLOAT sampleRate,
               BL_FLOAT decimationFactor = 1.0);
  
    void SetSampleRate(BL_FLOAT sampleRate);
  
    BL_FLOAT GetDecimationFactor();
  
    void AddSamples(const BL_FLOAT *samples, int nFrames,
                    WDL_TypedBuf<BL_FLOAT> *outResponse);
  
    static void AddSamples2(ImpulseResponseExtractor *ext0,
                            ImpulseResponseExtractor *ext1,
                            const BL_FLOAT *samples0,
                            const BL_FLOAT *samples1,
                            int nFrames,
                            WDL_TypedBuf<BL_FLOAT> *outResponse0,
                            WDL_TypedBuf<BL_FLOAT> *outResponse1);
  
    /*static*/ void AddWithDecimation(const WDL_TypedBuf<BL_FLOAT> &samples,
                                      BL_FLOAT decFactor,
                                      WDL_TypedBuf<BL_FLOAT> *outSamples);
  
 protected:
    static long DetectImpulseResponsePeak(const WDL_TypedBuf<BL_FLOAT> &samples);
  
    void AddSamples(const BL_FLOAT *samples, int nFrames);
  
    long DetectResponseStartIndex();
  
    void ExtractResponse(long startIndex, WDL_TypedBuf<BL_FLOAT> *outResponse);
  
    long mResponseSize;
    BL_FLOAT mDecimationFactor;
  
    WDL_TypedBuf<BL_FLOAT> mSamples;
  
    BL_FLOAT mSampleRate;

private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
};

#endif
