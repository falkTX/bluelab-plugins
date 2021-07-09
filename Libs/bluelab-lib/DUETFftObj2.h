//
//  BL_DUETFftObj2.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_DUET__DUETFftObj2__
#define __BL_DUET__DUETFftObj2__

#ifdef IGRAPHICS_NANOVG

#include "FftProcessObj16.h"

// From BatFftObj5 (directly)
//
class BLSpectrogram4;
class SpectrogramDisplay2;
class BLImage;
class ImageDisplay2;
class GraphControl12;
class DUETSeparator2;

class DUETFftObj2 : public MultichannelProcess
{
public:
    DUETFftObj2(GraphControl12 *graph,
                int bufferSize, int oversampling, int freqRes,
                BL_FLOAT sampleRate);
    
    virtual ~DUETFftObj2();
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // For Phase Aliasing Correction
    // See: file:///Users/applematuer/Downloads/1-s2.0-S1063520313000043-main.pdf
    void ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);
    
    // To be called from the audio thread, to force update if a parameter changed.
    void Update();
    
    void SetGraph(GraphControl12 *graph);
    
    BLSpectrogram4 *GetSpectrogram();
    void SetSpectrogramDisplay(SpectrogramDisplay2 *spectroDisplay);
    
    BLImage *GetImage();
    void SetImageDisplay(ImageDisplay2 *imageDisplay);
    
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
    BLSpectrogram4 *mSpectrogram; // Unused
    SpectrogramDisplay2 *mSpectroDisplay;
    
    // For histogram display
    BLImage *mImage;
    ImageDisplay2 *mImageDisplay;
    
    GraphControl12 *mGraph;
    
    //
    DUETSeparator2 *mSeparator;
    
    bool mUseSoftMasksComp;
    
    // Parameter changed
    bool mMustReprocess;
    
    bool mUsePhaseAliasingCorrection;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPACOversampledFft[2];
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_BL_DUET__BL_DUETFftObj2__) */
