//
//  SimpleSpectrogramFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SimpleSpectrogramFftObj__
#define __BL_Ghost__SimpleSpectrogramFftObj__

#ifdef IGRAPHICS_NANOVG

#include "FftProcessObj16.h"

class BLSpectrogram4;
class SimpleSpectrogramFftObj : public ProcessObj
{
public:
    SimpleSpectrogramFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~SimpleSpectrogramFftObj();
    
    void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) override;
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    //
    BLSpectrogram4 *mSpectrogram;
};

#endif

#endif /* defined(__BL_Ghost__SimpleSpectrogramFftObj__) */
