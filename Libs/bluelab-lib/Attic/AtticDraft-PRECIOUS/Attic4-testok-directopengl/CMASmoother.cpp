//
//  CMASmoother.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#include "CMASmoother.h"

#include <Debug.h>

CMASmoother::CMASmoother(int bufferSize, int windowSize)
{
    mBufferSize = bufferSize;
    mWindowSize = windowSize;
    mFirstTime = true;
    mPrevVal = 0.0;
}

CMASmoother::~CMASmoother() {}

bool
CMASmoother::Process(const double *data, double *smoothedData, int nFrames)
{
    if (mFirstTime)
        // Process one time with zeros (for continuity after).
        // Consider we have some silence before, then process it for dummy
        // Then at the next buffer, we will have the correct state, with the correct mPrevVal etc.
    {
        mFirstTime = false;
        
        WDL_TypedBuf<double> zeros;
        zeros.Resize(nFrames);
        for (int i = 0; i < nFrames; i++)
            zeros.Get()[i] = 0.0;
        
        WDL_TypedBuf<double> smoothed;
        smoothed.Resize(nFrames);
        
        ProcessInternal(zeros.Get(), smoothed.Get(), nFrames);
    }
    
    // Normal processing
    bool processed = ProcessInternal(data, smoothedData, nFrames);
    
    return processed;
}

bool
CMASmoother::ProcessInternal(const double *data, double *smoothedData, int nFrames)
{
    ManageConstantValues(data, nFrames);
    
    mInData.Add(data, nFrames);
    
    WDL_TypedBuf<double> outData;
    bool processed = CentralMovingAverage(mInData, outData, mWindowSize);
    if (processed)
    {
        // Add out data
        mOutData.Add(outData.Get(), outData.GetSize());
        
        int outDataSize = outData.GetSize();
        int inDataSize = mInData.GetSize();
        
        // Consume in data
        WDL_TypedBuf<double> newInData;
        newInData.Add(&mInData.Get()[outDataSize], inDataSize - outDataSize);
        mInData = newInData;
    
        if (outDataSize >= nFrames)
        {
            // Return out data
            memcpy(smoothedData, mOutData.Get(), nFrames*sizeof(double));
        
            // Consume out data
            WDL_TypedBuf<double> newOutData;
            newOutData.Add(&mOutData.Get()[nFrames], outDataSize - nFrames);
            mOutData = newOutData;
        
            return true;
        }
    }
    
    return false;
}

void
CMASmoother::Reset()
{
    mInData.Resize(0);
    mOutData.Resize(0);
    
    mFirstTime = true;
    
    mPrevVal = 0.0;
}

bool
CMASmoother::ProcessOne(const double *data, double *smoothedData, int nFrames, int windowSize)
{
    if (nFrames < windowSize)
        return false;
    
    WDL_TypedBuf<double> inData;
    
    // First, fill with zeros
    for (int i = 0; i < windowSize; i++)
        inData.Add(0.0);
    
    // Copy mirrored data at the beginning and at the end to avoid zero
    // values at the boudaries, after smoothing
    
    // Copy mirrored data at the beginning
    for (int i = 0; i < windowSize; i++)
    {
        //double val = data[windowSize/2 - i];
        
        // Try a fix (12/09/2017) => Worked well for EqHack
        double val = data[windowSize - i];
        
        inData.Add(val);
    }
    
    inData.Add(data, nFrames);
    
    // Copy mirrored data at the end
    for (int i = 0; i < windowSize; i++)
    {
        double val = data[nFrames - i - 1];
        
        inData.Add(val);
    }
    
    WDL_TypedBuf<double> outData;
    
    // Centered moving average
    double prevVal = 0.0;
    for (int i = windowSize/2; i < inData.GetSize() - windowSize/2; i++)
    {
        double xn = prevVal + (inData.Get()[i + windowSize/2] - inData.Get()[i - windowSize/2])/windowSize;
        
        outData.Add(xn);
        
        prevVal = xn;
    }
    
    if (outData.GetSize() < nFrames)
        return false;
    
    // Get data from index "windowSize", this is where it begins to be valid
    // Because we add extra data at the beginning
    memcpy(smoothedData, &outData.Get()[windowSize + windowSize/2], nFrames*sizeof(double));
           
    return true;
}

bool
CMASmoother::CentralMovingAverage(WDL_TypedBuf<double> &inData, WDL_TypedBuf<double> &outData, int windowSize)
{
    if (inData.GetSize() < windowSize)
    {
        return false;
    }
    
    // Centered moving average
    for (int i = windowSize/2; i < inData.GetSize() - windowSize/2; i++)
    {
        double xn = mPrevVal + (inData.Get()[i + windowSize/2] - inData.Get()[i - windowSize/2])/windowSize;
        
        outData.Add(xn);
        
        mPrevVal = xn;
    }
    
    return true;
}

void
CMASmoother::ManageConstantValues(const double *data, int nFrames)
{
    if (nFrames == 0)
        return;
    
    double firstValue = data[0];
    
    bool constantValue = true;
    for (int i = 1; (i < nFrames) && constantValue; i++)
    {
        constantValue = (data[i] == firstValue);
    }
    
    if (constantValue)
        mPrevVal = (mPrevVal + firstValue)/2.0;
}
