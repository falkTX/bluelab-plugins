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
//  StereoPhasesProcess.h
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_PitchShift__StereoPhasesProcess__
#define __BL_PitchShift__StereoPhasesProcess__

#include "FftProcessObj16.h"

#include "PhasesDiff.h"

class StereoPhasesProcess : public MultichannelProcess
{
public:
    StereoPhasesProcess(int bufferSize);
    
    virtual ~StereoPhasesProcess();
    
    void Reset() override;
    
    // For PhasesDiff::USE_LINERP_PHASES
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate) override;
    
    void SetActive(bool flag);
    
    void
    ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                    const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) override;
    
    void
    ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                     const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) override;
    
protected:
    PhasesDiff mPhasesDiff;
    
    bool mIsActive;
};

#endif /* defined(__BL_PitchShift__StereoPhasesProcess__) */
