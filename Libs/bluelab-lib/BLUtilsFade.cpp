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
 
#include <BLUtilsMath.h>

#include "BLUtilsFade.h"

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                  const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                  FLOAT_TYPE *resultBuf, FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd)
{
    int buf0Size = buf0.GetSize() - 1;
    FLOAT_TYPE *buf0Data = buf0.Get();
    FLOAT_TYPE *buf1Data = buf1.Get();
    
    for (int i = 0; i < buf0Size; i++)
    {
        FLOAT_TYPE prevVal = buf0Data[i];
        FLOAT_TYPE newVal = buf1Data[i];
        
        // Fades only on the part of the frame
        FLOAT_TYPE t = 0.0;
        if ((i >= buf0Size*fadeStart) &&
            (i < buf0Size*fadeEnd))
        {
            t = (i - buf0Size*fadeStart)/(buf0Size*(fadeEnd - fadeStart));
        }
        
        if (i >= buf0Size*fadeEnd)
            t = 1.0;
        
        FLOAT_TYPE result = (1.0 - t)*prevVal + t*newVal;
        
        resultBuf[i] = result;
    }
}
template void BLUtilsFade::Fade(const WDL_TypedBuf<float> &buf0,
                                const WDL_TypedBuf<float> &buf1,
                                float *resultBuf, float fadeStart, float fadeEnd);
template void BLUtilsFade::Fade(const WDL_TypedBuf<double> &buf0,
                                const WDL_TypedBuf<double> &buf1,
                                double *resultBuf, double fadeStart, double fadeEnd);

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(WDL_TypedBuf<FLOAT_TYPE> *buf,
                  FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd, bool fadeIn)
{
    Fade(buf->Get(), buf->GetSize(), fadeStart, fadeEnd, fadeIn);
}
template void BLUtilsFade::Fade(WDL_TypedBuf<float> *buf,
                                float fadeStart, float fadeEnd, bool fadeIn);
template void BLUtilsFade::Fade(WDL_TypedBuf<double> *buf,
                                double fadeStart, double fadeEnd, bool fadeIn);

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(FLOAT_TYPE *buf, int origBufSize,
                  FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd, bool fadeIn)
{
    int bufSize = origBufSize - 1;
    
    for (int i = 0; i < origBufSize; i++)
    {
        FLOAT_TYPE val = buf[i];
        
        // Fades only on the part of the frame
        FLOAT_TYPE t = 0.0;
        if ((i >= bufSize*fadeStart) &&
            (i < bufSize*fadeEnd))
        {
            t = (i - bufSize*fadeStart)/(bufSize*(fadeEnd - fadeStart));
        }
        
        if (i >= bufSize*fadeEnd)
            t = 1.0;
        
        FLOAT_TYPE result;
        
        if (fadeIn)
            result = t*val;
        else
            result = (1.0 - t)*val;
        
        buf[i] = result;
    }
}
template void BLUtilsFade::Fade(float *buf, int origBufSize,
                                float fadeStart, float fadeEnd, bool fadeIn);
template void BLUtilsFade::Fade(double *buf, int origBufSize,
                                double fadeStart, double fadeEnd, bool fadeIn);

// BUG: regression Spatializer5.0.8 => Spatialize 5.0.9 (clicks)
#define FIX_REGRESSION_SPATIALIZER 1
#if !FIX_REGRESSION_SPATIALIZER
template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                  const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                  FLOAT_TYPE *resultBuf,
                  FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd,
                  FLOAT_TYPE startT, FLOAT_TYPE endT)
{
    int buf0Size = buf0.GetSize() - 1;
    FLOAT_TYPE *buf0Data = buf0.Get();
    FLOAT_TYPE *buf1Data = buf1.Get();
    
    for (int i = 0; i < buf0Size; i++)
    {
        FLOAT_TYPE prevVal = buf0Data[i];
        FLOAT_TYPE newVal = buf1Data[i];
        
        // Fades only on the part of the frame
        FLOAT_TYPE u = 0.0;
        if ((i >= buf0Size*fadeStart) &&
            (i < buf0Size*fadeEnd))
        {
            u = (i - buf0Size*fadeStart)/(buf0Size*(fadeEnd - fadeStart));
        }
        
        if (i >= buf0Size*fadeEnd)
            u = 1.0;
        
        FLOAT_TYPE t = startT + u*(endT - startT);
        
        FLOAT_TYPE result = (1.0 - t)*prevVal + t*newVal;
        
        resultBuf[i] = result;
    }
}
template void BLUtilsFade::Fade(const WDL_TypedBuf<float> &buf0,
                                const WDL_TypedBuf<float> &buf1,
                                float *resultBuf,
                                float fadeStart, float fadeEnd,
                                float startT, float endT);
template void BLUtilsFade::Fade(const WDL_TypedBuf<double> &buf0,
                                const WDL_TypedBuf<double> &buf1,
                                double *resultBuf,
                                double fadeStart, double fadeEnd,
                                double startT, double endT);
#else // Fixed version
template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                  const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                  FLOAT_TYPE *resultBuf,
                  FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd,
                  FLOAT_TYPE startT, FLOAT_TYPE endT)
{
    //int bufSize = buf0.GetSize() - 1;
    
    int buf0Size = buf0.GetSize();
    FLOAT_TYPE *buf0Data = buf0.Get();
    FLOAT_TYPE *buf1Data = buf1.Get();
    
    for (int i = 0; i < buf0Size; i++)
    {
        FLOAT_TYPE prevVal = buf0Data[i];
        FLOAT_TYPE newVal = buf1Data[i];
        
        // Fades only on the part of the frame
        FLOAT_TYPE u = 0.0;
        if ((i >= buf0Size*fadeStart) &&
            (i < buf0Size*fadeEnd))
        {
            u = (i - buf0Size*fadeStart)/(buf0Size*(fadeEnd - fadeStart));
        }
        
        if (i >= buf0Size*fadeEnd)
            u = 1.0;
        
        FLOAT_TYPE t = startT + u*(endT - startT);
        
        FLOAT_TYPE result = (1.0 - t)*prevVal + t*newVal;
        
        resultBuf[i] = result;
    }
}
template void BLUtilsFade::Fade(const WDL_TypedBuf<float> &buf0,
                                const WDL_TypedBuf<float> &buf1,
                                float *resultBuf,
                                float fadeStart, float fadeEnd,
                                float startT, float endT);
template void BLUtilsFade::Fade(const WDL_TypedBuf<double> &buf0,
                                const WDL_TypedBuf<double> &buf1,
                                double *resultBuf,
                                double fadeStart, double fadeEnd,
                                double startT, double endT);
#endif

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(WDL_TypedBuf<FLOAT_TYPE> *ioBuf0,
                  const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                  FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd,
                  bool fadeIn,
                  FLOAT_TYPE startPos, FLOAT_TYPE endPos)
{
    // NEW: check for bounds !
    // Added for Ghost-X and FftProcessObj15 (latency fix)
    if (startPos < 0.0)
        startPos = 0.0;
    if (startPos > 1.0)
        startPos = 1.0;
    
    if (endPos < 0.0)
        endPos = 0.0;
    if (endPos > 1.0)
        endPos = 1.0;
    
    long startIdx = startPos*ioBuf0->GetSize();
    long endIdx = endPos*ioBuf0->GetSize();
 
    int buf0Size = ioBuf0->GetSize() - 1;
    FLOAT_TYPE *buf0Data = ioBuf0->Get();
    FLOAT_TYPE *buf1Data = buf1.Get();
    
    for (int i = startIdx; i < endIdx; i++)
    {
        FLOAT_TYPE prevVal = buf0Data[i];
        FLOAT_TYPE newVal = buf1Data[i];
        
        // Fades only on the part of the frame
        FLOAT_TYPE t = 0.0;
        if ((i >= buf0Size*fadeStart) &&
            (i < buf0Size*fadeEnd))
        {
            t = (i - buf0Size*fadeStart)/(buf0Size*(fadeEnd - fadeStart));
        }
        
        if (i >= buf0Size*fadeEnd)
            t = 1.0;
        
        FLOAT_TYPE result;
        if (fadeIn)
            result = (1.0 - t)*prevVal + t*newVal;
        else
            result = t*prevVal + (1.0 - t)*newVal;
        
        buf0Data[i] = result;
    }
}
template void BLUtilsFade::Fade(WDL_TypedBuf<float> *ioBuf0,
                                const WDL_TypedBuf<float> &buf1,
                                float fadeStart, float fadeEnd,
                                bool fadeIn,
                                float startPos, float endPos);
template void BLUtilsFade::Fade(WDL_TypedBuf<double> *ioBuf0,
                                const WDL_TypedBuf<double> &buf1,
                                double fadeStart, double fadeEnd,
                                bool fadeIn,
                                double startPos, double endPos);

//
template <typename FLOAT_TYPE>
void
BLUtilsFade::FadeOut(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                     int startSampleId, int endSampleId)
{
    if (ioBuf->GetSize() == 0)
        return;
    
    if (startSampleId < 0)
        startSampleId = 0;
    if (endSampleId >= ioBuf->GetSize())
        endSampleId = ioBuf->GetSize() - 1;
    
    if (startSampleId == endSampleId)
        return;
    
    for (int i = startSampleId; i <= endSampleId; i++)
    {
        FLOAT_TYPE t = ((FLOAT_TYPE)(i - startSampleId))/
            (endSampleId - startSampleId);
        
        t = 1.0 - t;
        
        FLOAT_TYPE val = ioBuf->Get()[i];
        val *= t;
        ioBuf->Get()[i] = val;
    }
}
template void BLUtilsFade::FadeOut(WDL_TypedBuf<float> *ioBuf,
                                   int startSampleId, int endSampleId);
template void BLUtilsFade::FadeOut(WDL_TypedBuf<double> *ioBuf,
                                   int startSampleId, int endSampleId);


template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(WDL_TypedBuf<FLOAT_TYPE> *ioBuf0,
                  const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                  FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd)
{
    Fade(ioBuf0, buf1, fadeStart, fadeEnd, true, (FLOAT_TYPE)0.0, (FLOAT_TYPE)0.5);
    Fade(ioBuf0, buf1, (FLOAT_TYPE)1.0 - fadeEnd, (FLOAT_TYPE)1.0 - fadeStart,
         false, (FLOAT_TYPE)0.5, (FLOAT_TYPE)1.0);
}
template void BLUtilsFade::Fade(WDL_TypedBuf<float> *ioBuf0,
                                const WDL_TypedBuf<float> &buf1,
                                float fadeStart, float fadeEnd);
template void BLUtilsFade::Fade(WDL_TypedBuf<double> *ioBuf0,
                                const WDL_TypedBuf<double> &buf1,
                                double fadeStart, double fadeEnd);

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade(FLOAT_TYPE *ioBuf0Data,
                  const FLOAT_TYPE *buf1Data,
                  int bufSize,
                  FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd)
{
    WDL_TypedBuf<FLOAT_TYPE> buf0;
    buf0.Resize(bufSize);
    memcpy(buf0.Get(), ioBuf0Data, bufSize*sizeof(FLOAT_TYPE));
    
    WDL_TypedBuf<FLOAT_TYPE> buf1;
    buf1.Resize(bufSize);
    memcpy(buf1.Get(), buf1Data, bufSize*sizeof(FLOAT_TYPE));
    
    Fade(&buf0, buf1, fadeStart, fadeEnd, true, (FLOAT_TYPE)0.0, (FLOAT_TYPE)0.5);
    Fade(&buf0, buf1, (FLOAT_TYPE)1.0 - fadeEnd, (FLOAT_TYPE)1.0 - fadeStart,
         false, (FLOAT_TYPE)0.5, (FLOAT_TYPE)1.0);
    
    memcpy(ioBuf0Data, buf0.Get(), bufSize*sizeof(FLOAT_TYPE));
}
template void BLUtilsFade::Fade(float *ioBuf0Data,
                                const float *buf1Data,
                                int bufSize,
                                float fadeStart, float fadeEnd);
template void BLUtilsFade::Fade(double *ioBuf0Data,
                                const double *buf1Data,
                                int bufSize,
                                double fadeStart, double fadeEnd);

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade2(FLOAT_TYPE *ioBuf0Data, const FLOAT_TYPE *buf1Data, int bufSize,
                   FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                   FLOAT_TYPE startT, FLOAT_TYPE endT,
                   FLOAT_TYPE sigmoA)
{
    // NEW: check for bounds !
    // Added for Ghost-X and FftProcessObj15 (latency fix)
    if (fadeStartPos < 0.0)
        fadeStartPos = 0.0;
    if (fadeStartPos > 1.0)
        fadeStartPos = 1.0;
    
    if (fadeEndPos < 0.0)
        fadeEndPos = 0.0;
    if (fadeEndPos > 1.0)
        fadeEndPos = 1.0;
                       
    for (int i = 0; i < bufSize; i++)
    {
        // Avoid accessing buffer out of bounds just after
        if ((i < bufSize*fadeStartPos) ||
            (i >= bufSize*fadeEndPos))
            continue;
            
        FLOAT_TYPE prevVal = ioBuf0Data[i];
        FLOAT_TYPE newVal = buf1Data[i];
        
        // Fades only on the part of the frame
        FLOAT_TYPE u = 0.0;
        if ((i >= bufSize*fadeStartPos) &&
            (i < bufSize*fadeEndPos))
        {
            u = (i - bufSize*fadeStartPos)/(bufSize*(fadeEndPos - fadeStartPos));
        }
        
        if (i >= bufSize*fadeEndPos)
            u = 1.0;
        
        FLOAT_TYPE t = startT + u*(endT - startT);

        // OLD: use power
        //t = std::pow(t, fadeShapePower);

        // NEW: use sigmoid
        t = BLUtilsMath::ApplySigmoid(t, sigmoA);
        
        FLOAT_TYPE result = (1.0 - t)*prevVal + t*newVal;
        
        ioBuf0Data[i] = result;
    }
}
template void BLUtilsFade::Fade2(float *ioBuf0Data,
                                 const float *buf1Data, int bufSize,
                                 float fadeStartPos, float fadeEndPos,
                                 float startT, float endT,
                                 float fadeShapePower);
                             
template void BLUtilsFade::Fade2(double *ioBuf0Data,
                                 const double *buf1Data, int bufSize,
                                 double fadeStartPos, double fadeEndPos,
                                 double startT, double endT,
                                 double fadeShapePower);

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade2Left(FLOAT_TYPE *ioBuf0Data, const FLOAT_TYPE *buf1Data,
                       int bufSize,
                       FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                       FLOAT_TYPE startT, FLOAT_TYPE endT,
                       FLOAT_TYPE sigmoA)
{
    Fade2(ioBuf0Data, buf1Data, bufSize,
          fadeStartPos, fadeEndPos,
          startT, endT, sigmoA);
}
template void BLUtilsFade::Fade2Left(float *ioBuf0Data, const float *buf1Data,
                                     int bufSize,
                                     float fadeStartPos, float fadeEndPos,
                                     float startT, float endT,
                                     float sigmoA);
template void BLUtilsFade::Fade2Left(double *ioBuf0Data, const double *buf1Data,
                                     int bufSize,
                                     double fadeStartPos, double fadeEndPos,
                                     double startT, double endT,
                                     double sigmoA);


template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade2Right(FLOAT_TYPE *ioBuf0Data, const FLOAT_TYPE *buf1Data,
                        int bufSize,
                        FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                        FLOAT_TYPE startT, FLOAT_TYPE endT,
                        FLOAT_TYPE sigmoA)
{
    Fade2(ioBuf0Data, buf1Data, bufSize,
          (FLOAT_TYPE)1.0 - fadeEndPos, (FLOAT_TYPE)1.0 - fadeStartPos,
          endT, startT, sigmoA);
}
template void BLUtilsFade::Fade2Right(float *ioBuf0Data, const float *buf1Data,
                                      int bufSize,
                                      float fadeStartPos, float fadeEndPos,
                                      float startT, float endT,
                                      float sigmoA);
template void BLUtilsFade::Fade2Right(double *ioBuf0Data, const double *buf1Data,
                                      int bufSize,
                                      double fadeStartPos, double fadeEndPos,
                                      double startT, double endT,
                                      double sigmoA);

template <typename FLOAT_TYPE>
void
BLUtilsFade::Fade2Double(FLOAT_TYPE *ioBuf0Data, const FLOAT_TYPE *buf1Data,
                         int bufSize,
                         FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                         FLOAT_TYPE startT, FLOAT_TYPE endT,
                         FLOAT_TYPE sigmoA)
{
    Fade2Left(ioBuf0Data, buf1Data, bufSize,
              fadeStartPos, fadeEndPos,
              startT, endT, sigmoA);
    Fade2Right(ioBuf0Data, buf1Data, bufSize,
               fadeStartPos, fadeEndPos,
               startT, endT, sigmoA);
}
template void BLUtilsFade::Fade2Double(float *ioBuf0Data, const float *buf1Data,
                                       int bufSize,
                                       float fadeStartPos, float fadeEndPos,
                                       float startT, float endT,
                                       float sigmoA);
template void BLUtilsFade::Fade2Double(double *ioBuf0Data, const double *buf1Data,
                                       int bufSize,
                                       double fadeStartPos, double fadeEndPos,
                                       double startT, double endT,
                                       double sigmoA);
