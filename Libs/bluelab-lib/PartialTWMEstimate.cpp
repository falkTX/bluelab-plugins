//
//  PartialTWMEstimate.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/8/19.
//
//

#include <cmath>

#include <BLUtils.h>

#include "PartialTWMEstimate.h"

#define MIN_DB -120.0


#define MIN_FREQ_FIND_F0 50.0
// OPTIMIZE: Optimize by a factor x5 (with 2000)
#define MAX_FREQ_FIND_F0 2000.0


PartialTWMEstimate::PartialTWMEstimate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

PartialTWMEstimate::~PartialTWMEstimate() {}

BL_FLOAT
PartialTWMEstimate::Estimate(const vector<PartialTracker3::Partial> &partials)
{
    // By default, estimate with 1Hz precision
    BL_FLOAT res = Estimate(partials, 1.0, MIN_FREQ_FIND_F0, mSampleRate/2.0);
    
    return res;
}

// NOTE: maybe it is not optimally accurate (check precision ?)
BL_FLOAT
PartialTWMEstimate::EstimateMultiRes(const vector<PartialTracker3::Partial> &partials)
{    
// Take a larger search interval (100)
// Example that failed; voic oohhooo, second verse
// (first frequency was 800 instead of 500)
// (so with 100, search between 0 and 1000 at second step)
#define MARGIN_COEFF 100.0
    
#define MIN_PRECISION 0.1
#define MAX_PRECISION 10.0
    
#define MAX_FREQ mSampleRate/2.0
//#define MAX_FREQ 1000.0 // TEST
    
    BL_FLOAT precision = MAX_PRECISION;
    BL_FLOAT minFreq = MIN_FREQ_FIND_F0;
    BL_FLOAT maxFreq = MAX_FREQ;
    
    BL_FLOAT freq = 0.0;
    while(precision >= MIN_PRECISION)
    {
        freq = Estimate(partials, precision, minFreq, maxFreq);
        
        minFreq = freq - precision*MARGIN_COEFF;
        if (minFreq < MIN_FREQ_FIND_F0)
            minFreq = MIN_FREQ_FIND_F0;
        
        maxFreq = freq + precision*MARGIN_COEFF;
        if (maxFreq > MAX_FREQ)
            maxFreq = MAX_FREQ;
        
        precision /= 10.0;
    }
    
    return freq;
}

#if 0 // BUGGY
BL_FLOAT
PartialTWMEstimate::EstimateOptim(const vector<PartialTracker3::Partial> &partials)
{
#define INF 1e15
#define EPS 1e-15
    
    if (partials.empty())
        return 0.0;
    
    if (partials.size() == 1)
    {
        BL_FLOAT result = partials[0].mFreq;
        
        return result;
    }
    
    // Search range
    BL_FLOAT minFreq = MIN_FREQ_FIND_F0;
    
    // Compute possible harmonics from the input partial frequencies
    vector<BL_FLOAT> harmos;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &p = partials[i];
        
        int hNum = 1;
        while(true)
        {
            BL_FLOAT h = p.mFreq/hNum;
            
            if (h < minFreq)
                break;
            
            harmos.push_back(h);
            
            hNum++;
        }
    }
    
    if (harmos.empty())
        return 0.0;
    
    // Algo
    // Compute max amplitude
    BL_FLOAT Amax = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        if (partial.mAmp > Amax)
            Amax = partial.mAmp;
    }
    
    //Amax = -1.0/AmpToDB(Amax); // TEST
    
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
    
    BL_FLOAT minError = INF;
    BL_FLOAT bestFreq0 = harmos[0];
    for (int i = 0; i < harmos.size(); i++)
    {
        BL_FLOAT freq = harmos[i];
        
        BL_FLOAT err = ComputeTWMError(partials, harmos, /*freq,*/ AmaxInv);
        
        if (err < minError)
        {
            minError = err;
            bestFreq0 = freq;
        }
    }
    
    fprintf(stderr, "min error: %g\n", minError);
    
#if 1 // Refine ?
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
        minFreq = bestFreq0 - margin;
        if (minFreq < MIN_FREQ_FIND_F0)
            minFreq = MIN_FREQ_FIND_F0;
    
        BL_FLOAT maxFreq = bestFreq0 + margin;
        if (maxFreq > MAX_FREQ)
            maxFreq = MAX_FREQ;
    
        bestFreq0 = Estimate(partials, precision, minFreq, maxFreq);
        
        precision /= DECREASE_COEFF;
        margin /= DECREASE_COEFF;
    }
#endif
    
    return bestFreq0;
}
#endif

BL_FLOAT
PartialTWMEstimate::EstimateOptim(const vector<PartialTracker3::Partial> &partials)
{
#define INF 1e15
#define EPS 1e-15
    
    if (partials.empty())
        return 0.0;
    
    if (partials.size() == 1)
    {
        BL_FLOAT result = partials[0].mFreq;
        
        return result;
    }
    
    // Compute max amplitude
    BL_FLOAT AmaxDB = MIN_DB;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    BL_FLOAT Amax = BLUtils::DBToAmp(AmaxDB);
    
    //Amax = -1.0/AmpToDB(Amax); // TEST
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
    
    // Compute possible harmonics from the input partial
    vector<BL_FLOAT> candidates;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &p = partials[i];
        
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
    
    //BL_FLOAT maxFreq = mSampleRate/2.0;
    BL_FLOAT maxFreq = MAX_FREQ_FIND_F0;
    
    // Algo
    BL_FLOAT minError = INF;
    BL_FLOAT bestFreq0 = partials[0].mFreq;
    for (int i = 0; i < candidates.size(); i++)
    {
        BL_FLOAT testFreq = candidates[i];
        
        BL_FLOAT err = ComputeTWMError(partials, testFreq, maxFreq, AmaxInv);
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
        
        bestFreq0 = Estimate(partials, precision, minFreq, maxFreq);
        
        precision /= DECREASE_COEFF;
        margin /= DECREASE_COEFF;
    }
#endif
    
    return bestFreq0;
}

BL_FLOAT
PartialTWMEstimate::Estimate(const vector<PartialTracker3::Partial> &partials,
                             BL_FLOAT freqAccuracy,
                             BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
#define INF 1e15
#define EPS 1e-15
    
    if (partials.empty())
        return 0.0;
    
    if (partials.size() == 1)
    {
        BL_FLOAT result = partials[0].mFreq;
        
        return result;
    }
    
    // Compute max amplitude
    // (and inverse max amplitude, for optimization)
    BL_FLOAT AmaxDB = MIN_DB;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    
    BL_FLOAT Amax = BLUtils::DBToAmp(AmaxDB);
    
    //Amax = -1.0/AmpToDB(Amax); // TEST
    BL_FLOAT AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
        
    BL_FLOAT testFreq = minFreq;
    
    BL_FLOAT minError = INF;
    BL_FLOAT bestFreq0 = testFreq;
    while(testFreq < maxFreq)
    {
        BL_FLOAT err = ComputeTWMError(partials, testFreq, maxFreq, AmaxInv);
        
        if (err < minError)
        {
            minError = err;
            bestFreq0 = testFreq;
        }
        
        testFreq += freqAccuracy;
    }
    
    return bestFreq0;
}

BL_FLOAT
PartialTWMEstimate::ComputeTWMError(const vector<PartialTracker3::Partial> &partials,
                                    BL_FLOAT testFreq, BL_FLOAT maxFreq,
                                    BL_FLOAT AmaxInv)
{
#define INF 1e15
    
    if (partials.empty())
        return 0.0;
    
    // Generate the vector of harmonics
    vector<BL_FLOAT> harmos;
    BL_FLOAT h = testFreq;
    
    //while(h < mSampleRate/2.0) // optim
    while(h < maxFreq)
    {
        harmos.push_back(h);
        h += testFreq;
    }
    
    BL_FLOAT rho = 0.33;
    
    // First pass
    BL_FLOAT Ek = 0.0;
    for (int i = 0; i < harmos.size(); i++)
    {
        BL_FLOAT h0 = harmos[i];
        
        // Find the nearest partial
        int nearestPartialIdx = -1;
        BL_FLOAT minDiff = INF;
        for (int j = 0; j < partials.size(); j++)
        {
            const PartialTracker3::Partial &partial = partials[j];
            
            BL_FLOAT diff = std::fabs(partial.mFreq - h0);
            if (diff < minDiff)
            {
                minDiff = diff;
                nearestPartialIdx = j;
            }
        }
        
        if (nearestPartialIdx != -1)
        {
            const PartialTracker3::Partial &nearestPartial =
                partials[nearestPartialIdx];
            Ek += ComputeErrorK(nearestPartial, h0, AmaxInv);
        }
    }
    
    Ek /= harmos.size();
    
    
    // Second pass
    BL_FLOAT En = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        // Find the nearest harmonic
        BL_FLOAT nearestHarmo = 0.0;
        BL_FLOAT minDiff = INF;
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
        
        En += ComputeErrorN(partial, nearestHarmo, AmaxInv);
    }
    
    En /= partials.size();
    
    BL_FLOAT Etotal = En + rho*Ek;
    
    return Etotal;
}

// Harmo
BL_FLOAT
PartialTWMEstimate::ComputeErrorK(const PartialTracker3::Partial &partial,
                                  BL_FLOAT harmo, BL_FLOAT AmaxInv)
{
#define EPS 1e-15
    
    // Parameters
    BL_FLOAT p = 0.5;
    BL_FLOAT q = 1.4;
    BL_FLOAT r = 0.5;
    
    // Compute error
    BL_FLOAT deltaF = std::fabs(partial.mFreq - harmo);
    BL_FLOAT fnp = std::pow(harmo, -p);
    
    BL_FLOAT err0 = deltaF*fnp;
    BL_FLOAT err1 = 0.0;
    if (AmaxInv > EPS)
    {
        BL_FLOAT aDB = partial.mAmpDB;
        BL_FLOAT a = BLUtils::DBToAmp(aDB);
        
        //a = -1.0/AmpToDB(a); // TEST
        
        err1 = (a*AmaxInv)*(q*deltaF*fnp - r);
    }
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

// Partial
BL_FLOAT
PartialTWMEstimate::ComputeErrorN(const PartialTracker3::Partial &partial,
                                  BL_FLOAT harmo, BL_FLOAT AmaxInv)
{
#define EPS 1e-15
    
    // Parameters
    BL_FLOAT p = 0.5;
    BL_FLOAT q = 1.4;
    BL_FLOAT r = 0.5;
    
    // Compute error
    BL_FLOAT deltaF = std::fabs(partial.mFreq - harmo);
    BL_FLOAT fkp = std::pow(partial.mFreq, -p);
    
    BL_FLOAT err0 = deltaF*fkp;
    BL_FLOAT err1 = 0.0;
    if (AmaxInv > EPS)
    {
        BL_FLOAT aDB = partial.mAmpDB;
        
        BL_FLOAT a = BLUtils::DBToAmp(aDB);
        
        //a = -1.0/AmpToDB(a); // TEST
        
        err1 = (a*AmaxInv)*(q*deltaF*fkp - r);
    }
    
    BL_FLOAT err = err0 + err1;
    
    return err;
}

BL_FLOAT
PartialTWMEstimate::FixFreqJumps(BL_FLOAT freq0, BL_FLOAT prevFreq)
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

// Use this to avoid harmonic jumps of the fundamental frequency
BL_FLOAT
PartialTWMEstimate::GetNearestOctave(BL_FLOAT freq0, BL_FLOAT refFreq)
{
#define INF 1e15
    
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
            
            // Arbitrary stop condition
            //if (currentFreq > refFreq*2.0)
            //    break;
            
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
            
            //currentFreq -= freq0;
            currentFreq /= 2.0;
        }
    }
    
    return result;
}

BL_FLOAT
PartialTWMEstimate::GetNearestHarmonic(BL_FLOAT freq0, BL_FLOAT refFreq)
{
#define INF 1e15
#define EPS 1e-15
    
    // 1 Hz
#define FREQ_RES 1.0
    
    if (refFreq < MIN_FREQ_FIND_F0)
        return freq0;
    
    //if (refFreq < EPS)
    //    return freq0;
        
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
            
            // Arbitrary stop condition
            //if (currentFreq > refFreq*2.0)
            //    break;
            
            currentFreq += freq0;
            //currentFreq *= 2.0;
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
            
            // Arbitrary stop condition
            //if (currentFreq < refFreq/2.0)
            //    break;
            
            //currentFreq -= freq0;
            //currentFreq /= 2.0;
            
            hNum++;
        }
    }
#endif
    
    return result;
}
