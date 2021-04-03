//
//  GhostViewerFftObj.h
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_GhostViewer__GhostViewerFftObj__
#define __BL_GhostViewer__GhostViewerFftObj__

#include <bl_queue.h>

#include <BLTypes.h>

#include <FftProcessObj16.h>

// From ChromaFftObj
//

// SpectrogramDisplayScroll => SpectrogramDisplayScroll3

class BLSpectrogram4;
//class SpectrogramDisplayScroll3;
class SpectrogramDisplayScroll4;
class GhostViewerFftObj : public ProcessObj
{
public:
    GhostViewerFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~GhostViewerFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    BLSpectrogram4 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay);
    
    void SetSpeedMod(int speedMod);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);

    //
    BLSpectrogram4 *mSpectrogram;
    
    SpectrogramDisplayScroll4 *mSpectroDisplay;
    
    long mLineCount;
    
    //deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    int mSpeedMod;

private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
};

#endif /* defined(__BL_GhostViewer__GhostViewerFftObj__) */
