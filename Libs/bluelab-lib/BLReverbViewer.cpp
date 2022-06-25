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
//  BLReverbViewer.cpp
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#if IGRAPHICS_NANOVG

//#include <MultiViewer.h>
#include <MultiViewer2.h>
#include <BLReverb.h>

#include <BLUtils.h>

#include "BLReverbViewer.h"

#define DIRAC_VALUE 1.0

// 1ms
//#define OFFSET_TIME 0.001

// 10%
#define TIME_OFFSET_PERCENT 0.1


BLReverbViewer::BLReverbViewer(BLReverb *reverb, MultiViewer2 *viewer,
                               BL_FLOAT durationSeconds, BL_FLOAT sampleRate)
{
    mReverb = reverb;
    mViewer = viewer;
    
    mDurationSeconds = durationSeconds;
    mSampleRate = sampleRate;
    
    mViewer->SetTime(mDurationSeconds, mDurationSeconds*(1.0 - TIME_OFFSET_PERCENT));
    
    Update();
}
    
BLReverbViewer::~BLReverbViewer() {}

void
BLReverbViewer::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
BLReverbViewer::SetDuration(BL_FLOAT durationSeconds)
{
    mDurationSeconds = durationSeconds;
    
    mViewer->SetTime(mDurationSeconds, mDurationSeconds*(1.0 - TIME_OFFSET_PERCENT));
    
    // NOTE: originally commented...
    // We zoomed with the mouse (do not update the spectrogram each time)
    Update();
}

void
BLReverbViewer::Update()
{
    long numSamples = mSampleRate*mDurationSeconds;
    long offsetSamples = numSamples*TIME_OFFSET_PERCENT;
    
    // Generate impulse
    WDL_TypedBuf<BL_FLOAT> impulse;
    BLUtils::ResizeFillZeros(&impulse, numSamples);;
    if (numSamples > offsetSamples)
    {
        impulse.Get()[offsetSamples] = DIRAC_VALUE;
        
#if 0 // DEBUG
        int secondImpulsePos = mSampleRate*0.1; //1.0;
        if (offsetSamples + secondImpulsePos < impulse.GetSize())
        {
            impulse.Get()[offsetSamples + secondImpulsePos] = DIRAC_VALUE;
        }
#endif
        
    }
    else
    {
        impulse.Get()[0] = DIRAC_VALUE;
    }
    
    // Clone the reverb (to keep the original untouched)
    //BLReverb revClone = *mReverb;
    BLReverb *revClone = mReverb->Clone();
    
    // Apply the reverb to the impulse
    WDL_TypedBuf<BL_FLOAT> outL;
    WDL_TypedBuf<BL_FLOAT> outR;
    
    revClone->Process(impulse, &outL, &outR);
    
#if 0 // DEBUG
    outL = impulse;
#endif
    
    // Generate the spectrogram
    mViewer->SetSamples(outL);
    
    delete revClone;
}

#endif // IGRAPHICS_NANOVG
