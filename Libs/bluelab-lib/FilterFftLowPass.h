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
//  FilterFftLowPass.h
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#ifndef __UST__FilterFftLowPass__
#define __UST__FilterFftLowPass__

#define DEFAULT_FFT_SIZE 2048

class FftProcessObj16;
class FftLowPassProcess;

class FilterFftLowPass
{
public:
    FilterFftLowPass();
    
    virtual ~FilterFftLowPass();
    
    void Init(BL_FLOAT fc, BL_FLOAT sampleRate, int fftSize = DEFAULT_FFT_SIZE);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    BL_FLOAT mFC;
    int mFftSize;
    int mBlockSize;
    
    FftProcessObj16 *mFftObj;
    FftLowPassProcess *mProcessObj;
    
    BL_FLOAT mPrevSampleRate;
};

#endif /* defined(__UST__FilterFftLowPass__) */
