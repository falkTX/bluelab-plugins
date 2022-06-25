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
//  PartialsToFreq.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#include <algorithm>
using namespace std;

#include <PartialTWMEstimate2.h>

#include "PartialsToFreq.h"


#define EPS 1e-15
#define INF 1e15


#define PREV_FREQ_SMOOTH_COEFF 0.95

// Frequency computation
#define COMPUTE_FREQ_MAX_NUM_PARTIALS 4 //
#define COMPUTE_FREQ_MAX_FREQ         2000.0
// Avoid computing freq with very low amps
#define COMPUTE_FREQ_AMP_THRESHOLD    -60.0 //-40.0 // TODO: check and adjust this threshold

// Debug
#define DBG_FREQ     0 //1 // 0


PartialsToFreq::PartialsToFreq(int bufferSize, BL_FLOAT sampleRate)
{
    mFreqEstimate = new PartialTWMEstimate2(bufferSize, sampleRate);
    
    mPrevFrequency = 0.0;
    mPrevNumPartials = 0;
}

PartialsToFreq::~PartialsToFreq()
{
    delete mFreqEstimate;
}

void
PartialsToFreq::Reset(BL_FLOAT sampleRate)
{
    mFreqEstimate->Reset(sampleRate);
    
    mPrevFrequency = 0.0;
    mPrevNumPartials = 0;
}

BL_FLOAT
PartialsToFreq::ComputeFrequency(const vector<PartialTracker3::Partial> &partials)
{
    BL_FLOAT resultFreq = 0.0;
    
    if (partials.empty())
    {
        mPrevFrequency = 0.0;
        
        return 0.0;
    }
    
#if 0 // Naive method
    // Find the first alive partial
    int idx = -1;
    for (int i = 0; i < mPartials.size(); i++)
    {
        const PartialTracker3::Partial &p = mPartials[i];
        
        if (p.mState == PartialTracker3::Partial::ALIVE)
        {
            idx = i;
            break;
        }
    }
    
    if (idx != -1)
    {
        const PartialTracker3::Partial &p = mPartials[idx];
        frequency = p.mFreq;
    }
    else
    {
        frequency = mPrevFrequency;
    }
    
    mPrevFrequency = mFrequency;
#endif
    
#if 1 // Estimate frequency from the set of partials
    
    // OPTIM: Keep only the first partials
    // (works well in case of more than 4 partials in input)
    vector<PartialTracker3::Partial> partials0 = partials;
    
#if 0 // TEST MONDAY
    SelectLowFreqPartials(&partials0);
#endif
    
    // Works well, avoid freq jumps
#if 1
    // If amps are under a threshold, do not return partials
    // This will avoid computing bad freq later,
    // and put it in prevFreq
    SelectHighAmpPartials(&partials0);
#endif
    
#if DBG_FREQ
    PartialTracker3::DBG_DumpPartials(partials0, 10);
#endif
    
    // NOTE: Estimate and EstimateMultiRes worked well but now
    // only EstimateOptim works
    
    //BL_FLOAT freq = mFreqEstimate->Estimate(mPartials);
    //BL_FLOAT freq = mFreqEstimate->EstimateMultiRes(mPartials);
    
    // Works well
    BL_FLOAT freq = mFreqEstimate->EstimateOptim(partials0);
    
#if DBG_FREQ
    BLDebug::AppendValue("freq0.txt", freq);
#endif
    
    BL_FLOAT minFreq = 0.0;
    
    // TEST MONDAY
#if 0 // GOOD !
    // Avoid octave jumps
    if (!partials0.empty())
    {
        /*BL_FLOAT*/ minFreq = INF;
        for (int i = 0; i < partials0.size(); i++)
        {
            const PartialTracker3::Partial &p = partials0[i];
            if (p.mFreq < minFreq)
                minFreq = p.mFreq;
        }
        
        //freq = mFreqEstimate->GetNearestOctave(freq, mPartials[0].mFreq);
        
        freq = mFreqEstimate->GetNearestHarmonic(freq, minFreq); //mPartials[0].mFreq); /// ????
    }
#endif
    
#if DBG_FREQ
    BLDebug::AppendValue("prev-freq.txt", mPrevFrequency);
#endif
    
    // TEST MONDAY
    // Works better than min freq !
#if 1
    // Try to avoid octave jumps
    if (!partials0.empty())
    {
        BL_FLOAT minFreqDiff = INF;
        for (int i = 0; i < partials0.size(); i++)
        {
            const PartialTracker3::Partial &p = partials0[i];
            BL_FLOAT diff = std::fabs(p.mFreq - mPrevFrequency);
            if (diff < minFreqDiff)
            {
                minFreqDiff = diff;
                minFreq = p.mFreq;
            }
        }
        
        //freq = mFreqEstimate->GetNearestOctave(freq, mPartials[0].mFreq);
        
        freq = mFreqEstimate->GetNearestHarmonic(freq, minFreq); //mPartials[0].mFreq); /// ????
    }
#endif
    
#if DBG_FREQ
    BLDebug::AppendValue("min-freq.txt", minFreq);
#endif
    
#if DBG_FREQ
    BLDebug::AppendValue("freq1.txt", freq);
#endif
    
    // Use this to avoid harmonic jumps of the fundamental frequency
    
#if 0 // Fix freq jumps
    
    // Allow freq jumps if we had only one partial before
    // (because we don't really now the real frequency if we have only
    // have one partial
    if (mPrevNumPartials > 1)
    {
        if (mPartials.size() > 1)
        {
            if (mPrevFrequency > EPS)
                freq = mFreqEstimate->FixFreqJumps(freq, mPrevFrequency);
        }
    }
#endif
    
#if 0 // Smooth frequency
    if (mPrevFrequency > EPS)
    {
        mFrequency = FREQ_SMOOTH_COEFF*mPrevFrequency + (1.0 - FREQ_SMOOTH_COEFF)*freq/*2*/;
    }
    else
    {
        mFrequency = freq;
    }
    
    mFrequency = mFreqEstimate->GetNearestHarmonic(mFrequency, mPrevFrequency);
    
#else
    resultFreq = freq;
#endif
    
    // If we have only 1 partial, the computed freq may not be the correct one
    // but the frequency of the harmonic instead.
    //
    // Wait for frequency estimation from several partials before considering the
    // computed frequency as correct.
    if ((partials0.size() > 1) && (mPrevNumPartials > 1))
    {
#if 0 // No smooth
        mPrevFrequency = mFrequency;
#else // Smooth Prev frequency
        if (mPrevFrequency < EPS)
            mPrevFrequency = resultFreq;
        else
        {
            mPrevFrequency =
            PREV_FREQ_SMOOTH_COEFF*mPrevFrequency +
            (1.0 - PREV_FREQ_SMOOTH_COEFF)*resultFreq;
        }
#endif
    }
#endif
    
    mPrevNumPartials = partials.size();
    
    return resultFreq;
}

void
PartialsToFreq::SelectLowFreqPartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> partials0 = *partials;
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
    partials->clear();
    for (int i = 0; i < partials0.size(); i++)
    {
        // TEST SUNDAY
        //if (i >= COMPUTE_FREQ_MAX_NUM_PARTIALS)
        //    break;
        
        const PartialTracker3::Partial &p = partials0[i];
        partials->push_back(p);
        
        // TEST
        if (partials->size() >= COMPUTE_FREQ_MAX_NUM_PARTIALS)
            break;
    }
}

// TEST MONDAY
void
PartialsToFreq::SelectHighAmpPartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> partials0 = *partials;
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::AmpLess);
    reverse(partials0.begin(), partials0.end());
    
    partials->clear();
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &p = partials0[i];
        if (p.mAmpDB > COMPUTE_FREQ_AMP_THRESHOLD)
            partials->push_back(p);
        
        if (partials->size() >= COMPUTE_FREQ_MAX_NUM_PARTIALS)
            break;
    }
    
    sort(partials->begin(), partials->end(), PartialTracker3::Partial::FreqLess);
}
