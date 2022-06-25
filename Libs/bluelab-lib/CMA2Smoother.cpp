/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
