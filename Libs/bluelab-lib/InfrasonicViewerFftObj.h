//
//  InfrasonicViewerFftObj.h
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_GhostViewer__InfrasonicViewerFftObj__
#define __BL_GhostViewer__InfrasonicViewerFftObj__

#include "FftProcessObj16.h"

// From ChromaFftObj
//

// Disable for debugging
#define USE_SPECTRO_SCROLL 1

class BLSpectrogram4;
class SpectrogramDisplayScroll;

class InfrasonicViewerFftObj : public ProcessObj
{
public:
    InfrasonicViewerFftObj(int bufferSize, int oversampling, int freqRes,
                           BL_FLOAT sampleRate);
    
    virtual ~InfrasonicViewerFftObj();
    
    void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) override;
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
#if USE_SPECTRO_SCROLL
    void SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay);
#else
    void SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay);
#endif
    
    BL_FLOAT GetMaxFreq();
    void SetMaxFreq(BL_FLOAT freq);
    
    void SetSpeed(BL_FLOAT mSpeed);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void SelectSubSonic(WDL_TypedBuf<BL_FLOAT> *magns,
                        WDL_TypedBuf<BL_FLOAT> *phases);

    int ComputeLastBin(BL_FLOAT freq);
    
    BLSpectrogram4 *mSpectrogram;
    
#if USE_SPECTRO_SCROLL
    SpectrogramDisplayScroll *mSpectroDisplay;
#else
    SpectrogramDisplay *mSpectroDisplay;
#endif
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    // Infrasonic
    BL_FLOAT mMaxFreq;
    
    BL_FLOAT mSpeed;
};

#endif /* defined(__BL_GhostViewer__InfrasonicViewerFftObj__) */
