//
//  SamplesPyramid3.cpp
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#include <BLUtils.h>
#include <BLUtilsDecim.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "SamplesPyramid3.h"

// FIX: Reaper, buffer size 447: when in capture mode,
// the right part of the waveform becomes glitchy
// To void this, bufferize input and output, to push
// and pop buffer of size power of two
#define PUSH_POP_POW_TWO 1

// FIX: UST, clipper display: waveform jittered one time just after
// we had scrolled exactly one time (one full "band")
// (improvement of PUSH_POP_POW_TWO)
#define FIX_PUSH_POP_POW_TWO 1

// Num samples to overlap at level 0,
// to avoid discontinuities
// (will decrease when going deeper in the pyramid)
//#define NUM_SAMPLES_OVERLAP 0 // ORIGIN

// UST => avoids holes when testing with white noise
// NOTE: crashes Ghost, in capture mode
//#define NUM_SAMPLES_OVERLAP 512

// For Ghost, without crash
#define NUM_SAMPLES_OVERLAP 0

// NOTE: in case of problem of jittering or other, thing about
// buffering in the calling code
// (to give constant size power of two buffers to SamplesPyramid3


// WDL_TypedFastQueue doesn't like to be in a vector and resized
#define FIX_VEC_FAST_QUEUE_RESIZE 1

// FIX: Ghost, big zoom on the right part of the waveform
// => after a given zoom level, the waveform stops zooming
#define FIX_BIG_ZOOM 1 //0 //1

// Good (just some slight dark lines, due to overlay 
#define DECIMATE_SAMPLES_FUNC BLUtilsDecim::DecimateSamples3

// Not so good (many dark lines, due to missing maxima)
//#define DECIMATE_SAMPLES_FUNC BLUtilsDecim::DecimateSamples4


SamplesPyramid3::SamplesPyramid3(int maxLevel)
{
    mMaxPyramidLevel = maxLevel;
    
#if FIX_VEC_FAST_QUEUE_RESIZE
    mSamplesPyramid.resize(mMaxPyramidLevel + 1);
#endif

    ResetTmpBuffers();
}

SamplesPyramid3::~SamplesPyramid3() {}

void
SamplesPyramid3::Reset()
{
#if PUSH_POP_POW_TWO
    //mPushBuf.Resize(0);
    mPushBuf.Clear();
    
    mRemainToPop = 0;
#endif
    
    mSamplesPyramid.clear();

#if FIX_VEC_FAST_QUEUE_RESIZE
    mSamplesPyramid.resize(mMaxPyramidLevel + 1);
#endif

    ResetTmpBuffers();
}

void
SamplesPyramid3::SetValues(const WDL_TypedBuf<BL_FLOAT> &samples0)
{    
#if PUSH_POP_POW_TWO
    mPushBuf.Clear();
    
    mRemainToPop = 0;
#endif

    // Compute the size
    int pyrLevel = 0;
    int pyrLevelSize = samples0.GetSize();
    while(true)
    {
        if (pyrLevelSize < 2)
            break;

        if (pyrLevel >= mMaxPyramidLevel)
            break;
        
        pyrLevel++;
        
        pyrLevelSize /= 2;
    }

#if !FIX_VEC_FAST_QUEUE_RESIZE
    // NOTE: maybe this is not exactly the same level as before FastQueue
    // (maybe +1 or -1)
    mSamplesPyramid.resize(pyrLevel + 1);
#endif

    WDL_TypedBuf<SP_FLOAT> samples;
    BLUtils::ConvertToFloat(&samples, samples0);
        
    // Level0
    BLUtils::BufToFastQueue(samples, &mSamplesPyramid[0]);
    
    int pyramidLevel = 1;
    while(true)
    {        
        //WDL_TypedBuf<BL_FLOAT> &prevLevel = mTmpBuf7[pyramidLevel];
        WDL_TypedBuf<SP_FLOAT> prevLevel;
        
        BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel - 1], &prevLevel);
        
        if (prevLevel.GetSize() < 2)
            break;

        if (pyramidLevel > mMaxPyramidLevel)
            break;
        
        //WDL_TypedBuf<BL_FLOAT> &newLevel = mTmpBuf8[pyramidLevel];
        WDL_TypedBuf<SP_FLOAT> newLevel;
        DECIMATE_SAMPLES_FUNC(&newLevel, prevLevel, (SP_FLOAT)0.5);
        
        BLUtils::BufToFastQueue(newLevel, &mSamplesPyramid[pyramidLevel]);
        
        if (pyramidLevel >= mMaxPyramidLevel)
            break;

        pyramidLevel++;
    }

    // DEBUG
    //DBG_DisplayMemory();
}

void
SamplesPyramid3::PushValues(const WDL_TypedBuf<BL_FLOAT> &inSamples)
{
    WDL_TypedBuf<SP_FLOAT> samples;
    BLUtils::ConvertToFloat(&samples, inSamples);
    
#if PUSH_POP_POW_TWO
    // Bufferize pushed buffers to manage only power of two buffers
    // Otherwise there will be artefacts in the waveform
    // (Ghost-X, capture mode, Reaper, buffer size 447)
    //
    mPushBuf.Add(samples.Get(), samples.GetSize());
    
    int numToAdd = mPushBuf.Available();
    numToAdd = BLUtilsMath::NextPowerOfTwo(numToAdd);
    
#if !FIX_PUSH_POP_POW_TWO
    numToAdd /= 2;
#else
    // Check if we were exactly power of two at the beginning
    // And divide only if were not
    if (numToAdd > mPushBuf.Available())
        numToAdd /= 2;
#endif

    WDL_TypedBuf<SP_FLOAT> &samples0 = mTmpBuf6;
    BLUtils::FastQueueToBuf(mPushBuf, &samples0, numToAdd);

    mPushBuf.Advance(numToAdd);
#else
    WDL_TypedBuf<SP_FLOAT> &samples0 = mTmpBuf6;
    samples0 = samples;
#endif

#if !FIX_VEC_FAST_QUEUE_RESIZE
    if (mSamplesPyramid.empty())
        // First time, create pyramid
        mSamplesPyramid.resize(1);
#endif
    
    mSamplesPyramid[0].Add(samples0.Get(), samples0.GetSize());
    
    int pyramidLevel = 0;
    
#if !FIX_GLITCH
    int numSamplesOverlap = NUM_SAMPLES_OVERLAP;
    int numSamplesAdd = samples0.GetSize();
#else
    BL_FLOAT numSamplesOverlap = NUM_SAMPLES_OVERLAP;
    BL_FLOAT numSamplesAdd = samples0.GetSize();
#endif
    
    while(true)
    {
        if (mSamplesPyramid[pyramidLevel].Available() < 2)
            // Top of the pyramid
            break;
        
        pyramidLevel++;

        // 
        if (pyramidLevel + 1 > mMaxPyramidLevel)
            break;

#if !FIX_VEC_FAST_QUEUE_RESIZE
        // Must go up
        if (mSamplesPyramid.size() < pyramidLevel + 1)
            // Grow the pyramid if necessary
            mSamplesPyramid.resize(pyramidLevel + 1);
#endif
        
        WDL_TypedBuf<SP_FLOAT> &currentLevel = mTmpBuf2[pyramidLevel];
        
        BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel - 1], &currentLevel);
        
        // Take twice the size of the input buffer, to avoid discontinuities
        int start = currentLevel.GetSize() - numSamplesAdd - numSamplesOverlap;
        if (start < 0)
            start = 0;
        int end = currentLevel.GetSize();
        int size = end - start;

        WDL_TypedBuf<SP_FLOAT> &samplesCurrentLevel = mTmpBuf3[pyramidLevel];
        BLUtils::SetBufResize(&samplesCurrentLevel, currentLevel, start, size);
        
        WDL_TypedBuf<SP_FLOAT> &samplesNextLevel = mTmpBuf4[pyramidLevel];
        DECIMATE_SAMPLES_FUNC(&samplesNextLevel,
                              samplesCurrentLevel, (SP_FLOAT)0.5);
                                       
        numSamplesOverlap /= 2;
        
        // Take only the second half of the buffer
        // (because we previously tooke twice the buffer size)
        WDL_TypedBuf<SP_FLOAT> &samplesNextLevel0 = mTmpBuf5[pyramidLevel];

        BLUtils::ConsumeLeft(samplesNextLevel, &samplesNextLevel0, numSamplesOverlap);
        
        mSamplesPyramid[pyramidLevel].Add(samplesNextLevel0.Get(),
                                          samplesNextLevel0.GetSize());
                                          
        numSamplesAdd /= 2;
        
        if (pyramidLevel > mMaxPyramidLevel)
            break;
    }
}

void
SamplesPyramid3::PopValues(long numSamples)
{
#if PUSH_POP_POW_TWO
    // Because we bufferize when pushing, we must
    // bufferize when poping too, to avoid shifting
    // (Otherwise in Ghost-X view mode as plugins, the waveform
    // will not scroll correclly).
    long numSamples0 = numSamples + mRemainToPop;
    
    numSamples0 = BLUtilsMath::NextPowerOfTwo((int)numSamples0);
    
#if !FIX_PUSH_POP_POW_TWO
    numSamples0 /= 2;
#else
    // Check if we were exactly power of two at the beginning
    // And divide only if were not
    if (numSamples0 > numSamples + mRemainToPop)
        numSamples0 /= 2;
#endif
    
    if (mSamplesPyramid.empty() ||
        (mSamplesPyramid[0].Available() < numSamples0))
    {
        mRemainToPop += numSamples;
        
        return;
    }
    
    mRemainToPop += numSamples - numSamples0;
    
    numSamples = numSamples0;
#endif
    
    // NOTE: not sur BL_FLOAT type is really useful
    BL_FLOAT numSamplesD = numSamples;
    
    for (int i = 0; i < mSamplesPyramid.size(); i++)
    {
        BLUtils::ConsumeLeft(&mSamplesPyramid[i], numSamplesD);
        
        numSamplesD *= 0.5;
        
        if (mSamplesPyramid[i].Available() < 2)
        {
#if !FIX_VEC_FAST_QUEUE_RESIZE
            // NOTE: a bit strange to resize inside the loop over mSamplesPyramid
            
            // Throw away the top of the pyramid
            // (useless because we have less data)
            mSamplesPyramid.resize(i + 1);
#endif
            
            break;
        }
    }
}

void
SamplesPyramid3::ReplaceValues(long start, const WDL_TypedBuf<BL_FLOAT> &samples)
{
    int pyramidLevel = 0;
    
    //WDL_TypedBuf<BL_FLOAT> samples0 = samples;
    WDL_TypedBuf<SP_FLOAT> samples0;
    BLUtils::ConvertToFloat(&samples0, samples);
    
    SP_FLOAT startD = start;
    
    // Iterate over the pyramid levels and replace
    while(true)
    {
        if (pyramidLevel >= mSamplesPyramid.size())
        {
            break;
        }
        
        // Replace
        BLUtils::Replace(&mSamplesPyramid[pyramidLevel], startD, samples0);
        
        WDL_TypedBuf<SP_FLOAT> tmp = samples0;
        DECIMATE_SAMPLES_FUNC(&samples0, tmp, (SP_FLOAT)0.5);
        
        startD /= 2.0;
        pyramidLevel++;
    }
}

void
SamplesPyramid3::GetValues(BL_FLOAT start, BL_FLOAT end, long numValues,
                           WDL_TypedBuf<BL_FLOAT> *samples)

{
    // First, check the bounds
    // and maybe return early
    if (!mSamplesPyramid.empty())
    {
        int numTotalSamples = mSamplesPyramid[0].Available();
            
            // Check if we are totally out of bounds
        if (((start <= 0.0) && (end <= 0.0)) ||
            ((start >= numTotalSamples) && (end >= numTotalSamples)))
            // Everything out of view
        {
            // Simply return a buffer of zeros
            // (so the waveform won't disappear,
            // but will be displayed as a simple line)
            
            samples->Resize(numValues);
            BLUtils::FillAllZero(samples);
            
            return;
        }
    }
        
    // Check if we must add zeros at the beginning
    //int numZerosBegin = 0;
    BL_FLOAT numZerosBegin = 0.0;
    if (start < 0)
    {
        numZerosBegin = -start;
        start = 0;
    }
    
    // Check if we must add zeros at the end
    //int numZerosEnd = 0;
    BL_FLOAT numZerosEnd = 0.0;
    if (!mSamplesPyramid.empty() && (end > mSamplesPyramid[0].Available()))
    {
        numZerosEnd = end - mSamplesPyramid[0].Available();
        end = mSamplesPyramid[0].Available();
    }

    // NEW: fixed negative size
    if (end - start < 0)
    {
        samples->Resize(0);
        return;
    }
    
    // Get the data
    int pyramidLevel = 0;
    
    // Find the correct pyramid level
    while(true)
    {
        long size = end - start;
        
        // We must start at the first bigger interval
        if ((pyramidLevel >= mSamplesPyramid.size()) || (size <= numValues))
        {
#if !FIX_BIG_ZOOM
            if (size < numValues)
            {
                // Rewind one step
                pyramidLevel--;
                if (pyramidLevel < 0)
                    pyramidLevel = 0;
            
                start *= 2;
                end *= 2;
            
                numZerosBegin *= 2;
                numZerosEnd *= 2;
            }
#endif       
            break;
        }
        
        start /= 2;
        end /= 2;
        
        numZerosBegin /= 2;
        numZerosEnd /= 2;
        
        pyramidLevel++;

        if (pyramidLevel >= mMaxPyramidLevel)
            break;
    }

#if 0 // Costly for big files
    mTmpBuf0.resize(mMaxPyramidLevel + 1);
    WDL_TypedBuf<SP_FLOAT> &currentPyramidLevel = mTmpBuf0[pyramidLevel];
    // Very costly for long files
    // (will copy the buffer at each step)
    BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel], &currentPyramidLevel);
    
    mTmpBuf1.resize(mMaxPyramidLevel + 1);
    WDL_TypedBuf<SP_FLOAT> &level = mTmpBuf1[pyramidLevel];
    
    // Use ceil to avoid missing last sample...
    //int size = ceil(end - start);
    // ... and add 1, to be very sure to get the last sample in any case
    int size = ceil(end - start) + 1;
    
    // NOTE: crashed here, fixed SetBufResize()
    BLUtils::SetBufResize(&level, currentPyramidLevel, start, size);
#endif

#if 1 // Optimized version (copy only the necessary from queue to buf
    // Use ceil to avoid missing last sample...
    //int size = ceil(end - start);
    // ... and add 1, to be very sure to get the last sample in any case
    int size = ceil(end - start) + 1;

    WDL_TypedBuf<SP_FLOAT> &level = mTmpBuf9;
    BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel], start,
                            &level, size);
#endif
    
    // Add zeros if necessary
    if (numZerosBegin > 0)
    {
        BLUtils::PadZerosLeft(&level, (int)numZerosBegin);
    }
    
    if (numZerosEnd > 0)
    {
        BLUtils::PadZerosRight(&level, (int)numZerosEnd);
    }
    
    // Update the size with the zeros added
    size = level.GetSize();

    // Decimate
    SP_FLOAT decimFactor = ((SP_FLOAT)numValues)/size;

    // Final step decimate only if the level size is really bigger
    // Problem: this last step decimation if done, would make waveform
    // "jitter" while zooming or translating
    // (small peaks popping quickly while zooming).
    //
    // To avoid this, choose a sufficient big max pyramid level
    // (and this will also save resources by avoiding decimating e.g 50000 samples
    // at the end)
    //
    WDL_TypedBuf<SP_FLOAT> samples0;
    samples0.Resize(samples->GetSize());
    if (level.GetSize() > numValues*2)
    {
        DECIMATE_SAMPLES_FUNC(&samples0, level, decimFactor);
    }
    else
    {
        if (level.GetSize() > numValues*0.5)
        {    
            // We don't have too many samples at the and => simple copy
            samples0 = level;
        }
        else
        {
            // We have too few samples
            // Must increase the number of samples,
            // and adjust very accurately to float start and end
            // (otherwise we would have small jitter when full zooming,
            // and translating very progressively)

            // Take the two border values
            BL_FLOAT start0 = floor(start);
            BL_FLOAT end0 = ceil(end);
            BL_FLOAT size0 = end0 - start0;

            // Extract again, with the addtion to left and right values
            // ( ceil() / floor() )
            WDL_TypedBuf<SP_FLOAT> &level0 = mTmpBuf10;
            BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel], start0,
                                    &level0, size0);

            // Must pad again because we just extracted the level
            if (numZerosBegin > 0)
            {
                BLUtils::PadZerosLeft(&level0, (int)numZerosBegin);
            }
            
            if (numZerosEnd > 0)
            {
                BLUtils::PadZerosRight(&level0, (int)numZerosEnd);
            }
            
            BL_FLOAT leftT = start - start0;
            BL_FLOAT rightT = end0 - end;

            // Adjust with zeros positions if necessary
            // NOTE: without this, zoom a lot on begin or end of the signal
            // then translate => it jitters
            leftT -= (numZerosBegin - (int)numZerosBegin);
            rightT -= (numZerosEnd - (int)numZerosEnd);
            
            // Upsample
            UpsampleResult(numValues, &samples0, level0, leftT, rightT);
        }
    }

    BLUtils::ConvertFromFloat(samples, samples0);
}

void
SamplesPyramid3::ResetTmpBuffers()
{
    mTmpBuf2.resize(0);
    mTmpBuf2.resize(mMaxPyramidLevel + 1);
    
    mTmpBuf3.resize(0);
    mTmpBuf3.resize(mMaxPyramidLevel + 1);

    mTmpBuf4.resize(0);
    mTmpBuf4.resize(mMaxPyramidLevel + 1);

    mTmpBuf5.resize(0);
    mTmpBuf5.resize(mMaxPyramidLevel + 1);

    mTmpBuf7.resize(0);
    mTmpBuf7.resize(mMaxPyramidLevel + 1);
    
    mTmpBuf8.resize(0);
    mTmpBuf8.resize(mMaxPyramidLevel + 1);
}

void
SamplesPyramid3::UpsampleResult(int numValues,
                                WDL_TypedBuf<SP_FLOAT> *result,
                                const WDL_TypedBuf<SP_FLOAT> &buffer,
                                SP_FLOAT leftT, SP_FLOAT rightT)
{
    if (buffer.GetSize() < 3)
        return;
    
    // Init with undefined
    result->Resize(numValues);
    BLUtils::FillAllValue(result, (SP_FLOAT)UTILS_VALUE_UNDEFINED);

    // Left value
    SP_FLOAT leftVal = (1.0 - leftT)*buffer.Get()[0] + leftT*buffer.Get()[1];
    result->Get()[0] = leftVal;
    
    // Right value
    SP_FLOAT rightVal =
        rightT*buffer.Get()[buffer.GetSize() - 2] +
        (1.0 - rightT)*buffer.Get()[buffer.GetSize() - 1];
    result->Get()[result->GetSize() - 1] = rightVal;
    
    // Intermediate values
    for (int i = 1; i < buffer.GetSize() - 1; i++)
    {
        SP_FLOAT val = buffer.Get()[i];

        SP_FLOAT t = ((SP_FLOAT)i - leftT)/(buffer.GetSize() - 1 - (leftT + rightT));
        
        int targetIdx = t*result->GetSize();
        if ((targetIdx > 0) && (targetIdx < result->GetSize() - 1))
            result->Get()[targetIdx] = val;
    }
    
    bool extendBounds = false;
    BLUtils::FillMissingValues(result, extendBounds,
                               (SP_FLOAT)UTILS_VALUE_UNDEFINED);
}

void
SamplesPyramid3::DBG_DisplayMemory()
{
    long numSamples = 0;

    for (int i = 0; i < mSamplesPyramid.size(); i++)
    {
        numSamples += mSamplesPyramid[i].Available();
    }

    long numBytes = numSamples*sizeof(SP_FLOAT);
    long numMB = ((double)numBytes)/1000000;

    fprintf(stderr, "SamplePyramid3 - MEM: %ld MB\n", numMB);
}
