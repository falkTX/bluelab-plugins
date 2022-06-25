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
 
#include <stdio.h>
#include <math.h>

#include <BLUtilsPlug.h>

#include "PitchShifterPV.h"

using namespace stekyne;

//
PitchShifterPV::PitchShifterPV()
{
    for (int i = 0; i < 2; i++)
        mPitchObjs[i] = new PitchShifter<BL_FLOAT>();
        //mPitchObjs[i] = new PeakShifter<BL_FLOAT>();
}

PitchShifterPV::~PitchShifterPV()
{
    for (int i = 0; i < 2; i++)
    {
        if (mPitchObjs[i] != NULL)
            delete mPitchObjs[i];
    }
}

void
PitchShifterPV::Reset(BL_FLOAT sampleRate, int blockSize)
{
    for (int i = 0; i < 2; i++)
    {
        if (mPitchObjs[i] != NULL)
            mPitchObjs[i]->reset();
    }
}

void
PitchShifterPV::Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                        vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    if (in.empty())
        return;

#if 1 // DEBUG: process a single channel
    if (mPitchObjs[0] != NULL)
    {
        (*out)[0] = in[0];
        
        mPitchObjs[0]->process((*out)[0].Get(), (*out)[0].GetSize());

        if (out->size() == 2)
            (*out)[1] = (*out)[0];
    }

    return;
#endif
    
    for (int i = 0; i < in.size(); i++)
    {
        if (i >= out->size())
            break;

        if (mPitchObjs[i] == NULL)
            break;

        (*out)[i] = in[i];
        
        mPitchObjs[i]->process((*out)[i].Get(), (*out)[i].GetSize());
    }
}
    
void
PitchShifterPV::SetFactor(BL_FLOAT factor)
{
    for (int i = 0; i < 2; i++)
    {
        if (mPitchObjs[i] != NULL)
            mPitchObjs[i]->setPitchRatio(factor);
    }
}

int
PitchShifterPV::ComputeLatency(int blockSize)
{
    int latency = 0;

    if (mPitchObjs[0] != NULL)
        latency = mPitchObjs[0]->getLatencyInSamples();
    
    return latency;
}
