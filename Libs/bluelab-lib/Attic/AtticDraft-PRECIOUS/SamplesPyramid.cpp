//
//  SamplesPyramid.cpp
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#include <Utils.h>

#include "SamplesPyramid.h"


SamplesPyramid::SamplesPyramid(/*long bufferSize*/)
{
    //mBufferSize = bufferSize;
}

SamplesPyramid::~SamplesPyramid() {}

void
SamplesPyramid::SetValues(const WDL_TypedBuf<double> &samples)
{
    // Clear, in case we previously build the pyramid
    mSamplesPyramid.clear();
    
    // Gen the pyramid
    mSamplesPyramid.resize(1);
    mSamplesPyramid[0] = samples;
    
    while(true)
    {
        int pyramidLevel = mSamplesPyramid.size() /*+ 1*/;
        mSamplesPyramid.resize(pyramidLevel + 1/**/);
        
        const WDL_TypedBuf<double> &prevLevel = mSamplesPyramid[pyramidLevel - 1];
        if (prevLevel.GetSize() < 2/*mBufferSize*/)
            break;
        
#if 0
        for (int i = 0; i < prevLevel.GetSize(); i += mBufferSize)
        {
            int numSamples = (i + mBufferSize < prevLevel.GetSize()) ?
                                    mBufferSize : prevLevel.GetSize() - i;
            
            WDL_TypedBuf<double> prevLevelBuffer;
            prevLevelBuffer.Add(&prevLevel.Get()[i], numSamples);
        }
#endif
        
        Utils::DecimateSamples(&mSamplesPyramid[pyramidLevel], prevLevel, 0.5);
    }
}

void
SamplesPyramid::PushValues(const WDL_TypedBuf<double> &samples)
{
    if (mSamplesPyramid.empty())
        // First time, create pyramid
        mSamplesPyramid.resize(1);
    
    mSamplesPyramid[0].Add(samples.Get(), samples.GetSize());
    
    int pyramidLevel = 0;
    while(true)
    {
        if (mSamplesPyramid[pyramidLevel].GetSize() < 2)
            // Top of the pyramid
            break;
        
        const WDL_TypedBuf<double> &currentLevel = mSamplesPyramid[pyramidLevel];
        
        pyramidLevel++;
        
        // Must go up
        if (mSamplesPyramid.size() < pyramidLevel + 1)
            // Grow the pyramid if necessary
            mSamplesPyramid.resize(pyramidLevel + 1);
        
        // Take twice the size of the input buffer, to avoid discontinuities
        int start = currentLevel.GetSize() - samples.GetSize()*2;
        if (start < 0)
            start = 0;
        int end = currentLevel.GetSize();
        int size = end - start;
        
        WDL_TypedBuf<double> samplesCurrentLevel;
        samplesCurrentLevel.Add(&currentLevel.Get()[start], size);
        
        WDL_TypedBuf<double> samplesNextLevel;
        Utils::DecimateSamples(&samplesNextLevel, samplesCurrentLevel, 0.5);
        
        // Take only the second half of the buffer
        // (because we previously tooke twice the buffer size)
        int nextSize = samplesNextLevel.GetSize();
        Utils::ConsumeLeft(&samplesNextLevel, nextSize/2);
        
        mSamplesPyramid[pyramidLevel].Add(samplesNextLevel.Get(), samplesNextLevel.GetSize());
    }
}

void
SamplesPyramid::PopValues(long numSamples)
{
    for (int i = 0; i < mSamplesPyramid.size(); i++)
    {
        Utils::ConsumeLeft(&mSamplesPyramid[i], numSamples);
        
        numSamples /= 2;
    }
}

void
SamplesPyramid::GetValues(long start, long end, long numValues,
                          WDL_TypedBuf<double> *samples)
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
    if (!mSamplesPyramid.empty() && (end > mSamplesPyramid[0].GetSize()))
    {
        numZerosEnd = end - mSamplesPyramid[0].GetSize();
        end = mSamplesPyramid[0].GetSize();
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
    
    const WDL_TypedBuf<double> &currentPyramidLevel = mSamplesPyramid[pyramidLevel];
    
    WDL_TypedBuf<double> level;
    int size = end - start;
    level.Add(&currentPyramidLevel.Get()[start], size);
    
    double decimFactor = ((double)numValues)/size;
    Utils::DecimateSamples(samples, level, decimFactor);
    
    // Add zeros if necessary
    if (numZerosBegin > 0)
    {
        Utils::PadZerosLeft(samples, numZerosBegin);
    }
    
    if (numZerosEnd > 0)
    {
        Utils::PadZerosLeft(samples, numZerosEnd);
    }
}

