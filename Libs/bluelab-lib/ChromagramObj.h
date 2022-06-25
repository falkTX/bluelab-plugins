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
 
#ifndef CHROMAGRAM_OBJ_H
#define CHROMAGRAM_OBJ_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#define USE_FREQ_OBJ 1

class HistoMaskLine2;

#if USE_FREQ_OBJ
class FreqAdjustObj3;
#endif

class ChromagramObj
{
 public:
    ChromagramObj(int bufferSize, int oversampling,
                  int freqRes, BL_FLOAT sampleRate);
    virtual ~ChromagramObj();

    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate);

    void SetATune(BL_FLOAT aTune);
    
    void SetSharpness(BL_FLOAT sharpness);

    void MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                           const WDL_TypedBuf<BL_FLOAT> &phases,
                           WDL_TypedBuf<BL_FLOAT> *chromaLine,
                           HistoMaskLine2 *maskLine = NULL);

    // NOTE: could be static, but needs mATune
    BL_FLOAT ChromaToFreq(BL_FLOAT chromaVal, BL_FLOAT minFreq) const;

    static BL_FLOAT FreqToChroma(BL_FLOAT freq, BL_FLOAT aTune);
    
 protected:
    void MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                           WDL_TypedBuf<BL_FLOAT> *chromaLine,
                           HistoMaskLine2 *maskLine = NULL);
    
#if USE_FREQ_OBJ
    void MagnsToChromaLineFreqs(const WDL_TypedBuf<BL_FLOAT> &magns,
                                const WDL_TypedBuf<BL_FLOAT> &realFreqs,
                                WDL_TypedBuf<BL_FLOAT> *chromaLine,
                                HistoMaskLine2 *maskLine = NULL);
#endif

    static BL_FLOAT ComputeC0Freq(BL_FLOAT aTune);

    //
    BL_FLOAT mSampleRate;
    int mBufferSize;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mATune;
    BL_FLOAT mSharpness;

#if USE_FREQ_OBJ
    FreqAdjustObj3 *mFreqObj;
#endif

 private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
};

#endif
