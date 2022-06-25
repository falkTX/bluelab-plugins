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
//  SpectrumViewFftObj.h
//  BL-SpectrumView
//
//  Created by applematuer on 7/8/19.
//
//

#ifndef __BL_SpectrumView__SpectrumViewFftObj__
#define __BL_SpectrumView__SpectrumViewFftObj__

#include <FftProcessObj16.h>

// SpectrumViewFftObj
class SpectrumViewFftObj : public ProcessObj
{
public:
    SpectrumViewFftObj(int bufferSize, int oversampling, int freqRes);
    
    virtual ~SpectrumViewFftObj();
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) override;
    
    void GetSignalBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mSignalBuf;

private:
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
};

#endif /* defined(__BL_SpectrumView__SpectrumViewFftObj__) */
