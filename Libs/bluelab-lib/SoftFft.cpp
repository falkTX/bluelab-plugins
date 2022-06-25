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
//  SoftFft.cpp
//  BL-PitchShift
//
//  Created by Pan on 03/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "SoftFft.h"

void
SoftFft::Ifft(const WDL_TypedBuf<BL_FLOAT> &magns,
              const WDL_TypedBuf<BL_FLOAT> &freqs,
              const WDL_TypedBuf<BL_FLOAT> &phases,
              BL_FLOAT sampleRate,
              WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    if (magns.GetSize() != freqs.GetSize())
        return;
    if (phases.GetSize() != magns.GetSize())
        return;
    
    //outSamples->Resize(magns.GetSize()*2);
    
    for (int i = 0; i < outSamples->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/sampleRate;
        
        BL_FLOAT samp = 0.0;
        
        for (int j = 0; j < magns.GetSize(); j++)
        {
            BL_FLOAT freq = freqs.Get()[j];
            
            // Nyquist condition
            if (freq >= sampleRate/2.0)
                continue;
            
            BL_FLOAT magn = magns.Get()[j];
            BL_FLOAT phase = phases.Get()[j];
            
            BL_FLOAT val = magn*sin(2.0*M_PI*freq*t + phase);
            
            samp += val;
        }
        
        outSamples->Get()[i] = samp;
    }
}
