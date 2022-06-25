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
//  FilterButterworthLPF.cpp
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#include <so_butterworth_lpf.h>

#include "FilterButterworthLPF.h"

FilterButterworthLPF::FilterButterworthLPF()
{
    mFilter = new SO_BUTTERWORTH_LPF();
}

FilterButterworthLPF::~FilterButterworthLPF()
{
    delete mFilter;
}

void
FilterButterworthLPF::Init(BL_FLOAT cutFreq, BL_FLOAT sampleRate)
{
    mFilter->calculate_coeffs(cutFreq, sampleRate);
}

BL_FLOAT
FilterButterworthLPF::Process(BL_FLOAT sample)
{
    sample = mFilter->filter(sample);
    
    return sample;
}
