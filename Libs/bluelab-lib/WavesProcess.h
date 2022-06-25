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
//  WavesProcess.h
//  BL-Waves
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Waves__WavesProcess__
#define __BL_Waves__WavesProcess__


#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include "FftProcessObj16.h"


class WavesRender;
class WavesProcess : public ProcessObj
{
public:
    WavesProcess(int bufferSize,
                 BL_FLOAT overlapping, BL_FLOAT oversampling,
                 BL_FLOAT sampleRate);
    
    virtual ~WavesProcess();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate) override;
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void SetWavesRender(WavesRender *wavesRender);
    
protected:
    int GetDisplayRefreshRate();
    
    //int mBufferSize;
    //BL_FLOAT mOverlapping;
    //BL_FLOAT mOversampling;
    //BL_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_FLOAT> mValues;
    
    // Vol Render
    WavesRender *mWavesRender;

private:
    // Tmp buffer
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
};

#endif /* defined(__BL_Waves__WavesProcess__) */
