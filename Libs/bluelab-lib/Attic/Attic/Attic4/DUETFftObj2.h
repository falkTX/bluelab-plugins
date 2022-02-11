//
//  BL_DUETFftObj2.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_DUET__DUETFftObj2__
#define __BL_DUET__DUETFftObj2__

#include "FftProcessObj16.h"

// From BatFftObj5 (directly)
//
class BLSpectrogram3;
class SpectrogramDisplay;
class ImageDisplay;

class DUETSeparator2;

class DUETFftObj2 : public MultichannelProcess
{
public:
    DUETFftObj2(GraphControl11 *graph,
               int bufferSize, int oversampling, int freqRes,
               BL_FLOAT sampleRate);
    
    virtual ~DUETFftObj2();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // For Phase Aliasing Correction
    // See: file:///Users/applematuer/Downloads/1-s2.0-S1063520313000043-main.pdf
    void ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);
    
    // To be called from the audio thread, to force update if a parameter changed.
    void Update();
    
    BLSpectrogram3 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay);
    
    void SetTimeSmooth(BL_FLOAT smoothFactor);
    
    //
    void SetThresholdFloor(BL_FLOAT threshold);
    void SetThresholdPeaks(BL_FLOAT threshold);
    void SetThresholdPeaksWidth(BL_FLOAT threshold);
    
    void SetDisplayThresholded(bool flag);
    void SetDisplayMaxima(bool flag);
    void SetDisplayMasks(bool flag);
    
    void SetUseSoftMasks(bool flag);
    void SetUseSoftMasksComp(bool flag);
    
    void SetSoftMaskSize(int size);
    
    //
    void SetUseKernelSmooth(bool kernelSmoothFlag);
    
    void SetExtractMax(bool extractMaxFlag);
    
    //
    void SetUseGradientMasks(bool flag);
    void SetThresholdAll(bool flag);
    
    //
    void SetImageDisplay(ImageDisplay *imageDisplay);
    
    //
    void SetPickingActive(bool flag);
    void SetPickPosition(BL_FLOAT x, BL_FLOAT y);
    void SetInvertPickSelection(bool flag);
    
    //
    void SetHistogramSize(int histoSize);
    void SetAlphaZoom(BL_FLOAT zoom);
    void SetDeltaZoom(BL_FLOAT zoom);
    
    //
    void SetUsePhaseAliasingCorrection(bool flag);
    
protected:
    void Process();
    
    //
    int mBufferSize;
    int mOverlapping;
    int mFreqRes;
    int mSampleRate;
    
    //
    BLSpectrogram3 *mSpectrogram; // Unused
    SpectrogramDisplay *mSpectroDisplay;
    GraphControl11 *mGraph;
    
    // For histogram display
    ImageDisplay *mImageDisplay;
    
    //
    DUETSeparator2 *mSeparator;
    
    bool mUseSoftMasksComp;
    
    // Parameter changed
    bool mMustReprocess;
    
    bool mUsePhaseAliasingCorrection;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPACOversampledFft[2];
};

#endif /* defined(__BL_BL_DUET__BL_DUETFftObj2__) */
