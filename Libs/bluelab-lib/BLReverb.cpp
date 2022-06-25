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
 
#include <BLUtils.h>

#include <BLReverb.h>

#define DIRAC_VALUE 1.0


BLReverb::~BLReverb() {}

void
BLReverb::GetIRs(WDL_TypedBuf<BL_FLOAT> irs[2], int numSamples)
{
    if (numSamples == 0)
        return;
    
    // Generate impulse
    WDL_TypedBuf<BL_FLOAT> impulse;
    BLUtils::ResizeFillZeros(&impulse, numSamples);
    
    impulse.Get()[0] = DIRAC_VALUE;
    
    // NOTE: sometimes this can crash (we may break the stack)
    //
    // Clone the reverb (to keep the original untouched)
    //BLReverb revClone = *this;
    
    // Use dynamic alloc to not break the stack
    BLReverb *revClone = Clone();
    
    // Apply the reverb to the impulse
    revClone->Process(impulse, &irs[0], &irs[1]);
    
    delete revClone;
}
