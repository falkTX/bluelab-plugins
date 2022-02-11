//
//  SpectrogramFftObjEXPE.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectrogramFftObjEXPE__
#define __BL_Ghost__SpectrogramFftObjEXPE__

#ifdef IGRAPHICS_NANOVG

#include "FftProcessObj16.h"

class BLSpectrogram4;
class TransientLib5;
class SpectrogramFftObjEXPE : public ProcessObj
{
public:
    SpectrogramFftObjEXPE(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~SpectrogramFftObjEXPE();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    BLSpectrogram4 *GetSpectrogram();
    
    // After editing, set again the full data
    //
    // NOTE: not optimized, set the full data after each editing
    //
    void SetFullData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                     const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
    
    enum Mode
    {
        EDIT = 0,
        VIEW,
        ACQUIRE,
        RENDER
    };
    
    void SetMode(enum Mode mode);
    
    // EXPE
    void SetSmoothFactor(BL_FLOAT factor);
    void SetFreqAmpRatio(BL_FLOAT ratio);
    void SetTransThreshold(BL_FLOAT thrs);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    BLSpectrogram4 *mSpectrogram;
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    // EXPE
    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
    
    BL_FLOAT mSmoothFactor;
    BL_FLOAT mFreqAmpRatio;
    BL_FLOAT mTransThreshold;

    TransientLib5 *mTransLib;
};

#endif

#endif /* defined(__BL_Ghost__SpectrogramFftObjEXPE__) */
