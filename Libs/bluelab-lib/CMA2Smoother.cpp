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
  mSmoother1(bufferSize, windowSize),
  mSmootherP1(bufferSize, windowSize) {}

CMA2Smoother::~CMA2Smoother() {}


bool
CMA2Smoother::Process(const BL_FLOAT *data, BL_FLOAT *smoothedData, int nFrames)
{
    // When debugging, we may have this case
    if (data == NULL)
        return false;
    
    WDL_TypedBuf<BL_FLOAT> tmpData;
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

//template <typename FLOAT_TYPE>
//bool CMA2Smoother::ProcessOne(const FLOAT_TYPE *data, FLOAT_TYPE *smoothedData, int nFrames, int windowSize)
bool
CMA2Smoother::ProcessOne(const BL_FLOAT *data, BL_FLOAT *smoothedData,
                         int nFrames, int windowSize)
{
    // Addition for TransientShaper
    if (windowSize <= 1)
        return false;
    
    WDL_TypedBuf<BL_FLOAT> &tmpData = mTmpBuf0;
    tmpData.Resize(nFrames);
    
    bool processed = mSmootherP1.ProcessOne(data, tmpData.Get(),
                                            nFrames, windowSize);
    if (processed)
        processed = mSmootherP1.ProcessOne(tmpData.Get(), smoothedData,
                                           nFrames, windowSize);
    
    return processed;
}
//template bool CMA2Smoother::ProcessOne(const float *data, float *smoothedData, int nFrames, int windowSize);
//template bool CMA2Smoother::ProcessOne(const double *data, double *smoothedData, int nFrames, int windowSize);

//template <typename FLOAT_TYPE>
bool
CMA2Smoother::ProcessOne(const WDL_TypedBuf<BL_FLOAT> &inData,
                         WDL_TypedBuf<BL_FLOAT> *outSmoothedData,
                         int windowSize)
{
    outSmoothedData->Resize(inData.GetSize());
    
    bool res = ProcessOne(inData.Get(), outSmoothedData->Get(),
                          inData.GetSize(), windowSize);
    
    return res;
}
//template bool CMA2Smoother::ProcessOne(const WDL_TypedBuf<float> &inData,
//                                       WDL_TypedBuf<float> *outSmoothedData,
//                                       int windowSize);
//template bool CMA2Smoother::ProcessOne(const WDL_TypedBuf<double> &inData,
//                                       WDL_TypedBuf<double> *outSmoothedData,
//                                       int windowSize);


void
CMA2Smoother::Reset()
{
    mSmoother0.Reset();
    mSmoother1.Reset();
}
