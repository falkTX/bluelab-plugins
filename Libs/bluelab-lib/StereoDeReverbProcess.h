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
class BLSpectrogram4;
class SpectrogramDisplay;
class ImageDisplay;
class DUETSeparator3;
class StereoDeReverbProcess : public MultichannelProcess
{
public:
    StereoDeReverbProcess(int bufferSize, int oversampling,
                          int freqRes, BL_FLOAT sampleRate);
    
    virtual ~StereoDeReverbProcess();
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    void
    ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                    const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) override;
    
#if 0
    // For Phase Aliasing Correction
    // See: file:///Users/applematuer/Downloads/1-s2.0-S1063520313000043-main.pdf
    void
    ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                           const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer) override;
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

private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0[2];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1[2];
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2[2];
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3[2];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf4[2];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5[2];
};

#endif /* defined(__BL_BL_DUET__BL_StereoDeReverbProcess__) */
