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
//  ChromaFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Chroma__ChromaFftObj__
#define __BL_Chroma__ChromaFftObj__

#include <bl_queue.h>

#include <FftProcessObj16.h>

// Without USE_FREQ_OBJ:
// - graphic is more clear
// - frequencies A Tune sometimes seems false
// - with pure low frequencies, there are several lines instead of a single one
//
// With USE_FREQ_OBJ: GOOD !
// - data is more fuzzy, but seems more accurate
// - A Tune seems correct
// - with pure low frequencies, there is only one line !
//

#define USE_FREQ_OBJ 1 //0

// From SpectrogramFftObj
//

class BLSpectrogram4;
class SpectrogramDisplayScroll4;
class HistoMaskLine2;

#if USE_FREQ_OBJ
class FreqAdjustObj3;
#endif

class ChromaFftObj2 : public ProcessObj
{
public:
    ChromaFftObj2(int bufferSize, int oversampling, int freqRes,
                  BL_FLOAT sampleRate);
    
    virtual ~ChromaFftObj2();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay);
    
    void SetATune(BL_FLOAT aTune);
    
    void SetSharpness(BL_FLOAT sharpness);

    void SetSpeedMod(int speedMod);
    
    // For external objects (such as SpectroExpe)
    void MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                           const WDL_TypedBuf<BL_FLOAT> &phases,
                           WDL_TypedBuf<BL_FLOAT> *chromaLine,
                           HistoMaskLine2 *maskLine = NULL);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                           WDL_TypedBuf<BL_FLOAT> *chromaLine,
                           HistoMaskLine2 *maskLine = NULL);
    
#if USE_FREQ_OBJ
    void MagnsToChromaLineFreqs(const WDL_TypedBuf<BL_FLOAT> &magns,
                                const WDL_TypedBuf<BL_FLOAT> &realFreqs,
                                WDL_TypedBuf<BL_FLOAT> *chromaLine,
                                HistoMaskLine2 *maskLine = NULL);
#endif
    
    BL_FLOAT ComputeC0Freq();

    void ResetQueue();

    //
    BLSpectrogram4 *mSpectrogram;
    SpectrogramDisplayScroll4 *mSpectroDisplay;
    
    long mLineCount;
    
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mATune;
    BL_FLOAT mSharpness;

    int mSpeedMod;
    
#if USE_FREQ_OBJ
    FreqAdjustObj3 *mFreqObj;
#endif
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
};

#endif /* defined(__BL_Chroma__ChromaFftObj__) */
