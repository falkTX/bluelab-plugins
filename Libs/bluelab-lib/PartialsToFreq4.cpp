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
//  PartialsToFreq4.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#include <algorithm>
using namespace std;

#include <PartialTWMEstimate2.h>

#include <BLUtils.h>

#include "PartialsToFreq4.h"


#define EPS 1e-15
#define INF 1e15

#define MIN_AMP_DB -120.0


// Threshold
#define THRESHOLD_PARTIALS     0
// -40 : good for "oohoo"
// -60 : good for "bell"
#define AMP_THRESHOLD           -60.0 //-80.0 //-40.0 //-60.0

// Threshold relative
#define THRESHOLD_PARTIALS_RELATIVE     0
// -40 : good for "oohoo"
// -60 : good for "bell"
#define AMP_THRESHOLD_RELATIVE          40.0


//
#define MAX_NUM_INTERVALS  1

#define NUM_OCTAVES_ADJUST 2

#define NUM_HARMO_GEN      4

#define NUM_OCTAVE_GEN     2


PartialsToFreq4::PartialsToFreq4(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mEstimate = new PartialTWMEstimate2(bufferSize, sampleRate);
}

PartialsToFreq4::~PartialsToFreq4()
{
    delete mEstimate;
}

void
PartialsToFreq4::SetHarmonicSoundFlag(bool flag)
{
    mEstimate->SetHarmonicSoundFlag(flag);
}

BL_FLOAT
PartialsToFreq4::ComputeFrequency(const vector<PartialTracker3::Partial> &partials)
{
    vector<PartialTracker3::Partial> partials0 = partials;
    
#if THRESHOLD_PARTIALS
    ThresholdPartials(&partials0);
#endif

#if THRESHOLD_PARTIALS_RELATIVE
    ThresholdPartialsRelative(&partials0);
#endif
 
    //BL_FLOAT error = -1.0;
    if (partials0.empty())
    {
        BL_FLOAT freq = 0.0;
        
        return freq;
    }
    
    if (partials0.size() == 1)
    {
        BL_FLOAT freq = partials0[0].mFreq;
        
        return freq;
    }
    
    //BL_FLOAT freq = mEstimate->Estimate(partials0); // profile: 100/200ms
    
#if 1 // ORIGIN
    BL_FLOAT freq = mEstimate->EstimateMultiRes(partials0); // profile: ~40ms
#endif
#if 0 // TEST
    BL_FLOAT freq = mEstimate->EstimateOptim2(partials0);
#endif
    
    //BL_FLOAT freq = mEstimate->EstimateOptim(partials0); // 200ms (and false results)
    
    // Do we adjust the frequency found to the nearest partial ?
    //freq = AdjustFreqToPartial(freq, partials0);
    //freq = AdjustFreqToPartialOctave(freq, partials0);
    
    // Debug
    //PartialTracker3::DBG_DumpPartials2("partials.txt", partials0,
    //                                   mBufferSize, mSampleRate);
    
    //BLDebug::AppendValue("freq.txt", freq);
    
    return freq;
}

// Take the principle that at least 1 partial has the correct frequency
// Adjust the found freq to the nearest partial
BL_FLOAT
PartialsToFreq4::AdjustFreqToPartial(BL_FLOAT freq,
                                     const vector<PartialTracker3::Partial> &partials)
{
    BL_FLOAT bestFreq = 0.0;
    BL_FLOAT minDiff = INF;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        BL_FLOAT diff = std::fabs(partial.mFreq - freq);
        if (diff < minDiff)
        {
            minDiff = diff;
            bestFreq = partial.mFreq;
        }
    }
    
    return bestFreq;
}

BL_FLOAT
PartialsToFreq4::AdjustFreqToPartialOctave(BL_FLOAT freq,
                                           const vector<PartialTracker3::Partial> &partials)
{    
    BL_FLOAT octaveCoeff = 1.0;
    
    BL_FLOAT bestFreq = 0.0;
    BL_FLOAT minDiff = INF;
    for (int j = 0; j < NUM_OCTAVES_ADJUST; j++)
    {
        for (int i = 0; i < partials.size(); i++)
        {
            const PartialTracker3::Partial &partial = partials[i];
        
            BL_FLOAT pf = partial.mFreq*octaveCoeff;
            
            BL_FLOAT diff = std::fabs(pf - freq);
            if (diff < minDiff)
            {
                minDiff = diff;
                bestFreq = pf;
            }
        }
        
        octaveCoeff /= 2.0;
    }
    
    return bestFreq;
}

void
PartialsToFreq4::ThresholdPartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> result;
    
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*partials)[i];
        
        if (partial.mAmpDB > AMP_THRESHOLD)
            result.push_back(partial);
    }
    
    *partials = result;
}

void
PartialsToFreq4::ThresholdPartialsRelative(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> result;
    
    // Find the maximum amp
    BL_FLOAT maxAmp = MIN_AMP_DB;
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*partials)[i];
        
        if (partial.mAmpDB > maxAmp)
            maxAmp = partial.mAmpDB;
            
    }
    
    // Threshold compared to the maximum peak
    BL_FLOAT ampThrs = maxAmp - AMP_THRESHOLD_RELATIVE;
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*partials)[i];
        
        if (partial.mAmpDB > ampThrs)
            result.push_back(partial);
    }
    
    *partials = result;
}

