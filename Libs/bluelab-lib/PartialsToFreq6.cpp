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

#include <BLDebug.h>

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

#define MIN_FREQ 20.0
#define PREV_PARTIAL_COEFF 0.75 

PartialsToFreq6::PartialsToFreq6(int bufferSize, int oversampling,
                                 int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mEstimate = new PartialTWMEstimate3(bufferSize, sampleRate);

    mChromaObj = new ChromagramObj(bufferSize, oversampling,
                                   freqRes, sampleRate);
    // Max smoothness
    mChromaObj->SetSharpness(1.0);
    // DEBUG: for hte example "oohoo", tune in order to not have a cut due to modulo
    mChromaObj->SetATune(440.0);
    //mChromaObj->SetATune(698.46); // F (half one actave higher, to avoid cut)
                         
    /*BLDebug::ResetFile("chroma0.txt");
      BLDebug::ResetFile("freq0.txt");
      BLDebug::ResetFile("freq.txt");
      BLDebug::ResetFile("min-partial.txt");
      BLDebug::ResetFile("min-partial0.txt");*/
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
    vector<PartialTracker5::Partial> partials0 = partials;

    WDL_TypedBuf<BL_FLOAT> &chromaLine = mTmpBuf0;
    mChromaObj->MagnsToChromaLine(magns, phases, &chromaLine);

    if (chromaLine.GetSize() == 0)
        return 0.0;
    
    //BL_FLOAT maxChroma = BLUtils::FindMaxValue(chromaLine);
    int maxChromaIdx = BLUtils::FindMaxIndex(chromaLine);
    BL_FLOAT maxChroma = ((BL_FLOAT)maxChromaIdx)/chromaLine.GetSize();
     
    //BLDebug::AppendValue("chroma0.txt", maxChroma);
    
    BL_FLOAT freq0 = mChromaObj->ChromaToFreq(maxChroma, MIN_FREQ);

    //BLDebug::AppendValue("freq0.txt", freq0);
    
    // Find closest partial (not working well)
#if 0 //1
    BL_FLOAT freq = FindClosestPartialFreq(freq0, partials);
#endif
    
#if 0 //1 //0 // TEST: Good!
    BL_FLOAT freq = freq0;
    while(freq < 100.0/*300.0*/)
        freq *= 2.0;
#endif
        
#if 0 //1 // Good, but if tracker loose tracking on lowest partial, this gives error
    BL_FLOAT freq = FindBestOctave(freq0, partials);
#endif

#if 1 // Good. Do not use prev tracked partials
    // This is a bit hackish, but looks efficient
    BL_FLOAT freq = FindBestOctave2(freq0, magns);
#endif
    
    //BLDebug::AppendValue("freq.txt", freq);
    
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

// Uunsed?
BL_FLOAT
PartialsToFreq6::
FindClosestPartialFreq(BL_FLOAT refFreq0,
                       const vector<PartialTracker5::Partial> &partials)
{
    if (partials.empty())
        return 0.0;

#if 0 //1
    // Get min partial freq
    BL_FLOAT minFreq = BL_INF;
    for (int i = 0; i < partials.size(); i++)
    {
        if (partials[i].mFreq < minFreq)
            minFreq = partials[i].mFreq;
    }

    return minFreq;
#endif
    
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
            //BL_FLOAT diff = std::fabs(f - pf);

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

// Take inFreq, and rise it to the best upper octave
// Take previously tracked partials as reference (may be risky)
BL_FLOAT
PartialsToFreq6::FindBestOctave(BL_FLOAT inFreq,
                                const vector<PartialTracker5::Partial> &partials)
{
    /*if (partials.empty())
      {
      BLDebug::AppendValue("min-partial.txt", 0.0);
      BLDebug::AppendValue("min-partial0.txt", 0.0);
      
      return 0.0;
      }*/
    
    vector<PartialTracker5::Partial> partials0 = partials;
    sort(partials0.begin(), partials0.end(), PartialTracker5::Partial::FreqLess);

    // Compute min partial freq
    BL_FLOAT minFreq = inFreq;
    int minIdx = -1;
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker5::Partial &p = partials0[i];
        if (p.mFreq >= MIN_FREQ)
        {
            minFreq = p.mFreq;
            minIdx = i;

            break;
        }
    }
        
    /*if (minIdx >= 0)
      BLDebug::AppendValue("min-partial.txt", partials0[minIdx].mFreq);
      
      if (!partials0.empty())
      BLDebug::AppendValue("min-partial0.txt", partials0[0].mFreq);
    */
    
    BL_FLOAT freq = inFreq;
    while(freq < minFreq*PREV_PARTIAL_COEFF)
        freq *= 2.0;

    return freq;
}

// Take inFreq, and rise it to the best upper octave
// Detect on the fly the first big partial (hackish, but more secure)
BL_FLOAT
PartialsToFreq6::FindBestOctave2(BL_FLOAT inFreq,
                                 const WDL_TypedBuf<BL_FLOAT> &magns)
{
    BLDebug::AppendValue("min-partial.txt", 0.0);

#if 0 // Very naive method, does not work well
    // Compute max amplitude
    // And consider this is the peak of the main partial
    int maxFreqIdx = BLUtils::FindMaxIndex(magns);
    BL_FLOAT hzPerBin =  mSampleRate/mBufferSize;
    BL_FLOAT minFreq = maxFreqIdx*hzPerBin;
#endif

#if 1
    BL_FLOAT minFreq = -1.0;

    // Find the first partial (kind of left border)
    BL_FLOAT maxMagn = BLUtils::FindMaxValue(magns);
    maxMagn = BLUtils::AmpToDB(maxMagn);
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT m = magns.Get()[i];
        // Threshold: 2 times smaller (in dB, this is reversed)
        if (BLUtils::AmpToDB(m) > maxMagn*2.0)
            // We have found the beginning of the first peak
        {
            // Now, find the center of the peak
            for (int j = i; j < magns.GetSize(); j++)
            {
                if (j < magns.GetSize() - 1)
                {
                    if (magns.Get()[j + 1] < magns.Get()[j])
                        // We start descending, we should be at the top of the peak
                    {
                        BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
                        minFreq = j*hzPerBin;
                        
                        break;
                    }
                }   
            }
        }

        if (minFreq > 0.0)
            // Peak found
            break;
    }

    if (minFreq < 0.0)
        return inFreq;
#endif
    
    //BLDebug::AppendValue("min-partial.txt", minFreq);
    
    BL_FLOAT freq = inFreq;
    while(freq < minFreq*PREV_PARTIAL_COEFF)
        freq *= 2.0;

    return freq;
}
