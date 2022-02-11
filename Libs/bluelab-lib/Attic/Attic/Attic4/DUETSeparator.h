//
//  DUETSeparator.h
//  BL-DUET
//
//  Created by applematuer on 5/3/20.
//
//

#ifndef __BL_DUET__DUETSeparator__
#define __BL_DUET__DUETSeparator__

#include <BLTypes.h>
#include <PeakDetector2D.h>

#include "IPlug_include_in_plug_hdr.h"

class DUETHistogram;
class ImageSmootherKernel;
//class PeakDetector2D;
class SoftMasking;
class SoftMaskingComp;

class DUETSeparator
{
public:
    DUETSeparator(BL_FLOAT sampleRate);
    
    virtual ~DUETSeparator();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetInputData(const WDL_TypedBuf<BL_FLOAT> magns[2],
                      const WDL_TypedBuf<BL_FLOAT> phases[2]);
    
    // Alternate method
    void SetInputData(const WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2]);
    
    void Process();
    
    void GetOutputData(WDL_TypedBuf<BL_FLOAT> magns[2],
                       WDL_TypedBuf<BL_FLOAT> phases[2]);
    
    // Alternate method
    void GetOutputData(WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2]);
    
    int GetHistogramWidth();
    int GetHistogramHeight();
    void GetHistogramData(WDL_TypedBuf<BL_FLOAT> *data);
    
    //
    void SetTimeSmooth(BL_FLOAT smoothFactor);
    
    //
    void SetKernelSmooth(bool kernelSmoothFlag);
    
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
    void SetUseDuetSoftMasks(bool flag);
    void SetThresholdAll(bool flag);
    
    //
    void SetPickingActive(bool flag);
    void SetPickPosition(BL_FLOAT x, BL_FLOAT y);
    void SetPickModeBg(bool flag);
    
protected:
    // Fiest version
    void FillHistogram();
    
    // Fixed version, closer to the paper
    // See: https://www.researchgate.net/publication/227143748_The_DUET_blind_source_separation_algorithm
    void FillHistogram2();
    
    void ThresholdHistogram(int width, int height, WDL_TypedBuf<BL_FLOAT> *data,
                            BL_FLOAT threshold);
    
    void ThresholdSound(int width, int height,
                        const WDL_TypedBuf<BL_FLOAT> &mask,
                        WDL_TypedBuf<BL_FLOAT> magns[2]);
    
    // Wiener
    void ApplySoftMask(int width, int height,
                       const WDL_TypedBuf<BL_FLOAT> &mask,
                       WDL_TypedBuf<BL_FLOAT> *magns);
    
    void ApplySoftMaskComp(int width, int height,
                           const WDL_TypedBuf<WDL_FFT_COMPLEX> &mask,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftData);
    
    //
    void ApplyDuetSoftMask(int width, int height,
                           const WDL_TypedBuf<BL_FLOAT> &mask,
                           WDL_TypedBuf<BL_FLOAT> *magns);
    
    void ApplyDuetSoftMaskComp(int width, int height,
                               const WDL_TypedBuf<BL_FLOAT> &mask,
                               WDL_TypedBuf<WDL_FFT_COMPLEX> *fftData);
    
    // Hard elbow
    //void ThresholdHistogramHE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data);
    //void ThresholdSoundHE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data,
    //                      WDL_TypedBuf<BL_FLOAT> magns[2]);

    // Soft elbow
    //void ThresholdHistogramSE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data);
    //void ThresholdSoundSE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data,
    //                      WDL_TypedBuf<BL_FLOAT> magns[2]);
    
    // Find maxima
    void DetectPeaks(int width, int height, WDL_TypedBuf<BL_FLOAT> *data);
    
    // Separate peaks regions
    //void SeparatePeaks(int width, int height, WDL_TypedBuf<BL_FLOAT> *data);
    
    
    // Extract masks
    //
    void ExtractPeaks(int width, int height,
                      const WDL_TypedBuf<BL_FLOAT> &histogramData,
                      vector<PeakDetector2D::Peak> *peaks);
    
    void CreateMaskDist(int width, int height,
                        const WDL_TypedBuf<BL_FLOAT> &histogramData,
                        const vector<PeakDetector2D::Peak> &peaks,
                        WDL_TypedBuf<BL_FLOAT> *mask);
    
    void CreateMaskPeakDetector(int width, int height,
                                const WDL_TypedBuf<BL_FLOAT> &histogramData,
                                const vector<PeakDetector2D::Peak> &peaks,
                                WDL_TypedBuf<BL_FLOAT> *mask);
    
    void ComputePeaksAreas(const WDL_TypedBuf<BL_FLOAT> &mask,
                           vector<PeakDetector2D::Peak> *peaks);
    
    void SelectMask(WDL_TypedBuf<BL_FLOAT> *mask, int peakId);
    void SelectMaskThresholdAll(WDL_TypedBuf<BL_FLOAT> *mask);
    void SelectMaskPicking(int width, int height, WDL_TypedBuf<BL_FLOAT> *mask);
    
    void DBG_MaskToImage(const WDL_TypedBuf<BL_FLOAT> &mask,
                         WDL_TypedBuf<BL_FLOAT> *image);

    void DBG_SoftMaskToImage(const WDL_TypedBuf<BL_FLOAT> &mask,
                             WDL_TypedBuf<BL_FLOAT> *image);
    
    void CreateDuetSoftMask(int width, int height,
                            const WDL_TypedBuf<BL_FLOAT> &mask,
                            const vector<PeakDetector2D::Peak> &peaks,
                            WDL_TypedBuf<BL_FLOAT> *softMask);
    
    void CreateSoftMasks(const WDL_TypedBuf<BL_FLOAT> magns[2],
                         const WDL_TypedBuf<BL_FLOAT> &mask,
                         WDL_TypedBuf<BL_FLOAT> softMasks[2]);

    void CreateSoftMasksComp(const WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2],
                             const WDL_TypedBuf<BL_FLOAT> &mask,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> softMasks[2]);

    
    //
    DUETHistogram *mHistogram;
    WDL_TypedBuf<BL_FLOAT> mCurrentHistogramData;
    
    //
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns[2];
    WDL_TypedBuf<BL_FLOAT> mCurrentPhases[2];
    
    //
    BL_FLOAT mThresholdFloor;
    BL_FLOAT mThresholdPeaks;
    BL_FLOAT mThresholdPeaksWidth;
    
    bool mKernelSmoothFlag;
    
    bool mDispThresholded;
    bool mDispMaxima;
    bool mDispMasks;
    
    //
    BL_FLOAT mSampleRate;
    
    //
    ImageSmootherKernel *mSmootherKernel;
    
    //
    PeakDetector2D *mPeakDetector;
    
    //
    bool mPickingActive;
    
    BL_FLOAT mPickX;
    BL_FLOAT mPickY;
    
    bool mPickModeBg;
    
    //
    SoftMasking *mSoftMasking[2];
    bool mUseSoftMasks;
    
    SoftMaskingComp *mSoftMaskingComp[2];
    bool mUseSoftMasksComp;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mCurrentFftData[2];
    
    //
    bool mUseDuetSoftMasks;
    bool mThresholdAll;
};

#endif /* defined(__BL_DUET__DUETSeparator__) */
