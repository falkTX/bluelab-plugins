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
//  DiracGenerator.h
//  Impulse
//
//  Created by Pan on 04/12/17.
//
//

#ifndef __Impulse__DiracGenerator__
#define __Impulse__DiracGenerator__

#include <BLTypes.h>

class DiracGenerator
{
public:
    DiracGenerator(BL_FLOAT sampleRate, BL_FLOAT frequency, BL_FLOAT value,
                   long sampleLatency);
    
    virtual ~DiracGenerator();
    
    // Return the index where a dirac was generated -1 otherwise
    // Assuming frequency is not too high
    // (i.e 1 dirac generated at the maximum during numSamples).
    int Process(BL_FLOAT *outSamples, int numSamples);
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetFrequency(BL_FLOAT frequency);
    
    bool FirstDiracGenerated();
    
protected:
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mFrequency;
    
    BL_FLOAT mValue;
    
    long mSampleLatency;
    
    long mSampleNum;
    
    bool mFirstDiracGenerated;
};

#endif /* defined(__Impulse__DiracGenerator__) */
