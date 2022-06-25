/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  DUETSeparator2.h
//  BL-DUET
//
//  Created by applematuer on 5/3/20.
//
//

#ifndef __BL_DUET__DUETSeparator2__
#define __BL_DUET__DUETSeparator2__

#include <PeakDetector2D2.h>

#include "IPlug_include_in_plug_hdr.h"

class DUETHistogram2;
class ImageSmootherKernel;
class SoftMasking2;
class SoftMaskingComp2;

// DUETSeparator2: from DUETSeparator2
// - code clean

class DUETSeparator2
{
public:
    DUETSeparator2(int histogramSize, BL_FLOAT sampleRate);
    
    virtual ~DUETSeparator2();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetHistogramSize(int histoSize);
    
    void SetInputData(const WDL_TypedBuf<BL_FLOAT> magns[2],
                      const WDL_TypedBuf<BL_FLOAT> phases[2]);
    
    // Alternate method
    void SetInputDataComp(const WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2]);
    
    void Process();
    
    void GetOutputData(WDL_TypedBuf<BL_FLOAT> magns[2],
                       WDL_TypedBuf<BL_FLOAT> phases[2]);
    
    // Alternate method
    void GetOutputDataComp(WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2]);
    
    //
    int GetHistogramWidth();
    int GetHistogramHeight();
    void GetHistogramData(WDL_TypedBuf<BL_FLOAT> *data);
    
    //
    void SetTimeSmooth(BL_FLOAT smoothFactor);
    
    //
    void SetUseKernelSmooth(bool kernelSmoothFlag);
    
    //
    void SetThresholdFloor(BL_FLOAT threshold);
    void SetThresholdPeaks(BL_FLOAT threshold);
    void SetThresholdPeaksWidth(BL_FLOAT threshold);
    
    void SetDisplayThresholded(bool flag);
    void SetDisplayMaxima(bool flag);
    void SetDisplayMasks(bool flag);
    
    // Wiener soft masking
    void SetUseSoftMasks(bool flag);
    void SetUseSoftMasksComp(bool flag);
    void SetSoftMaskSize(int size);
    
    // Do not use binary masks, but varying gradient masks
    void SetUseGradientMasks(bool flag);
    
    void SetThresholdAll(bool flag);
    
    // Picking
    void SetPickingActive(bool flag);
    void SetInvertPickSelection(bool flag);
    void SetPickPosition(BL_FLOAT x, BL_FLOAT y);
    
    //
    void SetAlphaZoom(BL_FLOAT zoom);
    void SetDeltaZoom(BL_FLOAT zoom);
    
    //
    void SetUsePhaseAliasingCorrection(bool flag);
    int GetPACOversampling();
    void SetPACOversamplesFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> oversampledFft[2]);
    
protected:
    // First version
    void FillHistogram(vector<int> *oobSamples = NULL);
    
    // Fixed version, closer to the paper
    // See: https://www.researchgate.net/publication/227143748_The_DUET_blind_source_separation_algorithm
    void FillHistogram2(vector<int> *oobSamples = NULL);
    
    // Phase aliasing correction
    // See: file:///Users/applematuer/Downloads/1-s2.0-S1063520313000043-main.pdf
    void FillHistogramPAC(vector<int> *oobSamples = NULL);
    BL_FLOAT ComputePACDelta(const WDL_TypedBuf<WDL_FFT_COMPLEX> oversampledFft[2],
                           int index, bool *success);
    
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
    
    // Gradients
    void ApplyGradientMask(int width, int height,
                           const WDL_TypedBuf<BL_FLOAT> &mask,
                           WDL_TypedBuf<BL_FLOAT> *magns);
    
    void ApplyGradientMaskComp(int width, int height,
                               const WDL_TypedBuf<BL_FLOAT> &mask,
                               WDL_TypedBuf<WDL_FFT_COMPLEX> *fftData);
    
    // Find maxima
    void DetectPeaks(int width, int height, WDL_TypedBuf<BL_FLOAT> *data);
    
    
    // Extract masks
    //
    void ExtractPeaks(int width, int height,
                      const WDL_TypedBuf<BL_FLOAT> &histogramData,
                      vector<PeakDetector2D2::Peak> *peaks);
    
    void CreateMaskDist(int width, int height,
                        const WDL_TypedBuf<BL_FLOAT> &histogramData,
                        const vector<PeakDetector2D2::Peak> &peaks,
                        WDL_TypedBuf<BL_FLOAT> *mask);
    
    void CreateMaskPeakDetector(int width, int height,
                                const WDL_TypedBuf<BL_FLOAT> &histogramData,
                                const vector<PeakDetector2D2::Peak> &peaks,
                                WDL_TypedBuf<int> *mask);
    
    void ComputePeaksAreas(const WDL_TypedBuf<int> &mask,
                           vector<PeakDetector2D2::Peak> *peaks);
    
    void MaskToFloatMask(const WDL_TypedBuf<int> &mask,
                         WDL_TypedBuf<BL_FLOAT> *floatMask);
    
    void InvertFloatMask(WDL_TypedBuf<BL_FLOAT> *floatMask);
    
    void SelectMask(WDL_TypedBuf<int> *mask, int peakId);

    int SelectMaskPicking(int width, int height,
                          WDL_TypedBuf<int> *mask,
                          WDL_TypedBuf<BL_FLOAT> *floatMask);
    
    void DBG_MaskToImage(const WDL_TypedBuf<int> &mask,
                         WDL_TypedBuf<BL_FLOAT> *image);
    
    // Wiener masks
    void CreateSoftMasks(const WDL_TypedBuf<BL_FLOAT> magns[2],
                         const WDL_TypedBuf<BL_FLOAT> &mask,
                         WDL_TypedBuf<BL_FLOAT> softMasks[2],
                         const vector<int> *oobSamples = NULL);
    
    void CreateSoftMasksComp(const WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2],
                             const WDL_TypedBuf<BL_FLOAT> &mask,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> softMasks[2],
                             const vector<int> *oobSamples = NULL);
    
    // Gradient masks
    void CreateGradientMask(int width, int height,
                            const WDL_TypedBuf<int> &mask,
                            const vector<PeakDetector2D2::Peak> &peaks,
                            WDL_TypedBuf<BL_FLOAT> *floatMask);

    // Thresholded histogram to float mask
    void HistogramToFloatMask(const WDL_TypedBuf<BL_FLOAT> &thresholdedHisto,
                              WDL_TypedBuf<BL_FLOAT> *floatMask);

    
    //
    //
    
    BL_FLOAT mSampleRate;
    
    //
    DUETHistogram2 *mHistogram;
    WDL_TypedBuf<BL_FLOAT> mCurrentHistogramData;
    
    //
    WDL_TypedBuf<BL_FLOAT> mInputMagns[2];
    WDL_TypedBuf<BL_FLOAT> mInputPhases[2];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mInputFftData[2];
    
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns[2];
    WDL_TypedBuf<BL_FLOAT> mCurrentPhases[2];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mCurrentFftData[2];
    
    //
    BL_FLOAT mThresholdFloor;
    BL_FLOAT mThresholdPeaks;
    BL_FLOAT mThresholdPeaksWidth;
    
    bool mUseKernelSmoothFlag;
    
    bool mDispThresholded;
    bool mDispMaxima;
    bool mDispMasks;
    
    //
    bool mUseGradientMasks;
    bool mThresholdAll;
    
    //
    ImageSmootherKernel *mSmootherKernel;
    
    //
    PeakDetector2D2 *mPeakDetector;
    
    // Picking
    bool mPickingActive;
    
    BL_FLOAT mPickX;
    BL_FLOAT mPickY;
    
    bool mInvertPickSelection;
    
    // Soft masking (Wiener)
    SoftMasking2 *mSoftMasking[2];
    bool mUseSoftMasks;
    
    SoftMaskingComp2 *mSoftMaskingComp[2];
    bool mUseSoftMasksComp;
    
    //
    BL_FLOAT mAlphaZoom;
    BL_FLOAT mDeltaZoom;
    
    //
    bool mMustResetHistogram;
    
    //
    bool mUsePhaseAliasingCorrection;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mPACOversampledFft[2];
};

#endif /* defined(__BL_DUET__DUETSeparator2__) */
