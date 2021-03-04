//
//  SamplesPyramid2.cpp
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#include <BLUtils.h>

#include "SamplesPyramid2.h"

// WORKAROUND: limit the maximum pyramid depth
//
// (Tested on long file, and long capture: perfs still ok)
//
// If the level is too high, the waveform gets messy
// (we don't detect well minima and maxima)
#define MAX_PYRAMID_LEVEL 8

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
// (to give constant size power of two buffers to SamplesPyramid2


// WDL_TypedFastQueue doesn't like to be in a vector and resized
#define FIX_VEC_FAST_QUEUE_RESIZE 1

SamplesPyramid2::SamplesPyramid2()
{
#if FIX_VEC_FAST_QUEUE_RESIZE
    mSamplesPyramid.resize(MAX_PYRAMID_LEVEL + 1);
#endif

    ResetTmpBuffers();
}

SamplesPyramid2::~SamplesPyramid2() {}

void
SamplesPyramid2::Reset()
{
#if PUSH_POP_POW_TWO
    //mPushBuf.Resize(0);
    mPushBuf.Clear();
    
    mRemainToPop = 0;
#endif
    
    mSamplesPyramid.clear();

#if FIX_VEC_FAST_QUEUE_RESIZE
    mSamplesPyramid.resize(MAX_PYRAMID_LEVEL + 1);
#endif

    ResetTmpBuffers();
}

// NOTE: this method has changes a lot since FastQueue
void
SamplesPyramid2::SetValues(const WDL_TypedBuf<BL_FLOAT> &samples)
{
#if PUSH_POP_POW_TWO
    //mPushBuf.Resize(0);
    mPushBuf.Clear();
    
    mRemainToPop = 0;
#endif

    // Compute the size
    int pyrLevel = 1; // start at 1, at least 1 level!
    int pyrLevelSize = samples.GetSize();
    while(true)
    {
        pyrLevel++;
        
        if (pyrLevelSize < 2)
            break;

        //if (pyrLevel >= MAX_PYRAMID_LEVEL)
        if (pyrLevel > MAX_PYRAMID_LEVEL)
            break;
        
        pyrLevelSize /= 2;
    }

#if !FIX_VEC_FAST_QUEUE_RESIZE
    // NOTE: maybe this is not exactly the same level as before FastQueue
    // (maybe +1 or -1)
    //mSamplesPyramid.resize(pyrLevelSize);
    mSamplesPyramid.resize(pyrLevel);
#endif
    
    BLUtils::BufToFastQueue(samples, &mSamplesPyramid[0]);
    
    int pyramidLevel = 1;
    while(true)
    {        
        //WDL_TypedBuf<BL_FLOAT> prevLevel;
        WDL_TypedBuf<BL_FLOAT> &prevLevel = mTmpBuf7[pyramidLevel /*- 1*/];
        
        BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel - 1], &prevLevel);
        
        if (prevLevel.GetSize() < 2)
            break;

        // TEST
        //if (pyramidLevel >= MAX_PYRAMID_LEVEL)
        if (pyramidLevel > MAX_PYRAMID_LEVEL)
            break;
        
        //WDL_TypedBuf<BL_FLOAT> newLevel;
        WDL_TypedBuf<BL_FLOAT> &newLevel = mTmpBuf8[pyramidLevel /*- 1*/];
        BLUtils::DecimateSamples(&newLevel, prevLevel, (BL_FLOAT)0.5);
        
        BLUtils::BufToFastQueue(newLevel, &mSamplesPyramid[pyramidLevel]);
        
        if (pyramidLevel >= MAX_PYRAMID_LEVEL)
            break;

        pyramidLevel++;
    }
}

void
SamplesPyramid2::PushValues(const WDL_TypedBuf<BL_FLOAT> &samples)
{
#if PUSH_POP_POW_TWO
    // Bufferize pushed buffers to manage only power of two buffers
    // Otherwise there will be artefacts in the waveform
    // (Ghost-X, capture mode, Reaper, buffer size 447)
    //
    mPushBuf.Add(samples.Get(), samples.GetSize());
    
    //int numToAdd = mPushBuf.GetSize();
    int numToAdd = mPushBuf.Available();
    numToAdd = BLUtils::NextPowerOfTwo(numToAdd);
    
#if !FIX_PUSH_POP_POW_TWO
    numToAdd /= 2;
#else
    // Check if we were exactly power of two at the beginning
    // And divide only if were not
    //if (numToAdd > mPushBuf.GetSize())
    if (numToAdd > mPushBuf.Available())
        numToAdd /= 2;
#endif

    //WDL_TypedBuf<BL_FLOAT> samples0;
    WDL_TypedBuf<BL_FLOAT> &samples0 = mTmpBuf6;

    //samples0.Add(mPushBuf.Get(), numToAdd);
    //BLUtils::FastQueueToBuf(mPushBuf, &samples0);
    BLUtils::FastQueueToBuf(mPushBuf, &samples0, numToAdd);

    //BLUtils::ConsumeLeft(&mPushBuf, numToAdd);
    mPushBuf.Advance(numToAdd);
#else
    //WDL_TypedBuf<BL_FLOAT> samples0;
    WDL_TypedBuf<BL_FLOAT> &samples0 = mTmpBuf6;
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

        // TEST
        if (pyramidLevel + 1 > MAX_PYRAMID_LEVEL)
            break;

#if !FIX_VEC_FAST_QUEUE_RESIZE
        // Must go up
        if (mSamplesPyramid.size() < pyramidLevel + 1)
            // Grow the pyramid if necessary
            mSamplesPyramid.resize(pyramidLevel + 1); // PROBLEM HERE ?
#endif
        
        //WDL_TypedBuf<BL_FLOAT> currentLevel;
        WDL_TypedBuf<BL_FLOAT> &currentLevel = mTmpBuf2[pyramidLevel /*- 1*/];
        
        BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel - 1], &currentLevel);
        
        // Take twice the size of the input buffer, to avoid discontinuities
        int start = currentLevel.GetSize() - numSamplesAdd - numSamplesOverlap;
        if (start < 0)
            start = 0;
        int end = currentLevel.GetSize();
        int size = end - start;

        //WDL_TypedBuf<BL_FLOAT> samplesCurrentLevel;
        //samplesCurrentLevel.Add(&currentLevel.Get()[start], size);

        //WDL_TypedBuf<BL_FLOAT> samplesCurrentLevel;
        WDL_TypedBuf<BL_FLOAT> &samplesCurrentLevel = mTmpBuf3[pyramidLevel /*- 1*/];

        /*samplesCurrentLevel.Resize(size);
          memcpy(samplesCurrentLevel.Get(),
          &currentLevel.Get()[start],
          size*sizeof(BL_FLOAT));*/

        BLUtils::SetBufResize(&samplesCurrentLevel, currentLevel, start, size);
        
        //WDL_TypedBuf<BL_FLOAT> samplesNextLevel;
        WDL_TypedBuf<BL_FLOAT> &samplesNextLevel = mTmpBuf4[pyramidLevel /*- 1*/];
        BLUtils::DecimateSamples(&samplesNextLevel,
                                 samplesCurrentLevel, (BL_FLOAT)0.5);
        
        numSamplesOverlap /= 2;
        
        // Take only the second half of the buffer
        // (because we previously tooke twice the buffer size)
        //WDL_TypedBuf<BL_FLOAT> samplesNextLevel0;
        WDL_TypedBuf<BL_FLOAT> &samplesNextLevel0 = mTmpBuf5[pyramidLevel /*- 1*/];

        //BLUtils::ConsumeLeft(&samplesNextLevel, numSamplesOverlap);
        BLUtils::ConsumeLeft(samplesNextLevel, &samplesNextLevel0, numSamplesOverlap);
        
        //mSamplesPyramid[pyramidLevel].Add(samplesNextLevel.Get(),
        //                                  samplesNextLevel.GetSize());

        mSamplesPyramid[pyramidLevel].Add(samplesNextLevel0.Get(),
                                          samplesNextLevel0.GetSize());
                                          
        numSamplesAdd /= 2;
        
        //if (pyramidLevel >= MAX_PYRAMID_LEVEL)
        if (pyramidLevel > MAX_PYRAMID_LEVEL)
            break;
    }
}

void
SamplesPyramid2::PopValues(long numSamples)
{
#if PUSH_POP_POW_TWO
    // Because we bufferize when pushing, we must
    // bufferize when poping too, to avoid shifting
    // (Otherwise in Ghost-X view mode as plugins, the waveform
    // will not scroll correclly).
    long numSamples0 = numSamples + mRemainToPop;
    
    numSamples0 = BLUtils::NextPowerOfTwo((int)numSamples0);
    
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
SamplesPyramid2::ReplaceValues(long start, const WDL_TypedBuf<BL_FLOAT> &samples)
{
    int pyramidLevel = 0;
    
    WDL_TypedBuf<BL_FLOAT> samples0 = samples;
    BL_FLOAT startD = start;
    
    // Iterate over the pyramid levels and replace
    while(true)
    {
        if (pyramidLevel >= mSamplesPyramid.size())
        {
            break;
        }
        
        // Replace
        BLUtils::Replace(&mSamplesPyramid[pyramidLevel], startD, samples0);
        
        WDL_TypedBuf<BL_FLOAT> tmp = samples0;
        BLUtils::DecimateSamples(&samples0, tmp, (BL_FLOAT)0.5);
        
        startD /= 2.0;
        pyramidLevel++;
    }
}

#if !FIX_GLITCH
void
SamplesPyramid2::GetValues(long start, long end, long numValues,
                          WDL_TypedBuf<BL_FLOAT> *samples)
#else
void
SamplesPyramid2::GetValues(BL_FLOAT start, BL_FLOAT end, long numValues,
                           WDL_TypedBuf<BL_FLOAT> *samples)

#endif
{
    // Check if we must add zeros at the beginning
    int numZerosBegin = 0;
    if (start < 0)
    {
        numZerosBegin = -start;
        start = 0;
    }
    
    // Check if we must add zeros at the end
    int numZerosEnd = 0;
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
        if ((pyramidLevel >= mSamplesPyramid.size()) || (size < numValues))
        {
            pyramidLevel--;
            if (pyramidLevel < 0)
                pyramidLevel = 0;
            
            start *= 2;
            end *= 2;
            
            numZerosBegin *= 2;
            numZerosEnd *= 2;
            
            break;
        }
        
        start /= 2;
        end /= 2;
        
        numZerosBegin /= 2;
        numZerosEnd /= 2;
        
        pyramidLevel++;
    }

    mTmpBuf0.resize(MAX_PYRAMID_LEVEL + 1);

    //WDL_TypedBuf<BL_FLOAT> currentPyramidLevel;
    WDL_TypedBuf<BL_FLOAT> &currentPyramidLevel = mTmpBuf0[pyramidLevel /*- 1*/];
        
    BLUtils::FastQueueToBuf(mSamplesPyramid[pyramidLevel], &currentPyramidLevel);

    mTmpBuf1.resize(MAX_PYRAMID_LEVEL + 1);

    //WDL_TypedBuf<BL_FLOAT> level;
    WDL_TypedBuf<BL_FLOAT> &level = mTmpBuf1[pyramidLevel/* - 1*/];
    
    int size = end - start;
    //level.Add(&currentPyramidLevel.Get()[(long)start], size);

    /*level.Resize(size);
      memcpy(level.Get(),
      &currentPyramidLevel.Get()[(long)start],
      size*sizeof(BL_FLOAT));*/

    // NOTE: crashed here, fixed SetBufResize()
    BLUtils::SetBufResize(&level, currentPyramidLevel, start, size);
                    
    // Add zeros if necessary
    if (numZerosBegin > 0)
    {
        BLUtils::PadZerosLeft(&level, numZerosBegin);
    }
    
    if (numZerosEnd > 0)
    {
        BLUtils::PadZerosRight(&level, numZerosEnd);
    }
    
    // Update the size with the zeros added
    size = level.GetSize();
    
    // Decimate
    BL_FLOAT decimFactor = ((BL_FLOAT)numValues)/size;
    BLUtils::DecimateSamples(samples, level, decimFactor);
}

void
SamplesPyramid2::ResetTmpBuffers()
{
    mTmpBuf2.resize(0);
    mTmpBuf2.resize(MAX_PYRAMID_LEVEL + 1);
    
    mTmpBuf3.resize(0);
    mTmpBuf3.resize(MAX_PYRAMID_LEVEL + 1);

    mTmpBuf4.resize(0);
    mTmpBuf4.resize(MAX_PYRAMID_LEVEL + 1);

    mTmpBuf5.resize(0);
    mTmpBuf5.resize(MAX_PYRAMID_LEVEL + 1);

    mTmpBuf7.resize(0);
    mTmpBuf7.resize(MAX_PYRAMID_LEVEL + 1);
    
    mTmpBuf8.resize(0);
    mTmpBuf8.resize(MAX_PYRAMID_LEVEL + 1);
}
