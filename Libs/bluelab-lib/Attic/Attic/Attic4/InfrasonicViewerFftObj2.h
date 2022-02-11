//
//  InfrasonicViewerFftObj2.h
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_GhostViewer__InfrasonicViewerFftObj2__
#define __BL_GhostViewer__InfrasonicViewerFftObj2__

#include "FftProcessObj16.h"

// From InfrasonicViewerFftObj
//
// Fixes for jittering

// Disable for debugging
#define USE_SPECTRO_SCROLL 1

class BLSpectrogram3;
class SpectrogramDisplayScroll2;

class InfrasonicViewerFftObj2 : public ProcessObj
{
public:
    InfrasonicViewerFftObj2(int bufferSize, int oversampling, int freqRes,
                            BL_FLOAT sampleRate);
    
    virtual ~InfrasonicViewerFftObj2();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate,
               BL_FLOAT timeWindowSec);
    
    BLSpectrogram3 *GetSpectrogram();
    
#if USE_SPECTRO_SCROLL
    void SetSpectrogramDisplay(SpectrogramDisplayScroll2 *spectroDisplay);
#else
    void SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay);
#endif
    
    BL_FLOAT GetMaxFreq();
    void SetMaxFreq(BL_FLOAT freq);
    
    void SetTimeWindowSec(BL_FLOAT timeWindowSec);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void SelectSubSonic(WDL_TypedBuf<BL_FLOAT> *magns,
                        WDL_TypedBuf<BL_FLOAT> *phases);

    int ComputeLastBin(BL_FLOAT freq);
    
    BLSpectrogram3 *mSpectrogram;
    
#if USE_SPECTRO_SCROLL
    SpectrogramDisplayScroll2 *mSpectroDisplay;
#else
    SpectrogramDisplay *mSpectroDisplay;
#endif
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    // Infrasonic
    BL_FLOAT mMaxFreq;
    
    BL_FLOAT mTimeWindowSec;
};

#endif /* defined(__BL_GhostViewer__InfrasonicViewerFftObj2__) */
