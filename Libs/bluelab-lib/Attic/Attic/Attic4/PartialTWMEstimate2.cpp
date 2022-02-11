//
//  PartialTWMEstimate2.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/8/19.
//
//

#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLDebug.h>

#include "PartialTWMEstimate2.h"

#define INF 1e15
#define EPS 1e-15

#define MIN_DB -120.0


#define MIN_FREQ_FIND_F0 50.0
#define MAX_FREQ_FIND_F0 10000.0

// Estimate and return all the errors and frequencies ?
#define ESTIMATE_MULTI 1

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

// Debug
#define DEBUG_NUM_LOOPS 0

#if DEBUG_NUM_LOOPS
int __NumLoops = 0;
int __maxNumLoops = 0;
#endif

bool
PartialTWMEstimate2::Freq::ErrorLess(const Freq &f1, const Freq &f2)
{
    return (f1.mError < f2.mError);
}


PartialTWMEstimate2::PartialTWMEstimate2(int bufferSize,
                                         BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mHarmonicSoundFlag = true;
}

PartialTWMEstimate2::~PartialTWMEstimate2() {}

void
PartialTWMEstimate2::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPrevPartials.clear();
}

void
PartialTWMEstimate2::SetHarmonicSoundFlag(bool flag)
{
    mHarmonicSoundFlag = flag;
}

BL_FLOAT
PartialTWMEstimate2::Estimate(const vector<PartialTracker3::Partial> &partials)
{
    vector<PartialTracker3::Partial> partials0 = partials;
    
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
    // Limit the number of partials
    LimitPartialsNumber(&partials0);
    
#if OPTIM_FIND_NEAREST || OPTIM_FIND_NEAREST_PRECOMP_COEFFS
    // Be sure the partials are sorted
    // (will be used in optimized fine nearest)
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
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
    
#if !ESTIMATE_MULTI
    // By default, estimate with 1Hz precision
    result = Estimate(partials0, 1.0, minFreqSearch, maxFreqSearch, maxFreqHarmo, &error);
#else
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
    
#if 0 // DEBUG
    //PartialTracker3::DBG_DumpPartials2("partials.txt", partials0,
    //                                   mBufferSize, mSampleRate);
    
    BLDebug::AppendValue("freq.txt", result);
    BLDebug::AppendValue("error.txt", error);
    
    static int count = 0;
    if (count == 41) // ok
    {
        int dummy = 0;
    }
    if (count == 42) // ko
    {
        int dummy = 0;
    }
    count++;
#endif
    
    return result;
}

BL_FLOAT
PartialTWMEstimate2::EstimateMultiRes(const vector<PartialTracker3::Partial> &partials)
{    
    vector<PartialTracker3::Partial> partials0 = partials;
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
    // Limit the number of partials
    LimitPartialsNumber(&partials0);
    
#if OPTIM_FIND_NEAREST || OPTIM_FIND_NEAREST_PRECOMP_COEFFS
    // Be sure the partials are sorted
    // (will be used in optimized fine nearest)
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
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
    
#if DEBUG_NUM_LOOPS
    __NumLoops = 0;
#endif
    
    BL_FLOAT range0 = range;
    BL_FLOAT maxFreq0 = maxFreqSearch;
    BL_FLOAT maxHarmo0 = maxFreqHarmo;
    
    BL_FLOAT freq = 0.0;
    while(precision >= ESTIM_MULTIRES_MIN_PRECISION)
    {
        //fprintf(stderr, "# freqs: %g, #harmo: %g\n",
        //        range/precision,
        //        (log(maxFreqHarmo)/log(2.0)));
        
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

#if DEBUG_NUM_LOOPS
    if (__NumLoops > __maxNumLoops)
    {
        __maxNumLoops = __NumLoops;
        
        fprintf(stderr, "loops: %d   partials: %ld   range: %g   max freq: %g   max harmo: %g\n",
                __NumLoops,
                partials0.size(),
                (BL_FLOAT)((int)range0),
                (BL_FLOAT)((int)maxFreq0),
                (BL_FLOAT)((int)maxHarmo0));
    }
#endif
    
    return freq;
}

BL_FLOAT
PartialTWMEstimate2::EstimateOptim(const vector<PartialTracker3::Partial> &partials)
{
    vector<PartialTracker3::Partial> partials0 = partials;
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
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
    BL_FLOAT AmaxDB = MIN_DB;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials0[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    
    BL_FLOAT Amax = BLUtils::DBToAmp(AmaxDB);
    //BL_FLOAT Amax = AmaxDB - MIN_DB;
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
    
    // Compute possible harmonics from the input partial
    vector<BL_FLOAT> candidates;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &p = partials0[i];
        
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
    BL_FLOAT minError = INF;
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
    
#if 0 // Refine ?
    // Refine the result
    
    // 100 Hz
#define MARGIN 100.0 //100.0 //100.0
    // 10Hz
#define PRECISION 10.0//1.0
#define MIN_PRECISION 1.0 //0.1
    
    // Divide by 10 at each iteration
#define DECREASE_COEFF 10.0
    
#define MAX_FREQ mSampleRate/2.0
    
    BL_FLOAT precision = PRECISION;
    BL_FLOAT margin = MARGIN;
    
    while(precision >= MIN_PRECISION)
    {
        BL_FLOAT minFreq = bestFreq0 - margin;
        if (minFreq < MIN_FREQ_FIND_F0)
            minFreq = MIN_FREQ_FIND_F0;
        
        BL_FLOAT maxFreq = bestFreq0 + margin;
        if (maxFreq > MAX_FREQ)
            maxFreq = MAX_FREQ;
        
        bestFreq0 = Estimate(partials0, precision, minFreq, maxFreq);
        
        precision /= DECREASE_COEFF;
        margin /= DECREASE_COEFF;
    }
#endif
    
    return bestFreq0;
}

BL_FLOAT
PartialTWMEstimate2::EstimateOptim2(const vector<PartialTracker3::Partial> &partials)
{
    vector<PartialTracker3::Partial> partials0 = partials;
    SelectPartials(&partials0);
    
    // Sort by frequencies
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
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
        const PartialTracker3::Partial &p0 = partials0[i];
        const PartialTracker3::Partial &p1 = partials0[i + 1];
        
        BL_FLOAT interval = p1.mFreq - p0.mFreq;
        intervals.push_back(interval);
    }
    
    // Compute max amplitude
    BL_FLOAT AmaxDB = MIN_DB;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials0[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    
    BL_FLOAT Amax = BLUtils::DBToAmp(AmaxDB);
    //BL_FLOAT Amax = AmaxDB - MIN_DB;
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
    
    BL_FLOAT maxFreq = MAX_FREQ_FIND_F0;
    
    // Algo
    BL_FLOAT minError = INF;
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
    
#if 0 // Refine ?
    // Refine the result
    
    // 100 Hz
#define MARGIN 100.0 //100.0 //100.0
    // 10Hz
#define PRECISION 10.0//1.0
#define MIN_PRECISION 1.0 //0.1
    
    // Divide by 10 at each iteration
#define DECREASE_COEFF 10.0
    
#define MAX_FREQ mSampleRate/2.0
    
    BL_FLOAT precision = PRECISION;
    BL_FLOAT margin = MARGIN;
    
    BL_FLOAT minFreqSearch = bestFreq0 - MARGIN/2; //MIN_FREQ_FIND_F0;
    BL_FLOAT maxFreqSearch = bestFreq0 + MARGIN/2; //FindMaxFreqSearch(partials0);
    
//#if OPTIM_LIMIT_FREQ
//    if (maxFreqSearch > MAX_FREQ_PARTIAL)
//        maxFreqSearch = MAX_FREQ_PARTIAL;
//#endif
    
    BL_FLOAT maxFreqHarmo = FindMaxFreqHarmo(partials0);
    
#if OPTIM_LIMIT_FREQ
    if (maxFreqHarmo > MAX_FREQ_HARMO)
        maxFreqHarmo = MAX_FREQ_HARMO;
#endif
    
    while(precision >= MIN_PRECISION)
    {
        BL_FLOAT minFreq = bestFreq0 - margin;
        if (minFreq < MIN_FREQ_FIND_F0)
            minFreq = MIN_FREQ_FIND_F0;
        
        BL_FLOAT maxFreq = bestFreq0 + margin;
        if (maxFreq > MAX_FREQ)
            maxFreq = MAX_FREQ;
        
        bestFreq0 = Estimate(partials0, precision, minFreqSearch,
                             maxFreqSearch, maxFreqHarmo);
        
        precision /= DECREASE_COEFF;
        margin /= DECREASE_COEFF;
    }
#endif
    
    return bestFreq0;
}

BL_FLOAT
PartialTWMEstimate2::Estimate(const vector<PartialTracker3::Partial> &partials,
                              BL_FLOAT freqAccuracy,
                              BL_FLOAT minFreqSearch, BL_FLOAT maxFreqSearch,
                              BL_FLOAT maxFreqHarmo,
                              BL_FLOAT *error)
{
    vector<PartialTracker3::Partial> partials0 = partials;
    
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
    BL_FLOAT AmaxDB = MIN_DB;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials0[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    
    BL_FLOAT Amax = BLUtils::DBToAmp(AmaxDB);
    //BL_FLOAT Amax = AmaxDB - MIN_DB;
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
        
    BL_FLOAT testFreq = minFreqSearch;
    
    // Debug
    //BL_FLOAT dbgBestErrorK = 0.0;
    //BL_FLOAT dbgBestErrorN = 0.0;
    
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
        BL_FLOAT aDB = partials0[i].mAmpDB;
    
        BL_FLOAT a = BLUtils::DBToAmp(aDB); // orig
        //BL_FLOAT a = aDB - MIN_DB;
        
        BL_FLOAT aNorm = a*AmaxInv;
        
        aNorms.push_back(aNorm);
    }
#endif
    
    BL_FLOAT minError = INF;
    BL_FLOAT bestFreq = testFreq;
    while(testFreq < maxFreqSearch)
    {
        //vector<BL_FLOAT> dbgHarmos;
        //BL_FLOAT dbgErrorK;
        //BL_FLOAT dbgErrorN;
        
#if !OPTIM_FIND_NEAREST && !OPTIM_FIND_NEAREST_PRECOMP_COEFFS
        BL_FLOAT err = ComputeTWMError(partials0, testFreq, maxFreqHarmo, AmaxInv);
                                     //&dbgHarmos, &dbgErrorK, &dbgErrorN);
#endif
        
#if OPTIM_FIND_NEAREST
        BL_FLOAT err = ComputeTWMError2(partials0, partialFreqs, testFreq, maxFreqHarmo, AmaxInv);
#endif
        
#if OPTIM_FIND_NEAREST_PRECOMP_COEFFS
        BL_FLOAT err = ComputeTWMError3(partials0, partialFreqs, testFreq,
                                      maxFreqHarmo, /*AmaxInv,*/ aNorms, fkps);
#endif

        if (err < minError)
        {
            minError = err;
            bestFreq = testFreq;
            
            //DBG_DumpFreqs("harmos.txt", dbgHarmos);
            
            //dbgBestErrorK = dbgErrorK;
            //dbgBestErrorN = dbgErrorN;
        }
        
        testFreq += freqAccuracy;
    }
    
    //BLDebug::AppendValue("error-n.txt", dbgBestErrorN);
    //BLDebug::AppendValue("error-k.txt", dbgBestErrorK);
    
    if (error != NULL)
        *error = minError;
    
    return bestFreq;
}

void
PartialTWMEstimate2::EstimateMulti(const vector<PartialTracker3::Partial> &partials,
                                   BL_FLOAT freqAccuracy,
                                   BL_FLOAT minFreqSearch, BL_FLOAT maxFreqSearch,
                                   BL_FLOAT maxFreqHarmo,
                                   vector<Freq> *freqs)
{
    freqs->resize(0);
    
    vector<PartialTracker3::Partial> partials0 = partials;
    
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
    BL_FLOAT AmaxDB = MIN_DB;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials0[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    
    BL_FLOAT Amax = BLUtils::DBToAmp(AmaxDB);
    //BL_FLOAT Amax = AmaxDB - MIN_DB;
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
    
    BL_FLOAT testFreq = minFreqSearch;
    
    BL_FLOAT dbgBestErrorK = 0.0;
    BL_FLOAT dbgBestErrorN = 0.0;
    
    BL_FLOAT minError = INF;
    BL_FLOAT bestFreq = testFreq;
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
PartialTWMEstimate2::ComputeTWMError(const vector<PartialTracker3::Partial> &partials,
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
            BL_FLOAT h = harmos[i];
        
            // Find the nearest partial
            int nearestPartialIdx = -1;
            BL_FLOAT minDiff = INF;
            for (int j = 0; j < partials.size(); j++)
            {
                const PartialTracker3::Partial &partial = partials[j];
            
                BL_FLOAT diff = std::fabs(partial.mFreq - h);
                if (diff < minDiff)
                {
                    minDiff = diff;
                    nearestPartialIdx = j;
                }
            }
        
            if (nearestPartialIdx != -1)
            {
                const PartialTracker3::Partial &nearestPartial = partials[nearestPartialIdx];
                En += ComputeErrorN(nearestPartial, h, AmaxInv);
            }
        }
        
        En /= harmos.size();
    }
    
    
    // Second pass
    BL_FLOAT Ek = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        // Find the nearest harmonic
        BL_FLOAT nearestHarmo = 0.0;
        BL_FLOAT minDiff = INF;
        for (int j = 0; j < harmos.size(); j++)
        {
            BL_FLOAT h = harmos[j];
            
            BL_FLOAT diff = std::fabs(partial.mFreq - h);
            if (diff < minDiff)
            {
                minDiff = diff;
                nearestHarmo = h;
            }
        }
        
        Ek += ComputeErrorK(partial, nearestHarmo, AmaxInv);
    }
    
    Ek /= partials.size();
    
    BL_FLOAT Etotal;
    //if (mHarmonicSoundFlag)
    {
        // Like in the paper
        Etotal = En + rho*Ek;
    }
    //else
    //{
    //    // Good for bell
    //    Etotal = Ek;
    //}
    
    if (dbgErrorK != NULL)
        *dbgErrorK = Ek;
    
    if (dbgErrorN != NULL)
        *dbgErrorN = En;
    
    return Etotal;
}

// Optimized nearest freq find
// Gain: 60ms => 40ms (30%)
BL_FLOAT
PartialTWMEstimate2::ComputeTWMError2(const vector<PartialTracker3::Partial> &partials,
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
        BL_FLOAT h = harmos[i];
        
        // Find the nearest partial
        int nearestPartialIdx = FindNearestIndex(partialFreqs, h);
        if (nearestPartialIdx != -1)
        {
            const PartialTracker3::Partial &nearestPartial = partials[nearestPartialIdx];
            En += ComputeErrorN(nearestPartial, h, AmaxInv);
        }
    }
    
    En /= harmos.size();
    
    
    // Second pass
    BL_FLOAT Ek = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
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
PartialTWMEstimate2::ComputeErrorN(const PartialTracker3::Partial &nearestPartial,
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
    
    BL_FLOAT aDB = nearestPartial.mAmpDB;
        
    BL_FLOAT a = BLUtils::DBToAmp(aDB);
    //BL_FLOAT a = aDB - MIN_DB;
        
    err1 = (a*AmaxInv)*(q*deltaF*fnp - r);
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

// Harmo
BL_FLOAT
PartialTWMEstimate2::ComputeErrorK(const PartialTracker3::Partial &partial,
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
    
    
    BL_FLOAT aDB = partial.mAmpDB;
        
    BL_FLOAT a = BLUtils::DBToAmp(aDB);
    //BL_FLOAT a = aDB - MIN_DB;
        
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
PartialTWMEstimate2::ComputeTWMError3(const vector<PartialTracker3::Partial> &partials,
                                      const vector<BL_FLOAT> &partialFreqs,
                                      BL_FLOAT testFreq, BL_FLOAT maxFreqHarmo,
                                      //BL_FLOAT AmaxInv,
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
        BL_FLOAT h = harmos[i];
        
        // Find the nearest partial
        int nearestPartialIdx = FindNearestIndex(partialFreqs, h);
        if (nearestPartialIdx != -1)
        {
            const PartialTracker3::Partial &nearestPartial = partials[nearestPartialIdx];
            En += ComputeErrorN2(nearestPartial, h, aNorms[nearestPartialIdx]/*AmaxInv*//*, fnps[i]*/);
        }
        
#if DEBUG_NUM_LOOPS
        __NumLoops++;
#endif
    }
    
    En /= harmos.size();
    
    
    // Second pass
    BL_FLOAT Ek = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        // Find the nearest harmonic
        int nearestHarmoIdx = FindNearestIndex(harmos, partial.mFreq);
        if (nearestHarmoIdx != -1)
        {
            BL_FLOAT nearestHarmo = harmos[nearestHarmoIdx];
            
            Ek += ComputeErrorK2(partial, nearestHarmo, aNorms[i], /*AmaxInv,*/ fkps[i]);
        }
        
#if DEBUG_NUM_LOOPS
        __NumLoops++;
#endif
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
PartialTWMEstimate2::ComputeErrorN2(const PartialTracker3::Partial &nearestPartial,
                                    BL_FLOAT harmo, //BL_FLOAT AmaxInv,
                                    BL_FLOAT aNorm)
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
    
    //BL_FLOAT aDB = nearestPartial.mAmpDB;
    
    //BL_FLOAT a = DBToAmp(aDB); // orig
    //BL_FLOAT a = aDB - MIN_DB;
    
    err1 = /*(a*AmaxInv)*/aNorm*(q*err0/*deltaF*fnp*/ - r);
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

// Harmo
BL_FLOAT
PartialTWMEstimate2::ComputeErrorK2(const PartialTracker3::Partial &partial,
                                    BL_FLOAT nearestHarmo, //BL_FLOAT AmaxInv,
                                    BL_FLOAT aNorm, BL_FLOAT fkp)
{
    // Parameters
    //BL_FLOAT p = 0.5;
    BL_FLOAT q = 1.4;
    BL_FLOAT r = 0.5;
    
    // Compute error
    //BL_FLOAT fkp = std::pow(partial.mFreq, -p);
    BL_FLOAT deltaF = std::fabs(partial.mFreq - nearestHarmo);
    
    BL_FLOAT err0 = deltaF*fkp;
    BL_FLOAT err1 = 0.0;
    
    
    //BL_FLOAT aDB = partial.mAmpDB;
    
    //BL_FLOAT a = DBToAmp(aDB); // orig
    //BL_FLOAT a = aDB - MIN_DB;
    
    err1 = aNorm/*(a*AmaxInv)**/*(q*err0 /*deltaF*fkp*/ - r);
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

// Unused
BL_FLOAT
PartialTWMEstimate2::FixFreqJumps(BL_FLOAT freq0, BL_FLOAT prevFreq)
{
    // 100 Hz
#define FREQ_JUMP_THRESHOLD 100.0
    
  BL_FLOAT diff = std::fabs(freq0 - prevFreq);
    if (diff > FREQ_JUMP_THRESHOLD)
    {
        //return prevFreq;
        
        BL_FLOAT result = GetNearestHarmonic(freq0, prevFreq);
        
        return result;
    }
    
    return freq0;
}


// Unused

// Use this to avoid harmonic jumps of the fundamental frequency
BL_FLOAT
PartialTWMEstimate2::GetNearestOctave(BL_FLOAT freq0, BL_FLOAT refFreq)
{
    // 1 Hz
#define FREQ_RES 1.0
    
    if (refFreq < MIN_FREQ_FIND_F0)
        return freq0;
    
    BL_FLOAT result = freq0;
    
    BL_FLOAT currentFreq = freq0;
    BL_FLOAT minFreqDiff = INF;
    if (freq0 <= refFreq)
    {
        while(currentFreq < mSampleRate/2.0)
        {
	  BL_FLOAT freqDiff = std::fabs(currentFreq - refFreq);
            if (freqDiff < minFreqDiff)
            {
                minFreqDiff = freqDiff;
                
                result = currentFreq;
            }
            
            currentFreq *= 2.0;
        }
    }
    else if (freq0 > refFreq)
    {
        while(currentFreq > FREQ_RES)
        {
	  BL_FLOAT freqDiff = std::fabs(currentFreq - refFreq);
            if (freqDiff < minFreqDiff)
            {
                minFreqDiff = freqDiff;
                
                result = currentFreq;
            }
            
            currentFreq /= 2.0;
        }
    }
    
    return result;
}

// Unused
BL_FLOAT
PartialTWMEstimate2::GetNearestHarmonic(BL_FLOAT freq0, BL_FLOAT refFreq)
{
    // 1 Hz
#define FREQ_RES 1.0
    
    if (refFreq < MIN_FREQ_FIND_F0)
        return freq0;
        
    BL_FLOAT result = freq0;
    
    BL_FLOAT currentFreq = freq0;
    BL_FLOAT minFreqDiff = INF;
    if (freq0 <= refFreq)
    {
        while(currentFreq < mSampleRate/2.0)
        {
	  BL_FLOAT freqDiff = std::fabs(currentFreq - refFreq);
            if (freqDiff < minFreqDiff)
            {
                minFreqDiff = freqDiff;
                
                result = currentFreq;
            }
            
            currentFreq += freq0;
        }
    }
#if 1 // This works well with this commented (with SASViewer)
      // (when uncommented, the freq goes near 1Hz and gets stuck)
    else if (freq0 > refFreq)
    {
        int hNum = 1;
        while(currentFreq > FREQ_RES)
        {
            currentFreq = freq0/hNum;
            
            BL_FLOAT freqDiff = std::fabs(currentFreq - refFreq);
            if (freqDiff < minFreqDiff)
            {
                minFreqDiff = freqDiff;
                
                result = currentFreq;
            }
            
            hNum++;
        }
    }
#endif
    
    return result;
}

BL_FLOAT
PartialTWMEstimate2::FindMaxFreqHarmo(const vector<PartialTracker3::Partial> &partials)
{
    BL_FLOAT maxFreq = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
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
PartialTWMEstimate2::FindMaxFreqSearch(const vector<PartialTracker3::Partial> &partials)
{
    if (partials.empty())
        return 0.0;
    
    vector<PartialTracker3::Partial> partials0 = partials;
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
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

// Unused
void
PartialTWMEstimate2::PartialsRange(vector<PartialTracker3::Partial> *partials,
                                   BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
    vector<PartialTracker3::Partial> result;
    
    for (int i = 0; i < partials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*partials)[i];
        
        if ((partial.mFreq >= minFreq) &&
            (partial.mFreq <= maxFreq))
            result.push_back(partial);
    }
    
    *partials = result;
}

void
PartialTWMEstimate2::SelectPartials(vector<PartialTracker3::Partial> *partials)
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
PartialTWMEstimate2::GetAlivePartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> partials0 = *partials;
    
    if (partials0.empty())
        return;
    
    partials->clear();
    
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &p = partials0[i];
        if (p.mState == PartialTracker3::Partial::ALIVE)
        {
            partials->push_back(p);
        }
    }
}

void
PartialTWMEstimate2::SuppressNewPartials(vector<PartialTracker3::Partial> *partials)
{
    const vector<PartialTracker3::Partial> partials0 = *partials;
    
    if (partials0.empty())
        return;
    
    partials->clear();
    
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &p = partials0[i];
        
        bool found = FindPartialById(mPrevPartials, p.mId);
        if (found)
        {
            partials->push_back(p);
        }
    }
    
    mPrevPartials = partials0;
}

bool
PartialTWMEstimate2::FindPartialById(const vector<PartialTracker3::Partial> &partials,
                                     int idx)
{
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        if (partial.mId == idx)
            return true;
    }
    
    return false;
}

// Optimized freq search
// Gain: 60ms => 40ms (30%)
int
PartialTWMEstimate2::FindNearestIndex(const vector<BL_FLOAT> &freqs, BL_FLOAT freq)
{
    // NOTE: partials freqs must be sorted
    int nearestIdx = -1;
    
    //fprintf(stderr, "freqs: ");
    //for (int i = 0; i < freqs.size(); i++)
    //    fprintf(stderr, "%g ", freqs[i]);
    //fprintf(stderr, "\n");
    
    vector<BL_FLOAT> &freqs0 = (vector<BL_FLOAT> &)freqs;
    
    // NOTE: with that, the performances would have dropped
    //vector<BL_FLOAT> freqs0 = freqs;
    
    vector<BL_FLOAT>::iterator it =
        lower_bound(freqs0.begin(), freqs0.end(), freq);
    
    nearestIdx = it - freqs0.begin();
    
    if (nearestIdx > 0)
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
        nearestIdx = freqs0.size() - 1;
    }
    
    return nearestIdx;
}

// Input partials  must be sorted by freq
void
PartialTWMEstimate2::LimitPartialsNumber(vector<PartialTracker3::Partial> *sortedPartials)
{
    vector<PartialTracker3::Partial> result;
    for (int i = 0; i < sortedPartials->size(); i++)
    {
        const PartialTracker3::Partial &partial = (*sortedPartials)[i];
        result.push_back(partial);
        
        if (i > MAX_NUM_PARTIALS)
            break;
    }
    
    *sortedPartials = result;
}

void
PartialTWMEstimate2::DBG_DumpFreqs(const char *fileName,
                                   const vector<BL_FLOAT> &freqs)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    WDL_TypedBuf<BL_FLOAT> buffer;
    BLUtils::ResizeFillValue(&buffer, mBufferSize/2, (BL_FLOAT)MIN_DB);
    
    for (int i = 0; i < freqs.size(); i++)
    {
        BL_FLOAT freq = freqs[i];
        
        BL_FLOAT binNum = freq/hzPerBin;
        binNum = bl_round(binNum);
        
        BL_FLOAT amp = -20.0;
        
        buffer.Get()[(int)binNum] = amp;
    }
    
    BLDebug::DumpData(fileName, buffer);
}

void
PartialTWMEstimate2::DBG_DumpFreqs(const vector<Freq> &freqs)
{
    WDL_TypedBuf<BL_FLOAT> freqsBuf;
    freqsBuf.Resize(freqs.size());
    
    WDL_TypedBuf<BL_FLOAT> errBuf;
    errBuf.Resize(freqs.size());
    
    for (int i = 0; i < freqs.size(); i++)
    {
        BL_FLOAT freq = freqs[i].mFreq;
        freqsBuf.Get()[i] = freq;
        
        BL_FLOAT err = freqs[i].mError;
        errBuf.Get()[i] = err;
    }
    
    BLDebug::DumpData("freqs.txt", freqsBuf);
    BLDebug::DumpData("errors.txt", errBuf);
}
