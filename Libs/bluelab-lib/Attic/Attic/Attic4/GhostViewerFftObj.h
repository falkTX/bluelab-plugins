//
//  GhostViewerFftObj.h
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_GhostViewer__GhostViewerFftObj__
#define __BL_GhostViewer__GhostViewerFftObj__

#include "FftProcessObj16.h"

// From ChromaFftObj
//

// Disable for debugging
#define USE_SPECTRO_SCROLL 1

class BLSpectrogram3;
class SpectrogramDisplayScroll;

class GhostViewerFftObj : public ProcessObj
{
public:
    GhostViewerFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~GhostViewerFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    BLSpectrogram3 *GetSpectrogram();
    
#if USE_SPECTRO_SCROLL
    void SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay);
#else
    void SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay);
#endif
    
    void SetSpeedMod(int speedMod);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    BLSpectrogram3 *mSpectrogram;
    
#if USE_SPECTRO_SCROLL
    SpectrogramDisplayScroll *mSpectroDisplay;
#else
    SpectrogramDisplay *mSpectroDisplay;
#endif
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    int mSpeedMod;
};

#endif /* defined(__BL_GhostViewer__GhostViewerFftObj__) */
