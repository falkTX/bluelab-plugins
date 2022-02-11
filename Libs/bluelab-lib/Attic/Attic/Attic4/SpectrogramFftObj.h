//
//  SpectrogramFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectrogramFftObj__
#define __BL_Ghost__SpectrogramFftObj__

#include "FftProcessObj16.h"

// Added during GHOST_OPTIM_GL
// Constant speed whatever the sample rate
#define CONSTANT_SPEED_FEATURE 1


class BLSpectrogram3;

class SpectrogramFftObj : public ProcessObj
{
public:
    SpectrogramFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~SpectrogramFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    BLSpectrogram3 *GetSpectrogram();
    
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
    
#if CONSTANT_SPEED_FEATURE
    void SetConstantSpeed(bool flag);
#endif
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
#if CONSTANT_SPEED_FEATURE
    int ComputeAddStep();
#endif
    
    BLSpectrogram3 *mSpectrogram;
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    // For constant speed
    bool mConstantSpeed;
    int mAddStep;
    
    Mode mMode;
};

#endif /* defined(__BL_Ghost__SpectrogramFftObj__) */
