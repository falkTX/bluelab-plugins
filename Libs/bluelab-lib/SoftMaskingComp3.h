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
//  SoftMaskingComp3.h
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#ifndef __BL_DUET__SoftMaskingComp3__
#define __BL_DUET__SoftMaskingComp3__

#include <deque>
using namespace std;

#include <bl_queue.h>

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// Wiener soft masking
//
// See: https://github.com/TUIlmenauAMS/ASP/blob/master/MaskingMethods.py
// and: http://www.jonathanleroux.org/pdf/Erdogan2015ICASSP04.pdf
// and: https://www.researchgate.net/publication/220736985_Degenerate_Unmixing_Estimation_Technique_using_the_Constant_Q_Transform
// and: https://hal.inria.fr/inria-00544949/document
//

// From SoftMasking, but with complex numbers. Returns a complex mask.
//

// SoftMaskingComp2: from SoftMaskingComp
// - code clean
//
// SoftMaskingComp3: from SoftMaskingComp2
// - try to remove the gating effect
//
class SoftMaskingComp3
{
public:
    SoftMaskingComp3(int historySize);
    
    virtual ~SoftMaskingComp3();
  
    void Reset();
    
    void SetHistorySize(int size);
    
    int GetHistorySize();
    
    void SetProcessingEnabled(bool flag);
    bool IsProcessingEnabled();
    
    // Mixture is the full sound
    // values is the estimated sound for a given mask
    void Process(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                 WDL_TypedBuf<WDL_FFT_COMPLEX> *softMask);
    
    // Return the centered data values in ioMixtureValues and ioValues
    //void ProcessCentered(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioMixtureValues,
    //                     WDL_TypedBuf<WDL_FFT_COMPLEX> *ioValues,
    //                     WDL_TypedBuf<WDL_FFT_COMPLEX> *softMask);
    void ProcessCentered(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioMixture,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *ioMaskedMixture,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *softMask);
    
protected:
    //void ComputeSigma2(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
    //                   WDL_TypedBuf<WDL_FFT_COMPLEX> *outVariance);
    void ComputeSigma2(bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                       WDL_TypedBuf<WDL_FFT_COMPLEX> *outVariance);
                       
    //    
    int mHistorySize;
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    //deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixtureHistory;
    bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixtureHistory;
    
    // The sound corresponding to the mask
    //deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mHistory;
    bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > mHistory;
    
    //
    WDL_TypedBuf<BL_FLOAT> mWindow;
    
    bool mProcessingEnabled;

private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf2;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf3;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf4;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf6;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf7;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf8;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf9;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf10;
};

#endif /* defined(__BL_DUET__SoftMaskingComp3__) */
