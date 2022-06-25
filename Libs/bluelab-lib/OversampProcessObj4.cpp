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
//  OversampProcessObj4.cpp
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#include <FilterRBJNX.h>

#include "OversampProcessObj4.h"

#define NYQUIST_COEFF 0.5

OversampProcessObj4::OversampProcessObj4(int oversampling, BL_FLOAT sampleRate,
                                         bool filterNyquist)
{
    mFilter = NULL;
    
    if (filterNyquist)
        mFilter = new FilterRBJNX(1/*2*//*4*/, FILTER_TYPE_LOWPASS,
                                 sampleRate*oversampling,
                                 sampleRate*NYQUIST_COEFF);
    
    mOversampling = oversampling;
    
    for (int i = 0; i < NUM_CASCADE_FILTERS; i++)
    {
        mAntiAlias[i].Calc(0.5 / (BL_FLOAT)mOversampling);
        mUpsample[i].Reset();
        mDownsample[i].Reset();
    }
}

OversampProcessObj4::~OversampProcessObj4()
{
    if (mFilter != NULL)
        delete mFilter;
}

void
OversampProcessObj4::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < NUM_CASCADE_FILTERS; i++)
    {
        mUpsample[i].Reset();
        mDownsample[i].Reset();
    }
    
    if (mFilter != NULL)
    {
        mFilter->SetSampleRate(sampleRate*mOversampling);
        mFilter->SetCutoffFreq(sampleRate*NYQUIST_COEFF);
    }
}

void
OversampProcessObj4::Process(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mOversampling == 1)
        // Nothing to do
    {
        
        ProcessSamplesBuffer(ioBuffer);
        
        return;
    }
    
    // Upsample
    WDL_TypedBuf<BL_FLOAT> upSampBuffer;
    upSampBuffer.Resize(ioBuffer->GetSize()*mOversampling);
    
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT sample = (*ioBuffer).Get()[i];
        
        for (int j = 0; j < mOversampling; j++)
        {
			if (j > 0)
                sample = 0.0;
            
            for (int k = 0; k < NUM_CASCADE_FILTERS; k++)
            {
                mUpsample[k].Process(sample, mAntiAlias[k].Coeffs());
            
                sample = (BL_FLOAT)mOversampling * mUpsample[k].Output();
            }
            
            upSampBuffer.Get()[i*mOversampling + j] = sample;
        }
    }
    
    ProcessSamplesBuffer(&upSampBuffer);
    
    // Filter Nyquist ?
    if (mFilter != NULL)
    {
        //WDL_TypedBuf<BL_FLOAT> filtBuffer;
        //mFilter->Process(&filtBuffer, upSampBuffer);
        ///upSampBuffer = filtBuffer;
        mFilter->Process(&upSampBuffer);
    }
    
    // Downsample
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT output = 0.0;
        for (int j = 0; j < mOversampling; j++)
        {
            BL_FLOAT sample = upSampBuffer.Get()[i*mOversampling + j];
            
            for (int k = 0; k < NUM_CASCADE_FILTERS; k++)
            {
                mDownsample[k].Process(sample, mAntiAlias[k].Coeffs());

                sample = mDownsample[k].Output();
                
                //if (j == 0)
                //    sample = mDownsample[k].Output();
                
                if (j == 0)
                    output = sample;
            }
        }
        
        (*ioBuffer).Get()[i] = output;
    }
}
