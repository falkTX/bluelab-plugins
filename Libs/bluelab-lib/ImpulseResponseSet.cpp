#include <BLUtils.h>
#include <BLUtilsFade.h>

#include "ImpulseResponseSet.h"

// Take into account the sample rate for aligning IR
// So the offset will be constant in time, and the latency
// will be constant whatever the sample rate
//
// Disable because not well tested
#define FIX_ALIGN_SR 0 //1
#define REF_SAMPLE_RATE 44100.0

#define FIX_NORMALIZE 1

#define LEFT_IMPULSES_MARGIN 0.1

// Prefer fade size in samples, to avoid cutting impulse for long responses
// Take a shorter fade-in, to not cut the maximum of the response
#define FADE_IN_SIZE_SAMPLES 64
//#define FADE_OUT_SIZE_SAMPLES 512
#define FADE_OUT_SIZE_RATIO 0.1

// For aligning just before applying
#define LEFT_IMPULSES_MARGIN_SAMPLES 512

// For optimization, consider only DISCARD_MAX_SAMPLES
// samples when comparing
// (useful for >= 1000ms)
#define DISCARD_MAX_SAMPLES 2048

ImpulseResponseSet::ImpulseResponseSet(long responseSize, BL_FLOAT sampleRate)
{
    mResponseSize = responseSize;
  
    mSampleRate = sampleRate;
}

ImpulseResponseSet::~ImpulseResponseSet() {}

void
ImpulseResponseSet::Reset(long responseSize, BL_FLOAT sampleRate)
{
    mResponseSize = responseSize;
  
    mResponses.clear();
  
    mSampleRate = sampleRate;
}

void
ImpulseResponseSet::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
ImpulseResponseSet::AddImpulseResponse(const WDL_TypedBuf<BL_FLOAT> &impulseResponse)
{
    if (impulseResponse.GetSize() != mResponseSize)
        return;
  
    mResponses.push_back(impulseResponse);
}

void
ImpulseResponseSet::GetLastImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse)
{
    if (mResponses.empty())
        return;
  
    const WDL_TypedBuf<BL_FLOAT> &lastResp = mResponses[mResponses.size() - 1];
  
    *impulseResponse = lastResp;
}

void
ImpulseResponseSet::GetAvgImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse)
{
    if (mResponses.empty())
        return;
  
    impulseResponse->Resize(mResponses[0].GetSize());
    BLUtils::FillAllZero(impulseResponse);
  
    for (int i = 0; i < mResponses.size(); i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &resp = mResponses[i];
    
        for (int j = 0; j < impulseResponse->GetSize(); j++)
        {
            BL_FLOAT val = resp.Get()[j];
      
            impulseResponse->Get()[j] += val;
        }
    }
  
    BL_FLOAT div = (BL_FLOAT)mResponses.size();
  
    for (int j = 0; j < impulseResponse->GetSize(); j++)
    {
        BL_FLOAT val = impulseResponse->Get()[j];
        val /= div;
    
        impulseResponse->Get()[j] = val;
    }
}

void
ImpulseResponseSet::Clear()
{
    mResponses.clear();
}

long
ImpulseResponseSet::GetSize() const
{
    return mResponses.size();
}

void
ImpulseResponseSet::AlignImpulseResponse(WDL_TypedBuf<BL_FLOAT> *response,
                                         long responseSize,
                                         BL_FLOAT decimFactor,
                                         BL_FLOAT sampleRate)
{
    // Do we have new instant responses ready ?
    if (response->GetSize() == 0)
        return;
  
#if !ALIGN_IR_ABSOLUTE
    // Keep 10% on the left to be sure to capture all the beginning of the response
    long alignIndex = responseSize*LEFT_IMPULSES_MARGIN/decimFactor;
#else
    long alignIndex = ALIGN_IR_TIME*sampleRate/decimFactor;
#endif
  
    AlignImpulseResponse(response, alignIndex);
}

void
ImpulseResponseSet::AlignImpulseResponseSamples(WDL_TypedBuf<BL_FLOAT> *response,
                                                long responseSize, BL_FLOAT sampleRate)
{
    if (response->GetSize() == 0)
        return;
  
    BL_FLOAT coeff = 1.0;
#if FIX_ALIGN_SR
    coeff = sampleRate/REF_SAMPLE_RATE;
#endif
  
    long alignIndex = LEFT_IMPULSES_MARGIN_SAMPLES*coeff;
  
    AlignImpulseResponse(response, alignIndex);
}

void
ImpulseResponseSet::AlignImpulseResponses(WDL_TypedBuf<BL_FLOAT> responses[2],
                                          long responseSize, BL_FLOAT sampleRate)
{
    // Do we have new instant responses ready ?
    if (responses[0].GetSize() == 0)
        return;
  
#if !ALIGN_IR_ABSOLUTE
    // Keep 10% on the left to be sure to capture all the beginning of the response
    long alignIndex = responseSize*LEFT_IMPULSES_MARGIN;
#else
    long alignIndex = ALIGN_IR_TIME*sampleRate;
#endif
  
    if (responses[1].GetSize() > 0)
        // Stereo
    {
        // Stereo align
        AlignImpulseResponse2(&responses[0], &responses[1], alignIndex);
    }
    else
        // Only mono
    {
        AlignImpulseResponse(&responses[0], alignIndex);
    }
}

void
ImpulseResponseSet::NormalizeImpulseResponses(WDL_TypedBuf<BL_FLOAT> responses[2])
{
    // Do we have new instant responses ready ?
    if (responses[0].GetSize() == 0)
        return;
  
    // Normalize (mono or stereo)
    if ((responses[0].GetSize() > 0) &&
        (responses[1].GetSize() > 0))
        // Stereo case
    {
        NormalizeImpulseResponse2(&responses[0], &responses[1]);
    }
    else
        // Mono case
        NormalizeImpulseResponse(&responses[0]);
}

void
ImpulseResponseSet::AlignImpulseResponsesAll(ImpulseResponseSet *impRespSets[2],
                                             long responseSize,
                                             BL_FLOAT sampleRate)
{
    for (int i = 0; i < impRespSets[0]->mResponses.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> resp0 = impRespSets[0]->mResponses[i];
    
        WDL_TypedBuf<BL_FLOAT> resp1;
        if (!impRespSets[1]->mResponses.empty())
            resp1 = impRespSets[1]->mResponses[i];
    
        WDL_TypedBuf<BL_FLOAT> responses[2] = { resp0, resp1 };
    
        AlignImpulseResponses(responses, responseSize, sampleRate);
    }
}

void
ImpulseResponseSet::AlignImpulseResponsesAll(ImpulseResponseSet *impRespSet,
                                             long responseSize,
                                             BL_FLOAT decimationFactor,
                                             BL_FLOAT sampleRate)
{
    for (int i = 0; i < impRespSet->mResponses.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &resp = impRespSet->mResponses[i];
    
        AlignImpulseResponse(&resp, responseSize, decimationFactor, sampleRate);
    }
}

void
ImpulseResponseSet::DiscardBadResponses(ImpulseResponseSet *responseSet,
                                        bool *lastRespValid, bool iterate)
{
    DiscardBadImpulseResponses(responseSet, lastRespValid,
                               DISCARD_MAX_SAMPLES, iterate);
}

void
ImpulseResponseSet::DiscardBadResponses(ImpulseResponseSet *responseSets[2],
                                        bool *lastRespValid, bool iterate)
{
    // Discard bad responses if necessary
    // (before displaying the average !)
    bool discardFlag = false;
    *lastRespValid = true;
    if (responseSets[0]->GetSize() > 0)
    {
        if (responseSets[1]->GetSize() > 0)
            // Stereo
        {
            discardFlag = DiscardBadImpulseResponses2(responseSets[0], responseSets[1],
                                                      lastRespValid, DISCARD_MAX_SAMPLES, iterate);
        }
        else
            // Mono
        {
            discardFlag = DiscardBadImpulseResponses(responseSets[0], lastRespValid,
                                                     DISCARD_MAX_SAMPLES, iterate);
        }
    }
}

bool
ImpulseResponseSet::DiscardBadImpulseResponses(ImpulseResponseSet *respSet,
                                               bool *lastRespValid,
                                               long discardMaxSamples, bool iterate)
{
    bool res = false;
  
    while(true)
    {
        vector<long> indicesToDiscard;
        respSet->DiscardBadImpulseResponses(&indicesToDiscard, discardMaxSamples);
  
        if (!indicesToDiscard.empty())
        {
            *lastRespValid = CheckLastRespValid(respSet, indicesToDiscard);
  
            respSet->RemoveResponses(indicesToDiscard);
      
            res = true;
      
            if (!iterate)
                // We don't iterate, stop ath the first loop
                break;
        }
        else
            // There is no worse response i.e nothing remaining to discard
            // Either all the data is good, or we have discarded all
            break;
    }
  
    return res;
}

bool
ImpulseResponseSet::DiscardBadImpulseResponses2(ImpulseResponseSet *respSet0,
                                                ImpulseResponseSet *respSet1,
                                                bool *lastRespValid,
                                                long discardMaxSamples, bool iterate)
{
#define PROFILE_BAD_IMPULSES 0
#if PROFILE_BAD_IMPULSES
    static BlaTimer timer;
    timer.Start();
#endif
  
    bool res = true;
  
    while(true)
    {
        vector<long> indicesToDiscard0;
        respSet0->DiscardBadImpulseResponses(&indicesToDiscard0, discardMaxSamples);
  
        vector<long> indicesToDiscard1;
        respSet1->DiscardBadImpulseResponses(&indicesToDiscard1, discardMaxSamples);
  
        // Make one list with the two others
        vector<long> allIndicesToDiscard;
        for (int i = 0; i < indicesToDiscard0.size(); i++)
        {
            allIndicesToDiscard.push_back(indicesToDiscard0[i]);
        }
  
        for (int i = 0; i < indicesToDiscard1.size(); i++)
        {
            allIndicesToDiscard.push_back(indicesToDiscard1[i]);
        }
  
        // Remove duplicates
        sort(allIndicesToDiscard.begin(), allIndicesToDiscard.end());
        allIndicesToDiscard.erase( unique(allIndicesToDiscard.begin(), allIndicesToDiscard.end()),
                                   allIndicesToDiscard.end());
  
        *lastRespValid = CheckLastRespValid(respSet0, allIndicesToDiscard);
  
        // Remove the indices to discard from both lists
        // (we must keep stereo consistence)
        respSet0->RemoveResponses(allIndicesToDiscard);
        respSet1->RemoveResponses(allIndicesToDiscard);
    
        res = res && (!allIndicesToDiscard.empty());
    
        if (!iterate)
            // We don't iterate, stop at the first loop
            break;
    
        if (allIndicesToDiscard.empty())
            // Nothing remaining to discard
            // Either all the data is good, or we have discarded all
            break;
    }
  
#if PROFILE_BAD_IMPULSES
    timer.Stop();
    long t = timer.Get();
    fprintf(stderr, "DiscardBadImpulseResponses2 - t: %ld\n", t);
#endif
  
    return res;
}

// Apply linear fades to the beginning and the end of the response
//
// Decim factor is used for example for response on 1s.
// If we choosed directly 64 samples, this cut the bugger part
// of the responses
void
ImpulseResponseSet::MakeFades(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                              long maxSize, BL_FLOAT decimFactor)
{
    if (impulseResponse->GetSize() == 0)
        return;
  
    int numDiscardSamples = impulseResponse->GetSize() - maxSize;
  
    int fadeInSizeSamples = FADE_IN_SIZE_SAMPLES/decimFactor;
    //int fadeOutSizeSamples = FADE_OUT_SIZE_SAMPLES/decimFactor;
    int fadeOutSizeSamples = maxSize*FADE_OUT_SIZE_RATIO;
  
    // Fade in
    for (int i = 0; i < fadeInSizeSamples; i++)
    {
        BL_FLOAT coeff = ((BL_FLOAT)i)/(fadeInSizeSamples - 1);
    
        if (i >= impulseResponse->GetSize())
            break;
    
        impulseResponse->Get()[i] *= coeff;
    }
  
    // Fade out
    int totalFadeSize = fadeOutSizeSamples + numDiscardSamples;
    for (int i = 0; i < totalFadeSize; i++)
    {
        BL_FLOAT coeff = 1.0 - ((BL_FLOAT)i)/(fadeOutSizeSamples - 1);
        if (coeff < 0.0)
            coeff = 0.0;
    
        int index = impulseResponse->GetSize() - totalFadeSize + i;
        if (index >= impulseResponse->GetSize())
            break;
    
        impulseResponse->Get()[index] *= coeff;
    }
}

void
ImpulseResponseSet::NormalizeImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse)
{
    BL_FLOAT maxVal = GetMax(*impulseResponse);
  
    if (maxVal <= 0.0)
        return;
  
    BL_FLOAT coeff = 1.0/maxVal;
  
    MultImpulseResponse(impulseResponse, coeff);
}

void
ImpulseResponseSet::NormalizeImpulseResponse2(WDL_TypedBuf<BL_FLOAT> *impulseResponse0,
                                              WDL_TypedBuf<BL_FLOAT> *impulseResponse1)
{
    BL_FLOAT maxVal0 = GetMax(*impulseResponse0);
    BL_FLOAT maxVal1 = GetMax(*impulseResponse1);
  
    if (maxVal0 <= 0.0)
        return;
  
#if !FIX_NORMALIZE
    // If we have mono IR, the second IR will be 0,
    // and we won't normalize...
    if (maxVal1 <= 0.0)
        return;
#endif
  
    BL_FLOAT maxVal = (maxVal0 > maxVal1) ? maxVal0 : maxVal1;
  
    BL_FLOAT coeff = 1.0/maxVal;
  
    MultImpulseResponse(impulseResponse0, coeff);
    MultImpulseResponse(impulseResponse1, coeff);
}

void
ImpulseResponseSet::DenoiseImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse)
{
    // Set the tail to 0 as long it is only noise (hiss)
    // And make the transition with a fade out
    //
    // Otherwise, if we keep a long tail with hiss, the resulting sound will continue
    // indefinitely (e.g one second of sine will continue to sound (lightly) for 10 seconds)
  
#define NOISE_THRESHOLD_DB -54.0
  
    // About one second
#define WINDOW_SIZE 44100
  
    int respSize = impulseResponse->GetSize();
    if (respSize < WINDOW_SIZE)
        // Short response, do not remove noise
        return;
  
    const BL_FLOAT noiseThreshold = DBToAmp(NOISE_THRESHOLD_DB);
  
    BL_FLOAT winAvg = BLUtils::ComputeAbsAvg(*impulseResponse,
                                           respSize - 1 - WINDOW_SIZE,
                                           respSize - 1);
  
    BL_FLOAT lastIndex = 0;
  
    // Start from the end, and stop as soon as we get above the threshold
    for (int i = respSize - 1 - WINDOW_SIZE; i >= 0; i--)
    {
        if (winAvg > noiseThreshold)
        {
            lastIndex = i;
      
            break;
        }
    
        int id0 = i;
        if (id0 < 0)
        {
            lastIndex = 0;
      
            break;
        }
    
        int id1 = i + WINDOW_SIZE;
    
        BL_FLOAT val0 = impulseResponse->Get()[id0];
        BL_FLOAT val1 = impulseResponse->Get()[id1];
    
        val0 = fabs(val0);
        val1 = fabs(val1);
    
        BL_FLOAT sum = winAvg*WINDOW_SIZE;
        sum -= val1;
        sum += val0;
    
        winAvg = sum/WINDOW_SIZE;
    }
  
    if ((lastIndex > 0) && (lastIndex < respSize - 1))
    {
        // Make fade out and then zero the noisy tail
        BL_FLOAT fadeStart = ((BL_FLOAT)lastIndex)/respSize;
        BL_FLOAT fadeEnd = ((BL_FLOAT)lastIndex + WINDOW_SIZE)/respSize;
        bool fadeIn = false;
    
        BLUtilsFade::Fade(impulseResponse, fadeStart, fadeEnd, fadeIn);
    }
}


BL_FLOAT
ImpulseResponseSet::GetMax(const WDL_TypedBuf<BL_FLOAT> &buf)
{
    BL_FLOAT maxVal = 0.0;
    for (int i = 0; i < buf.GetSize(); i++)
    {
        BL_FLOAT val = buf.Get()[i];
    
        val = fabs(val);
        if (val > maxVal)
            maxVal = val;
    }
  
    return maxVal;
}

void
ImpulseResponseSet::MultImpulseResponse(WDL_TypedBuf<BL_FLOAT> *buf, BL_FLOAT val)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        BL_FLOAT bufVal = buf->Get()[i];
    
        bufVal *= val;
    
        buf->Get()[i] = bufVal;
    }
}

void
ImpulseResponseSet::AlignImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                         long indexForMax)
{
    // Align
    long maxIndex = FindMaxValIndex(*impulseResponse);
    
    if (maxIndex < indexForMax)
    {
        WDL_TypedBuf<BL_FLOAT> newResponse;
      
        int numZeros = indexForMax - maxIndex;
        if (numZeros < 0)
            numZeros = 0;
    
        if (numZeros > impulseResponse->GetSize())
            numZeros = impulseResponse->GetSize();
    
        // Shift right and pad with zeros
        for (int i = 0; i < numZeros; i++)
            newResponse.Add(0.0);
    
        int size = impulseResponse->GetSize() - numZeros;
        if (size < 0)
            size = 0;
    
        for (int i = 0; i < size; i++)
        {
            BL_FLOAT val = impulseResponse->Get()[i];
            newResponse.Add(val);
        }
      
        *impulseResponse = newResponse;
    }
    else
        if (maxIndex > indexForMax)
        {
            WDL_TypedBuf<BL_FLOAT> newResponse;

            int numZeros = maxIndex - indexForMax;
            if (numZeros < 0)
                numZeros = 0;
    
            // Shift left and pad with zeros
            for (int i = numZeros; i < impulseResponse->GetSize(); i++)
            {
                BL_FLOAT val = impulseResponse->Get()[i];
                newResponse.Add(val);
            }
    
            for (int i = 0; i < numZeros; i++)
                newResponse.Add(0.0);
      
            *impulseResponse = newResponse;
        }
}

void
ImpulseResponseSet::AlignImpulseResponse2(WDL_TypedBuf<BL_FLOAT> *impulseResponse0,
                                          WDL_TypedBuf<BL_FLOAT> *impulseResponse1,
                                          long indexForMax)
{
    // indexForMax will be the index for the first response
  
    int maxIndex0 = FindMaxValIndex(*impulseResponse0);
    int maxIndex1 = FindMaxValIndex(*impulseResponse1);
  
    int diffIndex = maxIndex1 - maxIndex0;
  
    AlignImpulseResponse(impulseResponse0, indexForMax);
    AlignImpulseResponse(impulseResponse1, indexForMax + diffIndex);
}

void
ImpulseResponseSet::DiscardBadImpulseResponses(vector<long> *respToDiscard,
                                               long discardMaxSamples)
{  
    // previous one the worked not so bad
    // Never discard, with that...
    //#define SIGMA_DISCARD_RESP 0.05
  
    // More accurate new one
#define SIGMA_DISCARD_RESP 0.02
  
#define MIN_RESP_DISCARD 3 // 5 ?
  
    // Initial version, not logical but works
#define USE_MAX 1
  
    // Discard only one value at the maximum each time
    // Otherwise, we have the risk to discard all the values in one pass...

    // Start when we have at the minimum 3 responses
    // (otherwise it means nothing)
    if (mResponses.size() < MIN_RESP_DISCARD)
        return;
  
    WDL_TypedBuf<BL_FLOAT> avgResponse;
    GetAvgImpulseResponse(&avgResponse);
  
#if USE_MAX
    // Find the max different impulse response
    BL_FLOAT maxSigma = 0.0;
    long maxIndex = -1;
#endif
  
    respToDiscard->clear();
  
    for (int i = 0; i < mResponses.size(); i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &resp = mResponses[i];
    
        // Sigma is in [0, 1]
        BL_FLOAT sigma = ComputeSigma(resp, avgResponse, discardMaxSamples);
    
#if USE_MAX
        if (sigma > maxSigma)
        {
            maxSigma = sigma;
            maxIndex = i;
        }
#else
        if (sigma > SIGMA_DISCARD_RESP)
            // Error
            respToDiscard->push_back(i);
#endif
    }
  
#if USE_MAX
    // check if sigma is too different
    if (maxSigma > SIGMA_DISCARD_RESP)
        // Error
        respToDiscard->push_back(maxIndex);
#endif
}

// (More efficient than previous method)
void
ImpulseResponseSet::RemoveResponses(const vector<long> &indicesToRemove)
{
    if (indicesToRemove.size() == mResponses.size())
        // Remove all !
    {
        mResponses.clear();
    
        return;
    }
  
    vector<long> indicesToRemoveSorted = indicesToRemove;
    sort(indicesToRemoveSorted.begin(), indicesToRemoveSorted.end());
    reverse(indicesToRemoveSorted.begin(), indicesToRemoveSorted.end());
  
    // Now, the indices are sorted from the bigger to the smaller
    // Erase from the last to the first
    // (to avoid shifts) 
    for (int i = 0; i < indicesToRemoveSorted.size(); i++)
    {
        int index = indicesToRemoveSorted[i];
    
        mResponses.erase(mResponses.begin() + index);
    }
}

long
ImpulseResponseSet::FindMaxValIndex(const WDL_TypedBuf<BL_FLOAT> &buf,
                                    int maxIndexSearch)
{
    int maxLengthSearch = (maxIndexSearch == -1) ? buf.GetSize() : maxIndexSearch;
    if (maxLengthSearch > buf.GetSize())
        maxLengthSearch = buf.GetSize();
 
    // Compute max position
    long maxIndex = 0;
    BL_FLOAT maxVal = 0.0;
  
    for (int i = 0; i < maxLengthSearch; i++)
    {
        if (i >= buf.GetSize())
            break;
    
        BL_FLOAT val = buf.Get()[i];
        val = fabs(val);
  
        if (val > maxVal)
        {
            maxVal = val;
    
            maxIndex = i;
        }
    }
  
    return maxIndex;
}

BL_FLOAT
ImpulseResponseSet::ComputeSigma(const WDL_TypedBuf<BL_FLOAT> &impulseResponse,
                                 const WDL_TypedBuf<BL_FLOAT> &avgResponse,
                                 long maxSamples)
{
    // In case of big responses, step will be > 1
    int step = impulseResponse.GetSize()/maxSamples;
  
    if (step == 0)
        // The response size is smaller than the maximum
        step = 1;
  
    BL_FLOAT sigma = 0.0;
    long numValues = 0;
  
    for (int i = 0; i < impulseResponse.GetSize(); i += step)
    {
        BL_FLOAT samp = impulseResponse.Get()[i];
        BL_FLOAT avg = avgResponse.Get()[i];
    
        BL_FLOAT diff = fabs(samp - avg);
    
        sigma += diff;
    
        numValues++;
    }
  
    if (numValues > 0)
        sigma /= numValues;
  
    return sigma;
}

bool
ImpulseResponseSet::CheckLastRespValid(const ImpulseResponseSet *respSet,
                                       const vector<long> &indicesToDiscard)
{
    // Check if we remove the last added response
    bool lastRespValid = true;
    for (int i = 0; i < indicesToDiscard.size(); i++)
    {
        int idx = indicesToDiscard[i];
        if (idx == respSet->GetSize() - 1)
        {
            lastRespValid = false;
      
            break;
        }
    }
  
    return lastRespValid;
}
