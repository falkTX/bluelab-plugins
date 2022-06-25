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
//  SamplesDelayer.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#include "SamplesDelayer.h"

SamplesDelayer::SamplesDelayer(int nFrames)
{
    mNFrames = nFrames;
}

SamplesDelayer::~SamplesDelayer() {}

bool
SamplesDelayer::Process(const BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    mSamples.Add(input, nFrames);
    
    if (mSamples.GetSize() >= mNFrames + nFrames)
    {
        // Return nFrames
        memcpy(output, mSamples.Get(), nFrames*sizeof(BL_FLOAT));
        
        WDL_TypedBuf<BL_FLOAT> newBuf;
        newBuf.Add(&mSamples.Get()[nFrames], mSamples.GetSize() - nFrames);
        
        mSamples = newBuf;
        
        return true;
    }
    
    return false;
}

void
SamplesDelayer::Reset()
{
    mSamples.Resize(0);
}
