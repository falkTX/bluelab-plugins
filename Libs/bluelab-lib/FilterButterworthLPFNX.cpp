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
//  FilterButterworthLPFNX.cpp
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#include <FilterButterworthLPF.h>

#include "FilterButterworthLPFNX.h"


FilterButterworthLPFNX::FilterButterworthLPFNX(int numFilters)
{
    mFilters.resize(numFilters);
    for (int i = 0; i < mFilters.size(); i++)
    {
        mFilters[i] = new FilterButterworthLPF();
    }
    
    Init(400.0, 44100.0);
}

FilterButterworthLPFNX::~FilterButterworthLPFNX()
{
    for (int i = 0; i < mFilters.size(); i++)
        delete mFilters[i];
}

void
FilterButterworthLPFNX::Init(BL_FLOAT resoFreq, BL_FLOAT sampleRate)
{
    for (int i = 0; i < mFilters.size(); i++)
    {
        FilterButterworthLPF *filter = mFilters[i];
        
        filter->Init(resoFreq, sampleRate);
    }
}

BL_FLOAT
FilterButterworthLPFNX::Process(BL_FLOAT sample)
{
    for (int i = 0; i < mFilters.size(); i++)
    {
        FilterButterworthLPF *filter = mFilters[i];
        
        sample = filter->Process(sample);
    }
    
    return sample;
}

void
FilterButterworthLPFNX::Process(WDL_TypedBuf<BL_FLOAT> *result,
                               const WDL_TypedBuf<BL_FLOAT> &samples)
{
    result->Resize(samples.GetSize());
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT sample = samples.Get()[i];
        
        for (int j = 0; j < mFilters.size(); j++)
        {
            FilterButterworthLPF *filter = mFilters[j];
            
            sample = filter->Process(sample);
        }
        
        result->Get()[i] = sample;
    }
}
