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
//  FilterSincConvoLPF.cpp
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#include <Window.h>
#include <FastRTConvolver3.h>

#include <BLUtils.h>

#include "FilterSincConvoLPF.h"

#if 0
TODO: when Reset(), must call Initi() to recompute the filter data
#endif

FilterSincConvoLPF::FilterSincConvoLPF()
{
    mConvolver = NULL;
}

FilterSincConvoLPF::~FilterSincConvoLPF()
{
    if (mConvolver != NULL)
        delete mConvolver;
}

void
FilterSincConvoLPF::Init(BL_FLOAT fc, BL_FLOAT sampleRate, int filterSize)
{
    mFilterData.Resize(filterSize);
    
    // TODO: adjust well the cut frequency, so that the totally attenuated signal matches exactly fc
    // NOTE: for the moment, this is very approximative, and depends on filterSize
    
    // Adjust cut frequency, so that the signal becomes totally attenuated exactly at fc
    //BL_FLOAT bandwidth = (4.0/filterSize)*sampleRate;
    BL_FLOAT bandwidth = (4.6/filterSize)*sampleRate; // 4.6 for Blackman
    
    // Niko
    bandwidth *= 0.5;
    
    fc -= bandwidth;
    if (fc < 0.0)
        fc = 0.0;
    
    BL_FLOAT fcSr = fc/sampleRate;
    Window::MakeNormSincFilter(filterSize, fcSr, &mFilterData);
    
    WDL_TypedBuf<BL_FLOAT> blackman;
    Window::MakeBlackman(filterSize, &blackman);
    
    BLUtils::MultValues(&mFilterData, blackman);
    
    BLUtils::NormalizeFilter(&mFilterData);
    
    //
    if (mConvolver != NULL)
        delete mConvolver;
    mConvolver = new FastRTConvolver3(filterSize, sampleRate, mFilterData);
}

void
FilterSincConvoLPF::Reset(BL_FLOAT sampleRate, int blockSize)
{
    if (mConvolver != NULL)
        mConvolver->Reset(sampleRate, blockSize);

    // TODO: must re-call Init() here, to re-compute filter data!
}

int
FilterSincConvoLPF::GetLatency()
{
    if (mConvolver == NULL)
        return 0;
        
    int latency = mConvolver->GetLatency();
    
    return latency;
}

void
FilterSincConvoLPF::Process(WDL_TypedBuf<BL_FLOAT> *result,
                            const WDL_TypedBuf<BL_FLOAT> &samples)
{
    if (mConvolver == NULL)
    {
        *result = samples;
        
        return;
    }
    
    mConvolver->Process(samples, result);
}
