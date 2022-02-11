//
//  PartialTWMEstimate2.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/8/19.
//
//

#include <algorithm>
using namespace std;

#include <Utils.h>
#include <Debug.h>

#include "PartialTWMEstimate2.h"

#define MIN_DB -120.0


#define MIN_FREQ_FIND_F0 50.0
// OPTIMIZE: Optimize by a factor x5 (with 2000)

// 2000: works for "oohoo", 10000 (works for "bell")
#define MAX_FREQ_FIND_F0 10000.0 //2000.0 //10000.0 //2000.0


PartialTWMEstimate2::PartialTWMEstimate2(int bufferSize,
                                         double sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
}

PartialTWMEstimate2::~PartialTWMEstimate2() {}

void
PartialTWMEstimate2::Reset(double sampleRate)
{
    mSampleRate = sampleRate;
}

double
PartialTWMEstimate2::Estimate(const vector<PartialTracker3::Partial> &partials)
{
    //double maxFreq = mSampleRate/2.0;
    
    // Take the max partials frequency
    // This avoids generating many useless harmonics
    // => this decreases freqs jumps
    // => and this improves performances !
    //
    // For "bell": worse results. The error is less, but not at the good place
    //
    // NOTE: this is what is done in the article !
    double maxFreqHarmo = FindMaxFreqHarmo(partials);
    
    // TEST WEDNESDAY
    // False => not like that in the article !
    //double maxFreq = MAX_FREQ_FIND_F0;
    
    // NOTE: the start test freq must be below the frequency reange of the input signal
    
    
    // By default, estimate with 1Hz precision
    double error;
    double res = Estimate(partials, 1.0, MIN_FREQ_FIND_F0, maxFreqHarmo, &error);
    
    
    PartialTracker3::DBG_DumpPartials2("partials.txt", partials,
                                       mBufferSize, mSampleRate);
    
    Debug::AppendValue("freq.txt", res);
    Debug::AppendValue("error.txt", error);
    
    static int count = 0;
    if (count == 47)
    {
        int dummy = 0;
    }
    
    if (count == 55)
    {
        int dummy = 0;
    }
    count++;
    
    return res;
}

// NOTE: maybe it is not optimally accurate (check precision ?)
double
PartialTWMEstimate2::EstimateMultiRes(const vector<PartialTracker3::Partial> &partials)
{    
// Take a larger search interval (100)
// Example that failed; voic oohhooo, second verse
// (first frequency was 800 instead of 500)
// (so with 100, search between 0 and 1000 at second step)
#define MARGIN_COEFF 100.0
    
#define MIN_PRECISION 0.1
#define MAX_PRECISION 10.0
    
//#define MAX_FREQ mSampleRate/2.0
//#define MAX_FREQ 1000.0 // TEST
#define MAX_FREQ MAX_FREQ_FIND_F0 // TEST WEDNESDAY
    
    double precision = MAX_PRECISION;
    double minFreq = MIN_FREQ_FIND_F0;
    double maxFreq = MAX_FREQ;
    
    double freq = 0.0;
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

double
PartialTWMEstimate2::EstimateOptim(const vector<PartialTracker3::Partial> &partials)
{
#define INF 1e15
#define EPS 1e-15
    
    if (partials.empty())
        return 0.0;
    
    if (partials.size() == 1)
    {
        double result = partials[0].mFreq;
        
        return result;
    }
    
    // Compute max amplitude
    double AmaxDB = MIN_DB;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    double Amax = DBToAmp(AmaxDB);
    
    //Amax = -1.0/AmpToDB(Amax); // TEST
    double AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
    
    // Compute possible harmonics from the input partial
    vector<double> candidates;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &p = partials[i];
        
        int hNum = 1;
        while(true)
        {
            double h = p.mFreq/hNum;
            
            if (h < MIN_FREQ_FIND_F0)
                break;
            
            candidates.push_back(h);
            
            hNum++;
        }
    }
    
    //double maxFreq = mSampleRate/2.0;
    double maxFreq = MAX_FREQ_FIND_F0;
    
    // Algo
    double minError = INF;
    double bestFreq0 = partials[0].mFreq;
    for (int i = 0; i < candidates.size(); i++)
    {
        double testFreq = candidates[i];
        
        double err = ComputeTWMError(partials, testFreq, maxFreq, AmaxInv);
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
    
    double precision = PRECISION;
    double margin = MARGIN;
    
    while(precision >= MIN_PRECISION)
    {
        double minFreq = bestFreq0 - margin;
        if (minFreq < MIN_FREQ_FIND_F0)
            minFreq = MIN_FREQ_FIND_F0;
        
        double maxFreq = bestFreq0 + margin;
        if (maxFreq > MAX_FREQ)
            maxFreq = MAX_FREQ;
        
        bestFreq0 = Estimate(partials, precision, minFreq, maxFreq);
        
        precision /= DECREASE_COEFF;
        margin /= DECREASE_COEFF;
    }
#endif
    
    return bestFreq0;
}

double
PartialTWMEstimate2::Estimate(const vector<PartialTracker3::Partial> &partials,
                              double freqAccuracy,
                              double minFreq, double maxFreqHarmo,
                              double *error)
{
#define INF 1e15
#define EPS 1e-15
    
    vector<PartialTracker3::Partial> partials0 = partials;
    
    if (partials0.empty())
        return 0.0;
    
    if (partials0.size() == 1)
    {
        double result = partials0[0].mFreq;
        
        return result;
    }
    
    // TEST WEDNESDAY
    //double maxPartialsFreq = FindMaxFreq(partials0);
    //if (maxFreq > maxPartialsFreq)
    //    maxFreq = maxPartialsFreq;
    
    // TEST WEDNESDAY
    // Very good: avoids comparing partials out of bounds
    // (this was making the error great, artificially)
    //PartialsRange(&partials0, minFreq, maxFreq);
    
    PartialTracker3::DBG_DumpPartials2("partials-range.txt", partials,
                                       mBufferSize, mSampleRate);
    
    // Compute max amplitude
    // (and inverse max amplitude, for optimization)
    double AmaxDB = MIN_DB;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials0[i];
        if (partial.mAmpDB > AmaxDB)
            AmaxDB = partial.mAmpDB;
    }
    
    // TEST MONDAY
    double Amax = DBToAmp(AmaxDB); // origin
    //double Amax = AmaxDB - MIN_DB;
    
    //Amax = -1.0/AmpToDB(Amax); // TEST
    double AmaxInv = 0.0;
    if (Amax > EPS)
        AmaxInv = 1.0/Amax;
        
    double testFreq = minFreq;
    //double maxFreqSearch = maxFreq; // TEST WEDNESDAY (orig)
    
    // Limit the search range to the second partial freq
    // This would avoid finding incorrect results
    // (octave...)
    double maxFreqSearch = FindMaxFreqSearch(partials);
    
    double minError = INF;
    double bestFreq0 = testFreq;
    while(testFreq < maxFreqSearch)
    {
        vector<double> dbgHarmos;
        double err = ComputeTWMError(partials0, testFreq, maxFreqHarmo, AmaxInv, &dbgHarmos);
        
        if (err < minError)
        {
            minError = err;
            bestFreq0 = testFreq;
            
            DBG_DumpFreqs("harmos.txt", dbgHarmos);
        }
        
        testFreq += freqAccuracy;
    }
    
    if (error != NULL)
        *error = minError;
    
    return bestFreq0;
}

double
PartialTWMEstimate2::ComputeTWMError(const vector<PartialTracker3::Partial> &partials,
                                     double testFreq, double maxFreq,
                                     double AmaxInv,
                                     vector<double> *dbgHarmos)
{
#define INF 1e15
    
    if (partials.empty())
        return 0.0;
    
    // Generate the vector of harmonics
    vector<double> harmos;
    double h = testFreq;
    
    //while(h < mSampleRate/2.0) // optim
    //while(h < maxFreq) // TEST WEDNESDAY (orig)
    while(h < maxFreq)
    {
        harmos.push_back(h);
        h += testFreq;
    }
    
    // TEST WEDNESDAY
    // Add one harmonic more, to go after the frequency
    harmos.push_back(h);
    
    if (dbgHarmos != NULL)
        *dbgHarmos = harmos;
    
    double rho = 0.33;
    
    // First pass
    double En = 0.0;
    for (int i = 0; i < harmos.size(); i++)
    {
        double h = harmos[i];
        
        // Find the nearest partial
        int nearestPartialIdx = -1;
        double minDiff = INF;
        for (int j = 0; j < partials.size(); j++)
        {
            const PartialTracker3::Partial &partial = partials[j];
            
            double diff = fabs(partial.mFreq - h);
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
    
    
    // Second pass
    double Ek = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        // Find the nearest harmonic
        double nearestHarmo = 0.0;
        double minDiff = INF;
        for (int j = 0; j < harmos.size(); j++)
        {
            double h = harmos[j];
            
            double diff = fabs(partial.mFreq - h);
            if (diff < minDiff)
            {
                minDiff = diff;
                nearestHarmo = h;
            }
        }
        
        Ek += ComputeErrorK(partial, nearestHarmo, AmaxInv);
    }
    
    Ek /= partials.size();
    
    double Etotal = /*En + */ rho*Ek;
    
    return Etotal;
}

// Partial
double
PartialTWMEstimate2::ComputeErrorN(const PartialTracker3::Partial &partial,
                                   double harmo, double AmaxInv)
{
//#define EPS 1e-15
    
    // Parameters
    double p = 0.5;
    double q = 1.4;
    double r = 0.5;
    
    // Compute error
    double deltaF = fabs(partial.mFreq - harmo);
    double fnp = pow(harmo, -p);
    
    double err0 = deltaF*fnp;
    double err1 = 0.0;
    //if (AmaxInv > EPS)
    {
        double aDB = partial.mAmpDB;
        
        // TEST MONDAY...
        double a = DBToAmp(aDB); // origin
        //double a = aDB - MIN_DB;
        
        //a = -1.0/AmpToDB(a); // TEST
        
        err1 = (a*AmaxInv)*(q*deltaF*fnp - r);
    }
    
    double err = err0 + err1;
    
    return err;
}

// Harmo
double
PartialTWMEstimate2::ComputeErrorK(const PartialTracker3::Partial &partial,
                                  double harmo, double AmaxInv)
{
//#define EPS 1e-15
    
    // Parameters
    double p = 0.5;
    double q = 1.4;
    double r = 0.5;
    
    // Compute error
    double deltaF = fabs(partial.mFreq - harmo);
    double fkp = pow(partial.mFreq, -p);
    
    double err0 = deltaF*fkp;
    double err1 = 0.0;
    //if (AmaxInv > EPS)
    {
        double aDB = partial.mAmpDB;
        
        // TEST MONDAY
        double a = DBToAmp(aDB); // origin
        //double a = aDB - MIN_DB;
        
        
        //a = -1.0/AmpToDB(a); // TEST
        
        err1 = (a*AmaxInv)*(q*deltaF*fkp - r);
    }
    
    double err = err0 + err1;
    
    return err;
}

double
PartialTWMEstimate2::FixFreqJumps(double freq0, double prevFreq)
{
    // 100 Hz
#define FREQ_JUMP_THRESHOLD 100.0
    
    double diff = fabs(freq0 - prevFreq);
    if (diff > FREQ_JUMP_THRESHOLD)
    {
        //return prevFreq;
        
        double result = GetNearestHarmonic(freq0, prevFreq);
        
        return result;
    }
    
    return freq0;
}

// Use this to avoid harmonic jumps of the fundamental frequency
double
PartialTWMEstimate2::GetNearestOctave(double freq0, double refFreq)
{
#define INF 1e15
    
    // 1 Hz
#define FREQ_RES 1.0
    
    if (refFreq < MIN_FREQ_FIND_F0)
        return freq0;
    
    double result = freq0;
    
    double currentFreq = freq0;
    double minFreqDiff = INF;
    if (freq0 <= refFreq)
    {
        while(currentFreq < mSampleRate/2.0)
        {
            double freqDiff = fabs(currentFreq - refFreq);
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
            double freqDiff = fabs(currentFreq - refFreq);
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

double
PartialTWMEstimate2::GetNearestHarmonic(double freq0, double refFreq)
{
#define INF 1e15
#define EPS 1e-15
    
    // 1 Hz
#define FREQ_RES 1.0
    
    if (refFreq < MIN_FREQ_FIND_F0)
        return freq0;
    
    //if (refFreq < EPS)
    //    return freq0;
        
    double result = freq0;
    
    double currentFreq = freq0;
    double minFreqDiff = INF;
    if (freq0 <= refFreq)
    {
        while(currentFreq < mSampleRate/2.0)
        {
            double freqDiff = fabs(currentFreq - refFreq);
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
            
            double freqDiff = fabs(currentFreq - refFreq);
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

double
PartialTWMEstimate2::FindMaxFreqHarmo(const vector<PartialTracker3::Partial> &partials)
{
    double maxFreq = 0.0;
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
double
PartialTWMEstimate2::FindMaxFreqSearch(const vector<PartialTracker3::Partial> &partials)
{
    if (partials.empty())
        return 0.0;
    
    vector<PartialTracker3::Partial> partials0 = partials;
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
    if (partials0.size() == 1)
    {
        double res = partials0[0].mFreq*2.0;
        
        return res;
    }
    
    if (partials0.size() > 1)
    {
        // Take the second partial
        double res = partials0[1].mFreq;
        
        return res;
    }
}

void
PartialTWMEstimate2::PartialsRange(vector<PartialTracker3::Partial> *partials,
                                   double minFreq, double maxFreq)
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
PartialTWMEstimate2::DBG_DumpFreqs(const char *fileName,
                                   const vector<double> &freqs)
{
    double hzPerBin = mSampleRate/mBufferSize;
    
    WDL_TypedBuf<double> buffer;
    Utils::ResizeFillValue(&buffer, mBufferSize/2, MIN_DB);
    
    for (int i = 0; i < freqs.size(); i++)
    {
        double freq = freqs[i];
        
        double binNum = freq/hzPerBin;
        binNum = round(binNum);
        
        double amp = -20.0;
        
        buffer.Get()[(int)binNum] = amp;
    }
    
    Debug::DumpData(fileName, buffer);
}
