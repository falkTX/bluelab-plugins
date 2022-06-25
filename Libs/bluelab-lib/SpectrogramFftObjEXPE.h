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
//  SpectrogramFftObjEXPE.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectrogramFftObjEXPE__
#define __BL_Ghost__SpectrogramFftObjEXPE__

#ifdef IGRAPHICS_NANOVG

#include "FftProcessObj16.h"

class BLSpectrogram4;
class TransientLib5;
class SpectrogramFftObjEXPE : public ProcessObj
{
public:
    SpectrogramFftObjEXPE(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~SpectrogramFftObjEXPE();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    BLSpectrogram4 *GetSpectrogram();
    
    // After editing, set again the full data
    //
    // NOTE: not optimized, set the full data after each editing
    //
    void SetFullData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                     const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
    
    enum Mode
    {
        EDIT = 0,
        VIEW,
        ACQUIRE,
        RENDER
    };
    
    void SetMode(enum Mode mode);
    
    // EXPE
    void SetSmoothFactor(BL_FLOAT factor);
    void SetFreqAmpRatio(BL_FLOAT ratio);
    void SetTransThreshold(BL_FLOAT thrs);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    BLSpectrogram4 *mSpectrogram;
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    // EXPE
    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
    
    BL_FLOAT mSmoothFactor;
    BL_FLOAT mFreqAmpRatio;
    BL_FLOAT mTransThreshold;

    TransientLib5 *mTransLib;
};

#endif

#endif /* defined(__BL_Ghost__SpectrogramFftObjEXPE__) */
