//
//  ChromaFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Chroma__ChromaFftObj__
#define __BL_Chroma__ChromaFftObj__

//#include "FftProcessObj15.h"
#include "FftProcessObj16.h"

// Without USE_FREQ_OBJ:
// - graphic is more clear
// - frequencies A Tune sometimes seems false
// - with pure low frequencies, there are several lines instead of a single one
//
// With USE_FREQ_OBJ: GOOD !
// - data is more fuzzy, but seems more accurate
// - A Tune seems correct
// - with pure low frequencies, there is only one line !
//

#define USE_FREQ_OBJ 1 //0 1

// From SpectrogramFftObj
//

class BLSpectrogram3;
class SpectrogramDisplayScroll;

#if USE_FREQ_OBJ
class FreqAdjustObj3;
#endif

class ChromaFftObj : public ProcessObj
{
public:
    ChromaFftObj(int bufferSize, int oversampling, int freqRes,
                 BL_FLOAT sampleRate);
    
    virtual ~ChromaFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    BLSpectrogram3 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay);
    
    void SetATune(BL_FLOAT aTune);
    
    void SetSharpness(BL_FLOAT sharpness);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void MagnsToCromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                          WDL_TypedBuf<BL_FLOAT> *chromaLine);
    
#if USE_FREQ_OBJ
    void MagnsToCromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                          const WDL_TypedBuf<BL_FLOAT> &phases,
                          WDL_TypedBuf<BL_FLOAT> *chromaLine);
#endif
    
    BL_FLOAT ComputeC0Freq();
    
    BLSpectrogram3 *mSpectrogram;
    
    SpectrogramDisplayScroll *mSpectroDisplay;
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mATune;
    BL_FLOAT mSharpness;
    
#if USE_FREQ_OBJ
    FreqAdjustObj3 *mFreqObj;
#endif
};

#endif /* defined(__BL_Chroma__ChromaFftObj__) */
