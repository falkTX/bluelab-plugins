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
//  SineSynthSimple.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SineSynthSimple__
#define __BL_SASViewer__SineSynthSimple__

#include <vector>
using namespace std;

#include <PartialTracker4.h>

// SineSynth: from SASFrame
//
// SineSynth2: from SineSynth, for InfraSynthProcess
// - code clean
// - generates sample by sample

#define INFRA_SYNTH_OPTIM3 1

class SineSynthSimple
{
public:
    class Partial
    {
    public:
        Partial();
        
        Partial(const Partial &other);
        
        virtual ~Partial();

    public:
        long mId;
        
        BL_FLOAT mFreq;
        BL_FLOAT mPhase;
        
#if !INFRA_SYNTH_OPTIM3
        BL_FLOAT mAmpDB;
#else
        BL_FLOAT mAmp;
#endif
    };
    
    SineSynthSimple(BL_FLOAT sampleRate);
    
    virtual ~SineSynthSimple();
    
    void Reset(BL_FLOAT sampleRate);
    
    // Simple
    BL_FLOAT NextSample(vector<Partial> *partials);
    
    // For InfraSynth
    void TriggerSync();
    
    void SetDebug(bool flag);
    
protected:
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mSyncDirection;
    
    bool mDebug;
};

#endif /* defined(__BL_SASViewer__SineSynthSimple__) */
