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
//  PitchShiftFftObj3.h
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_PitchShift__PitchShiftFftObj3__
#define __BL_PitchShift__PitchShiftFftObj3__

#include "FftProcessObj16.h"
#include "FreqAdjustObj3.h"

// NOTE: the energy loss (when looking at the waveform)
// could seem strnage, but this is normal !
//

// GOOD !
#define PARAM_SMOOTH_OVERLAP 1


class PitchShift;
class PitchShiftFftObj3 : public ProcessObj
{
public:
    PitchShiftFftObj3(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~PitchShiftFftObj3();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    void Reset();
    
    void SetFactor(BL_FLOAT factor);
    
protected:
    // Use Smb
    void Convert(WDL_TypedBuf<BL_FLOAT> *ioMagns,
                 WDL_TypedBuf<BL_FLOAT> *ioPhases,
                 BL_FLOAT factor);
    
#if 0 // ...
    void FillMissingFreqs(WDL_TypedBuf<BL_FLOAT> *ioFreqs);
#endif
    
#if !PARAM_SMOOTH_OVERLAP
    BL_FLOAT mFactor;
#else
    BL_FLOAT mFactors[2];
    
    bool mParamChanged;
    int mBufferNum;
#endif
    
    FreqAdjustObj3 mFreqObj;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf6;
};

#endif /* defined(__BL_PitchShift__PitchShiftFftObj3__) */
