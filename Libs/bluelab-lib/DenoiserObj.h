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
//  DenoiserObj.h
//  BL-Denoiser
//
//  Created by applematuer on 6/25/20.
//
//

#ifndef __BL_Denoiser__DenoiserObj__
#define __BL_Denoiser__DenoiserObj__

#include <deque>
using namespace std;

#include <bl_queue.h>

#include "IPlug_include_in_plug_hdr.h"

#include <FftProcessObj16.h>

#include <Denoiser_defs.h>

using namespace iplug;

// DenoiserObj
class Denoiser;
//class SoftMaskingComp3;
class SoftMaskingComp4;
class SmoothAvgHistogram;
class DenoiserObj : public ProcessObj
{
public:
    DenoiserObj(Plugin *iPlug,
                int bufferSize, int oversampling, int freqRes,
                BL_FLOAT threshold, bool processNoise);
    
    virtual ~DenoiserObj();
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    void SetThreshold(BL_FLOAT threshold);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void GetSignalBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void GetNoiseBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    // Noise capture
    void SetBuildingNoiseStatistics(bool flag);
    
    void AddNoiseStatistics(const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf);
    
    //
    void GetNoisePattern(WDL_TypedBuf<BL_FLOAT> *noisePattern);
    void SetNoisePattern(const WDL_TypedBuf<BL_FLOAT> &noisePattern);
    
#if USE_VARIABLE_BUFFER_SIZE
    // Used for serialization
    void GetNativeNoisePattern(WDL_TypedBuf<BL_FLOAT> *noisePattern);
    void SetNativeNoisePattern(const WDL_TypedBuf<BL_FLOAT> &noisePattern);
#endif
    
    void SetResNoiseThrs(BL_FLOAT threshold);
#if USE_AUTO_RES_NOISE
    void SetAutoResNoise(bool autoResNoiseFlag);
#endif
    
    void SetTwinNoiseObj(DenoiserObj *twinObj);
    
    void SetTwinNoiseBuf(const WDL_TypedBuf<BL_FLOAT> &noiseBuf,
                         const WDL_TypedBuf<BL_FLOAT> &noiseBufPhases);
    
    static void ApplyThresholdToNoiseCurve(WDL_TypedBuf<BL_FLOAT> *ioNoiseCurve, BL_FLOAT threshold);

    int GetLatency();
    
protected:
    //
    // Residual denoise
    //
    
    // NEW (Fft15)
    void ResetResNoiseHistory();
    
    // Must keep and manage the phases
    // (there is an history, and we must have synchronous phases)
    void ResidualDenoise(WDL_TypedBuf<BL_FLOAT> *signalBuffer,
                         WDL_TypedBuf<BL_FLOAT> *noiseBuffer,
                         WDL_TypedBuf<BL_FLOAT> *phases);
    
#if USE_AUTO_RES_NOISE
    void AutoResidualDenoise(WDL_TypedBuf<BL_FLOAT> *ioSignalMagns,
                             WDL_TypedBuf<BL_FLOAT> *ioNoiseMagns,
                             WDL_TypedBuf<BL_FLOAT> *ioSignalPhases,
                             WDL_TypedBuf<BL_FLOAT> *ioNoisePhases);
#endif
    
    // Kernel can be NULL
    void NoiseFilter(BL_FLOAT *input, BL_FLOAT *output, int width, int height,
                     int winSize, WDL_TypedBuf<BL_FLOAT> *kernel, int lineNum,
                     BL_FLOAT threshold);
    
    // Take an fft buffer history and transform it to an image
    void SamplesHistoryToImage(//const deque<WDL_TypedBuf<BL_FLOAT> > *hist,
                               const bl_queue<WDL_TypedBuf<BL_FLOAT> > *hist,
                               WDL_TypedBuf<BL_FLOAT> *imageChunk);
    
    // Take an image and extract one line
    // Fill an Fft buffer
    // Take the phases from the history
    void ImageLineToSamples(const WDL_TypedBuf<BL_FLOAT> *image,
                            int width, int height, int lineNum,
                            //const deque<WDL_TypedBuf<BL_FLOAT> > *hist,
                            //const deque<WDL_TypedBuf<BL_FLOAT> > *phaseHist,
                            const bl_queue<WDL_TypedBuf<BL_FLOAT> > *hist,
                            const bl_queue<WDL_TypedBuf<BL_FLOAT> > *phaseHist,
                            WDL_TypedBuf<BL_FLOAT> *resultBuf,
                            WDL_TypedBuf<BL_FLOAT> *resultPhases);
    
    void ExtractResidualNoise(const WDL_TypedBuf<BL_FLOAT> *prevSignal,
                              const WDL_TypedBuf<BL_FLOAT> *signal,
                              WDL_TypedBuf<BL_FLOAT> *ioNoise);
    
    //
    // Soft or hard elbow
    //
    void Threshold(WDL_TypedBuf<BL_FLOAT> *ioSigMagns, WDL_TypedBuf<BL_FLOAT> *ioNoiseMagns);
    
#if USE_VARIABLE_BUFFER_SIZE
    void ResampleNoisePattern();
#endif

    //int mBufferSize;
    //int mOverlapping;
    
    WDL_TypedBuf<BL_FLOAT> mSignalBuf;
    WDL_TypedBuf<BL_FLOAT> mNoiseBuf;
    
    BL_FLOAT mThreshold;
    bool mProcessNoise;
    
    // Residual noise
    BL_FLOAT mResNoiseThrs;
    
#if USE_AUTO_RES_NOISE
    bool mAutoResNoise;
    // The first for the signal, the second for the noise
    //SoftMaskingComp3 *mSoftMaskingComps[2];
    SoftMaskingComp4 *mSoftMaskingComps[2];
#endif
    
    // Noise capture
    bool mIsBuildingNoiseStatistics;
    int mNumNoiseStatistics;
    
    WDL_TypedBuf<BL_FLOAT> mNoisePattern;
    SmoothAvgHistogram *mAvgHistoNoisePattern;
    
#if USE_VARIABLE_BUFFER_SIZE
    // Used to keep original noise pattern if need to rescale
    WDL_TypedBuf<BL_FLOAT> mNativeNoisePattern;
#endif
    
    //
    // Residual denoise
    //
    //deque<WDL_TypedBuf<BL_FLOAT> > mHistoryFftBufs;
    //deque<WDL_TypedBuf<BL_FLOAT> > mHistoryFftNoiseBufs;
    //deque<WDL_TypedBuf<BL_FLOAT> > mHistoryPhases;
    
    // NOTE: since using bl_queue, we push_back instead() of push_front()
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mHistoryFftBufs;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mHistoryFftNoiseBufs;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mHistoryPhases;
    
    WDL_TypedBuf<BL_FLOAT> mInputImageFilterChunk;
    WDL_TypedBuf<BL_FLOAT> mOutputImageFilterChunk;
    
    WDL_TypedBuf<BL_FLOAT> mHanningKernel;
    
    //
    Plugin *mPlug;
    
    // A twin denoiser obj.
    // It will be filled with noise after a signal denoiser obj has computed ProcessFftSamples
    // And will re-re-synthetize the noise fft to noise samples
    //
    // With this we can void computing fft and fft to magn phases for the twi objs
    // (and all the denoising processing)
    // Without this, we would compute fft and fft to magns (and all denoising processing) two times:
    // one for signal, one for noise.
    //
    DenoiserObj *mTwinNoiseObj;
    
    WDL_TypedBuf<BL_FLOAT> mNoiseBufPhases;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf7;
    //WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf11;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf12;
    //WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf13;
    //WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf14;
    //WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf15;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf16;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf17;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf18;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf19;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf20;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf21;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf22;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf23;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf24;
};

#endif /* defined(__BL_Denoiser__DenoiserObj__) */
