//
//  CMASmoother.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#include "CMASmoother.h"

#include <BLUtils.h>

// We gain 8% perfs
// Validated!
#define DENOISER_OPTIM0 1


CMASmoother::CMASmoother(int bufferSize, int windowSize)
{
    mBufferSize = bufferSize;
    mWindowSize = windowSize;
    mFirstTime = true;
    mPrevVal = 0.0;
}

CMASmoother::~CMASmoother() {}

void
CMASmoother::Reset()
{
    mInData.Resize(0);
    mOutData.Resize(0);
    
    mFirstTime = true;
    
    mPrevVal = 0.0;
}

void
CMASmoother::Reset(int bufferSize, int windowSize)
{
    mBufferSize = bufferSize;
    mWindowSize = windowSize;
    
    mInData.Resize(0);
    mOutData.Resize(0);
    
    mFirstTime = true;
    
    mPrevVal = 0.0;
}

bool
CMASmoother::Process(const BL_FLOAT *data, BL_FLOAT *smoothedData, int nFrames)
{
    if (mFirstTime)
        // Process one time with zeros (for continuity after).
        // Consider we have some silence before, then process it for dummy
        // Then at the next buffer, we will have the correct state, with the correct mPrevVal etc.
    {
        mFirstTime = false;
        
        WDL_TypedBuf<BL_FLOAT> zeros;
        zeros.Resize(nFrames);
        for (int i = 0; i < nFrames; i++)
            zeros.Get()[i] = 0.0;
        
        WDL_TypedBuf<BL_FLOAT> smoothed;
        smoothed.Resize(nFrames);
        
        ProcessInternal(zeros.Get(), smoothed.Get(), nFrames);
    }
    
    // Normal processing
    bool processed = ProcessInternal(data, smoothedData, nFrames);
    
    return processed;
}

bool
CMASmoother::ProcessInternal(const BL_FLOAT *data, BL_FLOAT *smoothedData, int nFrames)
{
    ManageConstantValues(data, nFrames);
    
    mInData.Add(data, nFrames);
    
    WDL_TypedBuf<BL_FLOAT> outData;
    bool processed = CentralMovingAverage(mInData, outData, mWindowSize);
    if (processed)
    {
        // Add out data
        mOutData.Add(outData.Get(), outData.GetSize());
        
        int outDataSize = outData.GetSize();
        int inDataSize = mInData.GetSize();
        
        // Consume in data
        WDL_TypedBuf<BL_FLOAT> newInData;
        newInData.Add(&mInData.Get()[outDataSize], inDataSize - outDataSize);
        mInData = newInData;
    
        if (outDataSize >= nFrames)
        {
            // Return out data
            memcpy(smoothedData, mOutData.Get(), nFrames*sizeof(BL_FLOAT));
        
            // Consume out data
            WDL_TypedBuf<BL_FLOAT> newOutData;
            newOutData.Add(&mOutData.Get()[nFrames], outDataSize - nFrames);
            mOutData = newOutData;
        
            return true;
        }
    }
    
    return false;
}

#if !DENOISER_OPTIM0
bool
CMASmoother::ProcessOne(const BL_FLOAT *data, BL_FLOAT *smoothedData, int nFrames, int windowSize)
{
    if (nFrames < windowSize)
        return false;
    
    WDL_TypedBuf<BL_FLOAT> inData;
    
    // First, fill with zeros
    for (int i = 0; i < windowSize; i++)
        inData.Add(0.0);
    
    // Copy mirrored data at the beginning and at the end to avoid zero
    // values at the boudaries, after smoothing
    
    // Copy mirrored data at the beginning
    for (int i = 0; i < windowSize; i++)
    {
        //BL_FLOAT val = data[windowSize/2 - i];
        
        // Try a fix (12/09/2017) => Worked well for EqHack
        BL_FLOAT val = data[windowSize - i];
        
        inData.Add(val);
    }
    
    inData.Add(data, nFrames);
    
    // Copy mirrored data at the end
    for (int i = 0; i < windowSize; i++)
    {
        BL_FLOAT val = data[nFrames - i - 1];
        
        inData.Add(val);
    }
    
    WDL_TypedBuf<BL_FLOAT> outData;
    
    // Centered moving average
    BL_FLOAT prevVal = 0.0;
    for (int i = windowSize/2; i < inData.GetSize() - windowSize/2; i++)
    {
        BL_FLOAT xn = prevVal + (inData.Get()[i + windowSize/2] - inData.Get()[i - windowSize/2])/windowSize;
        
        outData.Add(xn);
        
        prevVal = xn;
    }
    
    if (outData.GetSize() < nFrames)
        return false;
    
    // Get data from index "windowSize", this is where it begins to be valid
    // Because we add extra data at the beginning
    memcpy(smoothedData, &outData.Get()[windowSize + windowSize/2], nFrames*sizeof(BL_FLOAT));
    
    return true;
}
#else

// NOTE: does not optimize if we keep allocated previous buffers
// inData and outData
//template <typename FLOAT_TYPE>
bool
CMASmoother::ProcessOne(const BL_FLOAT *data, BL_FLOAT *smoothedData,
                        int nFrames, int windowSize)
{
    if (nFrames < windowSize)
        return false;
    
    WDL_TypedBuf<BL_FLOAT> &inData = mTmpBuf0;
    
    // First, fill with zeros
    int inDataFullSize = 3*windowSize + nFrames;
    
    if (inData.GetSize() != inDataFullSize)
        inData.Resize(inDataFullSize);
    
    BLUtils::FillZero(&inData, windowSize);
    
    // Copy mirrored data at the beginning and at the end to avoid zero
    // values at the bnoudaries, after smoothing
    
    // Copy mirrored data at the beginning
    int prevSize0 = windowSize;
    BL_FLOAT *inDataBuf = inData.Get();
    for (int i = 0; i < windowSize; i++)
    {
        // Try a fix (12/09/2017) => Worked well for EqHack
        BL_FLOAT val = data[windowSize - i];
        
        inDataBuf[prevSize0 + i] = val;
    }
    
    int prevSize1 = 2*windowSize;
    
    BLUtils::CopyBuf(&inData.Get()[prevSize1], data, nFrames);
    
    int prevSize2 = 2*windowSize + nFrames;
    
    /*BL_FLOAT * */inDataBuf = inData.Get();
    for (int i = 0; i < windowSize; i++)
    {
        BL_FLOAT val = data[nFrames - i - 1];
        
        inDataBuf[prevSize2 + i] = val;
    }
    
    WDL_TypedBuf<BL_FLOAT> &outData = mTmpBuf1;
    int outDataSize = inDataFullSize - 2*(windowSize/2); // FIX for odd window size
    if (outData.GetSize() != outDataSize)
        outData.Resize(outDataSize);
    
    int halfWindowSize = windowSize/2;
    BL_FLOAT windowSizeInv = 1.0/windowSize;
    
    int outStep = 0;
    
    // Centered moving average
    BL_FLOAT prevVal = 0.0;
    BL_FLOAT *outDataBuf = outData.Get();
    for (int i = halfWindowSize; i < inData.GetSize() - halfWindowSize; i++)
    {
        BL_FLOAT xn = prevVal + (inDataBuf[i + halfWindowSize] -
                               inDataBuf[i - halfWindowSize])*windowSizeInv;/*/windowSize;*/
        
        outDataBuf[outStep++] = xn;
        
        prevVal = xn;
    }
    
    if (outData.GetSize() < nFrames)
        return false;
    
    // Get data from index "windowSize", this is where it begins to be valid
    // Because we add extra data at the beginning
    memcpy(smoothedData,
           &outData.Get()[windowSize + halfWindowSize],
           nFrames*sizeof(BL_FLOAT));
    
    return true;
}
//template bool CMASmoother::ProcessOne(const float *data, float *smoothedData, int nFrames, int windowSize);
//template bool CMASmoother::ProcessOne(const double *data, double *smoothedData, int nFrames, int windowSize);
#endif

bool
CMASmoother::CentralMovingAverage(WDL_TypedBuf<BL_FLOAT> &inData, WDL_TypedBuf<BL_FLOAT> &outData, int windowSize)
{
    if (inData.GetSize() < windowSize)
    {
        return false;
    }
    
    // Centered moving average
    for (int i = windowSize/2; i < inData.GetSize() - windowSize/2; i++)
    {
        BL_FLOAT xn = mPrevVal + (inData.Get()[i + windowSize/2] - inData.Get()[i - windowSize/2])/windowSize;
        
        outData.Add(xn);
        
        mPrevVal = xn;
    }
    
    return true;
}

void
CMASmoother::ManageConstantValues(const BL_FLOAT *data, int nFrames)
{
    if (nFrames == 0)
        return;
    
    BL_FLOAT firstValue = data[0];
    
    bool constantValue = true;
    for (int i = 1; (i < nFrames) && constantValue; i++)
    {
        constantValue = (data[i] == firstValue);
    }
    
    if (constantValue)
        mPrevVal = (mPrevVal + firstValue)/2.0;
}
