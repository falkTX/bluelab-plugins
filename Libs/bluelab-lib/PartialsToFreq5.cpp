//
//  PartialsToFreq5.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#include <algorithm>
using namespace std;

#include <PartialTWMEstimate3.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "PartialsToFreq5.h"


#define MIN_AMP_DB -120.0


// Threshold
#define THRESHOLD_PARTIALS 0
// -40 : good for "oohoo"
// -60 : good for "bell"
#define AMP_THRESHOLD -60.0 //-80.0

// Threshold relative
#define THRESHOLD_PARTIALS_RELATIVE 0
// -40 : good for "oohoo"
// -60 : good for "bell"
#define AMP_THRESHOLD_RELATIVE 40.0


//
#define MAX_NUM_INTERVALS 1
#define NUM_OCTAVES_ADJUST 2
#define NUM_HARMO_GEN 4
#define NUM_OCTAVE_GEN 2


PartialsToFreq5::PartialsToFreq5(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mEstimate = new PartialTWMEstimate3(bufferSize, sampleRate);
}

PartialsToFreq5::~PartialsToFreq5()
{
    delete mEstimate;
}

void
PartialsToFreq5::SetHarmonicSoundFlag(bool flag)
{
    mEstimate->SetHarmonicSoundFlag(flag);
}

BL_FLOAT
PartialsToFreq5::ComputeFrequency(const vector<PartialTracker5::Partial> &partials)
{
    vector<PartialTracker5::Partial> partials0 = partials;
    
#if THRESHOLD_PARTIALS
    ThresholdPartials(&partials0);
#endif

#if THRESHOLD_PARTIALS_RELATIVE
    ThresholdPartialsRelative(&partials0);
#endif
 
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
    
    BL_FLOAT freq = mEstimate->Estimate(partials0);
    
    return freq;
}

// Take the principle that at least 1 partial has the correct frequency
// Adjust the found freq to the nearest partial
BL_FLOAT
PartialsToFreq5::AdjustFreqToPartial(BL_FLOAT freq,
                                     const vector<PartialTracker5::Partial> &partials)
{
    BL_FLOAT bestFreq = 0.0;
    BL_FLOAT minDiff = BL_INF;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials[i];
        
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
PartialsToFreq5::
AdjustFreqToPartialOctave(BL_FLOAT freq,
                          const vector<PartialTracker5::Partial> &partials)
{    
    BL_FLOAT octaveCoeff = 1.0;
    
    BL_FLOAT bestFreq = 0.0;
    BL_FLOAT minDiff = BL_INF;
    for (int j = 0; j < NUM_OCTAVES_ADJUST; j++)
    {
        for (int i = 0; i < partials.size(); i++)
        {
            const PartialTracker5::Partial &partial = partials[i];
        
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
PartialsToFreq5::ThresholdPartials(vector<PartialTracker5::Partial> *partials)
{
#if 0
    vector<PartialTracker5::Partial> result;
    
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker5::Partial &partial = (*partials)[i];
        
        if (partial.mAmpDB > AMP_THRESHOLD)
            result.push_back(partial);
    }
    
    *partials = result;
#endif
}

void
PartialsToFreq5::ThresholdPartialsRelative(vector<PartialTracker5::Partial> *partials)
{
#if 0
    vector<PartialTracker5::Partial> result;
    
    // Find the maximum amp
    BL_FLOAT maxAmp = MIN_AMP_DB;
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker5::Partial &partial = (*partials)[i];
        
        if (partial.mAmpDB > maxAmp)
            maxAmp = partial.mAmpDB;
            
    }
    
    // Threshold compared to the maximum peak
    BL_FLOAT ampThrs = maxAmp - AMP_THRESHOLD_RELATIVE;
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker5::Partial &partial = (*partials)[i];
        
        if (partial.mAmpDB > ampThrs)
            result.push_back(partial);
    }
    
    *partials = result;
#endif
}

