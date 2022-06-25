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
//  SoftMasking.h
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#ifndef __BL_DUET__SoftMasking__
#define __BL_DUET__SoftMasking__

#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// Wiener soft masking
//
// See: https://github.com/TUIlmenauAMS/ASP/blob/master/MaskingMethods.py
// and: http://www.jonathanleroux.org/pdf/Erdogan2015ICASSP04.pdf
// and: https://www.researchgate.net/publication/220736985_Degenerate_Unmixing_Estimation_Technique_using_the_Constant_Q_Transform
// and: https://hal.inria.fr/inria-00544949/document
//
class SoftMasking
{
public:
    SoftMasking(int historySize);
    
    virtual ~SoftMasking();
  
    void Reset();
    
    void SetHistorySize(int size);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                 const WDL_TypedBuf<BL_FLOAT> &magns,
                 WDL_TypedBuf<BL_FLOAT> *mask);
    
protected:
    void ProcessTime(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                     const WDL_TypedBuf<BL_FLOAT> &magns,
                     WDL_TypedBuf<BL_FLOAT> *mask);
    
    void ProcessFreq(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                     const WDL_TypedBuf<BL_FLOAT> &magns,
                     WDL_TypedBuf<BL_FLOAT> *mask);
    
    void ProcessTimeFreq(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                         const WDL_TypedBuf<BL_FLOAT> &magns,
                         WDL_TypedBuf<BL_FLOAT> *mask);
    
    void ComputeVariance(deque<WDL_TypedBuf<BL_FLOAT> > &history,
                         WDL_TypedBuf<BL_FLOAT> *variance);
    
    void ComputeVariance(const WDL_TypedBuf<BL_FLOAT> &data,
                         WDL_TypedBuf<BL_FLOAT> *variance);
    
    void ComputeVarianceWin(deque<WDL_TypedBuf<BL_FLOAT> > &history,
                            WDL_TypedBuf<BL_FLOAT> *variance);
    
    //
    int mHistorySize;
    int mFreqWinSize;
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    deque<WDL_TypedBuf<BL_FLOAT> > mMixtureHistory;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mHistory;
};

#endif /* defined(__BL_DUET__SoftMasking__) */
