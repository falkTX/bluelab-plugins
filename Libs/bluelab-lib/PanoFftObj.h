//
//  BL_PanoFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Pano__PanoFftObj__
#define __BL_Pano__PanoFftObj__

#include "FftProcessObj16.h"

// From ChromaFftObj
//

class BLSpectrogram4;
class SpectrogramDisplayScroll;

class PanoFftObj : public MultichannelProcess
{
public:
    PanoFftObj(int bufferSize, int oversampling, int freqRes,
               BL_FLOAT sampleRate);
    
    virtual ~PanoFftObj();
    
    void
    ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                    const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) override;
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay);
    
    void SetSharpness(BL_FLOAT sharpness);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void MagnsToPanoLine(const WDL_TypedBuf<BL_FLOAT> magns[2],
                         WDL_TypedBuf<BL_FLOAT> *panoLine);
    
    //
    BLSpectrogram4 *mSpectrogram;
    
    SpectrogramDisplayScroll *mSpectroDisplay;
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mSharpness;
    
    //
    int mBufferSize;
    int mOverlapping;
    int mFreqRes;
    int mSampleRate;
};

#endif /* defined(__BL_BL_Pano__BL_PanoFftObj__) */
