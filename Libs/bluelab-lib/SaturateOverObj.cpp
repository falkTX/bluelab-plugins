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
 
#include <SaturateOverObj.h>

SaturateOverObj::SaturateOverObj(int oversampling, BL_FLOAT sampleRate)
: OversampProcessObj(oversampling, sampleRate)
{
    mRatio = 0.0;
}

SaturateOverObj::~SaturateOverObj() {}

void
SaturateOverObj::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        ComputeSaturation(input, &output, mRatio);
        
        ioBuffer->Get()[i] = output;
    }
}

void
SaturateOverObj::SetRatio(BL_FLOAT ratio)
{
    mRatio = ratio;
}

// From Saturate plugin
void
SaturateOverObj::ComputeSaturation(BL_FLOAT inSample,
                                   BL_FLOAT *outSample,
                                   BL_FLOAT ratio)
{
    BL_FLOAT cut = 1.0 - ratio;
    BL_FLOAT factor = 1.0/(1.0 - ratio);
    
    *outSample = inSample;
    
    if (inSample > cut)
        *outSample = cut;
    
    if (inSample < -cut)
        *outSample = -cut;
    
    *outSample *= factor;
}
