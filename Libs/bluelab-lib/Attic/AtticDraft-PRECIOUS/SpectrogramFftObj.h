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

// Added for GHOST_OPTIM_GL
#define ADAPTIVE_SPEED_FEATURE 0 //1

#define ADAPTIVE_SPEED_FEATURE2 1

class BLSpectrogram3;

class SpectrogramFftObj : public ProcessObj
{
public:
    SpectrogramFftObj(int bufferSize, int oversampling, int freqRes,
                      double sampleRate);
    
    virtual ~SpectrogramFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, double sampleRate);
    
    BLSpectrogram3 *GetSpectrogram();
    
    // After editing, set again the full data
    //
    // NOTE: not optimized, set the full data after each editing
    //
    void SetFullData(const vector<WDL_TypedBuf<double> > &magns,
                     const vector<WDL_TypedBuf<double> > &phases);
    
    enum Mode
    {
        EDIT = 0,
        VIEW,
        ACQUIRE,
        RENDER
    };
    
    void SetMode(enum Mode mode);
    
#if (ADAPTIVE_SPEED_FEATURE || ADAPTIVE_SPEED_FEATURE2)
    void SetAdaptiveSpeed(bool flag);
#endif
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<double> &magns,
                            const WDL_TypedBuf<double> &phases);

#if ADAPTIVE_SPEED_FEATURE
    int GetSpectroNumCols();
#endif
    
#if ADAPTIVE_SPEED_FEATURE2
    int ComputeAddStep();
#endif
    
    BLSpectrogram3 *mSpectrogram;
    
    long mLineCount;
    
    deque<WDL_TypedBuf<double> > mOverlapLines;
    
    bool mAdaptiveSpeed;
    int mAddStep;
    
    Mode mMode;
};

#endif /* defined(__BL_Ghost__SpectrogramFftObj__) */
