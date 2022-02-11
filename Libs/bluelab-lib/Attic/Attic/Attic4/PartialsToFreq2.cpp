//
//  PartialsToFreq2.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#include <algorithm>
using namespace std;

#include <PartialTWMEstimate2.h>

#include "PartialsToFreq2.h"


#define EPS 1e-15
#define INF 1e15

#define MIN_AMP_DB -120.0

// Select best
#define SELECT_BEST_PARTIALS   0

#define MAX_NUM_PARTIALS       3 //4

#define AGE_SCORE_COEFF        0.0
#define AMP_SCORE_COEFF        1.0 // 1.0
#define FREQ_SCORE_COEFF       0.0

// Threshold
#define THRESHOLD_PARTIALS     1
// -40 : good for "oohoo"
// -60 : good for "bell"
#define AMP_THRESHOLD           -60.0 //-80.0 //-40.0 //-60.0

// Threshold relative
#define THRESHOLD_PARTIALS_RELATIVE     0
// -40 : good for "oohoo"
// -60 : good for "bell"
#define AMP_THRESHOLD_RELATIVE          40.0


PartialsToFreq2::PartialsToFreq2(int bufferSize, BL_FLOAT sampleRate)
{
    mFreqEstimate = new PartialTWMEstimate2(bufferSize, sampleRate);
}

PartialsToFreq2::~PartialsToFreq2()
{
    delete mFreqEstimate;
}

void
PartialsToFreq2::Reset(BL_FLOAT sampleRate)
{
    mFreqEstimate->Reset(sampleRate);
}

BL_FLOAT
PartialsToFreq2::ComputeFrequency(const vector<PartialTracker3::Partial> &partials)
{
    vector<PartialTracker3::Partial> partials0 = partials;
    
    //PartialTracker3::DBG_DumpPartials2/*Mel*/("partials0.txt", partials0, 2048, 44100.0);
    
    // Useless ?
    //SelectAlivePartials(&partials0);
    
#if SELECT_BEST_PARTIALS
    GetBestPartials(&partials0);
#endif

#if THRESHOLD_PARTIALS
    ThresholdPartials(&partials0);
#endif

#if THRESHOLD_PARTIALS_RELATIVE
    ThresholdPartialsRelative(&partials0);
#endif
    
    //PartialTracker3::DBG_DumpPartials2/*Mel*/("partials1.txt", partials0, 2048, 44100.0);
    
    BL_FLOAT freq = mFreqEstimate->EstimateOptim(partials0);
    
    //BLDebug::AppendValue("freq.txt", freq);
    
    return freq;
}

void
PartialsToFreq2::GetBestPartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> partials0 = *partials;
    
    vector<BL_FLOAT> ageScores;
    ComputeAgeScores(partials0, &ageScores);
    
    vector<BL_FLOAT> ampScores;
    ComputeAmpScores(partials0, &ampScores);
    
    vector<BL_FLOAT> freqScores;
    ComputeFreqScores(partials0, &freqScores);
    
    for (int i = 0; i < partials0.size(); i++)
    {
        PartialTracker3::Partial &p = partials0[i];
        
        BL_FLOAT ageScore = ageScores[i];
        ageScore *= AGE_SCORE_COEFF;
        
        BL_FLOAT ampScore = ampScores[i];
        ampScore *= AMP_SCORE_COEFF;
        
        BL_FLOAT freqScore = freqScores[i];
        freqScore *= FREQ_SCORE_COEFF;
        
        BL_FLOAT score = ageScore*ageScore + ampScore*ampScore + freqScore*freqScore;
        //if (score > 0.0)
        //    score = std::sqrt(score);
        
        p.mCookie = score;
    }
    
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::CookieLess);
    //sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::AmpLess); // DEBUG => works
    reverse(partials0.begin(), partials0.end());
    
    // Add
    partials->clear();
    
    for (int i = 0; i < partials0.size(); i++)
    {
        if (i >= MAX_NUM_PARTIALS)
            break;
        
        const PartialTracker3::Partial &partial = partials0[i];
        partials->push_back(partial);
    }
    
    sort(partials->begin(), partials->end(), PartialTracker3::Partial::FreqLess);
}

void
PartialsToFreq2::ComputeAgeScores(const vector<PartialTracker3::Partial> &partials,
                                  vector<BL_FLOAT> *ageScores)
{
    int sumAges = 0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        sumAges += partial.mAge;
    }
    
    ageScores->resize(partials.size());
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        int age = partial.mAge;
        
        BL_FLOAT t = 0.0;
        if (sumAges > 0)
            t = ((BL_FLOAT)age)/sumAges;
        
        // High ages are better
        (*ageScores)[i] = t;
    }
}

void
PartialsToFreq2::ComputeAmpScores(const vector<PartialTracker3::Partial> &partials,
                                  vector<BL_FLOAT> *ampScores)
{
    BL_FLOAT sumAmps = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        BL_FLOAT amp = partial.mAmpDB - MIN_AMP_DB;
        //BL_FLOAT amp = DBToAmp(partial.mAmpDB);
        
        sumAmps += amp;
    }
    
    ampScores->resize(partials.size());
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        BL_FLOAT amp = partial.mAmpDB - MIN_AMP_DB;
        //BL_FLOAT amp = DBToAmp(partial.mAmpDB);
        
        BL_FLOAT t = 0.0;
        if (sumAmps > 0.0)
            t = ((BL_FLOAT)amp)/sumAmps;
        
        // Big amps are better
        (*ampScores)[i] = t;
    }
}

void
PartialsToFreq2::ComputeFreqScores(const vector<PartialTracker3::Partial> &partials,
                                   vector<BL_FLOAT> *freqScores)
{
    BL_FLOAT sumFreqs = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        sumFreqs += partial.mFreq;
    }
    
    freqScores->resize(partials.size());
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        BL_FLOAT freq = partial.mFreq;
        
        BL_FLOAT t = 0.0;
        if (sumFreqs > 0)
            t = ((BL_FLOAT)freq)/sumFreqs;
        
        // Low freqs are better
        t = 1.0 - t;
        
        (*freqScores)[i] = t;
    }
}

void
PartialsToFreq2::ThresholdPartials(vector<PartialTracker3::Partial> *partials)
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
PartialsToFreq2::ThresholdPartialsRelative(vector<PartialTracker3::Partial> *partials)
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
PartialsToFreq2::SelectAlivePartials(vector<PartialTracker3::Partial> *partials)
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
