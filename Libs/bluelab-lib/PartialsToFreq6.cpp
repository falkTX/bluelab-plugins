//
//  PartialsToFreq6.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <PartialTWMEstimate3.h>

#include <ChromagramObj.h>

#include "PartialsToFreq6.h"


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


PartialsToFreq6::PartialsToFreq6(int bufferSize, int oversampling,
                                 int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mEstimate = new PartialTWMEstimate3(bufferSize, sampleRate);

    mChromaObj = new ChromagramObj(bufferSize, oversampling,
                                   freqRes, sampleRate);
}

PartialsToFreq6::~PartialsToFreq6()
{
    delete mEstimate;

    delete mChromaObj;
}

void
PartialsToFreq6::Reset(int bufferSize, int oversampling,
                       int freqRes, BL_FLOAT sampleRate)
{
    mChromaObj->Reset(bufferSize, oversampling, freqRes, sampleRate);
}

void
PartialsToFreq6::SetHarmonicSoundFlag(bool flag)
{
    mEstimate->SetHarmonicSoundFlag(flag);
}

#if 0
BL_FLOAT
PartialsToFreq6::ComputeFrequency(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const WDL_TypedBuf<BL_FLOAT> &phases,
                                  const vector<PartialTracker5::Partial> &partials)
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
#endif

BL_FLOAT
PartialsToFreq6::ComputeFrequency(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const WDL_TypedBuf<BL_FLOAT> &phases,
                                  const vector<PartialTracker5::Partial> &partials)
{
#define MIN_FREQ 20.0
    
    vector<PartialTracker5::Partial> partials0 = partials;

    WDL_TypedBuf<BL_FLOAT> &chromaLine = mTmpBuf0;
    mChromaObj->MagnsToChromaLine(magns, phases, &chromaLine);

    BL_FLOAT maxChroma = BLUtils::FindMaxValue(chromaLine);
    
    BL_FLOAT freq0 = mChromaObj->ChromaToFreq(maxChroma, MIN_FREQ);

    // Find closest partial
    BL_FLOAT freq = FindClosestPartialFreq(freq0, partials);
    
    return freq;
}

// Take the principle that at least 1 partial has the correct frequency
// Adjust the found freq to the nearest partial
BL_FLOAT
PartialsToFreq6::AdjustFreqToPartial(BL_FLOAT freq,
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
PartialsToFreq6::
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
PartialsToFreq6::ThresholdPartials(vector<PartialTracker5::Partial> *partials)
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
PartialsToFreq6::ThresholdPartialsRelative(vector<PartialTracker5::Partial> *partials)
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

BL_FLOAT
PartialsToFreq6::
FindClosestPartialFreq(BL_FLOAT refFreq0,
                       const vector<PartialTracker5::Partial> &partials)
{
    // Get max partial freq
    BL_FLOAT maxFreq = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        if (partials[i].mFreq > maxFreq)
            maxFreq = partials[i].mFreq;
    }

    // Go one octave uppper, just to be sure to not miss value
    maxFreq *= 2.0;
    
    // Find closest partial freq
    int bestPartialIdx = 0;
    BL_FLOAT bestDiff = BL_INF;
    for (int i = 0; i < partials.size(); i++)
    {
        BL_FLOAT pf = partials[i].mFreq;
       
        BL_FLOAT f = refFreq0;
        int octave = 1;
        while(f < maxFreq)
        {
            BL_FLOAT diff = std::fabs(f - pf)/octave;

            if (diff < bestDiff)
            {
                bestDiff = diff;
                bestPartialIdx = i;
            }
            
            f *= 2.0;
            octave++;
        }
    }

    BL_FLOAT result = partials[bestPartialIdx].mFreq;

    return result;
}
