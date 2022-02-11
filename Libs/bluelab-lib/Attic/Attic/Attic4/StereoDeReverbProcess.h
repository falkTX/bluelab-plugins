//
//  BL_StereoDeReverbProcess.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_DUET__StereoDeReverbProcess__
#define __BL_DUET__StereoDeReverbProcess__

#include "FftProcessObj16.h"

// From BatFftObj5 (directly)
//
class BLSpectrogram3;
class SpectrogramDisplay;
class ImageDisplay;

class DUETSeparator3;

class StereoDeReverbProcess : public MultichannelProcess
{
public:
    StereoDeReverbProcess(int bufferSize, int oversampling,
                          int freqRes, BL_FLOAT sampleRate);
    
    virtual ~StereoDeReverbProcess();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
#if 0
    // For Phase Aliasing Correction
    // See: file:///Users/applematuer/Downloads/1-s2.0-S1063520313000043-main.pdf
    void ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);
#endif
    
    void SetMix(BL_FLOAT mix);
    void SetOutputReverbOnly(bool flag);
    void SetThreshold(BL_FLOAT threshold);
    
    int GetAdditionalLatency();
    
protected:
    void Process();
    
    //
    int mBufferSize;
    int mOverlapping;
    int mFreqRes;
    int mSampleRate;
    
    //
    DUETSeparator3 *mSeparator;
    bool mUseSoftMasksComp;
    
    //
    BL_FLOAT mMix;
    bool mOutputReverbOnly;
    
#if 0
    bool mUsePhaseAliasingCorrection;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPACOversampledFft[2];
#endif
};

#endif /* defined(__BL_BL_DUET__BL_StereoDeReverbProcess__) */
