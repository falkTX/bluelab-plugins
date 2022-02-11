//
//  CMA2Smoother.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#include "CMA2Smoother.h"

CMA2Smoother::CMA2Smoother(int bufferSize, int windowSize)
: mSmoother0(bufferSize, windowSize),
mSmoother1(bufferSize, windowSize) {}

CMA2Smoother::~CMA2Smoother() {}


bool
CMA2Smoother::Process(const double *data, double *smoothedData, int nFrames)
{
    WDL_TypedBuf<double> tmpData;
    tmpData.Resize(nFrames);
    
#if 1
    bool processed = mSmoother0.Process(data, tmpData.Get(), nFrames);
    if (processed)
        processed = mSmoother1.Process(tmpData.Get(), smoothedData, nFrames);
#endif
    
#if 0 // DEBUG
    bool processed = mSmoother0.Process(data, smoothedData, nFrames);
#endif
    
    return processed;
}

bool
CMA2Smoother::ProcessOne(const double *data, double *smoothedData, int nFrames, int windowSize)
{
    WDL_TypedBuf<double> tmpData;
    tmpData.Resize(nFrames);
    
    bool processed = CMASmoother::ProcessOne(data, tmpData.Get(), nFrames, windowSize);
    if (processed)
        processed = CMASmoother::ProcessOne(tmpData.Get(), smoothedData, nFrames, windowSize);
    
    return processed;
}

void
CMA2Smoother::Reset()
{
    mSmoother0.Reset();
    mSmoother1.Reset();
}
