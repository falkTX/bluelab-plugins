//
//  PartialsToFreq3.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#include <algorithm>
using namespace std;

#include <PartialTWMEstimate2.h>

#include <BLUtils.h>

#include "PartialsToFreq3.h"


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


PartialsToFreq3::PartialsToFreq3(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mEstimate = new PartialTWMEstimate2(bufferSize, sampleRate);
}

PartialsToFreq3::~PartialsToFreq3()
{
    delete mEstimate;
}

BL_FLOAT
PartialsToFreq3::ComputeFrequency(const vector<PartialTracker3::Partial> &partials)
{
    vector<PartialTracker3::Partial> partials0 = partials;
    
    // Useless ?
    //SelectAlivePartials(&partials0);
    
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
    
    // Take the existing partials, and generate several harmonic partials for
    // each existing partial
    //GenerateHarmonics(&partials0);
    
    // Same, with octaves
    //GenerateOctaves(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
    // Orig method
    //BL_FLOAT freq = ComputeFreqDiff(partials0);
    
    // Doesn't work
    //BL_FLOAT freq = ComputeMinFreqDiff(partials0);
    
    // TEST TUESDAY
    BL_FLOAT freq = mEstimate->Estimate(partials);
    //BL_FLOAT freq = mEstimate->EstimateMultiRes(partials);
    //BL_FLOAT freq = mEstimate->EstimateOptim(partials);
    
    // Do we adjust the frequency found to the nearest partial ?
    
    //freq = AdjustFreqToPartial(freq, partials);
    //freq = AdjustFreqToPartialOctave(freq, partials);
    
    //PartialTracker3::DBG_DumpPartials2("partials.txt", partials0,
    //                                   mBufferSize, mSampleRate);
    
    //BLDebug::AppendValue("freq.txt", freq);
    
    return freq;
}

BL_FLOAT
PartialsToFreq3::ComputeFreqDiff(const vector<PartialTracker3::Partial> &partials)
{
    // Compute frequency differences
    vector<BL_FLOAT> freqDiffs;
    for (int i = 0; i < partials.size() - 1; i++)
    {
        BL_FLOAT diff = 0.0;
        if (i == 0)
            diff = partials[i].mFreq /* - 0.0 */;
        else
        {
            BL_FLOAT current = partials[i].mFreq;
            BL_FLOAT next = partials[i + 1].mFreq;
            
            diff = next - current;
        }
        
        // TEST
        //diff = diff / 2.0;
        
        // Do not put the first difference ?
        //if (i > 0)
        freqDiffs.push_back(diff);
    }
    
    //BLDebug::DumpData("diffs.txt", freqDiffs);
    
    // Eliminate progressively the freqs differences
    // by giving up the farthest from the mean
    // until keeping only the best interval
    BL_FLOAT prevMean = -1.0;
    while(freqDiffs.size() > MAX_NUM_INTERVALS)
    {
        BL_FLOAT mean = BLUtils::ComputeMean(freqDiffs);
        BL_FLOAT mean0 = mean;
        
        if (prevMean < 0.0)
            prevMean = mean;
        
        if (freqDiffs.size() <= 2)
            // Avoid computing mean of two values
            // (no sense)
        {
            mean0 = prevMean;
        }
        
        int maxIdx = -1;
        BL_FLOAT maxDiff = 0.0;
        for (int i = 0; i < freqDiffs.size(); i++)
        {
            BL_FLOAT fd = freqDiffs[i];
            
            BL_FLOAT diff = std::fabs(fd - mean0);
            
            if (diff > maxDiff)
            {
                maxDiff = diff;
                maxIdx = i;
            }
        }
        
        if (maxIdx == -1)
            break;
        
        // Remove the max diff interval
        freqDiffs.erase(freqDiffs.begin() + maxIdx);
        
        prevMean = mean;
    }
    
    BL_FLOAT freq = BLUtils::ComputeMean(freqDiffs);
    
    return freq;
}

// Doesn't work
BL_FLOAT
PartialsToFreq3::ComputeMinFreqDiff(const vector<PartialTracker3::Partial> &partials)
{
    // Compute frequency differences
    vector<BL_FLOAT> freqDiffs;
    for (int i = 0; i < partials.size() - 1; i++)
    {
        BL_FLOAT diff = 0.0;
        if (i == 0)
            diff = partials[i].mFreq /* - 0.0 */;
        else
        {
            BL_FLOAT current = partials[i].mFreq;
            BL_FLOAT next = partials[i + 1].mFreq;
            
            diff = next - current;
        }
        
        freqDiffs.push_back(diff);
    }
    
    sort(freqDiffs.begin(), freqDiffs.end());
    
    if (!freqDiffs.empty())
    {
        BL_FLOAT res = freqDiffs[0];
        
        return res;
    }
    
    return 0.0;
}

void
PartialsToFreq3::GenerateHarmonics(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> result;
    
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*partials)[i];
        
        result.push_back(partial);
        
        for (int j = 2; j < NUM_HARMO_GEN + 2; j++)
        {
            PartialTracker3::Partial ph = partial;
            ph.mFreq = partial.mFreq*j;
            
            result.push_back(ph);
        }
    }
    
    *partials = result;
}

void
PartialsToFreq3::GenerateOctaves(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> result;
    
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*partials)[i];
        
        result.push_back(partial);
        
        BL_FLOAT factor = 2.0;
        for (int j = 0; j < NUM_OCTAVE_GEN - 1; j++)
        {
            PartialTracker3::Partial po = partial;
            po.mFreq = partial.mFreq*factor;
            
            result.push_back(po);
            
            factor *= 2.0;
        }
    }
    
    *partials = result;
}

// Take the principle that at least 1 partial has the correct frequency
// Adjust the found freq to the nearest partial
BL_FLOAT
PartialsToFreq3::AdjustFreqToPartial(BL_FLOAT freq,
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
PartialsToFreq3::AdjustFreqToPartialOctave(BL_FLOAT freq,
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
PartialsToFreq3::ThresholdPartials(vector<PartialTracker3::Partial> *partials)
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
PartialsToFreq3::ThresholdPartialsRelative(vector<PartialTracker3::Partial> *partials)
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

void
PartialsToFreq3::SelectAlivePartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> result;
    
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*partials)[i];
        
        if (!partial.mWasAlive)
            continue;
        
        // TEST
        if (partial.mAge < 2)
            continue;
        
        result.push_back(partial);
    }
    
    *partials = result;
}
