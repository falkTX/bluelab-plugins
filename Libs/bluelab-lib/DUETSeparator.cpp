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
//  DUETSeparator.cpp
//  BL-DUET
//
//  Created by applematuer on 5/3/20.
//
//

#include <algorithm>
using namespace std;

#include <DUETHistogram.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include <ImageSmootherKernel.h>

#include <PeakDetector2D.h>

#include <SoftMasking.h>
#include <SoftMaskingComp.h>

#include "DUETSeparator.h"


#define TWO_PI 6.28318530717959

// GOOD ! (makes phases ok)
//#define USE_HISTO_VALUE_MAGN 1 //0
#define USE_HISTO_VALUE_MAGN 0

// For USE_HISTO_VALUE_MAGN 0
//#define HISTO_MAX_VALUE 8.0

// For USE_HISTO_VALUE_MAGN 1
#define HISTO_MAX_VALUE 1.0

//#define THRESHOLD_COEFF 0.05
#define THRESHOLD_COEFF 0.1

//
#define THRESHOLD_HARD_ELBOW 0 //1
#define THRESHOLD_SOFT_ELBOW 0

// TMP
#define SOFT_ELBOW_COEFF 40.0

// Paper: 35x50
#define HISTO_WIDTH 32 //64 //128 //32 //8
#define HISTO_HEIGHT 32 //64 //128 //32 //8

#define SMOOTH_KERNEL_SIZE 5 //3 //15

// Was just a test, not really finished
#define TEST_WAVELENGTH 0 //1

// Better with 0!
// The sound is better with 0, the separation sounds more accurate.
// With 1, the phases look more spread, but the sound may be worse.
#define SPREAD_PHASES 0 //1

// Simple coefficient
#define SPREAD_PHASES_X 1
#define SPREAD_PHASES_X_COEFF 1.0 //64.0 //16.0 //4.0

#define SPREAD_PHASES_Y 1
#define SPREAD_PHASES_Y_COEFF 1.0 //16.0 //4.0 //1.0


// Artifial coeff
#define HISTO_COEFF 16.0

#define PEAK_DETECTOR_NUM_POINTS 16 //64 //16

// Seems good with 16, (maybe better with 32? but too CPU costly!)
#define SOFT_MASKING_HISTO_SIZE 8 //4 //8 //16 //32 //8 //16 //4 //1 //16

// NOTE: for the moment, SoftMaskingComp makes strange stereo artifacts
// => the sound is better with the 1 value.
#define APPLY_MONO_MASK 1 //0


DUETSeparator::DUETSeparator(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mHistogram = new DUETHistogram(HISTO_WIDTH, HISTO_HEIGHT, HISTO_MAX_VALUE);
    
    mKernelSmoothFlag = false;
    
    //
    mThresholdFloor = 0.0;
    mThresholdPeaks = 0.0;
    mThresholdPeaksWidth = 0.0;
    
    mDispThresholded = false;
    mDispMaxima = false;
    mDispMasks = false;
    
    //
    mSmootherKernel = new ImageSmootherKernel(SMOOTH_KERNEL_SIZE);
    
    mPeakDetector = new PeakDetector2D(PEAK_DETECTOR_NUM_POINTS);
    
    //
    mPickingActive = false;
    
    mPickX = 0.0;
    mPickY = 0.0;
    
    mPickModeBg = false;
    
    for (int i = 0; i < 2; i++)
        mSoftMasking[i] = new SoftMasking(SOFT_MASKING_HISTO_SIZE);
    mUseSoftMasks = false;
    
    for (int i = 0; i < 2; i++)
        mSoftMaskingComp[i] = new SoftMaskingComp(SOFT_MASKING_HISTO_SIZE);
    mUseSoftMasksComp = false;
    
    mUseDuetSoftMasks = false;
    mThresholdAll = false;
}

DUETSeparator::~DUETSeparator()
{
    delete mHistogram;
    
    delete mSmootherKernel;
    
    delete mPeakDetector;
    
    for (int i = 0; i < 2; i++)
        delete mSoftMasking[i];
    
    for (int i = 0; i < 2; i++)
        delete mSoftMaskingComp[i];
}

void
DUETSeparator::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mHistogram->Reset();
    
    for (int i = 0; i < 2; i++)
        mSoftMasking[i]->Reset();
    
    for (int i = 0; i < 2; i++)
        mSoftMaskingComp[i]->Reset();
}

void
DUETSeparator::SetInputData(const WDL_TypedBuf<BL_FLOAT> magns[2],
                            const WDL_TypedBuf<BL_FLOAT> phases[2])
{
    mCurrentMagns[0] = magns[0];
    mCurrentMagns[1] = magns[1];
    
    mCurrentPhases[0] = phases[0];
    mCurrentPhases[1] = phases[1];
    
    //FillHistogram();
    FillHistogram2();
}

void
DUETSeparator::SetInputData(const WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2])
{
    mCurrentFftData[0] = fftData[0];
    mCurrentFftData[1] = fftData[1];
}

void
DUETSeparator::Process()
{
    int width = mHistogram->GetWidth();
    int height = mHistogram->GetHeight();
    mHistogram->GetData(&mCurrentHistogramData);
    
    BLUtils::MultValues(&mCurrentHistogramData, (BL_FLOAT)HISTO_COEFF);
    
    if (mKernelSmoothFlag)
    {
        // Smooth
        mSmootherKernel->SmoothImage(width, height, &mCurrentHistogramData);
    }
    
    WDL_TypedBuf<BL_FLOAT> thresholdedHistogram = mCurrentHistogramData;
    ThresholdHistogram(width, height, &thresholdedHistogram,
                       mThresholdFloor);

    
    // Detect peaks
    WDL_TypedBuf<BL_FLOAT> maximaHistogram = mCurrentHistogramData/*thresholdedHistogram*/;
    DetectPeaks(width, height, &maximaHistogram);
    
    WDL_TypedBuf<BL_FLOAT> maximaHistogramThrs = maximaHistogram;
    ThresholdHistogram(width, height, &maximaHistogramThrs,
                       mThresholdPeaks);
    
    vector<PeakDetector2D::Peak> peaks;
    ExtractPeaks(width, height, maximaHistogramThrs, &peaks);
    //sort(peaks.begin(), peaks.end(), PeakDetector2D::Peak::IntensityGreater);
    
    WDL_TypedBuf<BL_FLOAT> mask;
    //CreateMaskDist(width, height, thresholdedHistogram, peaks, &mask);
    CreateMaskPeakDetector(width, height, thresholdedHistogram, peaks, &mask);
    
    ComputePeaksAreas(mask, &peaks);
    sort(peaks.begin(), peaks.end(), PeakDetector2D::Peak::AreaGreater);
    
    // Debug
    int peakId = -1;
    if (!peaks.empty())
        peakId = peaks[0].mId;
    
    // TEST
    //SelectMask(&mask, peakId);
    //SelectMask(&mask, -1);
    
    // Sound
    //ThresholdSound(width, height, &mask, mCurrentMagns);
    
    // TMP
    //SelectMaskPicking(width, height, &mask);
    
    // Display
    WDL_TypedBuf<BL_FLOAT> maskImage;
    DBG_MaskToImage(mask, &maskImage);
    
    if (mThresholdAll)
    {
        SelectMaskThresholdAll(&mask);
        
        DBG_MaskToImage(mask, &maskImage);
    }
    
    WDL_TypedBuf<BL_FLOAT> duetSoftMask;
    if (mUseDuetSoftMasks)
    {
        CreateDuetSoftMask(width, height, mask,
                           peaks, &duetSoftMask);
        
        if (mPickModeBg)
        {
            for (int i = 0; i < duetSoftMask.GetSize(); i++)
            {
                BL_FLOAT val = duetSoftMask.Get()[i];
                val = 1.0 - val;
                duetSoftMask.Get()[i] = val;
            }
        }
        
        DBG_SoftMaskToImage(duetSoftMask, &maskImage);
        BLUtils::MultValues(&maskImage, (BL_FLOAT)0.01);
    }
    
    // Sound
    if (!mThresholdAll)
        SelectMaskPicking(width, height, &mask);
    //else
    //{
    //    SelectMaskThresholdAll(&mask);
    //    DBG_MaskToImage(mask, &maskImage);
    //}
    
    if (mUseSoftMasks && !mUseSoftMasksComp)
    {
        if (mUseDuetSoftMasks)
        {
            for (int i = 0; i < 2; i++)
                ApplyDuetSoftMask(width, height, duetSoftMask, &mCurrentMagns[i]);
        }
        
        WDL_TypedBuf<BL_FLOAT> softMasks[2];
        CreateSoftMasks(mCurrentMagns, mask, softMasks);
        
        for (int i = 0; i < 2; i++)
        {
            int maskIndex = i;
#if APPLY_MONO_MASK
            maskIndex = 0;
#endif
            
            ApplySoftMask(width, height, softMasks[maskIndex], &mCurrentMagns[i]);
        }
    }
    else if (mUseSoftMasksComp)
    {
        if (mUseDuetSoftMasks)
        {
            for (int i = 0; i < 2; i++)
                ApplyDuetSoftMaskComp(width, height, duetSoftMask, &mCurrentFftData[i]);
        }
        
        WDL_TypedBuf<WDL_FFT_COMPLEX> softMasksComp[2];
        CreateSoftMasksComp(mCurrentFftData, mask, softMasksComp);
                
        for (int i = 0; i < 2; i++)
        {
            int maskIndex = i;
#if APPLY_MONO_MASK
            maskIndex = 0;
#endif
            
            ApplySoftMaskComp(width, height, softMasksComp[maskIndex], &mCurrentFftData[i]);
        }
    }
    else
    {
        if (mUseDuetSoftMasks)
        {
            for (int i = 0; i < 2; i++)
                ApplyDuetSoftMask(width, height, duetSoftMask, &mCurrentMagns[i]);
        }
        
        ThresholdSound(width, height, mask, mCurrentMagns);
    }
    
    if (mDispThresholded)
        mCurrentHistogramData = thresholdedHistogram;
    
    if (mDispMaxima)
        mCurrentHistogramData = maximaHistogramThrs;
    
    if (mDispMasks)
        mCurrentHistogramData = maskImage;
    
    // Othe method: elbow.. (TEST)
    
    // Hard elbow
#if THRESHOLD_HARD_ELBOW
    //ThresholdHistogramHE(width, height, &mCurrentHistogramData);
    ThresholdSoundHE(width, height, &mCurrentHistogramData, mCurrentMagns);
#endif
    
    // Soft elbow
#if THRESHOLD_SOFT_ELBOW
    ThresholdHistogramSE(width, height, &mCurrentHistogramData);
    ThresholdSoundSE(width, height, &mCurrentHistogramData, mCurrentMagns);
#endif
}

void
DUETSeparator::GetOutputData(WDL_TypedBuf<BL_FLOAT> magns[2],
                             WDL_TypedBuf<BL_FLOAT> phases[2])
{
    magns[0] = mCurrentMagns[0];
    magns[1] = mCurrentMagns[1];
    
    phases[0] = mCurrentPhases[0];
    phases[1] = mCurrentPhases[1];
}

void
DUETSeparator::GetOutputData(WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2])
{
    fftData[0] = mCurrentFftData[0];
    fftData[1] = mCurrentFftData[1];
}

int
DUETSeparator::GetHistogramWidth()
{
    int width = mHistogram->GetWidth();
    
    return width;
}

int
DUETSeparator::GetHistogramHeight()
{
    int height = mHistogram->GetHeight();
    
    return height;
}

void
DUETSeparator::GetHistogramData(WDL_TypedBuf<BL_FLOAT> *data)
{
    *data = mCurrentHistogramData;
}

void
DUETSeparator::SetThresholdFloor(BL_FLOAT threshold)
{
    mThresholdFloor = threshold;
}

void
DUETSeparator::SetThresholdPeaks(BL_FLOAT threshold)
{
    mThresholdPeaks = threshold;
}

void
DUETSeparator::SetThresholdPeaksWidth(BL_FLOAT threshold)
{
    mThresholdPeaksWidth = threshold;
}

void
DUETSeparator::SetDisplayThresholded(bool flag)
{
    mDispThresholded = flag;
}

void
DUETSeparator::SetDisplayMaxima(bool flag)
{
    mDispMaxima = flag;
}

void
DUETSeparator::SetDisplayMasks(bool flag)
{
    mDispMasks = flag;
}

void
DUETSeparator::SetUseSoftMasks(bool flag)
{
    mUseSoftMasks = flag;
}

void
DUETSeparator::SetUseSoftMasksComp(bool flag)
{
    mUseSoftMasksComp = flag;
}

void
DUETSeparator::SetSoftMaskSize(int size)
{
    for (int i = 0; i < 2; i++)
        mSoftMasking[i]->SetHistorySize(size);
    
    for (int i = 0; i < 2; i++)
        mSoftMaskingComp[i]->SetHistorySize(size);
}

//
void
DUETSeparator::SetUseDuetSoftMasks(bool flag)
{
    mUseDuetSoftMasks = flag;
}

void
DUETSeparator::SetThresholdAll(bool flag)
{
    mThresholdAll = flag;
}

void
DUETSeparator::SetPickingActive(bool flag)
{
    mPickingActive = flag;
}

void
DUETSeparator::SetPickPosition(BL_FLOAT x, BL_FLOAT y)
{
    mPickX = x;
    mPickY = y;
}

void
DUETSeparator::SetPickModeBg(bool flag)
{
    mPickModeBg = flag;
}

void
DUETSeparator::SetTimeSmooth(BL_FLOAT smoothFactor)
{
    mHistogram->SetTimeSmooth(smoothFactor);
}

void
DUETSeparator::SetKernelSmooth(bool kernelSmoothFlag)
{
    mKernelSmoothFlag = kernelSmoothFlag;
}

void
DUETSeparator::FillHistogram()
{
#define EPS 1e-15
    
    mHistogram->Clear();
    
#if TEST_WAVELENGTH
    int numBins = mCurrentMagns[0].GetSize();
    WDL_TypedBuf<BL_FLOAT> freqs;
    BLUtils::FftFreqs(&freqs, numBins, mSampleRate);
    BL_FLOAT freq0 = freqs.Get()[1];
    BL_FLOAT freqMax = freqs.Get()[freqs.GetSize() - 1];
#endif
    
    for (int i = 0; i < mCurrentMagns[0].GetSize(); i++)
    {
        // Alpha
        BL_FLOAT magn0 = mCurrentMagns[0].Get()[i];
        BL_FLOAT magn1 = mCurrentMagns[1].Get()[i];
        
        //
        BL_FLOAT alpha = 0.0;
        if ((magn0 > EPS) && (magn1 > EPS))
        {
            if (magn0 >= magn1)
            {
                alpha = magn1/magn0;
                
                alpha = ((1.0 - alpha) + 1.0)*0.5;
            }
            
            if (magn1 > magn0)
            {
                alpha = magn0/magn1;
                
                alpha = alpha*0.5;
            }
        }
        
        alpha = 1.0 - alpha;
        
#if SPREAD_PHASES_Y
        alpha = (alpha - 0.5)*SPREAD_PHASES_Y_COEFF + 0.5;
#endif

        // Delta
        BL_FLOAT phase0 = mCurrentPhases[0].Get()[i];
        BL_FLOAT phase1 = mCurrentPhases[1].Get()[i];
        
        BL_FLOAT delta = (phase0 - phase1)*(1.0/TWO_PI);
        
#if TEST_WAVELENGTH
        BL_FLOAT freq = freqs.Get()[i];
        delta *= freq;
        delta /= freqMax;
#endif
        
#if SPREAD_PHASES_X
        delta *= SPREAD_PHASES_X_COEFF;
#endif

#if SPREAD_PHASES
        if (delta > 0.0)
            delta = BLUtils::ApplyParamShape(delta, 2.0);
        else
        {
            delta = -delta;
            delta = BLUtils::ApplyParamShape(delta, 2.0);
            delta = -delta;
        }
#endif
        
        /// Normalize
        delta = (delta + 1.0)*0.5;
        
        // Add to histogram
        BL_FLOAT value = 0.001;
#if USE_HISTO_VALUE_MAGN
        value = magn0;
#endif
        
        mHistogram->AddValue(alpha, delta, value, i);
    }
    
    //
    mHistogram->Process();
}

void
DUETSeparator::FillHistogram2()
{
#define KEEP_OUT_BOUNDS_SAMPLES 1
    
#define EPS 1e-15
    
    mHistogram->Clear();
        
    for (int i = 0; i < mCurrentMagns[0].GetSize(); i++)
    {
        // Alpha
        BL_FLOAT magn0 = mCurrentMagns[0].Get()[i];
        BL_FLOAT magn1 = mCurrentMagns[1].Get()[i];
        
        //
        BL_FLOAT alpha = (magn1 + EPS)/(magn0 + EPS);
        
        // Symetric attenuation
        alpha = alpha - 1.0/alpha;
        
        // Niko
#define MAX_ALPHA 2.0 //5.0 //20.0 //1.0 //20.0
        // Paper
//#define MAX_ALPHA 0.7
        alpha *= 1.0/MAX_ALPHA;
        
        alpha = (alpha + 1.0)*0.5;
        
#if KEEP_OUT_BOUNDS_SAMPLES
        if (alpha < 0.0)
            alpha = 0.0;
        if (alpha > 1.0)
            alpha = 1.0;
#endif
        
        // Delta
        BL_FLOAT phase0 = mCurrentPhases[0].Get()[i];
        BL_FLOAT phase1 = mCurrentPhases[1].Get()[i];
        
        BL_FLOAT delta = (phase0 - phase1);
        //delta /= TWO_PI*((BL_FLOAT)i)/(mCurrentMagns[0].GetSize() + 1);
        
        // TEST
        // See: http://mural.maynoothuniversity.ie/2318/1/Final_Thesis_Ye_Liang.pdf
        //delta = 1.0/(delta + EPS);
        //delta *= 0.01;
        
        // Niko
#define MAX_DELTA 5.0 //10.0 //20.0 //80.0 //40.0 //10.0 //40.0
        // Paper
        //#define MAX_DELTA 3.6
        delta *= 1.0/MAX_DELTA;
        
        delta = (delta + 1.0)*0.5;
        
#if KEEP_OUT_BOUNDS_SAMPLES
        if (delta < 0.0)
            delta = 0.0;
        if (delta > 1.0)
            delta = 1.0;
#endif
        
        // Weight, from the paper
        //
        // p=0,q= 0: the counting histogram proposed in the original DUETalgorithm
        // p=1,q= 0: motivated by the ML symmetric attenuation estimator
        // p=1,q= 2: motivated by the ML delay estimator
        // p=2,q= 0: in order to reduce delay estimator bias
        // p=2,q= 2: for low signal-to-noise ratio or speech mixtures
        //
        
        // Paper: (1, 0)
#define WEIGHT_P 1.0 //0.0 //1.0
#define WEIGHT_Q 0.0 //1.0 //0.0
        BL_FLOAT w0 = magn0*magn1;
        if (std::fabs(WEIGHT_P) <= 0.0)
        {
            w0 = 1.0;
        }
        else if (std::fabs(WEIGHT_P - 1.0) > 0.0)
        {
	  w0 = std::pow(w0, WEIGHT_P);
        }
        
        // See Euler formula
        // See: https://blog.endaq.com/fourier-transform-basics
        BL_FLOAT w1 = TWO_PI*((BL_FLOAT)i)/(mCurrentMagns[0].GetSize() + 1);
        if (std::fabs(WEIGHT_Q) <= 0.0)
        {
            w1 = 1.0;
        }
        else if (std::fabs(WEIGHT_Q - 1.0) > 0.0)
        {
	  w1 = std::pow(w1, WEIGHT_Q);
        }
        
        // Final weight
        BL_FLOAT weight = w0*w1;
        
        // Niko
#define WEIGHT_COEFF 128.0 //32.0
        if (std::fabs(WEIGHT_P) > 0.0) // Hack
            weight *= WEIGHT_COEFF;
        else
            weight *= 0.001;
        
        mHistogram->AddValue(alpha, delta, weight, i);
    }
    
    //
    mHistogram->Process();
}

void
DUETSeparator::ThresholdHistogram(int width, int height, WDL_TypedBuf<BL_FLOAT> *data,
                                  BL_FLOAT threshold)
{
    for (int i = 0; i < data->GetSize(); i++)
    {
        BL_FLOAT val = data->Get()[i];
        
        if (val < threshold*THRESHOLD_COEFF)
            val = 0.0;
        
        data->Get()[i] = val;
    }
}

void
DUETSeparator::ThresholdSound(int width, int height,
                              const WDL_TypedBuf<BL_FLOAT> &mask,
                              WDL_TypedBuf<BL_FLOAT> magns[2])
{
    for (int i = 0; i < mask.GetSize(); i++)
    {
        BL_FLOAT val = mask.Get()[i];
        
        //if (std::fabs(val) > EPS)
        if (std::fabs(val) < EPS) // Logical
        // Thresholded => mute sound
        {
            vector<int> indices;
            mHistogram->GetIndices(i, &indices);
            
            for (int j = 0; j < indices.size(); j++)
            {
                int idx = indices[j];
                
                if ((idx >= 0) && (idx < magns[0].GetSize()))
                {
                    magns[0].Get()[idx] = 0.0;
                    magns[1].Get()[idx] = 0.0;
                }
            }
        }
    }
}

void
DUETSeparator::ApplySoftMask(int width, int height,
                             const WDL_TypedBuf<BL_FLOAT> &mask,
                             WDL_TypedBuf<BL_FLOAT> *magns)
{
    for (int i = 0; i < mask.GetSize(); i++)
    {
        BL_FLOAT maskVal = mask.Get()[i];
        BL_FLOAT val = magns->Get()[i];
        
        val *= maskVal;
        
        magns->Get()[i] = val;
    }
}

void
DUETSeparator::ApplySoftMaskComp(int width, int height,
                                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &mask,
                                 WDL_TypedBuf<WDL_FFT_COMPLEX> *fftData)
{
    for (int i = 0; i < mask.GetSize(); i++)
    {
        WDL_FFT_COMPLEX maskVal = mask.Get()[i];
        WDL_FFT_COMPLEX val = fftData->Get()[i];
        
        WDL_FFT_COMPLEX res;
        COMP_MULT(val, maskVal, res);
        
        //val *= maskVal;
        
        fftData->Get()[i] = res;
    }
}

void
DUETSeparator::ApplyDuetSoftMask(int width, int height,
                                 const WDL_TypedBuf<BL_FLOAT> &mask,
                                 WDL_TypedBuf<BL_FLOAT> *magns)
{
    for (int i = 0; i < mask.GetSize(); i++)
    {
        BL_FLOAT maskVal = mask.Get()[i];
        
        vector<int> indices;
        mHistogram->GetIndices(i, &indices);
            
        for (int j = 0; j < indices.size(); j++)
        {
            int idx = indices[j];
                
            if ((idx >= 0) && (idx < magns->GetSize()))
            {
                BL_FLOAT magn = magns->Get()[idx];
                
                magn *= maskVal;
                
                magns->Get()[idx] = magn;
            }
        }
    }
}

void
DUETSeparator::ApplyDuetSoftMaskComp(int width, int height,
                                     const WDL_TypedBuf<BL_FLOAT> &mask,
                                     WDL_TypedBuf<WDL_FFT_COMPLEX> *fftData)
{
    for (int i = 0; i < mask.GetSize(); i++)
    {
        BL_FLOAT maskVal = mask.Get()[i];
        
        vector<int> indices;
        mHistogram->GetIndices(i, &indices);
        
        for (int j = 0; j < indices.size(); j++)
        {
            int idx = indices[j];
            
            if ((idx >= 0) && (idx < fftData->GetSize()))
            {
                WDL_FFT_COMPLEX fftVal = fftData->Get()[idx];
                
                fftVal.re *= maskVal;
                fftVal.im *= maskVal;
                
                fftData->Get()[idx] = fftVal;
            }
        }
    }
}

#if 0
void
DUETSeparator::ThresholdHistogramHE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data)
{
    for (int i = 0; i < data->GetSize(); i++)
    {
        BL_FLOAT val = data->Get()[i];
        
        //if (val > mThreshold*THRESHOLD_COEFF)
        //    val = HISTO_MAX_VALUE;
        
        if (val < mThresholdFloor*THRESHOLD_COEFF)
            val = 0.0;
        
        data->Get()[i] = val;
    }
}
#endif

#if 0
void
DUETSeparator::ThresholdSoundHE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data,
                                WDL_TypedBuf<BL_FLOAT> magns[2])
{
    for (int i = 0; i < data->GetSize(); i++)
    {
        BL_FLOAT val = data->Get()[i];
        
        if (val <= 0.0)
            // Thresholded => mute sound
        {
            vector<int> indices;
            mHistogram->GetIndices(i, &indices);
            
            for (int j = 0; j < indices.size(); j++)
            {
                int idx = indices[j];
                
                if ((idx >= 0) && (idx < magns[0].GetSize()))
                {
                    magns[0].Get()[idx] = 0.0;
                    magns[1].Get()[idx] = 0.0;
                }
            }
        }
    }
}
#endif

#if 0
void
DUETSeparator::ThresholdHistogramSE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data)
{
    for (int i = 0; i < data->GetSize(); i++)
    {
        BL_FLOAT val = data->Get()[i];
        
        val -= mThresholdFloor*THRESHOLD_COEFF;
        if (val < 0.0)
            val = 0.0;
        
        data->Get()[i] = val;
    }
}
#endif

#if 0
void
DUETSeparator::ThresholdSoundSE(int width, int height, WDL_TypedBuf<BL_FLOAT> *data,
                                WDL_TypedBuf<BL_FLOAT> magns[2])
{
    for (int i = 0; i < mCurrentHistogramData.GetSize(); i++)
    {
        BL_FLOAT val = mCurrentHistogramData.Get()[i];
        
        val *= SOFT_ELBOW_COEFF;
        if (val > 1.0)
            val = 1.0;
        
        vector<int> indices;
        mHistogram->GetIndices(i, &indices);
            
        for (int j = 0; j < indices.size(); j++)
        {
            int idx = indices[j];
                
            if ((idx >= 0) && (idx < magns[0].GetSize()))
            {
                magns[0].Get()[idx] *= val;
                magns[1].Get()[idx] *= val;
            }
        }
    }
}
#endif

void
DUETSeparator::DetectPeaks(int width, int height,
                           WDL_TypedBuf<BL_FLOAT> *data)
{
    BLUtils::FindMaxima2D(width, height, data, true);
}

#if 0
void
DUETSeparator::SeparatePeaks(int width, int height,
                             WDL_TypedBuf<BL_FLOAT> *data)
{
    BLUtils::SeparatePeaks2D2(width, height, data, true);
}
#endif

void
DUETSeparator::ExtractPeaks(int width, int height,
                            const WDL_TypedBuf<BL_FLOAT> &histogramData,
                            vector<PeakDetector2D::Peak> *peaks)
{
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT val = histogramData.Get()[i + j*width];
            if (val > 0.0)
            {
                PeakDetector2D::Peak p;
                p.mX = i;
                p.mY = j;
                
                p.mId = peaks->size() + 1;
                
                p.mIntensity = val;
                
                peaks->push_back(p);
            }
        }
    }
}

void
DUETSeparator::CreateMaskDist(int width, int height,
                              const WDL_TypedBuf<BL_FLOAT> &histogramData,
                              const vector<PeakDetector2D::Peak> &peaks,
                              WDL_TypedBuf<BL_FLOAT> *mask)
{
    mask->Resize(histogramData.GetSize());
    BLUtils::FillAllZero(mask);
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT val = histogramData.Get()[i + j*width];
            
            if (val > 0.0)
            {
                int closestIdx = -1;
                BL_FLOAT closestDistance2 = width*width;
                
                for (int k = 0; k < peaks.size(); k++)
                {
                    const PeakDetector2D::Peak &p = peaks[k];
                    
                    BL_FLOAT dist2 = (p.mX - i)*(p.mX - i) + (p.mY - j)*(p.mY - j);
                    if (dist2 < closestDistance2)
                    {
                        closestDistance2 = dist2;
                        
                        closestIdx = p.mId;
                    }
                }
                
                mask->Get()[i + j*width] = closestIdx;
            }
        }
    }
}

void
DUETSeparator::CreateMaskPeakDetector(int width, int height,
                                      const WDL_TypedBuf<BL_FLOAT> &histogramData,
                                      const vector<PeakDetector2D::Peak> &peaks,
                                      WDL_TypedBuf<BL_FLOAT> *mask)
{
    //mPeakDetector->DetectPeaks(width, height, histogramData,
    //peaks, mThresholdPeaks, mask);
    
    mPeakDetector->DetectPeaksNoOverlap(width, height, histogramData,
                                        peaks, mThresholdPeaks, mThresholdPeaksWidth, mask);
}

void
DUETSeparator::ComputePeaksAreas(const WDL_TypedBuf<BL_FLOAT> &mask,
                                 vector<PeakDetector2D::Peak> *peaks)
{
#define EPS 1e-15
    
    for (int i = 0; i < peaks->size(); i++)
    {
        PeakDetector2D::Peak &peak = (*peaks)[i];
        
        BL_FLOAT area = 0.0;
        for (int j = 0; j < mask.GetSize(); j++)
        {
            BL_FLOAT val = mask.Get()[j];
            
            if (std::fabs(val - peak.mId) < EPS)
                area++;
        }
        
        // Normalize
        if (mask.GetSize() > 0)
            area /= mask.GetSize();
        
        peak.mArea = area;
    }
}

void
DUETSeparator::SelectMask(WDL_TypedBuf<BL_FLOAT> *mask, int peakId)
{
    for (int i = 0; i < mask->GetSize(); i++)
    {
        BL_FLOAT val = mask->Get()[i];
        
        if (peakId >= 0)
        {
	  if (std::fabs(val - peakId) > EPS)
                mask->Get()[i] = 0.0;
        }
        else
        {
	  if (std::fabs(val) < EPS)
                mask->Get()[i] = 1.0;
            else
                mask->Get()[i] = 0.0;
        }
    }
}

void
DUETSeparator::SelectMaskThresholdAll(WDL_TypedBuf<BL_FLOAT> *mask)
{
    for (int i = 0; i < mask->GetSize(); i++)
    {
        BL_FLOAT val = mask->Get()[i];
        
        if (std::fabs(val) < EPS)
            mask->Get()[i] = 0.0;
        else
            mask->Get()[i] = 1.0;
    }
}

void
DUETSeparator::SelectMaskPicking(int width, int height,
                                 WDL_TypedBuf<BL_FLOAT> *mask)
{
    if (!mPickingActive)
    {
        BLUtils::FillAllValue(mask, (BL_FLOAT)1.0);
        
        return;
    }
    
    int x = mPickX*width;
    int y = mPickY*height;
    
    if ((x < 0) || (x >= width) ||
        (y < 0) || (y >= height))
    {
        for (int i = 0; i < mask->GetSize(); i++)
        {
            mask->Get()[i] = 0.0;
        }
        
        return;
    }
    
    BL_FLOAT pickValue = mask->Get()[x + y*width];
    
    for (int i = 0; i < mask->GetSize(); i++)
    {
        BL_FLOAT val = mask->Get()[i];
        
        if (!mPickModeBg)
        {
            if (val > 0.0)
            {
	      if (std::fabs(pickValue - val) > EPS)
                    mask->Get()[i] = 0.0;
            }
        }
        else
        {
            if (val <= 0.0)
            // Outside any mask
            {
	      if (std::fabs(pickValue) > EPS)
                    mask->Get()[i] = 0.0;
                else
                    mask->Get()[i] = 1.0;
            }
            else
            // Inside a mask
            {
                // Select all masks at the same time when alt is pressed
	      if (std::fabs(pickValue) > EPS)
                    mask->Get()[i] = 1.0;
                else
                    mask->Get()[i] = 0.0;
            }
        }
    }
}

void
DUETSeparator::DBG_MaskToImage(const WDL_TypedBuf<BL_FLOAT> &mask,
                               WDL_TypedBuf<BL_FLOAT> *image)
{
    image->Resize(mask.GetSize());
    BLUtils::FillAllZero(image);
    
    for (int i = 0; i < mask.GetSize(); i++)
    {
        BL_FLOAT val = mask.Get()[i];
        if (val > 0.0)
        {
            // Let's say we won't have more than 100 mask different values
            BL_FLOAT col = (((int)val) % 5 + 1)*0.01;
            
            image->Get()[i] = col;
        }
    }
}

void
DUETSeparator::DBG_SoftMaskToImage(const WDL_TypedBuf<BL_FLOAT> &mask,
                                   WDL_TypedBuf<BL_FLOAT> *image)
{
    *image = mask;
}

void
DUETSeparator::CreateDuetSoftMask(int width, int height,
                                  const WDL_TypedBuf<BL_FLOAT> &mask,
                                  const vector<PeakDetector2D::Peak> &peaks,
                                  WDL_TypedBuf<BL_FLOAT> *softMask)
{
#define EPS 1e-15
    
    softMask->Resize(mask.GetSize());
    BLUtils::FillAllZero(softMask);
    
    for (int k = 0; k < peaks.size(); k++)
    {
        const PeakDetector2D::Peak &peak = peaks[k];
        
        int id = peak.mId;
        int x = peak.mX;
        int y = peak.mY;
        
        //BL_FLOAT maxDist = std::sqrt(p.mArea)*0.5;
        //BL_FLOAT maxDist2 = peak.mArea*0.25;
        BL_FLOAT maxDist2 = peak.mArea*mask.GetSize();
        for (int i = 0; i < width; i++)
        {
            for (int j = 0; j < height; j++)
            {
                BL_FLOAT val = mask.Get()[i + j*width];
                if (std::fabs(val - id) < EPS)
                {
                    BL_FLOAT dist2 = (i - x)*(i - x) + (j - y)*(j- y);
                    
                    BL_FLOAT t = 1.0;
                    if (maxDist2 > EPS)
                    {
                        t = dist2 / maxDist2;
                        t = 1.0 - t;
                        
                        if (t < 0.0)
                            t = 0.0;
                        if (t > 1.0)
                            t = 1.0;
                    }
                    
                    softMask->Get()[i + j*width] = t;
                }
            }
        }
    }
}

void
DUETSeparator::CreateSoftMasks(const WDL_TypedBuf<BL_FLOAT> magns[2],
                               const WDL_TypedBuf<BL_FLOAT> &mask,
                               WDL_TypedBuf<BL_FLOAT> softMasks[2])
{
    // Create masks
    //
    
    WDL_TypedBuf<BL_FLOAT> thrsMagns[2];
    
    // By default, fill all
    thrsMagns[0] = magns[0];
    thrsMagns[1] = magns[1];
    
    // Then mute appropriate samples
    for (int i = 0; i < mask.GetSize(); i++)
    {
        BL_FLOAT val = mask.Get()[i];
        
        if (std::fabs(val) < EPS) // Logical
            // Thresholded => mute sound
        {
            vector<int> indices;
            mHistogram->GetIndices(i, &indices);
            
            for (int j = 0; j < indices.size(); j++)
            {
                int idx = indices[j];
                
                if ((idx >= 0) && (idx < magns[0].GetSize()))
                {
                    thrsMagns[0].Get()[idx] = 0.0;
                    thrsMagns[1].Get()[idx] = 0.0;
                }
            }
        }
    }
    
    // Create soft masks
    for (int i = 0; i < 2; i++)
    {
        mSoftMasking[i]->Process(magns[i], thrsMagns[i], &softMasks[i]);
    }
}

void
DUETSeparator::CreateSoftMasksComp(const WDL_TypedBuf<WDL_FFT_COMPLEX> fftData[2],
                                   const WDL_TypedBuf<BL_FLOAT> &mask,
                                   WDL_TypedBuf<WDL_FFT_COMPLEX> softMasks[2])
{
    // Create masks
    //
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> thrsFftData[2];
    
    // By default, fill all
    thrsFftData[0] = fftData[0];
    thrsFftData[1] = fftData[1];
    
    // Then mute appropriate samples
    for (int i = 0; i < mask.GetSize(); i++)
    {
        BL_FLOAT val = mask.Get()[i];
        
        if (std::fabs(val) < EPS) // Logical
            // Thresholded => mute sound
        {
            vector<int> indices;
            mHistogram->GetIndices(i, &indices);
            
            for (int j = 0; j < indices.size(); j++)
            {
                int idx = indices[j];
                
                if ((idx >= 0) && (idx < fftData[0].GetSize()))
                {
                    thrsFftData[0].Get()[idx].re = 0.0;
                    thrsFftData[0].Get()[idx].im = 0.0;
                    
                    thrsFftData[1].Get()[idx].re = 0.0;
                    thrsFftData[1].Get()[idx].im = 0.0;
                }
            }
        }
    }
    
    // Create soft masks
    for (int i = 0; i < 2; i++)
    {
        mSoftMaskingComp[i]->Process(fftData[i], thrsFftData[i], &softMasks[i]);
    }
}
