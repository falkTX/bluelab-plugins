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
//  PartialTWMEstimate3.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/8/19.
//
//

#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "PartialTWMEstimate3.h"

#define MIN_FREQ_FIND_F0 50.0
#define MAX_FREQ_FIND_F0 10000.0

// Estimate and return all the errors and frequencies ?
#define ESTIMATE_NAIVE  1 // 0 // TEST
#define ESTIMATE_SIMPLE 0
#define ESTIMATE_MULTI  0 //1 // ORIGIN

// Limit the number of partials
//
// Partials can be around 20/25, or until 200 if threshold is -120 !
#define MAX_NUM_PARTIALS   20

#define OPTIM_FIND_NEAREST 0
#define OPTIM_FIND_NEAREST_PRECOMP_COEFFS 1

// Optim: limit the number of loop count
// by bounding the range
//
// With 2500: Gain: 35ms => 25(15)ms ~30%
//
// Gives bad result for bell...
//
#define OPTIM_LIMIT_FREQ 0

#define MAX_FREQ_PARTIAL   2500.0
#define MAX_FREQ_HARMO     2500.0

// For EstimateMultiRes

// Basis
//#define ESTIM_MULTIRES_MIN_PRECISION 0.1
//#define ESTIM_MULTIRES_MAX_PRECISION 10.0

// Optim
// precision: 20 (like a fft bin)
// 35 => 22(15)ms ~30%
//
// And gives good result for bell !
#define ESTIM_MULTIRES_MIN_PRECISION 0.2
#define ESTIM_MULTIRES_MAX_PRECISION 20.0

bool
PartialTWMEstimate3::Freq::ErrorLess(const Freq &f1, const Freq &f2)
{
    return (f1.mError < f2.mError);
}


PartialTWMEstimate3::PartialTWMEstimate3(int bufferSize,
                                         BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mHarmonicSoundFlag = true;
}

PartialTWMEstimate3::~PartialTWMEstimate3() {}

void
PartialTWMEstimate3::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPrevPartials.clear();
}

void
PartialTWMEstimate3::SetHarmonicSoundFlag(bool flag)
{
    mHarmonicSoundFlag = flag;
}

BL_FLOAT
PartialTWMEstimate3::Estimate(const vector<PartialTracker5::Partial> &partials)
{
    vector<PartialTracker5::Partial> partials0 = partials;
    
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);
    
    // Limit the number of partials
    LimitPartialsNumber(&partials0);
    
#if OPTIM_FIND_NEAREST || OPTIM_FIND_NEAREST_PRECOMP_COEFFS
    // Be sure the partials are sorted
    // (will be used in optimized fine nearest)
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);
#endif
    
    // Take the max partials frequency
    // This avoids generating many useless harmonics
    // => this decreases freqs jumps
    // => and this improves performances !
    //
    // For "bell": worse results. The error is less, but not at the good place
    //
    // NOTE: this is what is done in the article !
    BL_FLOAT maxFreqHarmo = FindMaxFreqHarmo(partials0);
    
#if OPTIM_LIMIT_FREQ
    if (maxFreqHarmo > MAX_FREQ_HARMO)
        maxFreqHarmo = MAX_FREQ_HARMO;
#endif
    
    // NOTE: the start test freq must be below the frequency reange of the input signal
    BL_FLOAT minFreqSearch = MIN_FREQ_FIND_F0;
    
    // Limit the search range to the second partial freq
    // This would avoid finding incorrect results
    // (octave...)
    BL_FLOAT maxFreqSearch = FindMaxFreqSearch(partials0);

#if OPTIM_LIMIT_FREQ
    if (maxFreqSearch > MAX_FREQ_PARTIAL)
        maxFreqSearch = MAX_FREQ_PARTIAL;
#endif
    
    BL_FLOAT error = 0.0;
    BL_FLOAT result = 0.0;

#if ESTIMATE_NAIVE
    result = EstimateNaive(partials0);
#endif
#if ESTIMATE_SIMPLE
    // By default, estimate with 1Hz precision
    result = Estimate(partials0, 1.0, minFreqSearch, maxFreqSearch, maxFreqHarmo, &error);
#endif
#if ESTIMATE_MULTI
    vector<Freq> freqs;
    EstimateMulti(partials0, 1.0, minFreqSearch, maxFreqSearch,
                  maxFreqHarmo, &freqs);
    
    //DBG_DumpFreqs(freqs);
    
    // Get best freq (min error)
    sort(freqs.begin(), freqs.end(), Freq::ErrorLess);
    
    if (!freqs.empty())
    {
        error = freqs[0].mError;
        result = freqs[0].mFreq;
    }
#endif
    
    return result;
}

BL_FLOAT
PartialTWMEstimate3::EstimateMultiRes(const vector<PartialTracker5::Partial> &partials)
{    
    vector<PartialTracker5::Partial> partials0 = partials;
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);
    
    // Limit the number of partials
    LimitPartialsNumber(&partials0);
    
#if OPTIM_FIND_NEAREST || OPTIM_FIND_NEAREST_PRECOMP_COEFFS
    // Be sure the partials are sorted
    // (will be used in optimized fine nearest)
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);
#endif
    
    BL_FLOAT precision = ESTIM_MULTIRES_MAX_PRECISION;
    
    BL_FLOAT minFreqSearch = MIN_FREQ_FIND_F0;
    BL_FLOAT maxFreqSearch = FindMaxFreqSearch(partials0);
    
#if OPTIM_LIMIT_FREQ
    if (maxFreqSearch > MAX_FREQ_PARTIAL)
        maxFreqSearch = MAX_FREQ_PARTIAL;
#endif
    
    BL_FLOAT maxFreqHarmo = FindMaxFreqHarmo(partials0);
    
#if OPTIM_LIMIT_FREQ
    if (maxFreqHarmo > MAX_FREQ_HARMO)
        maxFreqHarmo = MAX_FREQ_HARMO;
#endif
    
    BL_FLOAT range = maxFreqSearch - minFreqSearch;
    BL_FLOAT freq = 0.0;
    while(precision >= ESTIM_MULTIRES_MIN_PRECISION)
    {
        freq = Estimate(partials0, precision, minFreqSearch, maxFreqSearch, maxFreqHarmo);
        
        // Increase precision / decrease interval
        precision /= 10.0;
        range /= 10.0;
        
        minFreqSearch = freq - range/2.0;
        if (minFreqSearch < MIN_FREQ_FIND_F0)
            minFreqSearch = MIN_FREQ_FIND_F0;
        
        maxFreqSearch = freq + range/2.0;
        if (maxFreqSearch > mSampleRate/2.0)
            maxFreqSearch = mSampleRate/2.0;
    }
    
    return freq;
}

BL_FLOAT
PartialTWMEstimate3::EstimateOptim(const vector<PartialTracker5::Partial> &partials)
{
    vector<PartialTracker5::Partial> partials0 = partials;
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);
    
    // Limit the number of partials
    LimitPartialsNumber(&partials0);
    
    if (partials0.empty())
        return 0.0;
    
    if (partials0.size() == 1)
    {
        BL_FLOAT result = partials0[0].mFreq;
        
        return result;
    }
    
    // Compute max amplitude
    BL_FLOAT Amax = -BL_INF;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials0[i];
        if (partial.mAmp > Amax)
            Amax = partial.mAmp;
    }
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > BL_EPS)
        AmaxInv = 1.0/Amax;
    
    // Compute possible harmonics from the input partial
    vector<BL_FLOAT> candidates;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &p = partials0[i];
        
        int hNum = 1;
        while(true)
        {
            BL_FLOAT h = p.mFreq/hNum;
            
            if (h < MIN_FREQ_FIND_F0)
                break;
            
            candidates.push_back(h);
            
            hNum++;
        }
    }
    
    BL_FLOAT maxFreq = MAX_FREQ_FIND_F0;
    
    // Algo
    BL_FLOAT minError = BL_INF;
    BL_FLOAT bestFreq0 = partials0[0].mFreq;
    for (int i = 0; i < candidates.size(); i++)
    {
        BL_FLOAT testFreq = candidates[i];
        
        BL_FLOAT err = ComputeTWMError(partials0, testFreq, maxFreq, AmaxInv);
        if (err < minError)
        {
            minError = err;
            bestFreq0 = testFreq;
        }
    }
    
    return bestFreq0;
}

BL_FLOAT
PartialTWMEstimate3::EstimateOptim2(const vector<PartialTracker5::Partial> &partials)
{
    vector<PartialTracker5::Partial> partials0 = partials;
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);
    
    // Limit the number of partials
    LimitPartialsNumber(&partials0);
    
    if (partials0.empty())
        return 0.0;
    
    if (partials0.size() == 1)
    {
        BL_FLOAT result = partials0[0].mFreq;
        
        return result;
    }
    
    // Compute the intervals
    vector<BL_FLOAT> intervals;
    if (!partials0.empty())
        intervals.push_back(partials0[0].mFreq);
    for (int i = 0; i < partials0.size() - 1; i++)
    {
        const PartialTracker5::Partial &p0 = partials0[i];
        const PartialTracker5::Partial &p1 = partials0[i + 1];
        
        BL_FLOAT interval = p1.mFreq - p0.mFreq;
        intervals.push_back(interval);
    }
    
    // Compute max amplitude
    BL_FLOAT Amax = -BL_INF;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials0[i];
        if (partial.mAmp > Amax)
            Amax = partial.mAmp;
    }
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > BL_EPS)
        AmaxInv = 1.0/Amax;
    
    BL_FLOAT maxFreq = MAX_FREQ_FIND_F0;
    
    // Algo
    BL_FLOAT minError = BL_INF;
    BL_FLOAT bestFreq0 = partials0[0].mFreq;
    for (int i = 0; i < intervals.size(); i++)
    {
        BL_FLOAT testFreq = intervals[i];
        
        BL_FLOAT err = ComputeTWMError(partials0, testFreq, maxFreq, AmaxInv);
        if (err < minError)
        {
            minError = err;
            bestFreq0 = testFreq;
        }
    }
    
    return bestFreq0;
}

BL_FLOAT
PartialTWMEstimate3::EstimateNaive(const vector<PartialTracker5::Partial> &partials)
{
    if (partials.empty())
        return 0.0;
    
    // Very simple
    //return partials[0].mFreq;
    
    // Take the frequency of the maximum umplitude partial
    int maxIdx = -1;
    BL_FLOAT maxAmp = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &p = partials[i];
        if (p.mAmp > maxAmp)
        {
            maxAmp = p.mAmp;
            maxIdx = i;
        }
    }
    
    BL_FLOAT freq = 0.0;
    if (maxIdx != -1)
        freq = partials[maxIdx].mFreq;
    
    return freq;
}

BL_FLOAT
PartialTWMEstimate3::Estimate(const vector<PartialTracker5::Partial> &partials,
                              BL_FLOAT freqAccuracy,
                              BL_FLOAT minFreqSearch, BL_FLOAT maxFreqSearch,
                              BL_FLOAT maxFreqHarmo,
                              BL_FLOAT *error)
{
    vector<PartialTracker5::Partial> partials0 = partials;
    
#if OPTIM_FIND_NEAREST || OPTIM_FIND_NEAREST_PRECOMP_COEFFS
    vector<BL_FLOAT> partialFreqs;
    for (int i = 0; i < partials.size(); i++)
    {
        BL_FLOAT freq = partials[i].mFreq;
        partialFreqs.push_back(freq);
    }
#endif
    
    if (partials0.empty())
        return 0.0;
    
    if (partials0.size() == 1)
    {
        BL_FLOAT result = partials0[0].mFreq;
        
        return result;
    }
    
    // Compute max amplitude
    // (and inverse max amplitude, for optimization)
    BL_FLOAT Amax = -BL_INF;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials0[i];
        if (partial.mAmp > Amax)
            Amax = partial.mAmp;
    }
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > BL_EPS)
        AmaxInv = 1.0/Amax;
        
    BL_FLOAT testFreq = minFreqSearch;
    
#if OPTIM_FIND_NEAREST_PRECOMP_COEFFS
    // Optim: pre-compute some error terms
    
    // Pre-compute fkp
    BL_FLOAT p = 0.5;
    vector<BL_FLOAT> fkps;
    for (int i = 0; i < partials0.size(); i++)
    {
        BL_FLOAT f = partials0[i].mFreq;
        BL_FLOAT fkp = std::pow(f, -p);
        
        fkps.push_back(fkp);
    }
    
    // Pre-compute norm amplitude
    vector<BL_FLOAT> aNorms;
    for (int i = 0; i < partials0.size(); i++)
    {
        BL_FLOAT a = partials0[i].mAmp;
        
        BL_FLOAT aNorm = a*AmaxInv;
        
        aNorms.push_back(aNorm);
    }
#endif
    
    BL_FLOAT minError = BL_INF;
    BL_FLOAT bestFreq = testFreq;
    while(testFreq < maxFreqSearch)
    {
#if !OPTIM_FIND_NEAREST && !OPTIM_FIND_NEAREST_PRECOMP_COEFFS
        BL_FLOAT err = ComputeTWMError(partials0, testFreq,
                                       maxFreqHarmo, AmaxInv);
#endif
        
#if OPTIM_FIND_NEAREST
        BL_FLOAT err = ComputeTWMError2(partials0, partialFreqs,
                                        testFreq, maxFreqHarmo, AmaxInv);
#endif
        
#if OPTIM_FIND_NEAREST_PRECOMP_COEFFS
        BL_FLOAT err = ComputeTWMError3(partials0, partialFreqs, testFreq,
                                        maxFreqHarmo, aNorms, fkps);
#endif

        if (err < minError)
        {
            minError = err;
            bestFreq = testFreq;
        }
        
        testFreq += freqAccuracy;
    }
    
    if (error != NULL)
        *error = minError;
    
    return bestFreq;
}

void
PartialTWMEstimate3::EstimateMulti(const vector<PartialTracker5::Partial> &partials,
                                   BL_FLOAT freqAccuracy,
                                   BL_FLOAT minFreqSearch, BL_FLOAT maxFreqSearch,
                                   BL_FLOAT maxFreqHarmo,
                                   vector<Freq> *freqs)
{
    freqs->resize(0);
    
    vector<PartialTracker5::Partial> partials0 = partials;
    
    if (partials0.empty())
    {
        Freq f;
        f.mFreq = 0.0;
        f.mError = 0.0;
        freqs->push_back(f);
        
        return ;
    }
    
    if (partials0.size() == 1)
    {
        Freq f;
        f.mFreq = partials0[0].mFreq;
        f.mError = 0.0;
        freqs->push_back(f);
        
        return;
    }
    
    // Compute max amplitude
    // (and inverse max amplitude, for optimization)
    BL_FLOAT Amax = -BL_INF;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials0[i];
        if (partial.mAmp > Amax)
            Amax = partial.mAmp;
    }
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > BL_EPS)
        AmaxInv = 1.0/Amax;
    
    BL_FLOAT testFreq = minFreqSearch;
    while(testFreq < maxFreqSearch)
    {
        vector<BL_FLOAT> dbgHarmos;
        BL_FLOAT dbgErrorK;
        BL_FLOAT dbgErrorN;
        
        BL_FLOAT err = ComputeTWMError(partials0, testFreq, maxFreqHarmo, AmaxInv,
                                     &dbgHarmos, &dbgErrorK, &dbgErrorN);
        
        Freq f;
        f.mFreq = testFreq;
        f.mError = err;
        freqs->push_back(f);
        
        testFreq += freqAccuracy;
    }
}

BL_FLOAT
PartialTWMEstimate3::ComputeTWMError(const vector<PartialTracker5::Partial> &partials,
                                     BL_FLOAT testFreq, BL_FLOAT maxFreqHarmo,
                                     BL_FLOAT AmaxInv,
                                     vector<BL_FLOAT> *dbgHarmos,
                                     BL_FLOAT *dbgErrorK,
                                     BL_FLOAT *dbgErrorN)
{
    if (partials.empty())
        return 0.0;
    
    // Generate the vector of harmonics
    vector<BL_FLOAT> harmos;
    BL_FLOAT h = testFreq;
    
    while(h < maxFreqHarmo)
    {
        harmos.push_back(h);
        h += testFreq;
    }
    
    // Add one harmonic more, to go after the frequency
    harmos.push_back(h);
    
    if (dbgHarmos != NULL)
        *dbgHarmos = harmos;
    
    BL_FLOAT rho = 0.33;
    
    // First pass
    BL_FLOAT En = 0.0;
    
    // If not harmonic (bell), we don't need En,
    // so don't compute it ! (perf gain)
    if (mHarmonicSoundFlag)
    {
        for (int i = 0; i < harmos.size(); i++)
        {
            BL_FLOAT h0 = harmos[i];
        
            // Find the nearest partial
            int nearestPartialIdx = -1;
            BL_FLOAT minDiff = BL_INF;
            for (int j = 0; j < partials.size(); j++)
            {
                const PartialTracker5::Partial &partial = partials[j];
            
                BL_FLOAT diff = std::fabs(partial.mFreq - h0);
                if (diff < minDiff)
                {
                    minDiff = diff;
                    nearestPartialIdx = j;
                }
            }
        
            if (nearestPartialIdx != -1)
            {
                const PartialTracker5::Partial &nearestPartial =
                    partials[nearestPartialIdx];
                En += ComputeErrorN(nearestPartial, h0, AmaxInv);
            }
        }
        
        En /= harmos.size();
    }
    
    
    // Second pass
    BL_FLOAT Ek = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials[i];
        
        // Find the nearest harmonic
        BL_FLOAT nearestHarmo = 0.0;
        BL_FLOAT minDiff = BL_INF;
        for (int j = 0; j < harmos.size(); j++)
        {
            BL_FLOAT h0 = harmos[j];
            
            BL_FLOAT diff = std::fabs(partial.mFreq - h0);
            if (diff < minDiff)
            {
                minDiff = diff;
                nearestHarmo = h0;
            }
        }
        
        Ek += ComputeErrorK(partial, nearestHarmo, AmaxInv);
    }
    
    Ek /= partials.size();
    
    BL_FLOAT Etotal = En + rho*Ek;
    
    if (dbgErrorK != NULL)
        *dbgErrorK = Ek;
    
    if (dbgErrorN != NULL)
        *dbgErrorN = En;
    
    return Etotal;
}

// Optimized nearest freq find
// Gain: 60ms => 40ms (30%)
BL_FLOAT
PartialTWMEstimate3::ComputeTWMError2(const vector<PartialTracker5::Partial> &partials,
                                      const vector<BL_FLOAT> &partialFreqs,
                                      BL_FLOAT testFreq, BL_FLOAT maxFreqHarmo,
                                      BL_FLOAT AmaxInv)
{
    if (partials.empty())
        return 0.0;
    
    // Generate the vector of harmonics
    vector<BL_FLOAT> harmos;
    BL_FLOAT h = testFreq;
    
    while(h < maxFreqHarmo)
    {
        harmos.push_back(h);
        h += testFreq;
    }
    
    // Add one harmonic more, to go after the frequency
    harmos.push_back(h);
    
    BL_FLOAT rho = 0.33;
    
    // First pass
    BL_FLOAT En = 0.0;
    for (int i = 0; i < harmos.size(); i++)
    {
        BL_FLOAT h0 = harmos[i];
        
        // Find the nearest partial
        int nearestPartialIdx = FindNearestIndex(partialFreqs, h0);
        if (nearestPartialIdx != -1)
        {
            const PartialTracker5::Partial &nearestPartial =
                partials[nearestPartialIdx];
            En += ComputeErrorN(nearestPartial, h0, AmaxInv);
        }
    }
    
    En /= harmos.size();
    
    // Second pass
    BL_FLOAT Ek = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials[i];
        
        // Find the nearest harmonic
        int nearestHarmoIdx = FindNearestIndex(harmos, partial.mFreq);
        if (nearestHarmoIdx != -1)
        {
            BL_FLOAT nearestHarmo = harmos[nearestHarmoIdx];
        
            Ek += ComputeErrorK(partial, nearestHarmo, AmaxInv);
        }
    }
    
    Ek /= partials.size();
    
    BL_FLOAT Etotal;
    if (mHarmonicSoundFlag)
    {
        // Like in the paper
        Etotal = En + rho*Ek;
    }
    else
    {
        // Good for bell
        Etotal = Ek;
    }
    
    return Etotal;
}

// Partial
BL_FLOAT
PartialTWMEstimate3::ComputeErrorN(const PartialTracker5::Partial &nearestPartial,
                                   BL_FLOAT harmo, BL_FLOAT AmaxInv)
{    
    // Parameters
    BL_FLOAT p = 0.5;
    BL_FLOAT q = 1.4;
    BL_FLOAT r = 0.5;
    
    // Compute error
    BL_FLOAT fnp = std::pow(harmo, -p);
    BL_FLOAT deltaF = std::fabs(nearestPartial.mFreq - harmo);
    
    BL_FLOAT err0 = deltaF*fnp;
    BL_FLOAT err1 = 0.0;
    
    BL_FLOAT a = nearestPartial.mAmp;
    
    err1 = (a*AmaxInv)*(q*deltaF*fnp - r);
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

// Harmo
BL_FLOAT
PartialTWMEstimate3::ComputeErrorK(const PartialTracker5::Partial &partial,
                                  BL_FLOAT nearestHarmo, BL_FLOAT AmaxInv)
{    
    // Parameters
    BL_FLOAT p = 0.5;
    BL_FLOAT q = 1.4;
    BL_FLOAT r = 0.5;
    
    // Compute error
    BL_FLOAT fkp = std::pow(partial.mFreq, -p);
    BL_FLOAT deltaF = std::fabs(partial.mFreq - nearestHarmo);
    
    BL_FLOAT err0 = deltaF*fkp;
    BL_FLOAT err1 = 0.0;
    
    BL_FLOAT a = partial.mAmp;
    
    err1 = (a*AmaxInv)*(q*deltaF*fkp - r);
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

// ComputeTWMError2
// Optimized nearest freq find
// Gain: 60ms => 40ms (30%)
//
// ComputeTWMError3: precompute anorm and fkp
// Gain: 40ms => 36(25)ms ~15%
//
BL_FLOAT
PartialTWMEstimate3::ComputeTWMError3(const vector<PartialTracker5::Partial> &partials,
                                      const vector<BL_FLOAT> &partialFreqs,
                                      BL_FLOAT testFreq, BL_FLOAT maxFreqHarmo,
                                      const vector<BL_FLOAT> &aNorms,
                                      const vector<BL_FLOAT> &fkps)
{
    if (partials.empty())
        return 0.0;
    
    // Generate the vector of harmonics
    vector<BL_FLOAT> harmos;
    BL_FLOAT h = testFreq;
    
    while(h < maxFreqHarmo)
    {
        harmos.push_back(h);
        h += testFreq;
    }
    
    // Add one harmonic more, to go after the frequency
    harmos.push_back(h);
    
    BL_FLOAT rho = 0.33;
    
    // First pass
    BL_FLOAT En = 0.0;
    for (int i = 0; i < harmos.size(); i++)
    {
        BL_FLOAT h0 = harmos[i];
        
        // Find the nearest partial
        int nearestPartialIdx = FindNearestIndex(partialFreqs, h0);
        if (nearestPartialIdx != -1)
        {
            const PartialTracker5::Partial &nearestPartial =
                partials[nearestPartialIdx];
            En += ComputeErrorN2(nearestPartial, h0, aNorms[nearestPartialIdx]);
        }
    }
    
    En /= harmos.size();
    
    // Second pass
    BL_FLOAT Ek = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials[i];
        
        // Find the nearest harmonic
        int nearestHarmoIdx = FindNearestIndex(harmos, partial.mFreq);
        if (nearestHarmoIdx != -1)
        {
            BL_FLOAT nearestHarmo = harmos[nearestHarmoIdx];
            
            Ek += ComputeErrorK2(partial, nearestHarmo, aNorms[i], fkps[i]);
        }
    }
    
    Ek /= partials.size();
    
    BL_FLOAT Etotal;
    if (mHarmonicSoundFlag)
    {
        // Like in the paper
        Etotal = En + rho*Ek;
    }
    else
    {
        // Good for bell
        Etotal = Ek;
    }
    
    return Etotal;
}

// No need to precompute fnp
// (we will compute it an use it only one time)
//
// Partial
BL_FLOAT
PartialTWMEstimate3::ComputeErrorN2(const PartialTracker5::Partial &nearestPartial,
                                    BL_FLOAT harmo, BL_FLOAT aNorm)
{
    // Parameters
    BL_FLOAT p = 0.5;
    BL_FLOAT q = 1.4;
    BL_FLOAT r = 0.5;
    
    // Compute error
    BL_FLOAT fnp = std::pow(harmo, -p);
    BL_FLOAT deltaF = std::fabs(nearestPartial.mFreq - harmo);
    
    BL_FLOAT err0 = deltaF*fnp;
    BL_FLOAT err1 = 0.0;
    
    err1 = aNorm*(q*err0 - r);
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

// Harmo
BL_FLOAT
PartialTWMEstimate3::ComputeErrorK2(const PartialTracker5::Partial &partial,
                                    BL_FLOAT nearestHarmo,
                                    BL_FLOAT aNorm, BL_FLOAT fkp)
{
    // Parameters
    //BL_FLOAT p = 0.5;
    BL_FLOAT q = 1.4;
    BL_FLOAT r = 0.5;
    
    // Compute error
    BL_FLOAT deltaF = std::fabs(partial.mFreq - nearestHarmo);
    
    BL_FLOAT err0 = deltaF*fkp;
    BL_FLOAT err1 = 0.0;
    
    err1 = aNorm*(q*err0 - r);
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

BL_FLOAT
PartialTWMEstimate3::FindMaxFreqHarmo(const vector<PartialTracker5::Partial> &partials)
{
    BL_FLOAT maxFreq = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials[i];
        
        if (partial.mFreq > maxFreq)
            maxFreq = partial.mFreq;
    }
    
    return maxFreq;
}

// Returns the max freq to test
// choose it around the first measured partial
//
// e.g the freq of the second partial
BL_FLOAT
PartialTWMEstimate3::FindMaxFreqSearch(const vector<PartialTracker5::Partial> &partials)
{
    if (partials.empty())
        return 0.0;
    
    vector<PartialTracker5::Partial> partials0 = partials;
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);
    
    if (partials0.size() == 1)
    {
        BL_FLOAT res = partials0[0].mFreq*2.0;
        
        return res;
    }
    
    if (partials0.size() > 1)
    {
        // Take the second partial
        BL_FLOAT res = partials0[1].mFreq;
        
        return res;
    }
    
    return 0.0;
}

void
PartialTWMEstimate3::SelectPartials(vector<PartialTracker5::Partial> *partials)
{
    // Get only alive partials
    // This avoids taking temporary low freq partials,
    // that would make an incorrect freq for FindMaxFreqSearch()
    //
    // (avoids several freq jump for "oohoo")
    GetAlivePartials(partials);
    
    // When a partial appears, it appears as alive (for one frame)
    // (then it becomes zombie)
    // If it is a bad partial, it will make a bad partial for one frame.
    //
    // (avoids one additional freq jump for "oohoo")
    SuppressNewPartials(partials);
}

void
PartialTWMEstimate3::GetAlivePartials(vector<PartialTracker5::Partial> *partials)
{
    vector<PartialTracker5::Partial> partials0 = *partials;
    
    if (partials0.empty())
        return;
    
    partials->clear();
    
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &p = partials0[i];
        if (p.mState == PartialTracker5::Partial::ALIVE)
        {
            partials->push_back(p);
        }
    }
}

void
PartialTWMEstimate3::SuppressNewPartials(vector<PartialTracker5::Partial> *partials)
{
    const vector<PartialTracker5::Partial> partials0 = *partials;
    
    if (partials0.empty())
        return;
    
    partials->clear();
    
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &p = partials0[i];
        
        bool found = FindPartialById(mPrevPartials, (int)p.mId);
        if (found)
        {
            partials->push_back(p);
        }
    }
    
    mPrevPartials = partials0;
}

bool
PartialTWMEstimate3::FindPartialById(const vector<PartialTracker5::Partial> &partials,
                                     int idx)
{
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &partial = partials[i];
        
        if (partial.mId == idx)
            return true;
    }
    
    return false;
}

// Optimized freq search
// Gain: 60ms => 40ms (30%)
int
PartialTWMEstimate3::FindNearestIndex(const vector<BL_FLOAT> &freqs, BL_FLOAT freq)
{
    // NOTE: partials freqs must be sorted
    int nearestIdx = -1;
    
    vector<BL_FLOAT> &freqs0 = (vector<BL_FLOAT> &)freqs;
    
    vector<BL_FLOAT>::iterator it =
        lower_bound(freqs0.begin(), freqs0.end(), freq);
    
    nearestIdx = (int)(it - freqs0.begin());
    
    if ((nearestIdx > 0) &&
        (nearestIdx < freqs0.size())) // should not happen
    {
      BL_FLOAT diff0 = std::fabs(freqs0[nearestIdx - 1] - freq);
      BL_FLOAT diff1 = std::fabs(freqs0[nearestIdx] - freq);
        if (diff0 < diff1)
            nearestIdx--;
    }
        
    if (nearestIdx > freqs0.size() - 1)
    // Index is at the end
    {
        // The nearest freq is the last one, because they are no more freqs after
        nearestIdx = (int)freqs0.size() - 1;
    }
    
    return nearestIdx;
}

// Input partials  must be sorted by freq
void
PartialTWMEstimate3::LimitPartialsNumber(vector<PartialTracker5::Partial> *sortedPartials)
{
    vector<PartialTracker5::Partial> result;
    for (int i = 0; i < sortedPartials->size(); i++)
    {
        const PartialTracker5::Partial &partial = (*sortedPartials)[i];
        result.push_back(partial);
        
        if (i > MAX_NUM_PARTIALS)
            break;
    }
    
    *sortedPartials = result;
}
