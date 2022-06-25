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
//  SoftMaskingComp2.h
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#ifndef __BL_DUET__SoftMaskingNComp__
#define __BL_DUET__SoftMaskingNComp__

#include <deque>
#include <vector>
using namespace std;

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
// SoftMaskingNComp: from SoftMaskingComp2
//
class SoftMaskingNComp
{
public:
    SoftMaskingNComp(int historySize);
    
    virtual ~SoftMaskingNComp();
  
    void Reset();
    
    void SetHistorySize(int size);
    
    // Mixture is the full sound
    // estimData is the estimated data for all the masks
    void Process(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureData,
                 const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &estimData,
                 vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *softMasks);
    
protected:
    void ComputeVariance(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *outVariance);
    
    //
    int mHistorySize;
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    vector<deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > > mMixtureHistory;
    
    // The sound corresponding to the mask
    vector<deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > > mHistory;
};

#endif /* defined(__BL_DUET__SoftMaskingNComp__) */
