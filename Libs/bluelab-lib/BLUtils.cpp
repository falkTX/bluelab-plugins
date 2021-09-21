//
//  BLUtils.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#include <math.h>

#include <cmath>

#include <vector>
#include <algorithm>
using namespace std;

#ifdef WIN32
#include <Windows.h>
#endif

#if 0
// Mel / Mfcc
extern "C" {
#include <libmfcc.h>
}
#endif

#ifndef WIN32
#include <sys/time.h>
#else
#include "GetTimeOfDay.h"
#endif

#include <UpTime.h>

#include "CMA2Smoother.h"

// For AmpToDB
#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

#include <IPlugPaths.h>

#include <FastMath.h>
#include <BLDebug.h>

#include <BLUtilsMath.h>
#include <BLUtilsPhases.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "BLUtils.h"

using namespace iplug;

// Optimizations
// - avoid "GetSize()" in loops
// - avoid "buf.Get()" in loops


// TODO: must check if it is used
/* BL_FLOAT
   BLUtils::ampToDB(BL_FLOAT amp, BL_FLOAT minDB)
   {
   if (amp <= 0.0)
   return minDB;
    
   BL_FLOAT db = 20. * std::log10(amp);
    
   return db;
   } */

#define FIND_VALUE_INDEX_EXPE 0

// Simd
//
// With AVX, he gain is not exceptional with SoundMetaViewer
// and may not support computers before 2011
//
// NOTE: AVX apperad in 2008, and was integrated in 2011
// (this could be risky to use it in plugins)
//
// TODO: make unit tests SIMD for SIMSD (not tested well)
// See class "TestSimd"

// NOTE: before re-enabling this, must check for all templates with float
// Use simdpp::float32 if possible...
#define USE_SIMD 0 //1

// For dispatch see: https://p12tic.github.io/libsimdpp/v2.1/libsimdpp/w/arch/dispatch.html
// For operations see: http://p12tic.github.io/libsimdpp/v2.2-dev/libsimdpp/w/

// For dynamic dispatch, define for the preprocessor:
// - SIMDPP_EMIT_DISPATCHER
// - SIMDPP_DISPATCH_NONE_NULL ?
// - SIMDPP_ARCH_X86_AVX
// Some others...
//
// NOTE: for the moment, did not succeed (need to compile several times the same cpp ?)
// => for the moment, crashes
//
#if USE_SIMD

// Configs
//

// None: 73%

// 66%
#define SIMDPP_ARCH_X86_AVX 1
#define SIMD_PACK_SIZE 4 // Native
//#define SIMD_PACK_SIZE 8 //64

//69%
//#define SIMDPP_ARCH_X86_SSE2 1
//#define SIMD_PACK_SIZE 2

// Not supported
//#define SIMDPP_ARCH_X86_AVX512F 1
//#define SIMD_PACK_SIZE 8


#include <simd.h>

#include <simdpp/dispatch/get_arch_gcc_builtin_cpu_supports.h>
#include <simdpp/dispatch/get_arch_raw_cpuid.h>
#include <simdpp/dispatch/get_arch_linux_cpuinfo.h>

#if SIMDPP_HAS_GET_ARCH_RAW_CPUID
#define SIMDPP_USER_ARCH_INFO ::simdpp::get_arch_raw_cpuid()
#elif SIMDPP_HAS_GET_ARCH_GCC_BUILTIN_CPU_SUPPORTS
#define SIMDPP_USER_ARCH_INFO ::simdpp::get_arch_gcc_builtin_cpu_supports()
#elif SIMDPP_HAS_GET_ARCH_LINUX_CPUINFO
#define SIMDPP_USER_ARCH_INFO ::simdpp::get_arch_linux_cpuinfo()
#else
#error "Unsupported platform"
#endif

#endif

// Additional optilizations added duging the implementation of USE_SIMD
// NOTE: this macro is diplicated in all BLUtils* classes
#define USE_SIMD_OPTIM 1

#define INF 1e15

bool _useSimd = false;

void
BLUtils::SetUseSimdFlag(bool flag)
{
#if USE_SIMD
    _useSimd = flag;
#endif
}

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeRMSAvg(const FLOAT_TYPE *values, int nFrames)
{
#if !USE_SIMD
    FLOAT_TYPE avg = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE value = values[i];
        avg += value*value;
    }
#else
    FLOAT_TYPE avg = ComputeSquareSum(values, nFrames);
#endif

    // Real formula !
    avg = std::sqrt(avg/nFrames);
    
    //avg = std::sqrt(avg)/nFrames;
    
    return avg;
}
template float BLUtils::ComputeRMSAvg(const float *values, int nFrames);
template double BLUtils::ComputeRMSAvg(const double *values, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeSquareSum(const FLOAT_TYPE *values, int nFrames)
{
    FLOAT_TYPE sum2 = 0.0;
    
#if USE_SIMD
    if (_useSimd && (nFrames % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < nFrames; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(values);
            simdpp::float64<SIMD_PACK_SIZE> s = v0*v0;
            
            FLOAT_TYPE r = simdpp::reduce_add(s);
            
            sum2 += r;
            
            values += SIMD_PACK_SIZE;
        }
        
        // Finished
        return sum2;
    }
#endif
    
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE value = values[i];
        sum2 += value*value;
    }
    
    return sum2;
}
template float BLUtils::ComputeSquareSum(const float *values, int nFrames);
template double BLUtils::ComputeSquareSum(const double *values, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeRMSAvg(const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    return ComputeRMSAvg(values.Get(), values.GetSize());
}
template float BLUtils::ComputeRMSAvg(const WDL_TypedBuf<float> &values);
template double BLUtils::ComputeRMSAvg(const WDL_TypedBuf<double> &values);

// Does not give good results with UST
// (this looks false)
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeRMSAvg2(const FLOAT_TYPE *values, int nFrames)
{
#if !USE_SIMD_OPTIM
    FLOAT_TYPE avg = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE value = values[i];
        avg += value*value;
    }
#else
    FLOAT_TYPE avg = ComputeSquareSum(values, nFrames);
#endif

    // Fake formula, see ComputeRMSAvg() for real formula 
    avg = std::sqrt(avg)/nFrames;
    
    return avg;
}
template float BLUtils::ComputeRMSAvg2(const float *values, int nFrames);
template double BLUtils::ComputeRMSAvg2(const double *values, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeRMSAvg2(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    return ComputeRMSAvg2(buf.Get(), buf.GetSize());
}
template float BLUtils::ComputeRMSAvg2(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeRMSAvg2(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAvg(const FLOAT_TYPE *buf, int nFrames)
{
#if !USE_SIMD_OPTIM
    FLOAT_TYPE avg = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE value = buf[i];
        avg += value;
    }
#else
    FLOAT_TYPE avg = ComputeSum(buf, nFrames);
#endif
    
    if (nFrames > 0)
        avg = avg/nFrames;
    
    return avg;
}
template float BLUtils::ComputeAvg(const float *buf, int nFrames);
template double BLUtils::ComputeAvg(const double *buf, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAvg(const vector<FLOAT_TYPE> &data)
{
    if (data.empty())
        return 0.0;
    
    FLOAT_TYPE sum = 0.0;
    for (int i = 0; i < data.size(); i++)
    {
        sum += data[i];
    }
    
    FLOAT_TYPE avg = sum/data.size();
    
    return avg;
}
template float BLUtils::ComputeAvg(const vector<float> &data);
template double BLUtils::ComputeAvg(const vector<double> &data);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    return ComputeAvg(buf.Get(), buf.GetSize());
}
template float BLUtils::ComputeAvg(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeAvg(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf,
                    int startIndex, int endIndex)
{
    FLOAT_TYPE sum = 0.0;
    int numValues = 0;
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    for (int i = startIndex; i <= endIndex ; i++)
    {
        if (i < 0)
            continue;
        if (i >= bufSize)
            break;
        
        FLOAT_TYPE value = bufData[i];
        sum += value;
        numValues++;
    }
    
    FLOAT_TYPE avg = 0.0;
    if (numValues > 0)
        avg = sum/numValues;
    
    return avg;
}
template float BLUtils::ComputeAvg(const WDL_TypedBuf<float> &buf,
                                   int startIndex, int endIndex);
template double BLUtils::ComputeAvg(const WDL_TypedBuf<double> &buf,
                                    int startIndex, int endIndex);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAvg(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image)
{
    FLOAT_TYPE sum = 0.0;
    int numValues = 0;
    
    for (int i = 0; i < image.size(); i++)
    {
#if !USE_SIMD_OPTIM
        for (int j = 0; j < image[i].GetSize(); j++)
        {
            FLOAT_TYPE val = image[i].Get()[j];
        
            sum += val;
            numValues++;
        }
#else
        sum += ComputeAvg(image[i]);
        numValues += image[i].GetSize();
#endif
    }
    
    FLOAT_TYPE avg = sum;
    if (numValues > 0)
        avg /= numValues;
    
    return avg;
}
template float BLUtils::ComputeAvg(const vector<WDL_TypedBuf<float> > &image);
template double BLUtils::ComputeAvg(const vector<WDL_TypedBuf<double> > &image);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAvgSquare(const FLOAT_TYPE *buf, int nFrames)
{
#if !USE_SIMD_OPTIM
    FLOAT_TYPE avg = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE value = buf[i];
        avg += value*value;
    }
#else
    FLOAT_TYPE avg = ComputeSquareSum(buf, nFrames);
#endif
    
    if (nFrames > 0)
        avg = std::sqrt(avg)/nFrames;
    
    return avg;
}
template float BLUtils::ComputeAvgSquare(const float *buf, int nFrames);
template double BLUtils::ComputeAvgSquare(const double *buf, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAvgSquare(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    return ComputeAvgSquare(buf.Get(), buf.GetSize());
}
template float BLUtils::ComputeAvgSquare(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeAvgSquare(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeSquare(WDL_TypedBuf<FLOAT_TYPE> *buf)
{
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            simdpp::float64<SIMD_PACK_SIZE> r = v0*v0;
            
            simdpp::store(bufData, r);
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = bufData[i];

        val = val*val;
        
        bufData[i] = val;
    }
}
template void BLUtils::ComputeSquare(WDL_TypedBuf<float> *buf);
template void BLUtils::ComputeSquare(WDL_TypedBuf<double> *buf);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeOpposite(WDL_TypedBuf<FLOAT_TYPE> *buf)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        FLOAT_TYPE val = buf->Get()[i];
        val = (FLOAT_TYPE)1.0 - val;
        buf->Get()[i] = val;
    }
}
template void BLUtils::ComputeOpposite(WDL_TypedBuf<float> *buf);
template void BLUtils::ComputeOpposite(WDL_TypedBuf<double> *buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAbsAvg(const FLOAT_TYPE *buf, int nFrames)
{
    FLOAT_TYPE avg = ComputeAbsSum(buf, nFrames);
    
    if (nFrames > 0)
        avg = avg/nFrames;
    
    return avg;
}
template float BLUtils::ComputeAbsAvg(const float *buf, int nFrames);
template double BLUtils::ComputeAbsAvg(const double *buf, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAbsAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    return ComputeAbsAvg(buf.Get(), buf.GetSize());
}
template float BLUtils::ComputeAbsAvg(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeAbsAvg(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAbsAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf,
                       int startIndex, int endIndex)
{
    FLOAT_TYPE sum = 0.0;
    int numValues = 0;
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    for (int i = startIndex; i <= endIndex ; i++)
    {
        if (i < 0)
            continue;
        if (i >= bufSize)
            break;
        
        FLOAT_TYPE value = bufData[i];
        value = std::fabs(value);
        
        sum += value;
        numValues++;
    }
    
    FLOAT_TYPE avg = 0.0;
    if (numValues > 0)
        avg = sum/numValues;
    
    return avg;
}
template float BLUtils::ComputeAbsAvg(const WDL_TypedBuf<float> &buf,
                                      int startIndex, int endIndex);
template double BLUtils::ComputeAbsAvg(const WDL_TypedBuf<double> &buf,
                                       int startIndex, int endIndex);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMax(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    return ComputeMax(buf.Get(), buf.GetSize());
}
template float BLUtils::ComputeMax(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeMax(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMax(const FLOAT_TYPE *output, int nFrames)
{
    FLOAT_TYPE maxVal = -1e16;
    
#if USE_SIMD
    if (_useSimd && (nFrames % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < nFrames; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(output);
            
            FLOAT_TYPE r = simdpp::reduce_max(v0);
            
            if (r > maxVal)
                maxVal = r;
            
            output += SIMD_PACK_SIZE;
        }
        
        // Finished
        return maxVal;
    }
#endif
    
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE value = output[i];
        
        if (value > maxVal)
            maxVal = value;
    }
    
    return maxVal;
}
template float BLUtils::ComputeMax(const float *output, int nFrames);
template double BLUtils::ComputeMax(const double *output, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMax(const vector<WDL_TypedBuf<FLOAT_TYPE> > &values)
{
    // SIMD
    FLOAT_TYPE maxValue = -1e15;
    
    for (int i = 0; i < values.size(); i++)
    {
#if !USE_SIMD_OPTIM
        for (int j = 0; j < values[i].GetSize(); j++)
        {
            FLOAT_TYPE val = values[i].Get()[j];
            if (val > maxValue)
                maxValue = val;
        }
#else
        FLOAT_TYPE max0 = ComputeMax(values[i].Get(), values[i].GetSize());
        if (max0 > maxValue)
            maxValue = max0;
#endif
    }
    
    return maxValue;
}
template float BLUtils::ComputeMax(const vector<WDL_TypedBuf<float> > &values);
template double BLUtils::ComputeMax(const vector<WDL_TypedBuf<double> > &values);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMaxAbs(const FLOAT_TYPE *buf, int nFrames)
{
    FLOAT_TYPE maxVal = 0.0;
    
#if USE_SIMD
    if (_useSimd && (nFrames % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < nFrames; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf);
            
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::abs(v0);
            
            FLOAT_TYPE r = simdpp::reduce_max(v1);
            
            if (r > maxVal)
                maxVal = r;
            
            buf += SIMD_PACK_SIZE;
        }
        
        // Finished
        return maxVal;
    }
#endif
    
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE value = buf[i];
        
        value = std::fabs(value);
        
        if (value > maxVal)
            maxVal = value;
    }
    
    return maxVal;
}
template float BLUtils::ComputeMaxAbs(const float *buf, int nFrames);
template double BLUtils::ComputeMaxAbs(const double *buf, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMaxAbs(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
#if !USE_SIMD_OPTIM
    FLOAT_TYPE maxVal = 0.0;
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE value = bufData[i];
        
        value = std::fabs(value);
        
        if (value > maxVal)
            maxVal = value;
    }
    
    return maxVal;
#else
    FLOAT_TYPE maxVal = ComputeMaxAbs(buf.Get(), buf.GetSize());
    
    return maxVal;
#endif
}
template float BLUtils::ComputeMaxAbs(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeMaxAbs(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeMax(WDL_TypedBuf<FLOAT_TYPE> *max,
                    const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    if (max->GetSize() != buf.GetSize())
        max->Resize(buf.GetSize());
    
    int maxSize = max->GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    FLOAT_TYPE *maxData = max->Get();
    
#if USE_SIMD
    if (_useSimd && (maxSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < maxSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(maxData);
            
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::max(v0, v1);
            
            simdpp::store(maxData, r);
            
            bufData += SIMD_PACK_SIZE;
            maxData += SIMD_PACK_SIZE;
        }
    }
    
    return;
#endif
    
    for (int i = 0; i < maxSize; i++)
    {
        FLOAT_TYPE val = bufData[i];
        FLOAT_TYPE m = maxData[i];
        
        if (val > m)
            maxData[i] = val;
    }
}
template void BLUtils::ComputeMax(WDL_TypedBuf<float> *max,
                                  const WDL_TypedBuf<float> &buf);
template void BLUtils::ComputeMax(WDL_TypedBuf<double> *max,
                                  const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeMax(WDL_TypedBuf<FLOAT_TYPE> *maxBuf, const FLOAT_TYPE *buf)
{
    int maxSize = maxBuf->GetSize();
    FLOAT_TYPE *maxData = maxBuf->Get();
    
#if USE_SIMD
    if (_useSimd && (maxSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < maxSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(maxData);
            
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::max(v0, v1);
            
            simdpp::store(maxData, r);
            
            buf += SIMD_PACK_SIZE;
            maxData += SIMD_PACK_SIZE;
        }
    }
    
    return;
#endif
    
    for (int i = 0; i < maxSize; i++)
    {
        FLOAT_TYPE val = buf[i];
        FLOAT_TYPE m = maxData[i];
        
        if (val > m)
            maxData[i] = val;
    }
}
template void BLUtils::ComputeMax(WDL_TypedBuf<float> *maxBuf, const float *buf);
template void BLUtils::ComputeMax(WDL_TypedBuf<double> *maxBuf, const double *buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMin(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    
    if (bufSize == 0)
        return 0.0;
    
    FLOAT_TYPE minVal = bufData[0];
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            
            FLOAT_TYPE r = simdpp::reduce_min(v0);
            
            if (r < minVal)
                minVal = r;
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return minVal;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = bufData[i];
        
        if (val < minVal)
            minVal = val;
    }
    
    return minVal;
}
template float BLUtils::ComputeMin(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeMin(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMin(const vector<WDL_TypedBuf<FLOAT_TYPE> > &values)
{
    FLOAT_TYPE minValue = 1e15;
    
    for (int i = 0; i < values.size(); i++)
    {
#if !USE_SIMD_OPTIM
        for (int j = 0; j < values[i].GetSize(); j++)
        {
            FLOAT_TYPE val = values[i].Get()[j];
            if (val < minValue)
                minValue = val;
        }
#else
        FLOAT_TYPE val = ComputeMin(values[i]);
        if (val < minValue)
            minValue = val;
#endif
    }
    
    return minValue;
}
template float BLUtils::ComputeMin(const vector<WDL_TypedBuf<float> > &values);
template double BLUtils::ComputeMin(const vector<WDL_TypedBuf<double> > &values);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeSum(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    
    FLOAT_TYPE result = 0.0;
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            
            FLOAT_TYPE r = simdpp::reduce_add(v0);
            
            result += r;
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return result;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = bufData[i];
        
        result += val;
    }
    
    return result;
}
template float BLUtils::ComputeSum(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeSum(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeSum(const FLOAT_TYPE *buf, int nFrames)
{
    FLOAT_TYPE result = 0.0;
    
#if USE_SIMD
    if (_useSimd && (nFrames % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < nFrames; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf);
            
            FLOAT_TYPE r = simdpp::reduce_add(v0);
            
            result += r;
            
            buf += SIMD_PACK_SIZE;
        }
        
        // Finished
        return result;
    }
#endif
    
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE val = buf[i];
        
        result += val;
    }
    
    return result;
}
template float BLUtils::ComputeSum(const float *buf, int nFrames);
template double BLUtils::ComputeSum(const double *buf, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAbsSum(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    FLOAT_TYPE result = 0.0;
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::abs(v0);
            
            FLOAT_TYPE r = simdpp::reduce_add(v1);
            
            result += r;
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return result;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = bufData[i];
        
        result += std::fabs(val);
    }
    
    return result;
}
template float BLUtils::ComputeAbsSum(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeAbsSum(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeAbsSum(const FLOAT_TYPE *buf, int nFrames)
{
    FLOAT_TYPE result = 0.0;
    
    int bufSize = nFrames;
    const FLOAT_TYPE *bufData = buf;
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::abs(v0);
            
            FLOAT_TYPE r = simdpp::reduce_add(v1);
            
            result += r;
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return result;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = bufData[i];
        
        result += std::fabs(val);
    }
    
    return result;
}
template float BLUtils::ComputeAbsSum(const float *buf, int nFrames);
template double BLUtils::ComputeAbsSum(const double *buf, int nFrames);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeClipSum(const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    FLOAT_TYPE result = 0.0;
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            
            FLOAT_TYPE z = 0.0;
            simdpp::float64<SIMD_PACK_SIZE> z0 = simdpp::load_splat(&z);
            
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::max(v0, z0);
            
            FLOAT_TYPE r = simdpp::reduce_add(v1);
            
            result += r;
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return result;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = bufData[i];
        
        // Clip if necessary
        if (val < 0.0)
            val = 0.0;
        
        result += val;
    }
    
    return result;
}
template float BLUtils::ComputeClipSum(const WDL_TypedBuf<float> &buf);
template double BLUtils::ComputeClipSum(const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeSum(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                    const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                    WDL_TypedBuf<FLOAT_TYPE> *result)
{
    if (buf0.GetSize() != buf1.GetSize())
        return;
    
    result->Resize(buf0.GetSize());
    
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    FLOAT_TYPE *buf0Data = buf0.Get();
    FLOAT_TYPE *buf1Data = buf1.Get();
    
#if USE_SIMD
    0 //if (_useSimd && (resultSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < resultSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf0Data);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(buf1Data);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0 + v1;
            
            simdpp::store(resultData, r);
            
            
            buf0Data += SIMD_PACK_SIZE;
            buf1Data += SIMD_PACK_SIZE;
            
            resultData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < resultSize; i++)
    {
        FLOAT_TYPE val0 = buf0Data[i];
        FLOAT_TYPE val1 = buf1Data[i];
        
        FLOAT_TYPE sum = val0 + val1;
        
        resultData[i] = sum;
    }
}
template void BLUtils::ComputeSum(const WDL_TypedBuf<float> &buf0,
                                  const WDL_TypedBuf<float> &buf1,
                                  WDL_TypedBuf<float> *result);
template void BLUtils::ComputeSum(const WDL_TypedBuf<double> &buf0,
                                  const WDL_TypedBuf<double> &buf1,
                                  WDL_TypedBuf<double> *result);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeProduct(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                        const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                        WDL_TypedBuf<FLOAT_TYPE> *result)
{
    if (buf0.GetSize() != buf1.GetSize())
        return;
    
    result->Resize(buf0.GetSize());
    
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    const FLOAT_TYPE *buf0Data = buf0.Get();
    const FLOAT_TYPE *buf1Data = buf1.Get();
    
#if USE_SIMD
    if (_useSimd && (resultSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < resultSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf0Data);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(buf1Data);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0 * v1;
            
            simdpp::store(resultData, r);
            
            
            buf0Data += SIMD_PACK_SIZE;
            buf1Data += SIMD_PACK_SIZE;
            
            resultData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < resultSize; i++)
    {
        // SIMD
        //FLOAT_TYPE val0 = buf0.Get()[i];
        //FLOAT_TYPE val1 = buf1.Get()[i];
        
        FLOAT_TYPE val0 = buf0Data[i];
        FLOAT_TYPE val1 = buf1Data[i];
        
        FLOAT_TYPE prod = val0 * val1;
        
        resultData[i] = prod;
    }
}
template void BLUtils::ComputeProduct(const WDL_TypedBuf<float> &buf0,
                                      const WDL_TypedBuf<float> &buf1,
                                      WDL_TypedBuf<float> *result);
template void BLUtils::ComputeProduct(const WDL_TypedBuf<double> &buf0,
                                      const WDL_TypedBuf<double> &buf1,
                                      WDL_TypedBuf<double> *result);


template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::NormalizedXTodB(FLOAT_TYPE x, FLOAT_TYPE mindB, FLOAT_TYPE maxdB)
{
    x = x*(maxdB - mindB) + mindB;
    
    if (x > 0.0)
        x = BLUtils::AmpToDB(x);
        
    FLOAT_TYPE lMin = BLUtils::AmpToDB(mindB);
    FLOAT_TYPE lMax = BLUtils::AmpToDB(maxdB);
        
    x = (x - lMin)/(lMax - lMin);
    
    return x;
}
template float BLUtils::NormalizedXTodB(float x, float mindB, float maxdB);
template double BLUtils::NormalizedXTodB(double x, double mindB, double maxdB);

// Same as NormalizedYTodBInv
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::NormalizedXTodBInv(FLOAT_TYPE x, FLOAT_TYPE mindB, FLOAT_TYPE maxdB)
{
    FLOAT_TYPE lMin = BLUtils::AmpToDB(mindB);
    FLOAT_TYPE lMax = BLUtils::AmpToDB(maxdB);
    
    FLOAT_TYPE result = x*(lMax - lMin) + lMin;
    
    if (result > 0.0)
        result = BLUtils::DBToAmp(result);
    
    result = (result - mindB)/(maxdB - mindB);
    
    return result;
}
template float BLUtils::NormalizedXTodBInv(float x, float mindB, float maxdB);
template double BLUtils::NormalizedXTodBInv(double x, double mindB, double maxdB);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::NormalizedYTodB(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB)
{
    if (std::fabs(y) < BL_EPS)
        y = mindB;
    else
        y = BLUtils::AmpToDB(y);
    
    y = (y - mindB)/(maxdB - mindB);
    
    return y;
}
template float BLUtils::NormalizedYTodB(float y, float mindB, float maxdB);
template double BLUtils::NormalizedYTodB(double y, double mindB, double maxdB);

template <typename FLOAT_TYPE>
void
BLUtils::NormalizedYTodB(const WDL_TypedBuf<FLOAT_TYPE> &yBuf,
                         FLOAT_TYPE mindB, FLOAT_TYPE maxdB,
                         WDL_TypedBuf<FLOAT_TYPE> *resBuf)
{
    resBuf->Resize(yBuf.GetSize());

    FLOAT_TYPE rangeInv = 1.0/(maxdB - mindB);

    int bufSize = yBuf.GetSize();
    FLOAT_TYPE *yBufData = yBuf.Get();
    FLOAT_TYPE *resBufData = resBuf->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE y = yBufData[i];
        
        if (std::fabs(y) < BL_EPS)
            y = mindB;
        else
            y = BLUtils::AmpToDB(y);
    
        //y = (y - mindB)/(maxdB - mindB);
        y = (y - mindB)*rangeInv;

        resBufData[i] = y;
    }
}
template void BLUtils::NormalizedYTodB(const WDL_TypedBuf<float> &yBuf,
                                       float mindB, float maxdB,
                                       WDL_TypedBuf<float> *resBuf);
template void BLUtils::NormalizedYTodB(const WDL_TypedBuf<double> &yBuf,
                                       double mindB, double maxdB,
                                       WDL_TypedBuf<double> *resBuf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::NormalizedYTodB2(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB)
{
    y = BLUtils::AmpToDB(y);
    
    y = (y - mindB)/(maxdB - mindB);
    
    return y;
}
template float BLUtils::NormalizedYTodB2(float y, float mindB, float maxdB);
template double BLUtils::NormalizedYTodB2(double y, double mindB, double maxdB);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::NormalizedYTodB3(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB)
{
    y = y*(maxdB - mindB) + mindB;
    
    if (y > 0.0)
        y = BLUtils::AmpToDB(y);
    
    FLOAT_TYPE lMin = BLUtils::AmpToDB(mindB);
    FLOAT_TYPE lMax = BLUtils::AmpToDB(maxdB);
    
    y = (y - lMin)/(lMax - lMin);
    
    return y;
}
template float BLUtils::NormalizedYTodB3(float y, float mindB, float maxdB);
template double BLUtils::NormalizedYTodB3(double y, double mindB, double maxdB);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::NormalizedYTodBInv(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB)
{
    FLOAT_TYPE result = y*(maxdB - mindB) + mindB;
    
    result = BLUtils::DBToAmp(result);
    
    return result;
}
template float BLUtils::NormalizedYTodBInv(float y, float mindB, float maxdB);
template double BLUtils::NormalizedYTodBInv(double y, double mindB, double maxdB);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::AverageYDB(FLOAT_TYPE y0, FLOAT_TYPE y1, FLOAT_TYPE mindB, FLOAT_TYPE maxdB)
{
    FLOAT_TYPE y0Norm = NormalizedYTodB(y0, mindB, maxdB);
    FLOAT_TYPE y1Norm = NormalizedYTodB(y1, mindB, maxdB);
    
    FLOAT_TYPE avg = (y0Norm + y1Norm)/2.0;
    
    FLOAT_TYPE result = NormalizedYTodBInv(avg, mindB, maxdB);
    
    return result;
}
template float BLUtils::AverageYDB(float y0, float y1,
                                   float mindB, float maxdB);
template double BLUtils::AverageYDB(double y0, double y1,
                                    double mindB, double maxdB);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeAvg(WDL_TypedBuf<FLOAT_TYPE> *avg,
                    const WDL_TypedBuf<FLOAT_TYPE> &values0,
                    const WDL_TypedBuf<FLOAT_TYPE> &values1)
{
    avg->Resize(values0.GetSize());
    
    int values0Size = values0.GetSize();
    FLOAT_TYPE *values0Data = values0.Get();
    FLOAT_TYPE *values1Data = values1.Get();
    FLOAT_TYPE *avgData = avg->Get();
    
#if USE_SIMD
    if (_useSimd && (values0Size % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < values0Size; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(values0Data);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(values1Data);
            
            FLOAT_TYPE c = 0.5;
            simdpp::float64<SIMD_PACK_SIZE> c0 = simdpp::load_splat(&c);
            
            simdpp::float64<SIMD_PACK_SIZE> r = (v0 + v1)*c0;
            
            simdpp::store(avgData, r);
            
            
            values0Data += SIMD_PACK_SIZE;
            values1Data += SIMD_PACK_SIZE;
            
            avgData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < values0Size; i++)
    {
        FLOAT_TYPE val0 = values0Data[i];
        FLOAT_TYPE val1 = values1Data[i];
        
        FLOAT_TYPE res = (val0 + val1)*0.5;
        
        avgData[i] = res;
    }
}
template void BLUtils::ComputeAvg(WDL_TypedBuf<float> *avg,
                                  const WDL_TypedBuf<float> &values0,
                                  const WDL_TypedBuf<float> &values1);
template void BLUtils::ComputeAvg(WDL_TypedBuf<double> *avg,
                                  const WDL_TypedBuf<double> &values0,
                                  const WDL_TypedBuf<double> &values1);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeAvg(FLOAT_TYPE *avg,
                    const FLOAT_TYPE *values0, const FLOAT_TYPE *values1,
                    int nFrames)
{
#if USE_SIMD
    if (_useSimd && (nFrames % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < nFrames; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(values0);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(values1);
            
            FLOAT_TYPE c = 0.5;
            simdpp::float64<SIMD_PACK_SIZE> c0 = simdpp::load_splat(&c);
            
            simdpp::float64<SIMD_PACK_SIZE> r = (v0 + v1)*c0;
            
            simdpp::store(avg, r);
            
            
            values0 += SIMD_PACK_SIZE;
            values1 += SIMD_PACK_SIZE;
            
            avg += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE val0 = values0[i];
        FLOAT_TYPE val1 = values1[i];
        
        FLOAT_TYPE res = (val0 + val1)*0.5;
        
        avg[i] = res;
    }
}
template void BLUtils::ComputeAvg(float *avg,
                                  const float *values0, const float *values1,
                                  int nFrames);
template void BLUtils::ComputeAvg(double *avg,
                                  const double *values0, const double *values1,
                                  int nFrames);

template <typename FLOAT_TYPE>
bool
BLUtils::IsAllZero(const WDL_TypedBuf<FLOAT_TYPE> &buffer)
{
    return IsAllZero(buffer.Get(), buffer.GetSize());
}
template bool BLUtils::IsAllZero(const WDL_TypedBuf<float> &buffer);
template bool BLUtils::IsAllZero(const WDL_TypedBuf<double> &buffer);

template <typename FLOAT_TYPE>
bool
BLUtils::IsAllZero(const FLOAT_TYPE *buffer, int nFrames)
{
    if (buffer == NULL)
        return true;

#if USE_SIMD
    if (_useSimd && (nFrames % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < nFrames; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buffer);
            
            simdpp::float64<SIMD_PACK_SIZE> a = simdpp::abs(v0);
            
            FLOAT_TYPE r = simdpp::reduce_max(a);
            if (r > BL_EPS)
                return false;
            
            buffer += SIMD_PACK_SIZE;
        }
        
        // Finished
        return true;
    }
#endif
    
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE val = std::fabs(buffer[i]);
        if (val > BL_EPS)
            return false;
    }
            
    return true;
}
template bool BLUtils::IsAllZero(const float *buffer, int nFrames);
template bool BLUtils::IsAllZero(const double *buffer, int nFrames);

template <typename FLOAT_TYPE>
bool
BLUtils::IsAllSmallerEps(const WDL_TypedBuf<FLOAT_TYPE> &buffer, FLOAT_TYPE eps)
{
    if (buffer.GetSize() == 0)
        return true;
    
    int bufferSize = buffer.GetSize();
    FLOAT_TYPE *bufferData = buffer.Get();
    
#if USE_SIMD
    if (_useSimd && (bufferSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufferSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufferData);
            
            simdpp::float64<SIMD_PACK_SIZE> a = simdpp::abs(v0);
            
            FLOAT_TYPE r = simdpp::reduce_max(a);
            if (r > eps)
                return false;
            
            bufferData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return true;
    }
#endif
    
    for (int i = 0; i < bufferSize; i++)
    {
        FLOAT_TYPE val = std::fabs(bufferData[i]);
        if (val > eps)
            return false;
    }
    
    return true;
}
template bool BLUtils::IsAllSmallerEps(const WDL_TypedBuf<float> &buffer,
                                       float eps);
template bool BLUtils::IsAllSmallerEps(const WDL_TypedBuf<double> &buffer,
                                       double eps);

// OPTIM PROF Infra
#if 0 // ORIGIN VERSION
void
BLUtils::FillAllZero(WDL_TypedBuf<FLOAT_TYPE> *ioBuf)
{
    int bufferSize = ioBuf->GetSize();
    FLOAT_TYPE *bufferData = ioBuf->Get();
    
    for (int i = 0; i < bufferSize; i++)
        bufferData[i] = 0.0;
}
#else // OPTIMIZED
template <typename FLOAT_TYPE>
void
BLUtils::FillAllZero(WDL_TypedBuf<FLOAT_TYPE> *ioBuf)
{
    memset(ioBuf->Get(), 0, ioBuf->GetSize()*sizeof(FLOAT_TYPE));
}
template void BLUtils::FillAllZero(WDL_TypedBuf<float> *ioBuf);
template void BLUtils::FillAllZero(WDL_TypedBuf<double> *ioBuf);
#endif

template <typename FLOAT_TYPE>
void
BLUtils::FillZero(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, int numZeros)
{
    if (numZeros > ioBuf->GetSize())
        numZeros = ioBuf->GetSize();
    
    memset(ioBuf->Get(), 0, numZeros*sizeof(FLOAT_TYPE));
}
template void BLUtils::FillZero(WDL_TypedBuf<float> *ioBuf, int numZeros);
template void BLUtils::FillZero(WDL_TypedBuf<double> *ioBuf, int numZeros);

// OPTIM PROF Infra
#if 0 // ORIGIN
void
BLUtils::FillAllZero(WDL_TypedBuf<int> *ioBuf)
{
    int bufferSize = ioBuf->GetSize();
    int *bufferData = ioBuf->Get();
    
    for (int i = 0; i < bufferSize; i++)
        bufferData[i] = 0;
}
#else // OPTIMIZED
void
BLUtils::FillAllZero(WDL_TypedBuf<int> *ioBuf)
{
    memset(ioBuf->Get(), 0, ioBuf->GetSize()*sizeof(int));
}
#endif

// OPTIM PROF Infra
#if 0 // ORIGIN
void
BLUtils::FillAllZero(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf)
{
    int bufferSize = ioBuf->GetSize();
    WDL_FFT_COMPLEX *bufferData = ioBuf->Get();
    
	for (int i = 0; i < bufferSize; i++)
	{
		bufferData[i].re = 0.0;
		bufferData[i].im = 0.0;
	}
}
#else // OPTIMIZED
void
BLUtils::FillAllZero(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf)
{
    memset(ioBuf->Get(), 0, ioBuf->GetSize()*sizeof(WDL_FFT_COMPLEX));
}
#endif

template <typename FLOAT_TYPE>
void
BLUtils::FillAllZero(FLOAT_TYPE *ioBuf, int size)
{
    memset(ioBuf, 0, size*sizeof(FLOAT_TYPE));
}
template void BLUtils::FillAllZero(float *ioBuf, int size);
template void BLUtils::FillAllZero(double *ioBuf, int size);

template <typename FLOAT_TYPE>
void
BLUtils::FillAllZero(vector<WDL_TypedBuf<FLOAT_TYPE> > *samples)
{
    for (int i = 0; i < samples->size(); i++)
    {
        FillAllZero(&(*samples)[i]);
    }
}
template void BLUtils::FillAllZero(vector<WDL_TypedBuf<float> > *samples);
template void BLUtils::FillAllZero(vector<WDL_TypedBuf<double> > *samples);

template <typename FLOAT_TYPE>
void
BLUtils::FillAllValue(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, FLOAT_TYPE val)
{
    int bufferSize = ioBuf->GetSize();
    FLOAT_TYPE *bufData = ioBuf->Get();
    
#if USE_SIMD
    if (_useSimd && (bufferSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufferSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> r =
                simdpp::splat<simdpp::float64<SIMD_PACK_SIZE> >(val);
            
            simdpp::store(bufData, r);
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufferSize; i++)
        bufData[i] = val;
}
template void BLUtils::FillAllValue(WDL_TypedBuf<float> *ioBuf, float val);
template void BLUtils::FillAllValue(WDL_TypedBuf<double> *ioBuf, double val);

template <typename FLOAT_TYPE>
void
BLUtils::AddZeros(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, int size)
{
    int prevSize = ioBuf->GetSize();
    int newSize = prevSize + size;
    
    ioBuf->Resize(newSize);
    
    FLOAT_TYPE *bufData = ioBuf->Get();
    
    for (int i = prevSize; i < newSize; i++)
        bufData[i] = 0.0;
}
template void BLUtils::AddZeros(WDL_TypedBuf<float> *ioBuf, int size);
template void BLUtils::AddZeros(WDL_TypedBuf<double> *ioBuf, int size);

template <typename FLOAT_TYPE>
void
BLUtils::StereoToMono(WDL_TypedBuf<FLOAT_TYPE> *monoResult,
                      const FLOAT_TYPE *in0, const FLOAT_TYPE *in1, int nFrames)
{
    if ((in0 == NULL) && (in1 == NULL))
        return;
    
    monoResult->Resize(nFrames);
    
    // Crashes with App mode, when using sidechain
    if ((in0 != NULL) && (in1 != NULL))
    {
        FLOAT_TYPE *monoResultBuf = monoResult->Get();
        
#if !USE_SIMD_OPTIM
        for (int i = 0; i < nFrames; i++)
            monoResultBuf[i] = (in0[i] + in1[i])/2.0;
#else
        BLUtils::ComputeAvg(monoResultBuf, in0, in1, nFrames);
#endif
    }
    else
    {
        FLOAT_TYPE *monoResultBuf = monoResult->Get();
    
#if !USE_SIMD_OPTIM
        for (int i = 0; i < nFrames; i++)
            monoResultBuf[i] = (in0 != NULL) ? in0[i] : in1[i];
#else
        if (in0 != NULL)
            memcpy(monoResultBuf, in0, nFrames*sizeof(FLOAT_TYPE));
        else
        {
            if (in1 != NULL)
                memcpy(monoResultBuf, in1, nFrames*sizeof(FLOAT_TYPE));
        }
#endif
    }
}
template void BLUtils::StereoToMono(WDL_TypedBuf<float> *monoResult,
                                    const float *in0, const float *in1, int nFrames);
template void BLUtils::StereoToMono(WDL_TypedBuf<double> *monoResult,
                                    const double *in0, const double *in1,
                                    int nFrames);


template <typename FLOAT_TYPE>
void
BLUtils::StereoToMono(WDL_TypedBuf<FLOAT_TYPE> *monoResult,
                      const WDL_TypedBuf<FLOAT_TYPE> &in0,
                      const WDL_TypedBuf<FLOAT_TYPE> &in1)
{
    if ((in0.GetSize() == 0) && (in1.GetSize() == 0))
        return;
    
    monoResult->Resize(in0.GetSize());
    
    // Crashes with App mode, when using sidechain
    if ((in0.GetSize() > 0) && (in1.GetSize() > 0))
    {
        FLOAT_TYPE *monoResultBuf = monoResult->Get();
        int in0Size = in0.GetSize();
        FLOAT_TYPE *in0Data = in0.Get();
        FLOAT_TYPE *in1Data = in1.Get();
        
#if !USE_SIMD_OPTIM
        for (int i = 0; i < in0Size; i++)
            monoResultBuf[i] = (in0Data[i] + in1Data[i])*0.5;
#else
        BLUtils::ComputeAvg(monoResultBuf, in0Data, in1Data, in0Size);
#endif
    }
    else
    {
        FLOAT_TYPE *monoResultBuf = monoResult->Get();
        int in0Size = in0.GetSize();
        FLOAT_TYPE *in0Data = in0.Get();
        FLOAT_TYPE *in1Data = in1.Get();
    
#if !USE_SIMD_OPTIM
        for (int i = 0; i < in0Size; i++)
            monoResultBuf[i] = (in0Size > 0) ? in0Data[i] : in1Data[i];
#else
        if (in0Size > 0)
            memcpy(monoResultBuf, in0Data, in0Size*sizeof(FLOAT_TYPE));
        else
            memcpy(monoResultBuf, in1Data, in0Size*sizeof(FLOAT_TYPE));
#endif
    }
}
template void BLUtils::StereoToMono(WDL_TypedBuf<float> *monoResult,
                                    const WDL_TypedBuf<float> &in0,
                                    const WDL_TypedBuf<float> &in1);
template void BLUtils::StereoToMono(WDL_TypedBuf<double> *monoResult,
                                    const WDL_TypedBuf<double> &in0,
                                    const WDL_TypedBuf<double> &in1);


template <typename FLOAT_TYPE>
void
BLUtils::StereoToMono(WDL_TypedBuf<FLOAT_TYPE> *monoResult,
                      const vector< WDL_TypedBuf<FLOAT_TYPE> > &in0)
{
    if (in0.empty())
        return;
    
    if (in0.size() == 1)
        *monoResult = in0[0];
    
    if (in0.size() == 2)
    {
        StereoToMono(monoResult, in0[0], in0[1]);
    }
}
template void BLUtils::StereoToMono(WDL_TypedBuf<float> *monoResult,
                                    const vector< WDL_TypedBuf<float> > &in0);
template void BLUtils::StereoToMono(WDL_TypedBuf<double> *monoResult,
                                    const vector< WDL_TypedBuf<double> > &in0);


template <typename FLOAT_TYPE>
void
BLUtils::StereoToMono(vector<WDL_TypedBuf<FLOAT_TYPE> > *samplesVec)
{
    // First, set to bi-mono channels
    if (samplesVec->size() == 1)
    {
        // Duplicate mono channel
        samplesVec->push_back((*samplesVec)[0]);
    }
    else if (samplesVec->size() == 2)
    {
        // Set to mono
        //WDL_TypedBuf<FLOAT_TYPE> mono;
        //BLUtils::StereoToMono(&mono, (*samplesVec)[0], (*samplesVec)[1]);
        //(*samplesVec)[0] = mono;
        //(*samplesVec)[1] = mono;
        
        for (int i = 0; i < (*samplesVec)[0].GetSize(); i++)
        {
            FLOAT_TYPE l = (*samplesVec)[0].Get()[i];
            FLOAT_TYPE r = (*samplesVec)[1].Get()[i];

            FLOAT_TYPE m = (l + r)*0.5;

            (*samplesVec)[0].Get()[i] = m;
            (*samplesVec)[1].Get()[i] = m;
        }
    }
}
template void BLUtils::StereoToMono(vector<WDL_TypedBuf<float> > *samplesVec);
template void BLUtils::StereoToMono(vector<WDL_TypedBuf<double> > *samplesVec);

void
BLUtils::StereoToMono(WDL_TypedBuf<WDL_FFT_COMPLEX> *monoResult,
                      const vector< WDL_TypedBuf<WDL_FFT_COMPLEX> > &in0)
{
    if (in0.empty())
        return;
    
    if (in0.size() == 1)
    {
        *monoResult = in0[0];
        
        return;
    }
    
    monoResult->Resize(in0[0].GetSize());
    for (int i = 0; i < in0[0].GetSize(); i++)
    {
        monoResult->Get()[i].re = 0.5*(in0[0].Get()[i].re + in0[1].Get()[i].re);
        monoResult->Get()[i].im = 0.5*(in0[0].Get()[i].im + in0[1].Get()[i].im);
    }
}

// Compute the similarity between two normalized curves
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeCurveMatchCoeff(const FLOAT_TYPE *curve0,
                                const FLOAT_TYPE *curve1,
                                int nFrames)
{
    // Use POW_COEFF, to make more difference between matching and out of matching
    // (added for EQHack)
#define POW_COEFF 4.0
    
    FLOAT_TYPE area = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE val0 = curve0[i];
        FLOAT_TYPE val1 = curve1[i];
        
        FLOAT_TYPE a = std::fabs(val0 - val1);
        
        a = std::pow(a, (FLOAT_TYPE)POW_COEFF);
        
        area += a;
    }
    
    FLOAT_TYPE coeff = area / nFrames;
    
    //
    coeff = std::pow(coeff, (FLOAT_TYPE)(1.0/POW_COEFF));
    
    // Should be Normalized
    
    // Clip, just in case.
    if (coeff < 0.0)
        coeff = 0.0;

    if (coeff > 1.0)
        coeff = 1.0;
    
    // If coeff is 1, this is a perfect match
    // so reverse
    coeff = 1.0 - coeff;
    
    return coeff;
}
template float BLUtils::ComputeCurveMatchCoeff(const float *curve0,
                                               const float *curve1, int nFrames);
template double BLUtils::ComputeCurveMatchCoeff(const double *curve0,
                                                const double *curve1, int nFrames);

// Compute the similarity between two normalized curves
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeCurveMatchCoeff2(const FLOAT_TYPE *curve0,
                                 const FLOAT_TYPE *curve1,
                                 int nFrames)
{
    // Use POW_COEFF, to make more difference between matching and out of matching
    // (added for EQHack)
#define POW_COEFF 4.0
    
    FLOAT_TYPE area = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE val0 = curve0[i];
        FLOAT_TYPE val1 = curve1[i];
        
        FLOAT_TYPE a = std::fabs(val0 - val1);
        
        //a = std::pow(a, (FLOAT_TYPE)POW_COEFF);
        
        area += a;
    }
    
    FLOAT_TYPE coeff = area / nFrames;
    
    //
    //coeff = std::pow(coeff, (FLOAT_TYPE)(1.0/POW_COEFF));
    
    // Should be Normalized
    
    // Clip, just in case.
    if (coeff < 0.0)
        coeff = 0.0;
    if (coeff > 1.0)
        coeff = 1.0;
    
    // If coeff is 1, this is a perfect match
    // so reverse
    coeff = 1.0 - coeff;

    // "rescale"

    // Measured some points empirically => they are aligned!
    // 
    // Find equation: y = ax + b
    BL_FLOAT p0[2] = { 0.9874, 0.0 };
    BL_FLOAT p1[2] = { 0.999997, 1.0 };
    BL_FLOAT a = (p1[1] - p0[1])/(p1[0] - p0[0]);
    BL_FLOAT b = p0[1] - a*p0[0];

    // Apply to coeff, to have coeff in [0, 1]
    coeff = a*coeff + b;
    
    // Clip again, just in case.
    if (coeff < 0.0)
        coeff = 0.0;
    if (coeff > 1.0)
        coeff = 1.0;
    
    return coeff;
}
template float BLUtils::ComputeCurveMatchCoeff2(const float *curve0,
                                                const float *curve1, int nFrames);
template double BLUtils::ComputeCurveMatchCoeff2(const double *curve0,
                                                 const double *curve1, int nFrames);

template <typename FLOAT_TYPE>
void
BLUtils::AntiClipping(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE maxValue)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesBuf = values->Get();
    
#if USE_SIMD
    if (_useSimd && (valuesSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < valuesSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(valuesBuf);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load_splat(&maxValue);
            
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::min(v0, v1);
            
            simdpp::store(valuesBuf, r);
            
            valuesBuf += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesBuf[i];
        
        if (val > maxValue)
            val = maxValue;
        
        valuesBuf[i] = val;
    }
}
template void BLUtils::AntiClipping(WDL_TypedBuf<float> *values, float maxValue);
template void BLUtils::AntiClipping(WDL_TypedBuf<double> *values, double maxValue);

template <typename FLOAT_TYPE>
void
BLUtils::SamplesAntiClipping(WDL_TypedBuf<FLOAT_TYPE> *samples, FLOAT_TYPE maxValue)
{
    int samplesSize = samples->GetSize();
    FLOAT_TYPE *samplesData = samples->Get();
    
#if USE_SIMD
    if (_useSimd && (samplesSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < samplesSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(samplesData);
            simdpp::float64<SIMD_PACK_SIZE> m0 = simdpp::load_splat(&maxValue);
            
            FLOAT_TYPE minValue = -maxValue;
            simdpp::float64<SIMD_PACK_SIZE> m1 = simdpp::load_splat(&minValue);
            
            simdpp::float64<SIMD_PACK_SIZE> r0 = simdpp::min(v0, m0);
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::max(r0, m1);
            
            simdpp::store(samplesData, r);
            
            samplesData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < samplesSize; i++)
    {
        FLOAT_TYPE val = samplesData[i];
        
        if (val > maxValue)
            val = maxValue;
        
        if (val < -maxValue)
            val = -maxValue;
        
        samplesData[i] = val;
    }
}
template void BLUtils::SamplesAntiClipping(WDL_TypedBuf<float> *samples,
                                           float maxValue);
template void BLUtils::SamplesAntiClipping(WDL_TypedBuf<double> *samples,
                                           double maxValue);

template <typename FLOAT_TYPE>
void
BLUtils::AppendValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer,
                      const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    if (ioBuffer->GetSize() == 0)
    {
        *ioBuffer = values;
        return;
    }
    
    ioBuffer->Add(values.Get(), values.GetSize());
}
template void BLUtils::AppendValues(WDL_TypedBuf<float> *ioBuffer,
                                    const WDL_TypedBuf<float> &values);
template void BLUtils::AppendValues(WDL_TypedBuf<double> *ioBuffer,
                                    const WDL_TypedBuf<double> &values);

#if 0 // ORIG
template <typename FLOAT_TYPE>
void
BLUtils::ConsumeLeft(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int numToConsume)
{
    int newSize = ioBuffer->GetSize() - numToConsume;
    if (newSize <= 0)
    {
        ioBuffer->Resize(0);
        
        return;
    }
    
    // Resize down, skipping left
    WDL_TypedBuf<FLOAT_TYPE> tmpChunk;
    tmpChunk.Add(&ioBuffer->Get()[numToConsume], newSize);
    *ioBuffer = tmpChunk;
}
template void BLUtils::ConsumeLeft(WDL_TypedBuf<float> *ioBuffer, int numToConsume);
template void BLUtils::ConsumeLeft(WDL_TypedBuf<double> *ioBuffer, int numToConsume);
#endif

#if 0 // 1 // OPTIM
// NOTE: this is risky
template <typename FLOAT_TYPE>
void
BLUtils::ConsumeLeft(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int numToConsume)
{
    int newSize = ioBuffer->GetSize() - numToConsume;
    if (newSize <= 0)
    {
        ioBuffer->Resize(0);
        
        return;
    }
    
    // This looks to work, but it is very risky,
    // memcopy from a buffer to the same buffer...
    memcpy(ioBuffer->Get(), &ioBuffer->Get()[numToConsume],
           newSize*sizeof(FLOAT_TYPE));
    ioBuffer->Resize(newSize);
}
template void BLUtils::ConsumeLeft(WDL_TypedBuf<float> *ioBuffer, int numToConsume);
template void BLUtils::ConsumeLeft(WDL_TypedBuf<double> *ioBuffer, int numToConsume);
#endif

#if 1 // Less OPTIM, less risky
template <typename FLOAT_TYPE>
void
BLUtils::ConsumeLeft(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int numToConsume)
{
    int newSize = ioBuffer->GetSize() - numToConsume;
    if (newSize <= 0)
    {
        ioBuffer->Resize(0);
        
        return;
    }
    
    WDL_TypedBuf<FLOAT_TYPE> result;
    result.Resize(newSize);
    
    memcpy(result.Get(), &ioBuffer->Get()[numToConsume], newSize*sizeof(FLOAT_TYPE));
    
    *ioBuffer = result;
}
template void BLUtils::ConsumeLeft(WDL_TypedBuf<float> *ioBuffer, int numToConsume);
template void BLUtils::ConsumeLeft(WDL_TypedBuf<double> *ioBuffer, int numToConsume);
#endif

template <typename FLOAT_TYPE>
void
BLUtils::ConsumeRight(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int numToConsume)
{
    int size = ioBuffer->GetSize();
    
    int newSize = size - numToConsume;
    if (newSize < 0)
        newSize = 0;
    
    ioBuffer->Resize(newSize);
}
template void BLUtils::ConsumeRight(WDL_TypedBuf<float> *ioBuffer, int numToConsume);
template void BLUtils::ConsumeRight(WDL_TypedBuf<double> *ioBuffer, int numToConsume);

#if 0 // Not optimized
template <typename FLOAT_TYPE>
void
BLUtils::ConsumeLeft(vector<WDL_TypedBuf<FLOAT_TYPE> > *ioBuffer)
{
    if (ioBuffer->empty())
        return;
    
    vector<WDL_TypedBuf<FLOAT_TYPE> > copy = *ioBuffer;
    
    ioBuffer->resize(0);
    for (int i = 1; i < copy.size(); i++)
    {
        WDL_TypedBuf<FLOAT_TYPE> &buf = copy[i];
        ioBuffer->push_back(buf);
    }
}
template void BLUtils::ConsumeLeft(vector<WDL_TypedBuf<float> > *ioBuffer);
template void BLUtils::ConsumeLeft(vector<WDL_TypedBuf<double> > *ioBuffer);
#endif
#if 1 // Optimized
template <typename FLOAT_TYPE>
void
BLUtils::ConsumeLeft(vector<WDL_TypedBuf<FLOAT_TYPE> > *ioBuffer)
{
    if (ioBuffer->empty())
        return;
    
    ioBuffer->erase(ioBuffer->begin());
}
template void BLUtils::ConsumeLeft(vector<WDL_TypedBuf<float> > *ioBuffer);
template void BLUtils::ConsumeLeft(vector<WDL_TypedBuf<double> > *ioBuffer);
#endif

template <typename FLOAT_TYPE>
void
BLUtils::TakeHalf(WDL_TypedBuf<FLOAT_TYPE> *buf)
{
    int halfSize = buf->GetSize() / 2;
    
    buf->Resize(halfSize);
}
template void BLUtils::TakeHalf(WDL_TypedBuf<float> *buf);
template void BLUtils::TakeHalf(WDL_TypedBuf<double> *buf);

void
BLUtils::TakeHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf)
{
    int halfSize = buf->GetSize() / 2;
    
    buf->Resize(halfSize);
}

template <typename FLOAT_TYPE>
void
BLUtils::TakeHalf(const WDL_TypedBuf<FLOAT_TYPE> &inBuf,
                  WDL_TypedBuf<FLOAT_TYPE> *outBuf)
{
    int halfSize = inBuf.GetSize() / 2;
    outBuf->Resize(halfSize);

    memcpy(outBuf->Get(), inBuf.Get(), halfSize*sizeof(FLOAT_TYPE));
}
template void BLUtils::TakeHalf(const WDL_TypedBuf<float> &inBuf,
                                WDL_TypedBuf<float> *outBuf);
template void BLUtils::TakeHalf(const WDL_TypedBuf<double> &inBuf,
                                WDL_TypedBuf<double> *outBuf);

void
BLUtils::TakeHalf(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inBuf,
                  WDL_TypedBuf<WDL_FFT_COMPLEX> *outBuf)
{
    int halfSize = inBuf.GetSize() / 2;
    outBuf->Resize(halfSize);

    memcpy(outBuf->Get(), inBuf.Get(), halfSize*sizeof(WDL_FFT_COMPLEX));
}

#if 0 // not optimal
void
BLUtils::TakeHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *res,
                  const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf)
{
    int halfSize = buf.GetSize() / 2;
    
    res->Resize(0);
    res->Add(buf.Get(), halfSize);
}
#endif

// OPTIM PROF Infra
#if 0 // ORIGIN
void
BLUtils::ResizeFillZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    if (newSize <= prevSize)
        return;
    
    for (int i = prevSize; i < newSize; i++)
    {
        if (i >= buf->GetSize())
            break;
        
        buf->Get()[i] = 0.0;
    }
}
#else // Optimized
template <typename FLOAT_TYPE>
void
BLUtils::ResizeFillZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    if (newSize <= prevSize)
        return;
    
    memset(&buf->Get()[prevSize], 0, (newSize - prevSize)*sizeof(FLOAT_TYPE));
}
template void BLUtils::ResizeFillZeros(WDL_TypedBuf<float> *buf, int newSize);
template void BLUtils::ResizeFillZeros(WDL_TypedBuf<double> *buf, int newSize);
#endif

template <typename FLOAT_TYPE>
void
BLUtils::ResizeFillAllZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    if (newSize <= prevSize)
        return;
    
    memset(buf->Get(), 0, buf->GetSize()*sizeof(FLOAT_TYPE));
}
template void BLUtils::ResizeFillAllZeros(WDL_TypedBuf<float> *buf, int newSize);
template void BLUtils::ResizeFillAllZeros(WDL_TypedBuf<double> *buf, int newSize);

template <typename FLOAT_TYPE>
void
BLUtils::ResizeFillValue(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize, FLOAT_TYPE value)
{
    buf->Resize(newSize);
    FillAllValue(buf, value);
}
template void BLUtils::ResizeFillValue(WDL_TypedBuf<float> *buf,
                                       int newSize, float value);
template void BLUtils::ResizeFillValue(WDL_TypedBuf<double> *buf,
                                       int newSize, double value);

template <typename FLOAT_TYPE>
void
BLUtils::ResizeFillValue2(WDL_TypedBuf<FLOAT_TYPE> *buf,
                          int newSize, FLOAT_TYPE value)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    FLOAT_TYPE *bufData = buf->Get();
    
    for (int i = prevSize; i < newSize; i++)
        bufData[i] = value;
}
template void BLUtils::ResizeFillValue2(WDL_TypedBuf<float> *buf,
                                        int newSize, float value);
template void BLUtils::ResizeFillValue2(WDL_TypedBuf<double> *buf,
                                        int newSize, double value);

template <typename FLOAT_TYPE>
void
BLUtils::ResizeFillRandom(WDL_TypedBuf<FLOAT_TYPE> *buf,
                          int newSize, FLOAT_TYPE coeff)
{
    buf->Resize(newSize);
    
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE r = ((FLOAT_TYPE)std::rand())/RAND_MAX;
        
        FLOAT_TYPE newVal = r*coeff;
        
        bufData[i] = newVal;
    }
}
template void BLUtils::ResizeFillRandom(WDL_TypedBuf<float> *buf,
                                        int newSize, float coeff);
template void BLUtils::ResizeFillRandom(WDL_TypedBuf<double> *buf,
                                        int newSize, double coeff);

void
BLUtils::ResizeFillZeros(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, int newSize)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    if (newSize <= prevSize)
        return;
    
    WDL_FFT_COMPLEX *bufData = buf->Get();
    
    for (int i = prevSize; i < newSize; i++)
    {
        bufData[i].re = 0.0;
        bufData[i].im = 0.0;
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::GrowFillZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int numGrow)
{
    int newSize = buf->GetSize() + numGrow;
    
    ResizeFillZeros(buf, newSize);
}
template void BLUtils::GrowFillZeros(WDL_TypedBuf<float> *buf, int numGrow);
template void BLUtils::GrowFillZeros(WDL_TypedBuf<double> *buf, int numGrow);

template <typename FLOAT_TYPE>
void
BLUtils::InsertZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int index, int numZeros)
{
    WDL_TypedBuf<FLOAT_TYPE> result;
    result.Resize(buf->GetSize() + numZeros);
    BLUtils::FillAllZero(&result);
    
    //
    if (buf->GetSize() < index)
    {
        *buf = result;
        
        return;
    }
        
    // Before zeros
    memcpy(result.Get(), buf->Get(), index*sizeof(FLOAT_TYPE));
    
    // After zeros
    memcpy(&result.Get()[index + numZeros], &buf->Get()[index],
           (buf->GetSize() - index)*sizeof(FLOAT_TYPE));
    
    *buf = result;
}
template void BLUtils::InsertZeros(WDL_TypedBuf<float> *buf,
                                   int index, int numZeros);
template void BLUtils::InsertZeros(WDL_TypedBuf<double> *buf,
                                   int index, int numZeros);

template <typename FLOAT_TYPE>
void
BLUtils::InsertValues(WDL_TypedBuf<FLOAT_TYPE> *buf, int index,
                      int numValues, FLOAT_TYPE value)
{
    for (int i = 0; i < numValues; i++)
        buf->Insert(value, index);
}
template void BLUtils::InsertValues(WDL_TypedBuf<float> *buf, int index,
                                    int numValues, float value);
template void BLUtils::InsertValues(WDL_TypedBuf<double> *buf, int index,
                                    int numValues, double value);

// BUGGY
template <typename FLOAT_TYPE>
void
BLUtils::RemoveValuesCyclic(WDL_TypedBuf<FLOAT_TYPE> *buf, int index, int numValues)
{
    // Remove too many => empty the result
    if (numValues >= buf->GetSize())
    {
        buf->Resize(0);
        
        return;
    }
    
    // Prepare the result with the new size
    WDL_TypedBuf<FLOAT_TYPE> result;
    result.Resize(buf->GetSize() - numValues);
    BLUtils::FillAllZero(&result);
    
    // Copy cyclicly
    int resultSize = result.GetSize();
    FLOAT_TYPE *resultData = result.Get();
    
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    
    for (int i = 0; i < resultSize; i++)
    {
        int idx = index + i;
        idx = idx % bufSize;
        
        FLOAT_TYPE val = bufData[idx];
        resultData[i] = val;
    }
    
    *buf = result;
}
template void BLUtils::RemoveValuesCyclic(WDL_TypedBuf<float> *buf,
                                          int index, int numValues);
template void BLUtils::RemoveValuesCyclic(WDL_TypedBuf<double> *buf,
                                          int index, int numValues);

// Remove before and until index
template <typename FLOAT_TYPE>
void
BLUtils::RemoveValuesCyclic2(WDL_TypedBuf<FLOAT_TYPE> *buf, int index, int numValues)
{
    // Remove too many => empty the result
    if (numValues >= buf->GetSize())
    {
        buf->Resize(0);
        
        return;
    }
    
    // Manage negative index
    if (index < 0)
        index += buf->GetSize();
    
    // Prepare the result with the new size
    WDL_TypedBuf<FLOAT_TYPE> result;
    result.Resize(buf->GetSize() - numValues);
    BLUtils::FillAllZero(&result); // Just in case
    
    // Copy cyclicly
    int bufPos = index + 1;
    int resultPos = index + 1 - numValues;
    if (resultPos < 0)
        resultPos += result.GetSize();
    
    int resultSize = result.GetSize();
    FLOAT_TYPE *resultData = result.Get();
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    
    for (int i = 0; i < resultSize; i++)
    {
        bufPos = bufPos % bufSize;
        resultPos = resultPos % resultSize;
        
        FLOAT_TYPE val = bufData[bufPos];
        resultData[resultPos] = val;
        
        bufPos++;
        resultPos++;
    }
    
    *buf = result;
}
template void BLUtils::RemoveValuesCyclic2(WDL_TypedBuf<float> *buf,
                                           int index, int numValues);
template void BLUtils::RemoveValuesCyclic2(WDL_TypedBuf<double> *buf,
                                           int index, int numValues);

template <typename FLOAT_TYPE>
void
BLUtils::AddValues(WDL_TypedBuf<FLOAT_TYPE> *buf, FLOAT_TYPE value)
{
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load_splat(&value);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0 + v1;
            
            simdpp::store(bufData, r);
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        bufData[i] += value;
    }
}
template void BLUtils::AddValues(WDL_TypedBuf<float> *buf, float value);
template void BLUtils::AddValues(WDL_TypedBuf<double> *buf, double value);

template <typename FLOAT_TYPE>
void
BLUtils::AddValues(WDL_TypedBuf<FLOAT_TYPE> *result,
                   const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                   const WDL_TypedBuf<FLOAT_TYPE> &buf1)
{
    result->Resize(buf0.GetSize());
    
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    FLOAT_TYPE *buf0Data = buf0.Get();
    FLOAT_TYPE *buf1Data = buf1.Get();
    
#if USE_SIMD
    if (_useSimd && (resultSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < resultSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf0Data);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(buf1Data);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0 + v1;
            
            simdpp::store(resultData, r);
            
            buf0Data += SIMD_PACK_SIZE;
            buf1Data += SIMD_PACK_SIZE;
            resultData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < resultSize; i++)
    {
        FLOAT_TYPE a = buf0Data[i];
        FLOAT_TYPE b = buf1Data[i];
        
        FLOAT_TYPE sum = a + b;
        
        resultData[i] = sum;
    }
}
template void BLUtils::AddValues(WDL_TypedBuf<float> *result,
                                 const WDL_TypedBuf<float> &buf0,
                                 const WDL_TypedBuf<float> &buf1);
template void BLUtils::AddValues(WDL_TypedBuf<double> *result,
                                 const WDL_TypedBuf<double> &buf0,
                                 const WDL_TypedBuf<double> &buf1);

template <typename FLOAT_TYPE>
void
BLUtils::AddValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, const FLOAT_TYPE *addBuf)
{
    int bufSize = ioBuf->GetSize();
    FLOAT_TYPE *bufData = ioBuf->Get();
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(addBuf);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0 + v1;
            
            simdpp::store(bufData, r);
            
            bufData += SIMD_PACK_SIZE;
            addBuf += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        bufData[i] += addBuf[i];
    }
}
template void BLUtils::AddValues(WDL_TypedBuf<float> *ioBuf, const float *addBuf);
template void BLUtils::AddValues(WDL_TypedBuf<double> *ioBuf, const double *addBuf);

template <typename FLOAT_TYPE>
void
BLUtils::AddValues(FLOAT_TYPE *ioBuf, const FLOAT_TYPE *addBuf, int bufSize)
{
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(ioBuf);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(addBuf);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0 + v1;
            
            simdpp::store(ioBuf, r);
            
            ioBuf += SIMD_PACK_SIZE;
            addBuf += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        ioBuf[i] += addBuf[i];
    }
}
template void BLUtils::AddValues(float *ioBuf, const float *addBuf, int bufSize);
template void BLUtils::AddValues(double *ioBuf, const double *addBuf, int bufSize);

template <typename FLOAT_TYPE>
void
BLUtils::MultValues(WDL_TypedBuf<FLOAT_TYPE> *buf, FLOAT_TYPE value)
{
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load_splat(&value);
        
            simdpp::float64<SIMD_PACK_SIZE> r = v0*v1;
        
            simdpp::store(bufData, r);
            
            bufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    // Normal version
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE v = bufData[i];
        v *= value;
        bufData[i] = v;
    }
}
template void BLUtils::MultValues(WDL_TypedBuf<float> *buf, float value);
template void BLUtils::MultValues(WDL_TypedBuf<double> *buf, double value);

#if !USE_SIMD_OPTIM
void
BLUtils::MultValues(vector<WDL_TypedBuf<FLOAT_TYPE> > *buf, FLOAT_TYPE value)
{
    for (int i = 0; i < buf->size(); i++)
    {
        for (int j = 0; j < (*buf)[i].GetSize(); j++)
        {
            FLOAT_TYPE v = (*buf)[i].Get()[j];
            
            v *= value;
            
            (*buf)[i].Get()[j] = v;
        }
    }
}
#else
template <typename FLOAT_TYPE>
void
BLUtils::MultValues(vector<WDL_TypedBuf<FLOAT_TYPE> > *buf, FLOAT_TYPE value)
{
    for (int i = 0; i < buf->size(); i++)
    {
        BLUtils::MultValues(&(*buf)[i], value);
    }
}
template void BLUtils::MultValues(vector<WDL_TypedBuf<float> > *buf, float value);
template void BLUtils::MultValues(vector<WDL_TypedBuf<double> > *buf, double value);
#endif

// OPTIM PROF Infra
#if 0 // ORIGIN
void
BLUtils::MultValuesRamp(WDL_TypedBuf<FLOAT_TYPE> *buf,
                        FLOAT_TYPE value0, FLOAT_TYPE value1)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        FLOAT_TYPE t = ((FLOAT_TYPE)i)/(buf->GetSize() - 1);
        FLOAT_TYPE value = (1.0 - t)*value0 + t*value1;
        
        FLOAT_TYPE v = buf->Get()[i];
        v *= value;
        buf->Get()[i] = v;
    }
}
#else // OPTIMIZED
template <typename FLOAT_TYPE>
void
BLUtils::MultValuesRamp(WDL_TypedBuf<FLOAT_TYPE> *buf,
                        FLOAT_TYPE value0, FLOAT_TYPE value1)
{
    //FLOAT_TYPE step = 0.0;
    //if (buf->GetSize() >= 2)
    //    step = 1.0/(buf->GetSize() - 1);
    //FLOAT_TYPE t = 0.0;
    FLOAT_TYPE value = value0;
    FLOAT_TYPE step = 0.0;
    if (buf->GetSize() >= 2)
        step = (value1 - value0)/(buf->GetSize() - 1);
    
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE v = bufData[i];
        v *= value;
        bufData[i] = v;
        
        value += step;
    }
}
template void BLUtils::MultValuesRamp(WDL_TypedBuf<float> *buf,
                                      float value0, float value1);
template void BLUtils::MultValuesRamp(WDL_TypedBuf<double> *buf,
                                      double value0, double value1);
#endif

template <typename FLOAT_TYPE>
void
BLUtils::MultValues(FLOAT_TYPE *buf, int size, FLOAT_TYPE value)
{
#if USE_SIMD
    if (_useSimd && (size % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < size; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load_splat(&value);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0*v1;
            
            simdpp::store(buf, r);
            
            buf += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < size; i++)
    {
        FLOAT_TYPE v = buf[i];
        v *= value;
        buf[i] = v;
    }
}
template void BLUtils::MultValues(float *buf, int size, float value);
template void BLUtils::MultValues(double *buf, int size, double value);

template <typename FLOAT_TYPE>
void
BLUtils::MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, FLOAT_TYPE value)
{
    int bufSize = buf->GetSize();
    WDL_FFT_COMPLEX *bufData = buf->Get();
    
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE/2 == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE/2)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load_splat(&value);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0*v1;
            
            simdpp::store(bufData, r);
            
            bufData += SIMD_PACK_SIZE/2;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX &v = bufData[i];
        v.re *= value;
        v.im *= value;
    }
}
template void BLUtils::MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, float value);
template void BLUtils::MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, double value);

template <typename FLOAT_TYPE>
void
BLUtils::MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                    const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    if (buf->GetSize() != values.GetSize())
        return;
    
    int bufSize = buf->GetSize();
    WDL_FFT_COMPLEX *bufData = buf->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE value = values.Get()[i];
        
        WDL_FFT_COMPLEX &v = bufData[i];
        v.re *= value;
        v.im *= value;
    }
}
template void BLUtils::MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                                  const WDL_TypedBuf<float> &values);
template void BLUtils::MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                                  const WDL_TypedBuf<double> &values);

void
BLUtils::MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                    const WDL_TypedBuf<WDL_FFT_COMPLEX> &values)
{
    if (buf->GetSize() != values.GetSize())
        return;
    
    int bufSize = buf->GetSize();
    WDL_FFT_COMPLEX *bufData = buf->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX &bv = bufData[i];
        
        WDL_FFT_COMPLEX value = values.Get()[i];

        WDL_FFT_COMPLEX res;
        COMP_MULT(bv, value, res);
        
        bv = res;
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::ApplyPow(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE exp)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];

        val = std::pow(val, exp);
        
        valuesData[i] = val;
    }
}
template void BLUtils::ApplyPow(WDL_TypedBuf<float> *values, float exp);
template void BLUtils::ApplyPow(WDL_TypedBuf<double> *values, double exp);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyExp(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        val = std::exp(val);
        
        valuesData[i] = val;
    }
}
template void BLUtils::ApplyExp(WDL_TypedBuf<float> *values);
template void BLUtils::ApplyExp(WDL_TypedBuf<double> *values);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyLog(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        val = std::log(val);
        
        valuesData[i] = val;
    }
}
template void BLUtils::ApplyLog(WDL_TypedBuf<float> *values);
template void BLUtils::ApplyLog(WDL_TypedBuf<double> *values);

template <typename FLOAT_TYPE>
void
BLUtils::MultValues(WDL_TypedBuf<FLOAT_TYPE> *buf,
                    const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    int bufSize = buf->GetSize();
    FLOAT_TYPE *bufData = buf->Get();
    FLOAT_TYPE *valuesData = values.Get();
 
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(bufData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(valuesData);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0*v1;
            
            simdpp::store(bufData, r);
            
            bufData += SIMD_PACK_SIZE;
            valuesData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        bufData[i] *= val;
    }
}
template void BLUtils::MultValues(WDL_TypedBuf<float> *buf,
                                  const WDL_TypedBuf<float> &values);
template void BLUtils::MultValues(WDL_TypedBuf<double> *buf,
                                  const WDL_TypedBuf<double> &values);

template <typename FLOAT_TYPE>
void
BLUtils::PadZerosLeft(WDL_TypedBuf<FLOAT_TYPE> *buf, int padSize)
{
    if (padSize == 0)
        return;
    
    WDL_TypedBuf<FLOAT_TYPE> newBuf;
    newBuf.Resize(buf->GetSize() + padSize);
    
    memset(newBuf.Get(), 0, padSize*sizeof(FLOAT_TYPE));
    
    memcpy(&newBuf.Get()[padSize], buf->Get(), buf->GetSize()*sizeof(FLOAT_TYPE));
    
    *buf = newBuf;
}
template void BLUtils::PadZerosLeft(WDL_TypedBuf<float> *buf, int padSize);
template void BLUtils::PadZerosLeft(WDL_TypedBuf<double> *buf, int padSize);

template <typename FLOAT_TYPE>
void
BLUtils::PadZerosRight(WDL_TypedBuf<FLOAT_TYPE> *buf, int padSize)
{
    if (padSize == 0)
        return;
    
    long prevSize = buf->GetSize();
    buf->Resize(prevSize + padSize);
    
    memset(&buf->Get()[prevSize], 0, padSize*sizeof(FLOAT_TYPE));
}
template void BLUtils::PadZerosRight(WDL_TypedBuf<float> *buf, int padSize);
template void BLUtils::PadZerosRight(WDL_TypedBuf<double> *buf, int padSize);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::Interp(FLOAT_TYPE val0, FLOAT_TYPE val1, FLOAT_TYPE t)
{
    FLOAT_TYPE res = (1.0 - t)*val0 + t*val1;
    
    return res;
}
template float BLUtils::Interp(float val0, float val1, float t);
template double BLUtils::Interp(double val0, double val1, double t);

template <typename FLOAT_TYPE>
void
BLUtils::Interp(WDL_TypedBuf<FLOAT_TYPE> *result,
                const WDL_TypedBuf<FLOAT_TYPE> *buf0,
                const WDL_TypedBuf<FLOAT_TYPE> *buf1,
                FLOAT_TYPE t)
{
    result->Resize(buf0->GetSize());
    
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    FLOAT_TYPE *buf0Data = buf0->Get();
    FLOAT_TYPE *buf1Data = buf1->Get();
    
    for (int i = 0; i < resultSize; i++)
    {
        FLOAT_TYPE val0 = buf0Data[i];
        FLOAT_TYPE val1 = buf1Data[i];
        
        FLOAT_TYPE res = (1.0 - t)*val0 + t*val1;
        
        resultData[i] = res;
    }
}
template void BLUtils::Interp(WDL_TypedBuf<float> *result,
                              const WDL_TypedBuf<float> *buf0,
                              const WDL_TypedBuf<float> *buf1,
                              float t);
template void BLUtils::Interp(WDL_TypedBuf<double> *result,
                              const WDL_TypedBuf<double> *buf0,
                              const WDL_TypedBuf<double> *buf1,
                              double t);


template <typename FLOAT_TYPE>
void
BLUtils::Interp(WDL_TypedBuf<WDL_FFT_COMPLEX> *result,
                const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf0,
                const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf1,
                FLOAT_TYPE t)
{
    result->Resize(buf0->GetSize());
    
    int resultSize = result->GetSize();
    WDL_FFT_COMPLEX *resultData = result->Get();
    WDL_FFT_COMPLEX *buf0Data = buf0->Get();
    WDL_FFT_COMPLEX *buf1Data = buf1->Get();
    
    for (int i = 0; i < resultSize; i++)
    {
        WDL_FFT_COMPLEX val0 = buf0Data[i];
        WDL_FFT_COMPLEX val1 = buf1Data[i];
        
        WDL_FFT_COMPLEX res;
        
        res.re = (1.0 - t)*val0.re + t*val1.re;
        res.im = (1.0 - t)*val0.im + t*val1.im;
        
        resultData[i] = res;
    }
}
template void BLUtils::Interp(WDL_TypedBuf<WDL_FFT_COMPLEX> *result,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf0,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf1,
                              float t);
template void BLUtils::Interp(WDL_TypedBuf<WDL_FFT_COMPLEX> *result,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf0,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf1,
                              double t);

template <typename FLOAT_TYPE>
void
BLUtils::Interp2D(WDL_TypedBuf<FLOAT_TYPE> *result,
                  const WDL_TypedBuf<FLOAT_TYPE> bufs[2][2],
                  FLOAT_TYPE u, FLOAT_TYPE v)
{
    WDL_TypedBuf<FLOAT_TYPE> bufv0;
    Interp(&bufv0, &bufs[0][0], &bufs[1][0], u);
    
    WDL_TypedBuf<FLOAT_TYPE> bufv1;
    Interp(&bufv1, &bufs[0][1], &bufs[1][1], u);
    
    Interp(result, &bufv0, &bufv1, v);
}
template void BLUtils::Interp2D(WDL_TypedBuf<float> *result,
                                const WDL_TypedBuf<float> bufs[2][2],
                                float u, float v);
template void BLUtils::Interp2D(WDL_TypedBuf<double> *result,
                                const WDL_TypedBuf<double> bufs[2][2],
                                double u, double v);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeAvg(WDL_TypedBuf<FLOAT_TYPE> *result,
                    const vector<WDL_TypedBuf<FLOAT_TYPE> > &bufs)
{
    if (bufs.empty())
        return;
    
    result->Resize(bufs[0].GetSize());
    
#if !USE_SIMD_OPTIM
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    for (int i = 0; i < resultSize; i++)
    {
        FLOAT_TYPE val = 0.0;
        for (int j = 0; j < bufs.size(); j++)
        {
            val += bufs[j].Get()[i];
        }
        
        val /= bufs.size();
        
        resultData[i] = val;
    }
#else
    BLUtils::FillAllZero(result);
    for (int j = 0; j < bufs.size(); j++)
    {
        BLUtils::AddValues(result, bufs[j]);
    }
    
    FLOAT_TYPE coeff = 1.0/bufs.size();
    BLUtils::MultValues(result, coeff);
#endif
}
template void BLUtils::ComputeAvg(WDL_TypedBuf<float> *result,
                                  const vector<WDL_TypedBuf<float> > &bufs);
template void BLUtils::ComputeAvg(WDL_TypedBuf<double> *result,
                                  const vector<WDL_TypedBuf<double> > &bufs);

template <typename FLOAT_TYPE>
void
BLUtils::Mix(FLOAT_TYPE *output,
             FLOAT_TYPE *buf0, FLOAT_TYPE *buf1, int nFrames, FLOAT_TYPE mix)
{
#if USE_SIMD
    if (_useSimd && (nFrames % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < nFrames; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf0);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(buf1);
            
            FLOAT_TYPE c0 = (1.0 - mix);
            FLOAT_TYPE c1 = mix;
            
            simdpp::float64<SIMD_PACK_SIZE> cc0 = simdpp::load_splat(&c0);
            simdpp::float64<SIMD_PACK_SIZE> cc1 = simdpp::load_splat(&c1);
            
            simdpp::float64<SIMD_PACK_SIZE> r = cc0*v0 + cc1*v1;
            
            simdpp::store(output, r);
            
            buf0 += SIMD_PACK_SIZE;
            buf1 += SIMD_PACK_SIZE;
            output += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE val = (1.0 - mix)*buf0[i] + mix*buf1[i];
        
        output[i] = val;
    }
}
template void BLUtils::Mix(float *output, float *buf0, float *buf1,
                           int nFrames, float mix);
template void BLUtils::Mix(double *output, double *buf0, double *buf1,
                           int nFrames, double mix);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::AmpToDB(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    FLOAT_TYPE result = minDB;
    FLOAT_TYPE absSample = std::fabs(sampleVal);
    if (absSample > eps/*EPS*/)
    {
        result = BLUtils::AmpToDB(absSample);
    }
    
    return result;
}
template float BLUtils::AmpToDB(float sampleVal, float eps, float minDB);
template double BLUtils::AmpToDB(double sampleVal, double eps, double minDB);

template <typename FLOAT_TYPE>
void
BLUtils::AmpToDB(WDL_TypedBuf<FLOAT_TYPE> *dBBuf,
                 const WDL_TypedBuf<FLOAT_TYPE> &ampBuf,
                 FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    dBBuf->Resize(ampBuf.GetSize());
    
    int ampBufSize = ampBuf.GetSize();
    FLOAT_TYPE *ampBufData = ampBuf.Get();
    FLOAT_TYPE *dBBufData = dBBuf->Get();
    
    for (int i = 0; i < ampBufSize; i++)
    {
        FLOAT_TYPE amp = ampBufData[i];
        FLOAT_TYPE dbAmp = AmpToDB(amp, eps, minDB);
        
        dBBufData[i] = dbAmp;
    }
}
template void BLUtils::AmpToDB(WDL_TypedBuf<float> *dBBuf,
                               const WDL_TypedBuf<float> &ampBuf,
                               float eps, float minDB);
template void BLUtils::AmpToDB(WDL_TypedBuf<double> *dBBuf,
                               const WDL_TypedBuf<double> &ampBuf,
                               double eps, double minDB);

template <typename FLOAT_TYPE>
void
BLUtils::AmpToDB(FLOAT_TYPE *dBBuf, const FLOAT_TYPE *ampBuf, int bufSize,
                 FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE amp = ampBuf[i];
        FLOAT_TYPE dbAmp = AmpToDB(amp, eps, minDB);
        
        dBBuf[i] = dbAmp;
    }
}
template void BLUtils::AmpToDB(float *dBBuf, const float *ampBuf, int bufSize,
                               float eps, float minDB);
template void BLUtils::AmpToDB(double *dBBuf, const double *ampBuf, int bufSize,
                               double eps, double minDB);

template <typename FLOAT_TYPE>
void
BLUtils::AmpToDB(WDL_TypedBuf<FLOAT_TYPE> *dBBuf,
                 const WDL_TypedBuf<FLOAT_TYPE> &ampBuf)
{
    *dBBuf = ampBuf;
    
    int dBBufSize = dBBuf->GetSize();
    FLOAT_TYPE *dBBufData = dBBuf->Get();
    
    for (int i = 0; i < dBBufSize; i++)
    {
        FLOAT_TYPE amp = dBBufData[i];
        FLOAT_TYPE dbAmp = BLUtils::AmpToDB(amp);
        
        dBBufData[i] = dbAmp;
    }
}
template void BLUtils::AmpToDB(WDL_TypedBuf<float> *dBBuf,
                               const WDL_TypedBuf<float> &ampBuf);
template void BLUtils::AmpToDB(WDL_TypedBuf<double> *dBBuf,
                               const WDL_TypedBuf<double> &ampBuf);

// OPTIM PROF Infra
// (compute in place)
template <typename FLOAT_TYPE>
void
BLUtils::AmpToDB(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                 FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    int ioBufSize = ioBuf->GetSize();
    FLOAT_TYPE *ioBufData = ioBuf->Get();
    
    for (int i = 0; i < ioBufSize; i++)
    {
        FLOAT_TYPE amp = ioBufData[i];
        FLOAT_TYPE dbAmp = AmpToDB(amp, eps, minDB);
        
        ioBufData[i] = dbAmp;
    }
}
template void BLUtils::AmpToDB(WDL_TypedBuf<float> *ioBuf,
                               float eps, float minDB);
template void BLUtils::AmpToDB(WDL_TypedBuf<double> *ioBuf,
                               double eps, double minDB);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::AmpToDBClip(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    FLOAT_TYPE result = minDB;
    FLOAT_TYPE absSample = std::fabs(sampleVal);
    if (absSample > BL_EPS)
    {
        result = BLUtils::AmpToDB(absSample);
    }
 
    // Avoid very low negative dB values, which are not significant
    if (result < minDB)
        result = minDB;
    
    return result;
}
template float BLUtils::AmpToDBClip(float sampleVal, float eps, float minDB);
template double BLUtils::AmpToDBClip(double sampleVal, double eps, double minDB);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::AmpToDBNorm(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    FLOAT_TYPE result = minDB;
    FLOAT_TYPE absSample = std::fabs(sampleVal);
    // EPS is 1e-8 here...
    // So fix it as it should be!
    //if (absSample > EPS) // (AMP_DB_CRITICAL_FIX)
    if (absSample > eps)
    {
        result = BLUtils::AmpToDB(absSample);
    }
    
    // Avoid very low negative dB values, which are not significant
    if (result < minDB)
        result = minDB;
    
    result += -minDB;
    result /= -minDB;
    
    return result;
}
template float BLUtils::AmpToDBNorm(float sampleVal, float eps, float minDB);
template double BLUtils::AmpToDBNorm(double sampleVal, double eps, double minDB);

#if !USE_SIMD_OPTIM
void
BLUtils::AmpToDBNorm(WDL_TypedBuf<FLOAT_TYPE> *dBBufNorm,
                     const WDL_TypedBuf<FLOAT_TYPE> &ampBuf,
                     FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    dBBufNorm->Resize(ampBuf.GetSize());
    
    int ampBufSize = ampBuf.GetSize();
    FLOAT_TYPE *ampBufData = ampBuf.Get();
    FLOAT_TYPE *dBBufNormData = dBBufNorm->Get();
    
    for (int i = 0; i < ampBufSize; i++)
    {
        FLOAT_TYPE amp = ampBufData[i];
        FLOAT_TYPE dbAmpNorm = AmpToDBNorm(amp, eps, minDB);
        
        dBBufNormData[i] = dbAmpNorm;
    }
}
#else
template <typename FLOAT_TYPE>
void
BLUtils::AmpToDBNorm(WDL_TypedBuf<FLOAT_TYPE> *dBBufNorm,
                     const WDL_TypedBuf<FLOAT_TYPE> &ampBuf,
                     FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    dBBufNorm->Resize(ampBuf.GetSize());
    
    int ampBufSize = ampBuf.GetSize();
    FLOAT_TYPE *ampBufData = ampBuf.Get();
    FLOAT_TYPE *dBBufNormData = dBBufNorm->Get();
    
    FLOAT_TYPE minDbInv = -1.0/minDB; //
    for (int i = 0; i < ampBufSize; i++)
    {
        FLOAT_TYPE amp = ampBufData[i];
        
        ///
        FLOAT_TYPE dbAmpNorm = minDB;
        FLOAT_TYPE absSample = std::fabs(amp);
        
        // EPS is 1e-8 here...
        // So fix it as it should be!
        //if (absSample > EPS) // (AMP_DB_CRITICAL_FIX)
        if (absSample > eps)
        {
            dbAmpNorm = BLUtils::AmpToDB(absSample);
        }
        
        // Avoid very low negative dB values, which are not significant
        if (dbAmpNorm < minDB)
            dbAmpNorm = minDB;
        
        dbAmpNorm += -minDB;
        dbAmpNorm *= minDbInv;
        ///
        
        dBBufNormData[i] = dbAmpNorm;
    }
}
template void BLUtils::AmpToDBNorm(WDL_TypedBuf<float> *dBBufNorm,
                                   const WDL_TypedBuf<float> &ampBuf,
                                   float eps, float minDB);
template void BLUtils::AmpToDBNorm(WDL_TypedBuf<double> *dBBufNorm,
                                   const WDL_TypedBuf<double> &ampBuf,
                                   double eps, double minDB);
#endif

static const double BL_AMP_DB = 8.685889638065036553;
// Magic number for dB to gain conversion.
// Approximates 10^(x/20)
static const double BL_IAMP_DB = 0.11512925464970;

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::DBToAmp(FLOAT_TYPE dB)
{
    //return FastMath::exp(((FLOAT_TYPE)BL_IAMP_DB)*dB);
    return FastMath::exp(((FLOAT_TYPE)BL_IAMP_DB)*dB);
}
template float BLUtils::DBToAmp(float dB);
template double BLUtils::DBToAmp(double dB);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::AmpToDB(FLOAT_TYPE dB)
{
    //return ((FLOAT_TYPE)BL_AMP_DB) * std::log(std::fabs(dB));
    return ((FLOAT_TYPE)BL_AMP_DB)*FastMath::log(std::fabs(dB));
}
template float BLUtils::AmpToDB(float dB);
template double BLUtils::AmpToDB(double dB);

template <typename FLOAT_TYPE>
void
BLUtils::AmpToDBNorm(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                     FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    WDL_TypedBuf<FLOAT_TYPE> ampBuf = *ioBuf;
    
    AmpToDBNorm(ioBuf, ampBuf, eps, minDB);
}
template void BLUtils::AmpToDBNorm(WDL_TypedBuf<float> *ioBuf,
                                   float eps, float minDB);
template void BLUtils::AmpToDBNorm(WDL_TypedBuf<double> *ioBuf,
                                   double eps, double minDB);

template <typename FLOAT_TYPE>
void
BLUtils::DBToAmp(WDL_TypedBuf<FLOAT_TYPE> *ioBuf)
{
    int ioBufSize = ioBuf->GetSize();
    FLOAT_TYPE *ioBufData = ioBuf->Get();
    
    for (int i = 0; i < ioBufSize; i++)
    {
        FLOAT_TYPE db = ioBufData[i];
        
        FLOAT_TYPE amp = BLUtils::DBToAmp(db);
        
        ioBufData[i] = amp;
    }
}
template void BLUtils::DBToAmp(WDL_TypedBuf<float> *ioBuf);
template void BLUtils::DBToAmp(WDL_TypedBuf<double> *ioBuf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::DBToAmpNorm(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    FLOAT_TYPE db = sampleVal;
    if (db < minDB)
        db = minDB;
    
    db *= -minDB;
    db += minDB;
    
    FLOAT_TYPE result = BLUtils::DBToAmp(db);
    
    return result;
}
template float BLUtils::DBToAmpNorm(float sampleVal, float eps, float minDB);
template double BLUtils::DBToAmpNorm(double sampleVal, double eps, double minDB);

template <typename FLOAT_TYPE>
void
BLUtils::DBToAmpNorm(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                     FLOAT_TYPE eps, FLOAT_TYPE minDB)
{
    int bufSize = ioBuf->GetSize();
    FLOAT_TYPE *bufData = ioBuf->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val = bufData[i];
        val = DBToAmpNorm(val, eps, minDB);
        bufData[i] = val;
    }
}
template void BLUtils::DBToAmpNorm(WDL_TypedBuf<float> *ioBuf,
                                   float eps, float minDB);
template void BLUtils::DBToAmpNorm(WDL_TypedBuf<double> *ioBuf,
                                   double eps, double minDB);

template <typename FLOAT_TYPE>
void
BLUtils::AddValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                   const WDL_TypedBuf<FLOAT_TYPE> &addBuf)
{
    int ioBufSize = ioBuf->GetSize();
    FLOAT_TYPE *ioBufData = ioBuf->Get();
    int addBufSize = addBuf.GetSize();
    FLOAT_TYPE *addBufData = addBuf.Get();
    
#if USE_SIMD_OPTIM
    if (ioBufSize == addBufSize)
    {
        AddValues(ioBuf, addBuf.Get());
        
        return;
    }
#endif
    
    for (int i = 0; i < ioBufSize; i++)
    {
        if (i > addBufSize - 1)
            break;
        
        FLOAT_TYPE val = ioBufData[i];
        FLOAT_TYPE add = addBufData[i];
        
        val += add;
        
        ioBufData[i] = val;
    }
}
template void BLUtils::AddValues(WDL_TypedBuf<float> *ioBuf,
                                 const WDL_TypedBuf<float> &addBuf);
template void BLUtils::AddValues(WDL_TypedBuf<double> *ioBuf,
                                 const WDL_TypedBuf<double> &addBuf);

void
BLUtils::AddValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf,
                   const WDL_TypedBuf<WDL_FFT_COMPLEX> &addBuf)
{
    int ioBufSize = ioBuf->GetSize();
    WDL_FFT_COMPLEX *ioBufData = ioBuf->Get();
    int addBufSize = addBuf.GetSize();
    WDL_FFT_COMPLEX *addBufData = addBuf.Get();
    
#if 0 //USE_SIMD_OPTIM
    if (ioBufSize == addBufSize)
    {
        FLOAT_TYPE *ioBufData = (FLOAT_TYPE *)ioBuf->Get();
        const FLOAT_TYPE *addBufData = (const FLOAT_TYPE *)addBuf.Get();
        int bufSize = ioBuf->GetSize()*2;
        
        AddValues(ioBufData, addBufData, bufSize);
        
        return;
    }
#endif
    
    for (int i = 0; i < ioBufSize; i++)
    {
        if (i > addBufSize - 1)
            break;
        
        WDL_FFT_COMPLEX val = ioBufData[i];
        WDL_FFT_COMPLEX add = addBufData[i];
        
        val.re += add.re;
        val.im += add.im;
        
        ioBufData[i] = val;
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::SubstractValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                         const WDL_TypedBuf<FLOAT_TYPE> &subBuf)
{
    int ioBufSize = ioBuf->GetSize();
    FLOAT_TYPE *ioBufData = ioBuf->Get();
    FLOAT_TYPE *subBufData = subBuf.Get();
    
#if USE_SIMD
    if (_useSimd && (ioBufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < ioBufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(ioBufData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(subBufData);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v0 - v1;
            
            simdpp::store(ioBufData, r);
            
            ioBufData += SIMD_PACK_SIZE;
            subBufData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < ioBufSize; i++)
    {
        FLOAT_TYPE val = ioBufData[i];
        FLOAT_TYPE sub = subBufData[i];
        
        val -= sub;
        
        ioBufData[i] = val;
    }
}
template void BLUtils::SubstractValues(WDL_TypedBuf<float> *ioBuf,
                                       const WDL_TypedBuf<float> &subBuf);
template void BLUtils::SubstractValues(WDL_TypedBuf<double> *ioBuf,
                                       const WDL_TypedBuf<double> &subBuf);

void
BLUtils::SubstractValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf,
                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &subBuf)
{
    int ioBufSize = ioBuf->GetSize();
    WDL_FFT_COMPLEX *ioBufData = ioBuf->Get();
    WDL_FFT_COMPLEX *subBufData = subBuf.Get();
    
    for (int i = 0; i < ioBufSize; i++)
    {
        WDL_FFT_COMPLEX val = ioBufData[i];
        WDL_FFT_COMPLEX sub = subBufData[i];
        
        val.re -= sub.re;
        val.im -= sub.im;
        
        ioBufData[i] = val;
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::ComputeDiff(WDL_TypedBuf<FLOAT_TYPE> *resultDiff,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf1)
{
    resultDiff->Resize(buf0.GetSize());
    
    int resultDiffSize = resultDiff->GetSize();
    FLOAT_TYPE *resultDiffData = resultDiff->Get();
    FLOAT_TYPE *buf0Data = buf0.Get();
    FLOAT_TYPE *buf1Data = buf1.Get();
    
#if USE_SIMD
    if (_useSimd && (resultDiffSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < resultDiffSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf0Data);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(buf1Data);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v1 - v0;
            
            simdpp::store(resultDiffData, r);
            
            buf0Data += SIMD_PACK_SIZE;
            buf1Data += SIMD_PACK_SIZE;
            resultDiffData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < resultDiffSize; i++)
    {
        FLOAT_TYPE val0 = buf0Data[i];
        FLOAT_TYPE val1 = buf1Data[i];
        
        FLOAT_TYPE diff = val1 - val0;
        
        resultDiffData[i] = diff;
    }
}
template void BLUtils::ComputeDiff(WDL_TypedBuf<float> *resultDiff,
                                   const WDL_TypedBuf<float> &buf0,
                                   const WDL_TypedBuf<float> &buf1);
template void BLUtils::ComputeDiff(WDL_TypedBuf<double> *resultDiff,
                                   const WDL_TypedBuf<double> &buf0,
                                   const WDL_TypedBuf<double> &buf1);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeDiff(FLOAT_TYPE *resultDiff,
                     const FLOAT_TYPE* buf0, const FLOAT_TYPE* buf1,
                     int bufSize)
{
#if USE_SIMD
    if (_useSimd && (bufSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf0);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(buf1);
            
            simdpp::float64<SIMD_PACK_SIZE> r = v1 - v0;
            
            simdpp::store(resultDiff, r);
            
            buf0 += SIMD_PACK_SIZE;
            buf1 += SIMD_PACK_SIZE;
            resultDiff += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE val0 = buf0[i];
        FLOAT_TYPE val1 = buf1[i];
        
        FLOAT_TYPE diff = val1 - val0;
        
        resultDiff[i] = diff;
    }
}
template void BLUtils::ComputeDiff(float *resultDiff,
                                   const float *buf0, const float *buf1,
                                   int bufSize);
template void BLUtils::ComputeDiff(double *resultDiff,
                                   const double *buf0, const double *buf1,
                                   int bufSize);
void
BLUtils::ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf0,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf1)
{
    resultDiff->Resize(buf0.GetSize());
    
    int resultDiffSize = resultDiff->GetSize();
    WDL_FFT_COMPLEX *resultDiffData = resultDiff->Get();
    WDL_FFT_COMPLEX *buf0Data = buf0.Get();
    WDL_FFT_COMPLEX *buf1Data = buf1.Get();
    
#if 0 //USE_SIMD_OPTIM
    FLOAT_TYPE *resD = (FLOAT_TYPE *)resultDiffData;
    FLOAT_TYPE *buf0D = (FLOAT_TYPE *)buf0Data;
    FLOAT_TYPE *buf1D = (FLOAT_TYPE *)buf1Data;
    
    int bufSizeD = resultDiffSize*2;
    
    ComputeDiff(resD, buf0D, buf1D, bufSizeD);
    
    return;
#endif
    
    for (int i = 0; i < resultDiffSize; i++)
    {
        WDL_FFT_COMPLEX val0 = buf0Data[i];
        WDL_FFT_COMPLEX val1 = buf1Data[i];
        
        WDL_FFT_COMPLEX diff;
        diff.re = val1.re - val0.re;
        diff.im = val1.im - val0.im;
        
        resultDiffData[i] = diff;
    }
}

void
BLUtils::ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
                     const vector<WDL_FFT_COMPLEX> &buf0,
                     const vector<WDL_FFT_COMPLEX> &buf1)
{
    resultDiff->Resize(buf0.size());
    
    int resultDiffSize = resultDiff->GetSize();
    WDL_FFT_COMPLEX *resultDiffData = resultDiff->Get();
    
    for (int i = 0; i < resultDiffSize; i++)
    {
        WDL_FFT_COMPLEX val0 = buf0[i];
        WDL_FFT_COMPLEX val1 = buf1[i];
        
        WDL_FFT_COMPLEX diff;
        diff.re = val1.re - val0.re;
        diff.im = val1.im - val0.im;
        
        resultDiffData[i] = diff;
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::Permute(WDL_TypedBuf<FLOAT_TYPE> *values,
                 const WDL_TypedBuf<int> &indices,
                 bool forward)
{
    if (values->GetSize() != indices.GetSize())
        return;
    
    WDL_TypedBuf<FLOAT_TYPE> origValues = *values;
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    int *indicesData = indices.Get();
    FLOAT_TYPE *origValuesData = origValues.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        int idx = indicesData[i];
        if (idx >= valuesSize)
            // Error
            return;
        
        if (forward)
            valuesData[idx] = origValuesData[i];
        else
            valuesData[i] = origValuesData[idx];
    }
}
template void BLUtils::Permute(WDL_TypedBuf<float> *values,
                               const WDL_TypedBuf<int> &indices,
                               bool forward);
template void BLUtils::Permute(WDL_TypedBuf<double> *values,
                               const WDL_TypedBuf<int> &indices,
                               bool forward);

void
BLUtils::Permute(vector< vector< int > > *values,
                 const WDL_TypedBuf<int> &indices,
                 bool forward)
{
    if (values->size() != indices.GetSize())
        return;
    
    vector<vector<int> > origValues = *values;
    
    for (int i = 0; i < values->size(); i++)
    {
        int idx = indices.Get()[i];
        if (idx >= values->size())
            // Error
            return;
        
        if (forward)
            (*values)[idx] = origValues[i];
        else
            (*values)[i] = origValues[idx];
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::ClipMin(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE minVal)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
#if USE_SIMD
    if (_useSimd && (valuesSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < valuesSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(valuesData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load_splat(&minVal);
            
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::max(v0, v1);
            
            simdpp::store(valuesData, r);
            
            valuesData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val < minVal)
            val = minVal;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ClipMin(WDL_TypedBuf<float> *values, float minVal);
template void BLUtils::ClipMin(WDL_TypedBuf<double> *values, double minVal);

template <typename FLOAT_TYPE>
void
BLUtils::ClipMin2(WDL_TypedBuf<FLOAT_TYPE> *values,
                  FLOAT_TYPE clipVal, FLOAT_TYPE minVal)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val < clipVal)
            val = minVal;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ClipMin2(WDL_TypedBuf<float> *values,
                                float clipVal, float minVal);
template void BLUtils::ClipMin2(WDL_TypedBuf<double> *values,
                                double clipVal, double minVal);

template <typename FLOAT_TYPE>
void
BLUtils::ClipMax(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE maxVal)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
#if USE_SIMD
    if (_useSimd && (valuesSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < valuesSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(valuesData);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load_splat(&maxVal);
            
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::min(v0, v1);
            
            simdpp::store(valuesData, r);
            
            valuesData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val > maxVal)
            val = maxVal;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ClipMax(WDL_TypedBuf<float> *values, float maxVal);
template void BLUtils::ClipMax(WDL_TypedBuf<double> *values, double maxVal);

template <typename FLOAT_TYPE>
void
BLUtils::ClipMinMax(FLOAT_TYPE *val, FLOAT_TYPE min, FLOAT_TYPE max)
{
    if (*val < min)
        *val = min;
    if (*val > max)
        *val = max;
}
template void BLUtils::ClipMinMax(float *val, float min, float max);
template void BLUtils::ClipMinMax(double *val, double min, double max);

template <typename FLOAT_TYPE>
void
BLUtils::ClipMinMax(WDL_TypedBuf<FLOAT_TYPE> *values,
                    FLOAT_TYPE minVal, FLOAT_TYPE maxVal)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
#if USE_SIMD
    if (_useSimd && (valuesSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < valuesSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(valuesData);
            simdpp::float64<SIMD_PACK_SIZE> m0 = simdpp::load_splat(&minVal);
            simdpp::float64<SIMD_PACK_SIZE> m1 = simdpp::load_splat(&maxVal);
            
            simdpp::float64<SIMD_PACK_SIZE> r0 = simdpp::max(v0, m0);
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::min(r0, m1);
            
            simdpp::store(valuesData, r);
            
            valuesData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val < minVal)
            val = minVal;
        if (val > maxVal)
            val = maxVal;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ClipMinMax(WDL_TypedBuf<float> *values,
                                  float minVal, float maxVal);
template void BLUtils::ClipMinMax(WDL_TypedBuf<double> *values,
                                  double minVal, double maxVal);

template <typename FLOAT_TYPE>
void
BLUtils::ThresholdMin(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE min)
{    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val < min)
            val = 0.0;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ThresholdMin(WDL_TypedBuf<float> *values, float min);
template void BLUtils::ThresholdMin(WDL_TypedBuf<double> *values, double min);

template <typename FLOAT_TYPE>
void
BLUtils::ThresholdMinRel(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE min)
{
    FLOAT_TYPE maxValue = BLUtils::FindMaxValue(*values);
    min *= maxValue;
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val < min)
            val = 0.0;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ThresholdMinRel(WDL_TypedBuf<float> *values, float min);
template void BLUtils::ThresholdMinRel(WDL_TypedBuf<double> *values, double min);

template <typename FLOAT_TYPE>
void
BLUtils::Diff(WDL_TypedBuf<FLOAT_TYPE> *diff,
              const WDL_TypedBuf<FLOAT_TYPE> &prevValues,
              const WDL_TypedBuf<FLOAT_TYPE> &values)
{
#if USE_SIMD_OPTIM
    // Diff and ComputeDiff are the same function
    ComputeDiff(diff, prevValues, values);
    
    return;
#endif
    
    diff->Resize(values.GetSize());
    
    int valuesSize = values.GetSize();
    FLOAT_TYPE *valuesData = values.Get();
    FLOAT_TYPE *prevValuesData = prevValues.Get();
    FLOAT_TYPE *diffData = diff->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        FLOAT_TYPE prevVal = prevValuesData[i];
        
        FLOAT_TYPE d = val - prevVal;
        
        diffData[i] = d;
    }
}
template void BLUtils::Diff(WDL_TypedBuf<float> *diff,
                            const WDL_TypedBuf<float> &prevValues,
                            const WDL_TypedBuf<float> &values);
template void BLUtils::Diff(WDL_TypedBuf<double> *diff,
                            const WDL_TypedBuf<double> &prevValues,
                            const WDL_TypedBuf<double> &values);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyDiff(WDL_TypedBuf<FLOAT_TYPE> *values,
                   const WDL_TypedBuf<FLOAT_TYPE> &diff)
{
#if USE_SIMD_OPTIM
    AddValues(values, diff);
    
    return;
#endif
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    FLOAT_TYPE *diffData = diff.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        FLOAT_TYPE d = diffData[i];
        
        val += d;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ApplyDiff(WDL_TypedBuf<float> *values,
                                 const WDL_TypedBuf<float> &diff);
template void BLUtils::ApplyDiff(WDL_TypedBuf<double> *values,
                                 const WDL_TypedBuf<double> &diff);


template <typename FLOAT_TYPE>
bool
BLUtils::IsEqual(const WDL_TypedBuf<FLOAT_TYPE> &values0,
                 const WDL_TypedBuf<FLOAT_TYPE> &values1)
{
    if (values0.GetSize() != values1.GetSize())
        return false;
    
    int values0Size = values0.GetSize();
    FLOAT_TYPE *values0Data = values0.Get();
    FLOAT_TYPE *values1Data = values1.Get();
    
#if USE_SIMD
    if (_useSimd && (values0Size % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < values0Size; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(values0Data);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(values1Data);
            
            simdpp::float64<SIMD_PACK_SIZE> d = v1 - v0;
            
            simdpp::float64<SIMD_PACK_SIZE> a = simdpp::abs(d);
            
            FLOAT_TYPE maxVal = simdpp::reduce_max(a);
            
            if (maxVal > BL_EPS)
                return false;
            
            values0Data += SIMD_PACK_SIZE;
            values1Data += SIMD_PACK_SIZE;
        }
    }
    
    return true;
#endif
    
    for (int i = 0; i < values0Size; i++)
    {
        FLOAT_TYPE val0 = values0Data[i];
        FLOAT_TYPE val1 = values1Data[i];
        
        if (std::fabs(val0 - val1) > BL_EPS)
            return false;
    }
    
    return true;
}
template bool BLUtils::IsEqual(const WDL_TypedBuf<float> &values0,
                               const WDL_TypedBuf<float> &values1);
template bool BLUtils::IsEqual(const WDL_TypedBuf<double> &values0,
                               const WDL_TypedBuf<double> &values1);

template <typename FLOAT_TYPE>
void
BLUtils::ReplaceValue(WDL_TypedBuf<FLOAT_TYPE> *values,
                      FLOAT_TYPE srcValue, FLOAT_TYPE dstValue)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        if (std::fabs(val - srcValue) < BL_EPS)
            val = dstValue;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ReplaceValue(WDL_TypedBuf<float> *values,
                                    float srcValue, float dstValue);
template void BLUtils::ReplaceValue(WDL_TypedBuf<double> *values,
                                    double srcValue, double dstValue);

template <typename FLOAT_TYPE>
void
BLUtils::MakeSymmetry(WDL_TypedBuf<FLOAT_TYPE> *symBuf,
                      const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    symBuf->Resize(buf.GetSize()*2);
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    FLOAT_TYPE *symBufData = symBuf->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        symBufData[i] = bufData[i];
        symBufData[bufSize*2 - i - 1] = bufData[i];
    }
}
template void BLUtils::MakeSymmetry(WDL_TypedBuf<float> *symBuf,
                                    const WDL_TypedBuf<float> &buf);
template void BLUtils::MakeSymmetry(WDL_TypedBuf<double> *symBuf,
                                    const WDL_TypedBuf<double> &buf);

#if 0 // old version...
void
BLUtils::Reverse(WDL_TypedBuf<int> *values)
{
    WDL_TypedBuf<int> origValues = *values;
    
    int valuesSize = values->GetSize();
    int *valuesData = values->Get();
    int *origValuesData = origValues.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        int val = origValuesData[i];
        
        int idx = valuesSize - i - 1;
        
        valuesData[idx] = val;
    }
}
#endif

// NOTE: not really tested for the moment...
//
// New version, don't use temporary buffer
void
BLUtils::Reverse(WDL_TypedBuf<int> *values)
{    
    int valuesHalfSize = values->GetSize()/2;
    for (int i = 0; i < valuesHalfSize; i++)
    {
        int val0 = values->Get()[i];
        int idx = values->GetSize() - i - 1;
        int val1 = values->Get()[idx];

        values->Get()[i] = val1;
        values->Get()[idx] = val0;
    }
}

#if 0 // old version
template <typename FLOAT_TYPE>
void
BLUtils::Reverse(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    WDL_TypedBuf<FLOAT_TYPE> origValues = *values;
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    FLOAT_TYPE *origValuesData = origValues.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = origValuesData[i];
        
        int idx = valuesSize - i - 1;
        
        valuesData[idx] = val;
    }
}
#endif
// New version (no tmeporary buffer)
template <typename FLOAT_TYPE>
void
BLUtils::Reverse(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    int valuesHalfSize = values->GetSize()/2;
    for (int i = 0; i < valuesHalfSize; i++)
    {
        FLOAT_TYPE val0 = values->Get()[i];
        int idx = values->GetSize() - i - 1;
        FLOAT_TYPE val1 = values->Get()[idx];

        values->Get()[i] = val1;
        values->Get()[idx] = val0;
    }
}
template void BLUtils::Reverse(WDL_TypedBuf<float> *values);
template void BLUtils::Reverse(WDL_TypedBuf<double> *values);

template <typename FLOAT_TYPE>
void
BLUtils::ApplySqrt(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        if (val > 0.0)
            val = std::sqrt(val);
        
        valuesData[i] = val;
    }
}
template void BLUtils::ApplySqrt(WDL_TypedBuf<float> *values);
template void BLUtils::ApplySqrt(WDL_TypedBuf<double> *values);

template <typename FLOAT_TYPE>
void
BLUtils::CopyBuf(WDL_TypedBuf<FLOAT_TYPE> *toBuf,
                 const WDL_TypedBuf<FLOAT_TYPE> &fromBuf)
{
    int toBufSize = toBuf->GetSize();
    FLOAT_TYPE *toBufData = toBuf->Get();
    FLOAT_TYPE *fromBufData = fromBuf.Get();
    
    int fromBufSize = fromBuf.GetSize();
    
#if USE_SIMD_OPTIM
    int size = toBufSize;
    if (fromBufSize < size)
        size = fromBufSize;
    memcpy(toBufData, fromBufData, size*sizeof(FLOAT_TYPE));
    
    return;
#endif
    
    for (int i = 0; i < toBufSize; i++)
    {
        if (i >= fromBufSize)
            break;
    
        FLOAT_TYPE val = fromBufData[i];
        toBufData[i] = val;
    }
}
template void BLUtils::CopyBuf(WDL_TypedBuf<float> *toBuf,
                               const WDL_TypedBuf<float> &fromBuf);
template void BLUtils::CopyBuf(WDL_TypedBuf<double> *toBuf,
                               const WDL_TypedBuf<double> &fromBuf);

template <typename FLOAT_TYPE>
void
BLUtils::CopyBuf(WDL_TypedBuf<FLOAT_TYPE> *toBuf,
                 const FLOAT_TYPE *fromData, int fromSize)
{
    toBuf->Resize(fromSize);
    memcpy(toBuf->Get(), fromData, fromSize*sizeof(FLOAT_TYPE));
}
template void BLUtils::CopyBuf(WDL_TypedBuf<float> *toBuf,
                               const float *fromData, int fromSize);
template void BLUtils::CopyBuf(WDL_TypedBuf<double> *toBuf,
                               const double *fromData, int fromSize);

template <typename FLOAT_TYPE>
void
BLUtils::CopyBuf(FLOAT_TYPE *toBuf, const FLOAT_TYPE *fromData, int fromSize)
{
    memcpy(toBuf, fromData, fromSize*sizeof(FLOAT_TYPE));
}
template void BLUtils::CopyBuf(float *toBuf, const float *fromData, int fromSize);
template void BLUtils::CopyBuf(double *toBuf, const double *fromData, int fromSize);

template <typename FLOAT_TYPE>
void
BLUtils::CopyBuf(FLOAT_TYPE *toData, const WDL_TypedBuf<FLOAT_TYPE> &fromBuf)
{
    memcpy(toData, fromBuf.Get(), fromBuf.GetSize()*sizeof(FLOAT_TYPE));
}
template void BLUtils::CopyBuf(float *toData, const WDL_TypedBuf<float> &fromBuf);
template void BLUtils::CopyBuf(double *toData, const WDL_TypedBuf<double> &fromBuf);

template <typename FLOAT_TYPE>
void
BLUtils::Replace(WDL_TypedBuf<FLOAT_TYPE> *dst, int startIdx,
                 const WDL_TypedBuf<FLOAT_TYPE> &src)
{
    int srcSize = src.GetSize();
    FLOAT_TYPE *srcData = src.Get();
    int dstSize = dst->GetSize();
    FLOAT_TYPE *dstData = dst->Get();
    
#if USE_SIMD_OPTIM
    int size = srcSize;
    if (srcSize + startIdx > dstSize)
        size = dstSize - startIdx;
    
    memcpy(&dstData[startIdx], srcData, size*sizeof(FLOAT_TYPE));
    
    return;
#endif
    
    for (int i = 0; i < srcSize; i++)
    {
        if (startIdx + i >= dstSize)
            break;
        
        FLOAT_TYPE val = srcData[i];
        
        dstData[startIdx + i] = val;
    }
}
template void BLUtils::Replace(WDL_TypedBuf<float> *dst,
                               int startIdx, const WDL_TypedBuf<float> &src);
template void BLUtils::Replace(WDL_TypedBuf<double> *dst,
                               int startIdx, const WDL_TypedBuf<double> &src);

#if !FIND_VALUE_INDEX_EXPE
// Current version
template <typename FLOAT_TYPE>
int
BLUtils::FindValueIndex(FLOAT_TYPE val,
                        const WDL_TypedBuf<FLOAT_TYPE> &values, FLOAT_TYPE *outT)
{
    if (outT != NULL)
        *outT = 0.0;
    
    int valuesSize = values.GetSize();
    FLOAT_TYPE *valuesData = values.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE v = valuesData[i];
        
        if (v > val)
        {
            int idx0 = i - 1;
            if (idx0 < 0)
                idx0 = 0;
            
            if (outT != NULL)
            {
                int idx1 = i;
            
                FLOAT_TYPE val0 = valuesData[idx0];
                FLOAT_TYPE val1 = valuesData[idx1];
                
                if (std::fabs(val1 - val0) > BL_EPS)
                    *outT = (val - val0)/(val1 - val0);
            }
            
            return idx0;
        }
    }
    
    return -1;
}
template int BLUtils::FindValueIndex(float val,
                                     const WDL_TypedBuf<float> &values,
                                     float *outT);
template int BLUtils::FindValueIndex(double val,
                                     const WDL_TypedBuf<double> &values,
                                     double *outT);

#else
// New version
// NOT very well tested yet (but should be better - or same -)
int
BLUtils::FindValueIndex(FLOAT_TYPE val,
                        const WDL_TypedBuf<FLOAT_TYPE> &values, FLOAT_TYPE *outT)
{
    *outT = 0.0;
    
    int valuesSize = values.GetSize();
    FLOAT_TYPE *valuesData = values.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE v = valuesData[i];
        
        if (v == val)
            // Same value
        {
            *outT = 0.0;
            
            return i;
        }
        
        if (v > val)
        {
            int idx0 = i - 1;
            if (idx0 < 0)
                // We have not found the value in the array
                // because the first value of the array is already grater than
                // the value we are testing
            {
                idx0 = 0;
                *outT = 0.0;
                
                return idx0;
            }
            
            // We are between values
            if (outT != NULL)
            {
                int idx1 = idx0 + 1;
                
                FLOAT_TYPE val0 = valuesData[idx0];
                FLOAT_TYPE val1 = valuesData[idx1];
                
                if (std::fabs(val1 - val0) > BL_EPS)
                    *outT = (val - val0)/(val1 - val0);
            }
            
            return idx0;
        }
    }
    
    return -1;
}
#endif

// Find the matching index from srcVal and the src list,
// and return the corresponding value from the dstValues
//
// The src values do not need to be sorted !
// (as opposite to FindValueIndex())
//
// Used to sort without loosing the consistency
template <typename FLOAT_TYPE>
class Value
{
public:
    static bool IsGreater(const Value& v0, const Value& v1)
    { return v0.mSrcValue < v1.mSrcValue; }
    
    FLOAT_TYPE mSrcValue;
    FLOAT_TYPE mDstValue;
};

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FindMatchingValue(FLOAT_TYPE srcVal,
                           const WDL_TypedBuf<FLOAT_TYPE> &srcValues,
                           const WDL_TypedBuf<FLOAT_TYPE> &dstValues)
{
    // Fill the vector
    vector<Value<FLOAT_TYPE> > values;
    values.resize(srcValues.GetSize());
    
    int srcValuesSize = srcValues.GetSize();
    FLOAT_TYPE *srcValuesData = srcValues.Get();
    FLOAT_TYPE *dstValuesData = dstValues.Get();
    
    for (int i = 0; i < srcValuesSize; i++)
    {
        FLOAT_TYPE srcValue = srcValuesData[i];
        FLOAT_TYPE dstValue = dstValuesData[i];
        
        Value<FLOAT_TYPE> val;
        val.mSrcValue = srcValue;
        val.mDstValue = dstValue;
        
        values[i] = val;
    }
    
    // Sort the vector
    sort(values.begin(), values.end(), Value<FLOAT_TYPE>::IsGreater);
    
    // Re-create the WDL list
    WDL_TypedBuf<FLOAT_TYPE> sortedSrcValues;
    sortedSrcValues.Resize(values.size());
    
    FLOAT_TYPE *sortedSrcValuesData = sortedSrcValues.Get();
    
    for (int i = 0; i < values.size(); i++)
    {
        FLOAT_TYPE val = values[i].mSrcValue;
        sortedSrcValuesData[i] = val;
    }
    
    // Find the index and the t parameter
    FLOAT_TYPE t;
    int idx = FindValueIndex(srcVal, sortedSrcValues, &t);
    if (idx < 0)
        idx = 0;
    
    // Find the new matching value
    FLOAT_TYPE dstVal0 = dstValues.Get()[idx];
    if (idx + 1 >= dstValues.GetSize())
        // LAst value
        return dstVal0;
    
    FLOAT_TYPE dstVal1 = dstValues.Get()[idx + 1];
    
    FLOAT_TYPE res = Interp(dstVal0, dstVal1, t);
    
    return res;
}
template float BLUtils::FindMatchingValue(float srcVal,
                                          const WDL_TypedBuf<float> &srcValues,
                                          const WDL_TypedBuf<float> &dstValues);
template double BLUtils::FindMatchingValue(double srcVal,
                                           const WDL_TypedBuf<double> &srcValues,
                                           const WDL_TypedBuf<double> &dstValues);

template <typename FLOAT_TYPE>
void
BLUtils::PrepareMatchingValueSorted(WDL_TypedBuf<FLOAT_TYPE> *srcValues,
                                    WDL_TypedBuf<FLOAT_TYPE> *dstValues)
{
    // Fill the vector
    vector<Value<FLOAT_TYPE> > values;
    values.resize(srcValues->GetSize());
    
    int srcValuesSize = srcValues->GetSize();
    FLOAT_TYPE *srcValuesData = srcValues->Get();
    FLOAT_TYPE *dstValuesData = dstValues->Get();
    
    for (int i = 0; i < srcValuesSize; i++)
    {
        FLOAT_TYPE srcValue = srcValuesData[i];

        FLOAT_TYPE dstValue = 0.0;
        if (dstValues != NULL)
            dstValue = dstValuesData[i];
        
        Value<FLOAT_TYPE> val;
        val.mSrcValue = srcValue;
        val.mDstValue = dstValue;
        
        values[i] = val;
    }
    
    // Sort the vector
    sort(values.begin(), values.end(), Value<FLOAT_TYPE>::IsGreater);
    
    // Re-create the WDL lists
    
    // Src values
    for (int i = 0; i < values.size(); i++)
    {
        FLOAT_TYPE val = values[i].mSrcValue;
        srcValuesData[i] = val;
    }
    
    // Dst values
    if (dstValues != NULL)
    {
        WDL_TypedBuf<FLOAT_TYPE> sortedDstValues;
        sortedDstValues.Resize(values.size());
        
        FLOAT_TYPE *dstValuesData2 = dstValues->Get();
        
        for (int i = 0; i < values.size(); i++)
        {
            FLOAT_TYPE val = values[i].mDstValue;
            dstValuesData2[i] = val;
        }
    }
}
template void BLUtils::PrepareMatchingValueSorted(WDL_TypedBuf<float> *srcValues,
                                                  WDL_TypedBuf<float> *dstValues);
template void BLUtils::PrepareMatchingValueSorted(WDL_TypedBuf<double> *srcValues,
                                                  WDL_TypedBuf<double> *dstValues);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FindMatchingValueSorted(FLOAT_TYPE srcVal,
                                 const WDL_TypedBuf<FLOAT_TYPE> &sortedSrcValues,
                                 const WDL_TypedBuf<FLOAT_TYPE> &sortedDstValues)
{
    // Find the index and the t parameter
    FLOAT_TYPE t;
    int idx = FindValueIndex(srcVal, sortedSrcValues, &t);
    if (idx < 0)
        idx = 0;
        
    // Find the new matching value
    FLOAT_TYPE dstVal0 = sortedDstValues.Get()[idx];
    if (idx + 1 >= sortedDstValues.GetSize())
        // Last value
        return dstVal0;
    
    FLOAT_TYPE dstVal1 = sortedDstValues.Get()[idx + 1];
    
    FLOAT_TYPE res = Interp(dstVal0, dstVal1, t);
    
    return res;
}
template float
BLUtils::FindMatchingValueSorted(float srcVal,
                                 const WDL_TypedBuf<float> &sortedSrcValues,
                                 const WDL_TypedBuf<float> &sortedDstValues);
template double
BLUtils::FindMatchingValueSorted(double srcVal,
                                 const WDL_TypedBuf<double> &sortedSrcValues,
                                 const WDL_TypedBuf<double> &sortedDstValues);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FactorToDivFactor(FLOAT_TYPE val, FLOAT_TYPE coeff)
{
    FLOAT_TYPE res = std::pow(coeff, val);
    
    return res;
}
template float BLUtils::FactorToDivFactor(float val, float coeff);
template double BLUtils::FactorToDivFactor(double val, double coeff);

// GOOD: makes good linerp !
// And fixed NaN
template <typename FLOAT_TYPE>
void
BLUtils::FillMissingValues(WDL_TypedBuf<FLOAT_TYPE> *values,
                           bool extendBounds, FLOAT_TYPE undefinedValue)
{
    if (extendBounds)
        // Extend the last value to the end
    {
        // Find the last max
        int lastIndex = values->GetSize() - 1;
        FLOAT_TYPE lastValue = 0.0;
        
        int valuesSize = values->GetSize();
        FLOAT_TYPE *valuesData = values->Get();
        
        for (int i = valuesSize - 1; i > 0; i--)
        {
            FLOAT_TYPE val = valuesData[i];
            if (val > undefinedValue)
            {
                lastValue = val;
                lastIndex = i;
                
                break;
            }
        }
        
        // Fill the last values with last max
        for (int i = valuesSize - 1; i > lastIndex; i--)
        {
            valuesData[i] = lastValue;
        }
    }
    
    // Fill the holes by linear interpolation
    FLOAT_TYPE startVal = 0.0;
    
    // First, find start val
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val > undefinedValue)
        {
            startVal = val;
        }
    }
    
    int loopIdx = 0;
    int startIndex = 0;
    //FLOAT_TYPE lastValidVal = 0.0;
    
    // Then start the main loop
    while(loopIdx < valuesSize)
    {
        FLOAT_TYPE val = valuesData[loopIdx];
        
        if (val > undefinedValue)
            // Defined
        {
            startVal = val;
            startIndex = loopIdx;
            
            loopIdx++;
        }
        else
            // Undefined
        {
            // Start at 0
            if (!extendBounds &&
                (loopIdx == 0))
                startVal = 0.0;
            
            // Find how many missing values we have
            int endIndex = startIndex + 1;
            FLOAT_TYPE endVal = 0.0;
            bool defined = false;
            
            while(endIndex < valuesSize)
            {
                if (endIndex < valuesSize)
                    endVal = valuesData[endIndex];
                
                defined = (endVal > undefinedValue);
                if (defined)
                    break;
                
                endIndex++;
            }
            
#if 0 // Make problems with series ending with zeros
            if (defined)
            {
                lastValidVal = endVal;
            }
            else
                // Not found at the end
            {
                endVal = lastValidVal;
            }
#endif
            
            // Fill the missing values with lerp
            for (int i = startIndex; i < endIndex; i++)
            {
                // FIX "+1": avoid NaN, and better linerp !
                FLOAT_TYPE t =
                    ((FLOAT_TYPE)(i - startIndex))/(endIndex - startIndex /*+ 1*/);
                
                FLOAT_TYPE newVal = (1.0 - t)*startVal + t*endVal;
                    
                valuesData[i] = newVal;
            }
            
            startIndex = endIndex;
            loopIdx = endIndex;
        }
    }
}
template void BLUtils::FillMissingValues(WDL_TypedBuf<float> *values,
                                         bool extendBounds, float undefinedValue);
template void BLUtils::FillMissingValues(WDL_TypedBuf<double> *values,
                                         bool extendBounds, double undefinedValue);

template <typename FLOAT_TYPE>
void
BLUtils::FillMissingValuesLagrange(WDL_TypedBuf<FLOAT_TYPE> *values,
                                   bool extendBounds, FLOAT_TYPE undefinedValue)
{
    int lastIndex = values->GetSize() - 1;
    if (extendBounds)
        // Extend the last value to the end
    {
        // Find the last max
        FLOAT_TYPE lastValue = 0.0;
        
        int valuesSize = values->GetSize();
        FLOAT_TYPE *valuesData = values->Get();
        
        for (int i = valuesSize - 1; i > 0; i--)
        {
            FLOAT_TYPE val = valuesData[i];
            //if (val > undefinedValue)
            if (std::fabs(val - undefinedValue) > BL_EPS)
            {
                lastValue = val;
                lastIndex = i;
                
                break;
            }
        }
        
        // Fill the last values with last max
        for (int i = valuesSize - 1; i > lastIndex; i--)
        {
            valuesData[i] = lastValue;
        }
    }

    // Fill x and y arrays
    //
    // Allocate to the max possible number of values
    vector<FLOAT_TYPE> xValues;
    xValues.resize(values->GetSize());

    vector<FLOAT_TYPE> yValues;
    yValues.resize(values->GetSize());

    // Fill
    int numValues = 0;
    for (int i = 0; i < values->GetSize(); i++)
    {
        FLOAT_TYPE val = values->Get()[i];
        
        //if (val > undefinedValue)
        if (std::fabs(val - undefinedValue) > BL_EPS)
            // Valid value
        {
            xValues[numValues] = i;
            yValues[numValues] = val;

            numValues++;
        }
    }
    
    // Resize down
    xValues.resize(numValues);
    yValues.resize(numValues);

    // Use lagrange to interpolate
    //
    FLOAT_TYPE p[4][2];
    for (int i = 0; i < values->GetSize(); i++)
    {
        typename vector<FLOAT_TYPE>::iterator it =
            lower_bound(xValues.begin(), xValues.end(), i);
        if (it == xValues.end())
        {
            // Fill the rest of the values with the last value
            if (i > 0)
            {
                FLOAT_TYPE val = values->Get()[i - 1];
                for (int j = i; j < values->GetSize(); j++)
                    values->Get()[j] = val;
            }
            
            break;
        }
        
        int idx = it - xValues.begin();

        // Points 0 and 1
        for (int j = idx - 2; j <= idx - 1; j++)
        {
            int jj = j;
            if (jj < 0)
                jj = 0;

            FLOAT_TYPE xVal = xValues[jj];
            // Avoid same x value (not supported by Lagrange)
            if (j < 0)
                xVal = xVal + j;
            
            p[j - idx + 2][0] = xVal;
            p[j - idx + 2][1] = yValues[jj];
        }

        // Points 2 and 3
        for (int j = idx; j <= idx + 1; j++)
        {
            int jj = j;
            if (jj > xValues.size() - 1)
                jj = xValues.size() - 1;

            FLOAT_TYPE xVal = xValues[jj];
            // Avoid same x value (not supported by Lagrange)
            if (j > xValues.size() - 1)
                xVal = xVal + (j - xValues.size() + 1);
            
            p[j - idx + 2][0] = xVal;
            p[j - idx + 2][1] = yValues[jj];
        }
        
        FLOAT_TYPE x = i;
        FLOAT_TYPE y = BLUtilsMath::LagrangeInterp4(x, p[0], p[1], p[2], p[3]);
            
        values->Get()[i] = y;
    }
}
template void
BLUtils::FillMissingValuesLagrange(WDL_TypedBuf<float> *values,
                                   bool extendBounds, float undefinedValue);
template void
BLUtils::FillMissingValuesLagrange(WDL_TypedBuf<double> *values,
                                   bool extendBounds, double undefinedValue);

template <typename FLOAT_TYPE>
void
BLUtils::FillMissingValuesLagrangeDB(WDL_TypedBuf<FLOAT_TYPE> *values,
                                     bool extendBounds, FLOAT_TYPE undefinedValue)
{
    // TODO:
    // - use a real db scale
    // - fix undefined values (SASViewer) 
    undefinedValue = std::exp(undefinedValue);
    
    BLUtils::ApplyExp(values);
    FillMissingValuesLagrange(values, extendBounds, undefinedValue);
    BLUtils::ApplyLog(values);
}
template void
BLUtils::FillMissingValuesLagrangeDB(WDL_TypedBuf<float> *values,
                                     bool extendBounds, float undefinedValue);
template void
BLUtils::FillMissingValuesLagrangeDB(WDL_TypedBuf<double> *values,
                                     bool extendBounds, double undefinedValue);

template <typename FLOAT_TYPE>
void
BLUtils::AddIntermediateValues(WDL_TypedBuf<FLOAT_TYPE> *values,
                               int targetMinNumValues,
                               FLOAT_TYPE undefinedValue)
{
    // Get known values
    
    // Fill x and y arrays
    //
    // Allocate to the max possible number of values
    vector<FLOAT_TYPE> xValues;
    xValues.resize(values->GetSize());

    vector<FLOAT_TYPE> yValues;
    yValues.resize(values->GetSize());

    // Fill
    int numValues = 0;
    for (int i = 0; i < values->GetSize(); i++)
    {
        FLOAT_TYPE val = values->Get()[i];
        
        //if (val > undefinedValue)
        if (std::fabs(val - undefinedValue) > BL_EPS)
            // Valid value
        {
            xValues[numValues] = i;
            yValues[numValues] = val;

            numValues++;
        }
    }
    
    // Resize down
    xValues.resize(numValues);
    yValues.resize(numValues);

    if (numValues == 1)
        return;

    if (numValues >= targetMinNumValues)
        return;
    
    // Compute the number of values to add at each step
    BL_FLOAT numValuesStep = ((BL_FLOAT)targetMinNumValues)/xValues.size();
    numValuesStep = ceil(numValuesStep);

    // Keep the current data!
    // Reset the data
    //BLUtils::FillAllValue(values, undefinedValue);

    // Populate
    for (int i = 0; i < xValues.size() - 1; i++)
    {
        BL_FLOAT x0 = xValues[i];
        BL_FLOAT x1 = xValues[i + 1];

        BL_FLOAT y0 = yValues[i];
        BL_FLOAT y1 = yValues[i + 1];

        for (int j = 0; j < numValuesStep; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)(j + 1))/(numValuesStep + 2);

            int xi = x0 + t*(x1 - x0);
            if ((xi < 0) || (xi > values->GetSize()))
                continue;
            
            BL_FLOAT vi = values->Get()[xi];
            if (std::fabs(vi - undefinedValue) > BL_EPS)
                // Value already set here, do not overwrite it!
                continue;

            BL_FLOAT yi = y0 + t*(y1 - y0);

            // Set the intermediate value
            values->Get()[xi] = yi;
        }
    }
}
template void
BLUtils::AddIntermediateValues(WDL_TypedBuf<float> *values,
                               int targetMinNumValues, float undefinedValue);
template void
BLUtils::AddIntermediateValues(WDL_TypedBuf<double> *values,
                               int targetMinNumValues, double undefinedValue);

template <typename FLOAT_TYPE>
void
BLUtils::FillMissingValues2(WDL_TypedBuf<FLOAT_TYPE> *values,
                            bool extendBounds, FLOAT_TYPE undefinedValue)
{
    if (extendBounds)
        // Extend the last value to the end
    {
        // Find the last max
        int lastIndex = values->GetSize() - 1;
        FLOAT_TYPE lastValue = undefinedValue;
        
        int valuesSize = values->GetSize();
        FLOAT_TYPE *valuesData = values->Get();
        
        for (int i = valuesSize - 1; i > 0; i--)
        {
            FLOAT_TYPE val = valuesData[i];
            if (val > undefinedValue)
            {
                lastValue = val;
                lastIndex = i;
                
                break;
            }
        }
        
        // Fill the last values with last max
        for (int i = valuesSize - 1; i > lastIndex; i--)
        {
            valuesData[i] = lastValue;
        }
    }
    
    // Fill the holes by linear interpolation
    FLOAT_TYPE startVal = undefinedValue;
    
    // First, find start val
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val > undefinedValue)
        {
            startVal = val;
        }
    }
    
    int loopIdx = 0;
    int startIndex = 0;
    //FLOAT_TYPE lastValidVal = 0.0;
    
    // Then start the main loop
    while(loopIdx < valuesSize)
    {
        FLOAT_TYPE val = valuesData[loopIdx];
        
        if (val > undefinedValue)
            // Defined
        {
            startVal = val;
            startIndex = loopIdx;
            
            loopIdx++;
        }
        else
            // Undefined
        {
            // Start at 0
            if (!extendBounds &&
                (loopIdx == 0))
                startVal = undefinedValue;
            
            // Find how many missing values we have
            int endIndex = startIndex + 1;
            FLOAT_TYPE endVal = undefinedValue;
            bool defined = false;
            
            while(endIndex < valuesSize)
            {
                if (endIndex < valuesSize)
                    endVal = valuesData[endIndex];
                
                defined = (endVal > undefinedValue);
                if (defined)
                    break;
                
                endIndex++;
            }
            
#if 0 // Make problems with series ending with zeros
            if (defined)
            {
                lastValidVal = endVal;
            }
            else
                // Not found at the end
            {
                endVal = lastValidVal;
            }
#endif
            
            // Fill the missing values with lerp
            for (int i = startIndex; i < endIndex; i++)
            {
                // FIX "+1": avoid NaN, and better linerp !
                FLOAT_TYPE t =
                    ((FLOAT_TYPE)(i - startIndex))/(endIndex - startIndex /*+ 1*/);
                
                FLOAT_TYPE newVal = (1.0 - t)*startVal + t*endVal;
                
                valuesData[i] = newVal;
            }
            
            startIndex = endIndex;
            loopIdx = endIndex;
        }
    }
}
template void BLUtils::FillMissingValues2(WDL_TypedBuf<float> *values,
                                          bool extendBounds, float undefinedValue);
template void BLUtils::FillMissingValues2(WDL_TypedBuf<double> *values,
                                          bool extendBounds, double undefinedValue);

template <typename FLOAT_TYPE>
void
BLUtils::FillMissingValues3(WDL_TypedBuf<FLOAT_TYPE> *values,
                            bool extendBounds, FLOAT_TYPE undefinedValue)
{
    if (extendBounds)
        // Extend the last value to the end
    {
        // Find the last max
        int lastIndex = values->GetSize() - 1;
        FLOAT_TYPE lastValue = undefinedValue;
        
        int valuesSize = values->GetSize();
        FLOAT_TYPE *valuesData = values->Get();
        
        for (int i = valuesSize - 1; i > 0; i--)
        {
            FLOAT_TYPE val = valuesData[i];
            if (val > undefinedValue)
            {
                lastValue = val;
                lastIndex = i;
                
                break;
            }
        }
        
        // Fill the last values with last max
        for (int i = valuesSize - 1; i > lastIndex; i--)
        {
            valuesData[i] = lastValue;
        }
    }
    
    // Fill the holes by linear interpolation
    FLOAT_TYPE startVal = undefinedValue;
    
    // First, find start val
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        if (val > undefinedValue)
        {
            startVal = val;
            
            break; // NEW
        }
    }
    
    int loopIdx = 0;
    int startIndex = 0;
    //FLOAT_TYPE lastValidVal = 0.0;
    
    // Then start the main loop
    while(loopIdx < valuesSize)
    {
        FLOAT_TYPE val = valuesData[loopIdx];
        
        if (val > undefinedValue)
            // Defined
        {
            startVal = val;
            startIndex = loopIdx;
            
            loopIdx++;
        }
        else
            // Undefined
        {
            // Start at 0
            if (!extendBounds &&
                (loopIdx == 0))
                startVal = undefinedValue;
            
            // Find how many missing values we have
            int endIndex = startIndex + 1;
            FLOAT_TYPE endVal = undefinedValue;
            bool defined = false;
            
            while(endIndex < valuesSize)
            {
                if (endIndex < valuesSize)
                    endVal = valuesData[endIndex];
                
                defined = (endVal > undefinedValue);
                if (defined)
                    break;
                
                endIndex++;
            }
            
#if 0 // Make problems with series ending with zeros
            if (defined)
            {
                lastValidVal = endVal;
            }
            else
                // Not found at the end
            {
                endVal = lastValidVal;
            }
#endif
            
            // Fill the missing values with lerp
            for (int i = startIndex; i < endIndex; i++)
            {
                // FIX "+1": avoid NaN, and better linerp !
                FLOAT_TYPE t =
                    ((FLOAT_TYPE)(i - startIndex))/(endIndex - startIndex /*+ 1*/);
                
                FLOAT_TYPE newVal = (1.0 - t)*startVal + t*endVal;
                
                valuesData[i] = newVal;
            }
            
            startIndex = endIndex;
            loopIdx = endIndex;
        }
    }
}
template void BLUtils::FillMissingValues3(WDL_TypedBuf<float> *values,
                                          bool extendBounds, float undefinedValue);
template void BLUtils::FillMissingValues3(WDL_TypedBuf<double> *values,
                                          bool extendBounds, double undefinedValue);

template <typename FLOAT_TYPE>
void
BLUtils::ScaleNearest(WDL_TypedBuf<FLOAT_TYPE> *values, int factor)
{
    WDL_TypedBuf<FLOAT_TYPE> newValues;
    newValues.Resize(values->GetSize()*factor);
    
    int newValuesSize = newValues.GetSize();
    FLOAT_TYPE *newValuesData = newValues.Get();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < newValuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i/factor];
        newValuesData[i] = val;
    }
    
    *values = newValues;
}
template void BLUtils::ScaleNearest(WDL_TypedBuf<float> *values, int factor);
template void BLUtils::ScaleNearest(WDL_TypedBuf<double> *values, int factor);

template <typename FLOAT_TYPE>
int
BLUtils::FindMaxIndex(const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    int maxIndex = -1;
    FLOAT_TYPE maxValue = -1e15;
    
    int valuesSize = values.GetSize();
    FLOAT_TYPE *valuesData = values.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE value = valuesData[i];
        
        if (value > maxValue)
        {
            maxValue = value;
            maxIndex = i;
        }
    }
    
    return maxIndex;
}
template int BLUtils::FindMaxIndex(const WDL_TypedBuf<float> &values);
template int BLUtils::FindMaxIndex(const WDL_TypedBuf<double> &values);

template <typename FLOAT_TYPE>
int
BLUtils::FindMaxIndex(const WDL_TypedBuf<FLOAT_TYPE> &values,
                      int startIdx, int endIdx)
{
    int maxIndex = -1;
    FLOAT_TYPE maxValue = -BL_INF;
    
    int valuesSize = values.GetSize();
    FLOAT_TYPE *valuesData = values.Get();

    if (endIdx > valuesSize - 1)
        endIdx = valuesSize - 1;
    
    for (int i = startIdx; i <= endIdx; i++)
    {
        FLOAT_TYPE value = valuesData[i];
        
        if (value > maxValue)
        {
            maxValue = value;
            maxIndex = i;
        }
    }
    
    return maxIndex;
}
template int BLUtils::FindMaxIndex(const WDL_TypedBuf<float> &values,
                                   int startIdx, int endIdx);
template int BLUtils::FindMaxIndex(const WDL_TypedBuf<double> &values,
                                   int startIdx, int endIdx);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FindMaxValue(const WDL_TypedBuf<FLOAT_TYPE> &values)
{
#if USE_SIMD_OPTIM
    // This is the same function...
    FLOAT_TYPE maxValue0 = ComputeMax(values);
    
    return maxValue0;
#endif
    
    FLOAT_TYPE maxValue = -1e15;
    
    int valuesSize = values.GetSize();
    FLOAT_TYPE *valuesData = values.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE value = valuesData[i];
        
        if (value > maxValue)
        {
            maxValue = value;
        }
    }
    
    return maxValue;
}
template float BLUtils::FindMaxValue(const WDL_TypedBuf<float> &values);
template double BLUtils::FindMaxValue(const WDL_TypedBuf<double> &values);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FindMaxValue(const vector<WDL_TypedBuf<FLOAT_TYPE> > &values)
{
    FLOAT_TYPE maxValue = -1e15;
    
    for (int i = 0; i < values.size(); i++)
    {
#if !USE_SIMD_OPTIM
        for (int j = 0; j < values[i].GetSize(); j++)
        {
            FLOAT_TYPE value = values[i].Get()[j];
        
            if (value > maxValue)
            {
                maxValue = value;
            }
        }
#else
        FLOAT_TYPE maxValue0 = ComputeMax(values[i]);
        if (maxValue0 > maxValue)
            maxValue = maxValue0;
#endif
    }
    
    return maxValue;
}
template float BLUtils::FindMaxValue(const vector<WDL_TypedBuf<float> > &values);
template double BLUtils::FindMaxValue(const vector<WDL_TypedBuf<double> > &values);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeAbs(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
#if USE_SIMD
    if (_useSimd && (valuesSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < valuesSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(valuesData);
            
            simdpp::float64<SIMD_PACK_SIZE> r = simdpp::abs(v0);
            
            simdpp::store(valuesData, r);
            
            valuesData += SIMD_PACK_SIZE;
        }
        
        // Finished
        return;
    }
#endif
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE value = valuesData[i];
        
        value = std::fabs(value);
        
        valuesData[i] = value;
    }
}
template void BLUtils::ComputeAbs(WDL_TypedBuf<float> *values);
template void BLUtils::ComputeAbs(WDL_TypedBuf<double> *values);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::LogScaleNormInv(FLOAT_TYPE value, FLOAT_TYPE maxValue, FLOAT_TYPE factor)
{
    FLOAT_TYPE result = value/maxValue;
    
    //result = LogScale(result, factor);
    result *= std::exp(factor) - 1.0;
    result += 1.0;
    result = std::log(result);
    result /= factor;
    
    result *= maxValue;
    
    return result;
}
template float BLUtils::LogScaleNormInv(float value,
                                        float maxValue, float factor);
template double BLUtils::LogScaleNormInv(double value,
                                         double maxValue, double factor);

// values should be already normalized before calling the function
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::LogScaleNorm2(FLOAT_TYPE value, FLOAT_TYPE factor)
{
    FLOAT_TYPE result =
        std::log((BL_FLOAT)(1.0 + value*factor))/std::log((BL_FLOAT)(1.0 + factor));
    
    return result;
}
template float BLUtils::LogScaleNorm2(float value, float factor);
template double BLUtils::LogScaleNorm2(double value, double factor);

// values should be already normalized before calling the function
template <typename FLOAT_TYPE>
void
BLUtils::LogScaleNorm2(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE factor)
{
    WDL_TypedBuf<FLOAT_TYPE> resultValues;
    resultValues.Resize(values->GetSize());
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        FLOAT_TYPE val = values->Get()[i];
        
        val =
            std::log((BL_FLOAT)(1.0 + val*factor))/std::log((BL_FLOAT)(1.0 + factor));
        
        resultValues.Get()[i] = val;
    }
    
    *values = resultValues;
}
template void BLUtils::LogScaleNorm2(WDL_TypedBuf<float> *values, float factor);
template void BLUtils::LogScaleNorm2(WDL_TypedBuf<double> *values, double factor);

// NOTE: not tested (done specifically for MetaSoundViewer, no other use for the moment)
// values should be already normalized before calling the function
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::LogScaleNormInv2(FLOAT_TYPE value, FLOAT_TYPE factor)
{
    FLOAT_TYPE result = std::exp(value*factor)/std::exp(factor) - 1.0;
        
    return result;
}
template float BLUtils::LogScaleNormInv2(float value, float factor);
template double BLUtils::LogScaleNormInv2(double value, double factor);

// NOTE: not tested (done specifically for MetaSoundViewer, no other use for the moment)
// values should be already normalized before calling the function
template <typename FLOAT_TYPE>
void
BLUtils::LogScaleNormInv2(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE factor)
{
    WDL_TypedBuf<FLOAT_TYPE> resultValues;
    resultValues.Resize(values->GetSize());
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        FLOAT_TYPE val = values->Get()[i];
        
        val = std::exp(val*factor)/std::exp(factor) - 1.0;
        
        resultValues.Get()[i] = val;
    }
    
    *values = resultValues;
}
template void BLUtils::LogScaleNormInv2(WDL_TypedBuf<float> *values, float factor);
template void BLUtils::LogScaleNormInv2(WDL_TypedBuf<double> *values, double factor);

template <typename FLOAT_TYPE>
void
BLUtils::FreqsToLogNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                        const WDL_TypedBuf<FLOAT_TYPE> &magns,
                        FLOAT_TYPE hzPerBin)
{
    BLUtils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    FLOAT_TYPE maxLog = std::log10(maxFreq);
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        FLOAT_TYPE logVal = i*maxLog/resultMagnsSize;
        FLOAT_TYPE freq = std::pow((FLOAT_TYPE)10.0, logVal);
        
        if (maxFreq < BL_EPS)
            return;
        
        FLOAT_TYPE id0 = (freq/maxFreq) * resultMagnsSize;
        FLOAT_TYPE t = id0 - (int)(id0);
        
        if ((int)id0 >= magnsSize)
            continue;
        
        int id1 = id0 + 1;
        if (id1 >= magnsSize)
            continue;
        
        FLOAT_TYPE magn0 = magnsData[(int)id0];
        FLOAT_TYPE magn1 = magnsData[id1];
        
        FLOAT_TYPE magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagnsData[i] = magn;
    }
}
template void BLUtils::FreqsToLogNorm(WDL_TypedBuf<float> *resultMagns,
                                      const WDL_TypedBuf<float> &magns,
                                      float hzPerBin);
template void BLUtils::FreqsToLogNorm(WDL_TypedBuf<double> *resultMagns,
                                      const WDL_TypedBuf<double> &magns,
                                      double hzPerBin);


template <typename FLOAT_TYPE>
void
BLUtils::LogToFreqsNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                        const WDL_TypedBuf<FLOAT_TYPE> &magns,
                        FLOAT_TYPE hzPerBin)
{
    BLUtils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    FLOAT_TYPE maxLog = std::log10(maxFreq);
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        FLOAT_TYPE freq = hzPerBin*i;
        FLOAT_TYPE logVal = 0.0;
        // Check for log(0) => -inf
        if (freq > 0)
            logVal = std::log10(freq);
        
        FLOAT_TYPE id0 = (logVal/maxLog) * resultMagnsSize;
        
        if ((int)id0 >= magnsSize)
            continue;
        
        // Linear
        FLOAT_TYPE t = id0 - (int)(id0);
        
        int id1 = id0 + 1;
        if (id1 >= magnsSize)
            continue;
        
        FLOAT_TYPE magn0 = magnsData[(int)id0];
        FLOAT_TYPE magn1 = magnsData[id1];
        
        FLOAT_TYPE magn = (1.0 - t)*magn0 + t*magn1;
        
        // Nearest
        //FLOAT_TYPE magn = magns.Get()[(int)id0];
        
        resultMagnsData[i] = magn;
    }
}
template void BLUtils::LogToFreqsNorm(WDL_TypedBuf<float> *resultMagns,
                                      const WDL_TypedBuf<float> &magns,
                                      float hzPerBin);
template void BLUtils::LogToFreqsNorm(WDL_TypedBuf<double> *resultMagns,
                                      const WDL_TypedBuf<double> &magns,
                                      double hzPerBin);

template <typename FLOAT_TYPE>
void
BLUtils::FreqsToDbNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                       const WDL_TypedBuf<FLOAT_TYPE> &magns,
                       FLOAT_TYPE hzPerBin,
                       FLOAT_TYPE minValue, FLOAT_TYPE maxValue)
{
    BLUtils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    //FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    //FLOAT_TYPE maxDb = BLUtils::NormalizedXTodB(1.0, minValue, maxValue);
    
    // We work in normalized coordinates
    FLOAT_TYPE maxFreq = 1.0;
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        FLOAT_TYPE dbVal = ((FLOAT_TYPE)i)/resultMagnsSize;
        FLOAT_TYPE freq = BLUtils::NormalizedXTodBInv(dbVal, minValue, maxValue);
        
        if (maxFreq < BL_EPS)
            return;
        
        FLOAT_TYPE id0 = (freq/maxFreq) * resultMagnsSize;
        FLOAT_TYPE t = id0 - (int)(id0);
        
        if ((int)id0 >= magnsSize)
            continue;
        
        int id1 = id0 + 1;
        if (id1 >= magnsSize)
            continue;
        
        FLOAT_TYPE magn0 = magnsData[(int)id0];
        FLOAT_TYPE magn1 = magnsData[id1];
        
        FLOAT_TYPE magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagnsData[i] = magn;
    }
}
template void BLUtils::FreqsToDbNorm(WDL_TypedBuf<float> *resultMagns,
                                     const WDL_TypedBuf<float> &magns,
                                     float hzPerBin,
                                     float minValue, float maxValue);
template void BLUtils::FreqsToDbNorm(WDL_TypedBuf<double> *resultMagns,
                                     const WDL_TypedBuf<double> &magns,
                                     double hzPerBin,
                                     double minValue, double maxValue);

// TODO: need to check this
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::LogNormToFreq(int idx, FLOAT_TYPE hzPerBin, int bufferSize)
{
    FLOAT_TYPE maxFreq = hzPerBin*(bufferSize - 1);
    FLOAT_TYPE maxLog = std::log10(maxFreq);
    
    FLOAT_TYPE freq = hzPerBin*idx;
    FLOAT_TYPE logVal = std::log10(freq);
    
    FLOAT_TYPE id0 = (logVal/maxLog) * bufferSize;
    
    FLOAT_TYPE result = id0*hzPerBin;
    
    return result;
}
template float BLUtils::LogNormToFreq(int idx, float hzPerBin, int bufferSize);
template double BLUtils::LogNormToFreq(int idx, double hzPerBin, int bufferSize);

// GOOD !
template <typename FLOAT_TYPE>
int
BLUtils::FreqIdToLogNormId(int idx, FLOAT_TYPE hzPerBin, int bufferSize)
{
    FLOAT_TYPE maxFreq = hzPerBin*(bufferSize/2);
    FLOAT_TYPE maxLog = std::log10(maxFreq);
    
    FLOAT_TYPE freq = hzPerBin*idx;
    FLOAT_TYPE logVal = std::log10(freq);
        
    FLOAT_TYPE resultId = (logVal/maxLog)*(bufferSize/2);
    
    resultId = bl_round(resultId);
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}
template int BLUtils::FreqIdToLogNormId(int idx, float hzPerBin, int bufferSize);
template int BLUtils::FreqIdToLogNormId(int idx, double hzPerBin, int bufferSize);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyWindow(WDL_TypedBuf<FLOAT_TYPE> *values,
                     const WDL_TypedBuf<FLOAT_TYPE> &window)
{
    if (values->GetSize() != window.GetSize())
        return;
    
#if USE_SIMD_OPTIM
    MultValues(values, window);
    
    return;
#endif
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    FLOAT_TYPE *windowData = window.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        FLOAT_TYPE w = windowData[i];
        
        val *= w;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ApplyWindow(WDL_TypedBuf<float> *values,
                                   const WDL_TypedBuf<float> &window);
template void BLUtils::ApplyWindow(WDL_TypedBuf<double> *values,
                                   const WDL_TypedBuf<double> &window);


template <typename FLOAT_TYPE>
void
BLUtils::ApplyWindowRescale(WDL_TypedBuf<FLOAT_TYPE> *values,
                            const WDL_TypedBuf<FLOAT_TYPE> &window)
{
    FLOAT_TYPE coeff = ((FLOAT_TYPE)window.GetSize())/values->GetSize();
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    int windowSize = window.GetSize();
    FLOAT_TYPE *windowData = window.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        int winIdx = i*coeff;
        if (winIdx >= windowSize)
            continue;
        
        FLOAT_TYPE w = windowData[winIdx];
        
        val *= w;
        
        valuesData[i] = val;
    }
}
template void BLUtils::ApplyWindowRescale(WDL_TypedBuf<float> *values,
                                          const WDL_TypedBuf<float> &window);
template void BLUtils::ApplyWindowRescale(WDL_TypedBuf<double> *values,
                                          const WDL_TypedBuf<double> &window);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyWindowFft(WDL_TypedBuf<FLOAT_TYPE> *ioMagns,
                        const WDL_TypedBuf<FLOAT_TYPE> &phases,
                        const WDL_TypedBuf<FLOAT_TYPE> &window)
{
    WDL_TypedBuf<int> samplesIds;
    BLUtilsFft::FftIdsToSamplesIds(phases, &samplesIds);
    
    WDL_TypedBuf<FLOAT_TYPE> sampleMagns = *ioMagns;
    
    BLUtils::Permute(&sampleMagns, samplesIds, true);
    
    BLUtils::ApplyWindow(&sampleMagns, window);
    
    BLUtils::Permute(&sampleMagns, samplesIds, false);
    
#if 1
    // Must multiply by 2... why ?
    //
    // Maybe due to hanning window normalization of windows
    //
    BLUtils::MultValues(&sampleMagns, (FLOAT_TYPE)2.0);
#endif
    
    *ioMagns = sampleMagns;
}
template void BLUtils::ApplyWindowFft(WDL_TypedBuf<float> *ioMagns,
                                      const WDL_TypedBuf<float> &phases,
                                      const WDL_TypedBuf<float> &window);
template void BLUtils::ApplyWindowFft(WDL_TypedBuf<double> *ioMagns,
                                      const WDL_TypedBuf<double> &phases,
                                      const WDL_TypedBuf<double> &window);

// boundSize is used to not divide by the extremities, which are often zero
template <typename FLOAT_TYPE>
void
BLUtils::UnapplyWindow(WDL_TypedBuf<FLOAT_TYPE> *values,
                       const WDL_TypedBuf<FLOAT_TYPE> &window,
                       int boundSize)
{    
    if (values->GetSize() != window.GetSize())
        return;
    
    FLOAT_TYPE max0 = BLUtils::ComputeMaxAbs(*values);
    
    // First, apply be the invers window
    int start = boundSize;
    
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    FLOAT_TYPE *windowData = window.Get();
    
    for (int i = boundSize; i < valuesSize - boundSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        FLOAT_TYPE w = windowData[i];
        
        if (w > BL_EPS6)
        {
            val /= w;
            valuesData[i] = val;
        }
        else
        {
            int newStart = i;
            if (newStart > valuesSize/2)
                newStart = valuesSize - i - 1;
            
            if (newStart > start)
                start = newStart;
        }
    }
    
    if (start >= valuesSize/2)
        // Error
        return;
    
    // Then fill the missing values
    
    // Start
    FLOAT_TYPE startVal = valuesData[start];
    for (int i = 0; i < start; i++)
    {
        valuesData[i] = startVal;
    }
    
    // End
    int end = valuesSize - start - 1;
    FLOAT_TYPE endVal = valuesData[end];
    for (int i = valuesSize - 1; i > end; i--)
    {
        valuesData[i] = endVal;
    }
    
    FLOAT_TYPE max1 = BLUtils::ComputeMaxAbs(*values);
    
    // Normalize
    if (max1 > 0.0)
    {
        FLOAT_TYPE coeff = max0/max1;
        
        BLUtils::MultValues(values, coeff);
    }
}
template void BLUtils::UnapplyWindow(WDL_TypedBuf<float> *values,
                                     const WDL_TypedBuf<float> &window,
                                     int boundSize);
template void BLUtils::UnapplyWindow(WDL_TypedBuf<double> *values,
                                     const WDL_TypedBuf<double> &window,
                                     int boundSize);

template <typename FLOAT_TYPE>
void
BLUtils::UnapplyWindowFft(WDL_TypedBuf<FLOAT_TYPE> *ioMagns,
                          const WDL_TypedBuf<FLOAT_TYPE> &phases,
                          const WDL_TypedBuf<FLOAT_TYPE> &window,
                          int boundSize)
{
    WDL_TypedBuf<int> samplesIds;
    BLUtilsFft::FftIdsToSamplesIds(phases, &samplesIds);
    
    WDL_TypedBuf<FLOAT_TYPE> sampleMagns = *ioMagns;
    
    BLUtils::Permute(&sampleMagns, samplesIds, true);
    
    BLUtils::UnapplyWindow(&sampleMagns, window, boundSize);
    
    BLUtils::Permute(&sampleMagns, samplesIds, false);
    
#if 1
    // Must multiply by 8... why ?
    //
    // Maybe due to hanning window normalization of windows
    //
    BLUtils::MultValues(&sampleMagns, (FLOAT_TYPE)8.0);
#endif
    
    *ioMagns = sampleMagns;
}
template void BLUtils::UnapplyWindowFft(WDL_TypedBuf<float> *ioMagns,
                                        const WDL_TypedBuf<float> &phases,
                                        const WDL_TypedBuf<float> &window,
                                        int boundSize);
template void BLUtils::UnapplyWindowFft(WDL_TypedBuf<double> *ioMagns,
                                        const WDL_TypedBuf<double> &phases,
                                        const WDL_TypedBuf<double> &window,
                                        int boundSize);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeTimeDelays(WDL_TypedBuf<FLOAT_TYPE> *timeDelays,
                           const WDL_TypedBuf<FLOAT_TYPE> &phasesL,
                           const WDL_TypedBuf<FLOAT_TYPE> &phasesR,
                           FLOAT_TYPE sampleRate)
{
    if (phasesL.GetSize() != phasesR.GetSize())
        // R can be empty if we are in mono
        return;
    
    timeDelays->Resize(phasesL.GetSize());
    
    WDL_TypedBuf<FLOAT_TYPE> samplesIdsL;
    BLUtilsFft::FftIdsToSamplesIdsFloat(phasesL, &samplesIdsL);
    
    WDL_TypedBuf<FLOAT_TYPE> samplesIdsR;
    BLUtilsFft::FftIdsToSamplesIdsFloat(phasesR, &samplesIdsR);
   
#if !USE_SIMD_OPTIM
    int timeDelaysSize = timeDelays->GetSize();
    FLOAT_TYPE *timeDelaysData = timeDelays->Get();
    FLOAT_TYPE *sampleIdsLData = samplesIdsL.Get();
    FLOAT_TYPE *sampleIdsRData = samplesIdsR.Get();
    
    for (int i = 0; i < timeDelaysSize; i++)
    {
        FLOAT_TYPE sampIdL = sampleIdsLData[i];
        FLOAT_TYPE sampIdR = sampleIdsRData[i];
        
        FLOAT_TYPE diff = sampIdR - sampIdL;
        
        FLOAT_TYPE delay = diff/sampleRate;
        
        timeDelaysData[i] = delay;
    }
#else
    *timeDelays = samplesIdsR;
    SubstractValues(timeDelays, samplesIdsL);
    FLOAT_TYPE coeff = 1.0/sampleRate;
    MultValues(timeDelays, coeff);
#endif
}
template void BLUtils::ComputeTimeDelays(WDL_TypedBuf<float> *timeDelays,
                                         const WDL_TypedBuf<float> &phasesL,
                                         const WDL_TypedBuf<float> &phasesR,
                                         float sampleRate);
template void BLUtils::ComputeTimeDelays(WDL_TypedBuf<double> *timeDelays,
                                         const WDL_TypedBuf<double> &phasesL,
                                         const WDL_TypedBuf<double> &phasesR,
                                         double sampleRate);

template <typename FLOAT_TYPE>
void
BLUtils::PolarToCartesian(const WDL_TypedBuf<FLOAT_TYPE> &Rs,
                          const WDL_TypedBuf<FLOAT_TYPE> &thetas,
                          WDL_TypedBuf<FLOAT_TYPE> *xValues,
                          WDL_TypedBuf<FLOAT_TYPE> *yValues)
{
    xValues->Resize(thetas.GetSize());
    yValues->Resize(thetas.GetSize());
    
    int thetasSize = thetas.GetSize();
    FLOAT_TYPE *thetasData = thetas.Get();
    FLOAT_TYPE *RsData = Rs.Get();
    FLOAT_TYPE *xValuesData = xValues->Get();
    FLOAT_TYPE *yValuesData = yValues->Get();
    
    for (int i = 0; i < thetasSize; i++)
    {
        FLOAT_TYPE theta = thetasData[i];
        
        FLOAT_TYPE r = RsData[i];
        
        FLOAT_TYPE x = r*std::cos(theta);
        FLOAT_TYPE y = r*std::sin(theta);
        
        xValuesData[i] = x;
        yValuesData[i] = y;
    }
}
template void BLUtils::PolarToCartesian(const WDL_TypedBuf<float> &Rs,
                                        const WDL_TypedBuf<float> &thetas,
                                        WDL_TypedBuf<float> *xValues,
                                        WDL_TypedBuf<float> *yValues);
template void BLUtils::PolarToCartesian(const WDL_TypedBuf<double> &Rs,
                                        const WDL_TypedBuf<double> &thetas,
                                        WDL_TypedBuf<double> *xValues,
                                        WDL_TypedBuf<double> *yValues);

template <typename FLOAT_TYPE>
void
BLUtils::PhasesPolarToCartesian(const WDL_TypedBuf<FLOAT_TYPE> &phasesDiff,
                                const WDL_TypedBuf<FLOAT_TYPE> *magns,
                                WDL_TypedBuf<FLOAT_TYPE> *xValues,
                                WDL_TypedBuf<FLOAT_TYPE> *yValues)
{
    xValues->Resize(phasesDiff.GetSize());
    yValues->Resize(phasesDiff.GetSize());
    
    int phaseDiffSize = phasesDiff.GetSize();
    FLOAT_TYPE *phaseDiffData = phasesDiff.Get();
    int magnsSize = magns->GetSize();
    FLOAT_TYPE *magnsData = magns->Get();
    FLOAT_TYPE *xValuesData = xValues->Get();
    FLOAT_TYPE *yValuesData = yValues->Get();
    
    for (int i = 0; i < phaseDiffSize; i++)
    {
        FLOAT_TYPE phaseDiff = phaseDiffData[i];
        
        // TODO: check this
        phaseDiff = BLUtilsPhases::MapToPi(phaseDiff);
        
        FLOAT_TYPE magn = 1.0;
        if ((magns != NULL) && (magnsSize > 0))
            magn = magnsData[i];
        
        FLOAT_TYPE x = magn*std::cos(phaseDiff);
        FLOAT_TYPE y = magn*std::sin(phaseDiff);
        
        xValuesData[i] = x;
        yValuesData[i] = y;
    }
}
template void BLUtils::PhasesPolarToCartesian(const WDL_TypedBuf<float> &phasesDiff,
                                              const WDL_TypedBuf<float> *magns,
                                              WDL_TypedBuf<float> *xValues,
                                              WDL_TypedBuf<float> *yValues);
template void BLUtils::PhasesPolarToCartesian(const WDL_TypedBuf<double> &phasesDiff,
                                              const WDL_TypedBuf<double> *magns,
                                              WDL_TypedBuf<double> *xValues,
                                              WDL_TypedBuf<double> *yValues);

// From (angle, distance) to (normalized angle on x, hight distance on y)
template <typename FLOAT_TYPE>
void
BLUtils::CartesianToPolarFlat(WDL_TypedBuf<FLOAT_TYPE> *xVector,
                              WDL_TypedBuf<FLOAT_TYPE> *yVector)
{
    int xVectorSize = xVector->GetSize();
    FLOAT_TYPE *xVectorData = xVector->Get();
    FLOAT_TYPE *yVectorData = yVector->Get();
    
    for (int i = 0; i < xVectorSize; i++)
    {
        FLOAT_TYPE x0 = xVectorData[i];
        FLOAT_TYPE y0 = yVectorData[i];
        
        FLOAT_TYPE angle = std::atan2(y0, x0);
        
        // Normalize x
        FLOAT_TYPE x = (angle/M_PI - 0.5)*2.0;
        
        // Keep y as it is ?
        //FLOAT_TYPE y = y0;
        
        // or change it ?
        FLOAT_TYPE y = std::sqrt(x0*x0 + y0*y0);
        
        xVectorData[i] = x;
        yVectorData[i] = y;
    }
}
template void BLUtils::CartesianToPolarFlat(WDL_TypedBuf<float> *xVector,
                                            WDL_TypedBuf<float> *yVector);
template void BLUtils::CartesianToPolarFlat(WDL_TypedBuf<double> *xVector,
                                            WDL_TypedBuf<double> *yVector);

template <typename FLOAT_TYPE>
void
BLUtils::PolarToCartesianFlat(WDL_TypedBuf<FLOAT_TYPE> *xVector,
                              WDL_TypedBuf<FLOAT_TYPE> *yVector)
{
    int xVectorSize = xVector->GetSize();
    FLOAT_TYPE *xVectorData = xVector->Get();
    FLOAT_TYPE *yVectorData = yVector->Get();
    
    for (int i = 0; i < xVectorSize; i++)
    {
        FLOAT_TYPE theta = xVectorData[i];
        FLOAT_TYPE r = yVectorData[i];
        
        theta = ((theta + 1.0)*0.5)*M_PI;
        
        FLOAT_TYPE x = r*std::cos(theta);
        FLOAT_TYPE y = r*std::sin(theta);
        
        xVectorData[i] = x;
        yVectorData[i] = y;
    }
}
template void BLUtils::PolarToCartesianFlat(WDL_TypedBuf<float> *xVector,
                                            WDL_TypedBuf<float> *yVector);
template void BLUtils::PolarToCartesianFlat(WDL_TypedBuf<double> *xVector,
                                            WDL_TypedBuf<double> *yVector);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyInverseWindow(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            const WDL_TypedBuf<FLOAT_TYPE> &window,
                            const WDL_TypedBuf<FLOAT_TYPE> *originEnvelope)
{
    WDL_TypedBuf<FLOAT_TYPE> magns;
    WDL_TypedBuf<FLOAT_TYPE> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, *fftSamples);
    
    ApplyInverseWindow(&magns, phases, window, originEnvelope);
    
    BLUtilsComp::MagnPhaseToComplex(fftSamples, magns, phases);
}
template void BLUtils::ApplyInverseWindow(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                          const WDL_TypedBuf<float> &window,
                                          const WDL_TypedBuf<float> *originEnvelope);
template void BLUtils::ApplyInverseWindow(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                          const WDL_TypedBuf<double> &window,
                                          const WDL_TypedBuf<double> *originEnvelope);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyInverseWindow(WDL_TypedBuf<FLOAT_TYPE> *magns,
                            const WDL_TypedBuf<FLOAT_TYPE> &phases,
                            const WDL_TypedBuf<FLOAT_TYPE> &window,
                            const WDL_TypedBuf<FLOAT_TYPE> *originEnvelope)
{
#define WIN_EPS 1e-3
    
    // Suppresses the amplification of noise at the border of the wndow
#define MAGN_EPS 1e-6
    
    WDL_TypedBuf<int> samplesIds;
    BLUtilsFft::FftIdsToSamplesIds(phases, &samplesIds);
    
    const WDL_TypedBuf<FLOAT_TYPE> origMagns = *magns;
    
    int magnsSize = magns->GetSize();
    FLOAT_TYPE *magnsData = magns->Get();
    int *samplesIdsData = samplesIds.Get();
    FLOAT_TYPE *origMagnsData = origMagns.Get();
    FLOAT_TYPE *windowData = window.Get();
    FLOAT_TYPE *originEnvelopeData = originEnvelope->Get();
    
    for (int i = 0; i < magnsSize; i++)
        //for (int i = 1; i < magns->GetSize() - 1; i++)
    {
        int sampleIdx = samplesIdsData[i];
        
        FLOAT_TYPE magn = origMagnsData[i];
        FLOAT_TYPE win1 = windowData[sampleIdx];
        
        FLOAT_TYPE coeff = 0.0;
        
        if (win1 > WIN_EPS)
            coeff = 1.0/win1;
        
        //coeff = win1;
        
        // Better with
        if (originEnvelope != NULL)
        {
            FLOAT_TYPE originSample = originEnvelopeData[sampleIdx];
            coeff *= std::fabs(originSample);
        }
        
        if (magn > MAGN_EPS) // TEST
            magn *= coeff;
        
#if 0
        // Just in case
        if (magn > 1.0)
            magn = 1.0;
        if (magn < -1.0)
            magn = -1.0;
#endif
        
        magnsData[i] = magn;
    }
}
template void BLUtils::ApplyInverseWindow(WDL_TypedBuf<float> *magns,
                                          const WDL_TypedBuf<float> &phases,
                                          const WDL_TypedBuf<float> &window,
                                          const WDL_TypedBuf<float> *originEnvelope);
template void BLUtils::ApplyInverseWindow(WDL_TypedBuf<double> *magns,
                                          const WDL_TypedBuf<double> &phases,
                                          const WDL_TypedBuf<double> &window,
                                          const WDL_TypedBuf<double> *originEnvelope);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ApplyParamShape(FLOAT_TYPE normVal, FLOAT_TYPE shape)
{
    return std::pow(normVal, (FLOAT_TYPE)(1.0/shape));
}
template float BLUtils::ApplyParamShape(float normVal, float shape);
template double BLUtils::ApplyParamShape(double normVal, double shape);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyParamShape(WDL_TypedBuf<FLOAT_TYPE> *normVals, FLOAT_TYPE shape)
{
    for (int i = 0; i < normVals->GetSize(); i++)
    {
        FLOAT_TYPE normVal = normVals->Get()[i];
        
        normVal = std::pow(normVal, (FLOAT_TYPE)(1.0/shape));
        
        normVals->Get()[i] = normVal;
    }
}
template void BLUtils::ApplyParamShape(WDL_TypedBuf<float> *normVals, float shape);
template void BLUtils::ApplyParamShape(WDL_TypedBuf<double> *normVals, double shape);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyParamShapeWaveform(WDL_TypedBuf<FLOAT_TYPE> *normVals, FLOAT_TYPE shape)
{
    for (int i = 0; i < normVals->GetSize(); i++)
    {
        FLOAT_TYPE normVal = normVals->Get()[i];
        
        bool neg = (normVal < 0.0);
        if(neg)
            normVal = -normVal;
        
        normVal = std::pow(normVal, (FLOAT_TYPE)(1.0/shape));
        
        if(neg)
            normVal = -normVal;
        
        normVals->Get()[i] = normVal;
    }
}
template void BLUtils::ApplyParamShapeWaveform(WDL_TypedBuf<float> *normVals,
                                               float shape);
template void BLUtils::ApplyParamShapeWaveform(WDL_TypedBuf<double> *normVals,
                                               double shape);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeShapeForCenter0(FLOAT_TYPE minKnobValue, FLOAT_TYPE maxKnobValue)
{
    // Normalized position of the zero
    FLOAT_TYPE normZero = -minKnobValue/(maxKnobValue - minKnobValue);
    
    FLOAT_TYPE shape = std::log((BL_FLOAT)0.5)/std::log(normZero);
    shape = 1.0/shape;
    
    return shape;
}
template float BLUtils::ComputeShapeForCenter0(float minKnobValue,
                                               float maxKnobValue);
template double BLUtils::ComputeShapeForCenter0(double minKnobValue,
                                                double maxKnobValue);

template <typename FLOAT_TYPE>
void
BLUtils::ResizeLinear(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer,
                      int newSize)
{
    FLOAT_TYPE *data = ioBuffer->Get();
    int size = ioBuffer->GetSize();
    
    WDL_TypedBuf<FLOAT_TYPE> result;
    BLUtils::ResizeFillZeros(&result, newSize);
    
    FLOAT_TYPE ratio = ((FLOAT_TYPE)(size - 1))/newSize;
    
    FLOAT_TYPE  *resultData = result.Get();
    
    FLOAT_TYPE pos = 0.0;
    for (int i = 0; i < newSize; i++)
    {
        // Optim
        //int x = (int)(ratio * i);
        //FLOAT_TYPE diff = (ratio * i) - x;
        
        int x = (int)pos;
        FLOAT_TYPE diff = pos - x;
        pos += ratio;
        
        if (x >= size - 1)
            continue;
        
        FLOAT_TYPE a = data[x];
        FLOAT_TYPE b = data[x + 1];
        
        FLOAT_TYPE val = a*(1.0 - diff) + b*diff;
        
        resultData[i] = val;
    }
    
    *ioBuffer = result;
}
template void BLUtils::ResizeLinear(WDL_TypedBuf<double> *ioBuffer, int newSize);
template void BLUtils::ResizeLinear(WDL_TypedBuf<float> *ioBuffer, int newSize);

template <typename FLOAT_TYPE>
void
BLUtils::ResizeLinear(WDL_TypedBuf<FLOAT_TYPE> *rescaledBuf,
                      const WDL_TypedBuf<FLOAT_TYPE> &buf,
                      int newSize)
{
    FLOAT_TYPE *data = buf.Get();
    int size = buf.GetSize();
    
    BLUtils::ResizeFillZeros(rescaledBuf, newSize);
    
    FLOAT_TYPE ratio = ((FLOAT_TYPE)(size - 1))/newSize;
    
    FLOAT_TYPE *rescaledBufData = rescaledBuf->Get();
    
    FLOAT_TYPE pos = 0.0;
    for (int i = 0; i < newSize; i++)
    {
        // Optim
        //int x = (int)(ratio * i);
        //FLOAT_TYPE diff = (ratio * i) - x;
        
        int x = (int)pos;
        FLOAT_TYPE diff = pos - x;
        pos += ratio;
        
        if (x >= size - 1)
            continue;
        
        FLOAT_TYPE a = data[x];
        FLOAT_TYPE b = data[x + 1];
        
        FLOAT_TYPE val = a*(1.0 - diff) + b*diff;
        
        rescaledBufData[i] = val;
    }
}
template void BLUtils::ResizeLinear(WDL_TypedBuf<float> *rescaledBuf,
                                    const WDL_TypedBuf<float> &buf,
                                    int newSize);
template void BLUtils::ResizeLinear(WDL_TypedBuf<double> *rescaledBuf,
                                    const WDL_TypedBuf<double> &buf,
                                    int newSize);

template <typename FLOAT_TYPE>
void
BLUtils::ResizeLinear2(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer,
                       int newSize)
{
    if (ioBuffer->GetSize() == 0)
        return;
    
    if (newSize == 0)
        return;
    
    // Fix last value
    FLOAT_TYPE lastValue = ioBuffer->Get()[ioBuffer->GetSize() - 1];
    
    FLOAT_TYPE *data = ioBuffer->Get();
    int size = ioBuffer->GetSize();
    
    WDL_TypedBuf<FLOAT_TYPE> result;
    BLUtils::ResizeFillZeros(&result, newSize);
    
    FLOAT_TYPE ratio = ((FLOAT_TYPE)(size - 1))/newSize;
    
    FLOAT_TYPE *resultData = result.Get();
    
    FLOAT_TYPE pos = 0.0;
    for (int i = 0; i < newSize; i++)
    {
        // Optim
        //int x = (int)(ratio * i);
        //FLOAT_TYPE diff = (ratio * i) - x;
        
        int x = (int)pos;
        FLOAT_TYPE diff = pos - x;
        pos += ratio;
        
        if (x >= size - 1)
            continue;
            
        FLOAT_TYPE a = data[x];
        FLOAT_TYPE b = data[x + 1];
        
        FLOAT_TYPE val = a*(1.0 - diff) + b*diff;
        
        resultData[i] = val;
    }
    
    *ioBuffer = result;
    
    // Fix last value
    ioBuffer->Get()[ioBuffer->GetSize() - 1] = lastValue;
}
template void BLUtils::ResizeLinear2(WDL_TypedBuf<float> *ioBuffer,
                                     int newSize);
template void BLUtils::ResizeLinear2(WDL_TypedBuf<double> *ioBuffer,
                                     int newSize);

#if 0 // Disable, to avoid including libmfcc in each plugins
// Use https://github.com/jsawruk/libmfcc
//
// NOTE: Not tested !
void
BLUtils::FreqsToMfcc(WDL_TypedBuf<FLOAT_TYPE> *result,
                     const WDL_TypedBuf<FLOAT_TYPE> freqs,
                     FLOAT_TYPE sampleRate)
{
    int numFreqs = freqs.GetSize();
    FLOAT_TYPE *spectrum = freqs.Get();
    
    int numFilters = 48;
    int numCoeffs = 13;
    
    result->Resize(numCoeffs);
    
    FLOAT_TYPE *resultData = result->Get();
    
    for (int coeff = 0; coeff < numCoeffs; coeff++)
	{
		FLOAT_TYPE res =
            GetCoefficient(spectrum, (int)sampleRate, numFilters, 128, coeff);
        
        resultData[coeff] = res;
	}
}
#endif

// See: https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FreqToMel(FLOAT_TYPE freq)
{
    FLOAT_TYPE res = 2595.0*std::log10((BL_FLOAT)(1.0 + freq/700.0));
    
    return res;
}
template float BLUtils::FreqToMel(float freq);
template double BLUtils::FreqToMel(double freq);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::MelToFreq(FLOAT_TYPE mel)
{
    FLOAT_TYPE res =
        700.0*(std::pow((FLOAT_TYPE)10.0, (FLOAT_TYPE)(mel/2595.0)) - 1.0);
    
    return res;
}
template float BLUtils::MelToFreq(float mel);
template double BLUtils::MelToFreq(double mel);

// VERY GOOD !

// OPTIM PROF Infra
#if 0 // ORIGIN
void
BLUtils::FreqsToMelNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                        const WDL_TypedBuf<FLOAT_TYPE> &magns,
                        FLOAT_TYPE hzPerBin,
                        FLOAT_TYPE zeroValue)
{
    //BLUtils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    // For dB
    resultMagns->Resize(magns.GetSize());
    BLUtils::FillAllValue(resultMagns, zeroValue);
    
    FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    // Optim
    FLOAT_TYPE melCoeff = maxMel/resultMagns->GetSize();
    FLOAT_TYPE idCoeff = (1.0/maxFreq)*resultMagns->GetSize();
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = mangs.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        // Optim
        //FLOAT_TYPE mel = i*maxMel/resultMagns->GetSize();
        FLOAT_TYPE mel = i*melCoeff;
        
        FLOAT_TYPE freq = MelToFreq(mel);
        
        if (maxFreq < BL_EPS)
            return;
        
        // Optim
        //FLOAT_TYPE id0 = (freq/maxFreq) * resultMagns->GetSize();
        FLOAT_TYPE id0 = freq*idCoeff;
        
        FLOAT_TYPE t = id0 - (int)(id0);
        
        if ((int)id0 >= magnsSize)
            continue;
        
        int id1 = id0 + 1;
        if (id1 >= magnsSize)
            continue;
        
        FLOAT_TYPE magn0 = magnsData[(int)id0];
        FLOAT_TYPE magn1 = magnsData[id1];
        
        FLOAT_TYPE magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagnsData[i] = magn;
    }
}
#else // OPTIMIZED
template <typename FLOAT_TYPE>
void
BLUtils::FreqsToMelNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                        const WDL_TypedBuf<FLOAT_TYPE> &magns,
                        FLOAT_TYPE hzPerBin,
                        FLOAT_TYPE zeroValue)
{
    //BLUtils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    // For dB
    resultMagns->Resize(magns.GetSize());
    BLUtils::FillAllValue(resultMagns, zeroValue);
    
    FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    // Optim
    FLOAT_TYPE melCoeff = maxMel/resultMagns->GetSize();
    FLOAT_TYPE idCoeff = (1.0/maxFreq)*resultMagns->GetSize();
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        FLOAT_TYPE mel = i*melCoeff;
        
        FLOAT_TYPE freq = MelToFreq(mel);
        
        if (maxFreq < BL_EPS)
            return;
        
        FLOAT_TYPE id0 = freq*idCoeff;
        
        // Optim
        // (cast from FLOAT_TYPE to int is costly)
        int id0i = (int)id0;
        
        FLOAT_TYPE t = id0 - id0i;
        
        if (id0i >= magnsSize)
            continue;
        
        // NOTE: this optim doesn't compute exactly the same thing than the original version
        int id1 = id0i + 1;
        if (id1 >= magnsSize)
            continue;
        
        FLOAT_TYPE magn0 = magnsData[id0i];
        FLOAT_TYPE magn1 = magnsData[id1];
        
        FLOAT_TYPE magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagnsData[i] = magn;
    }
}
template void BLUtils::FreqsToMelNorm(WDL_TypedBuf<float> *resultMagns,
                                      const WDL_TypedBuf<float> &magns,
                                      float hzPerBin,
                                      float zeroValue);
template void BLUtils::FreqsToMelNorm(WDL_TypedBuf<double> *resultMagns,
                                      const WDL_TypedBuf<double> &magns,
                                      double hzPerBin,
                                      double zeroValue);
#endif

template <typename FLOAT_TYPE>
void
BLUtils::MelToFreqsNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                        const WDL_TypedBuf<FLOAT_TYPE> &magns,
                        FLOAT_TYPE hzPerBin, FLOAT_TYPE zeroValue)
{
    //BLUtils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    // For dB
    resultMagns->Resize(magns.GetSize());
    BLUtils::FillAllValue(resultMagns, zeroValue);
    
    FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    
    // OPTIM: avoid division in loop
    BL_FLOAT coeff = (1.0/maxMel)*resultMagnsSize;
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        FLOAT_TYPE freq = hzPerBin*i;
        FLOAT_TYPE mel = FreqToMel(freq);
        
        //FLOAT_TYPE id0 = (mel/maxMel) * resultMagnsSize;
        FLOAT_TYPE id0 = mel*coeff;
        
        if ((int)id0 >= magnsSize)
            continue;
            
        // Linear
        FLOAT_TYPE t = id0 - (int)(id0);
        
        int id1 = id0 + 1;
        if (id1 >= magnsSize)
            continue;
        
        FLOAT_TYPE magn0 = magnsData[(int)id0];
        FLOAT_TYPE magn1 = magnsData[id1];
        
        FLOAT_TYPE magn = (1.0 - t)*magn0 + t*magn1;
        
        // Nearest
        //FLOAT_TYPE magn = magns.Get()[(int)id0];
        
        resultMagnsData[i] = magn;
    }
}
template void BLUtils::MelToFreqsNorm(WDL_TypedBuf<float> *resultMagns,
                                      const WDL_TypedBuf<float> &magns,
                                      float hzPerBin, float zeroValue);
template void BLUtils::MelToFreqsNorm(WDL_TypedBuf<double> *resultMagns,
                                      const WDL_TypedBuf<double> &magns,
                                      double hzPerBin, double zeroValue);

template <typename FLOAT_TYPE>
void
BLUtils::MelToFreqsNorm2(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                         const WDL_TypedBuf<FLOAT_TYPE> &magns,
                         FLOAT_TYPE hzPerBin, FLOAT_TYPE zeroValue)
{
    // For dB
    resultMagns->Resize(magns.GetSize());
    BLUtils::FillAllValue(resultMagns, zeroValue);
    
    FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        FLOAT_TYPE freq0 = hzPerBin*(i - 0.5);
        FLOAT_TYPE mel0 = FreqToMel(freq0);
        FLOAT_TYPE id0 = (mel0/maxMel) * resultMagnsSize;
        if (id0 < 0.0)
            id0 = 0.0;
        if ((int)id0 >= magnsSize)
            continue;
        
        FLOAT_TYPE freq1 = hzPerBin*(i + 0.5);
        FLOAT_TYPE mel1 = FreqToMel(freq1);
        FLOAT_TYPE id1 = (mel1/maxMel) * resultMagnsSize;
        if ((int)id1 >= magnsSize)
            id1 = magnsSize - 1;
        
        FLOAT_TYPE magn = 0.0;
        
        if (id1 - id0 > 1.0)
            // Scaling down
            // Scaling from biggest scale => must take several values
        {
            // Compute simple average
            FLOAT_TYPE avg = 0.0;
            int numValues = 0;
            for (int k = (int)id0; k <= (int)id1; k++)
            {
                avg += magnsData[k];
                numValues++;
            }
            if (numValues > 0)
                avg /= numValues;
            
            magn = avg;
        }
        else
        {
            // Scaling up
            // Scaling from smallest scale => must interpolate on the distination
            FLOAT_TYPE freq = hzPerBin*i;
            FLOAT_TYPE mel = FreqToMel(freq);
            
            FLOAT_TYPE i0 = (mel/maxMel) * resultMagnsSize;
            
            if ((int)i0 >= magnsSize)
                continue;
            
            // Linear
            FLOAT_TYPE t = i0 - (int)(i0);
            
            int i1 = i0 + 1;
            if (i1 >= magnsSize)
                continue;
            
            FLOAT_TYPE magn0 = magnsData[(int)i0];
            FLOAT_TYPE magn1 = magnsData[i1];
            
            magn = (1.0 - t)*magn0 + t*magn1;
        }
        
        // Nearest
        //FLOAT_TYPE magn = magns.Get()[(int)id0];
        
        resultMagnsData[i] = magn;
    }
}
template void BLUtils::MelToFreqsNorm2(WDL_TypedBuf<float> *resultMagns,
                                       const WDL_TypedBuf<float> &magns,
                                       float hzPerBin, float zeroValue);
template void BLUtils::MelToFreqsNorm2(WDL_TypedBuf<double> *resultMagns,
                                       const WDL_TypedBuf<double> &magns,
                                       double hzPerBin, double zeroValue);

template <typename FLOAT_TYPE>
void
BLUtils::FreqsToMelNorm2(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                         const WDL_TypedBuf<FLOAT_TYPE> &magns,
                         FLOAT_TYPE hzPerBin,
                         FLOAT_TYPE zeroValue)
{
    // For dB
    resultMagns->Resize(magns.GetSize());
    BLUtils::FillAllValue(resultMagns, zeroValue);
    
    FLOAT_TYPE maxFreq = hzPerBin*(magns.GetSize() - 1);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    if (maxFreq < BL_EPS)
        return;
    
    // Optim
    FLOAT_TYPE melCoeff = maxMel/resultMagns->GetSize();
    FLOAT_TYPE idCoeff = (1.0/maxFreq)*resultMagns->GetSize();
    
    int resultMagnsSize = resultMagns->GetSize();
    FLOAT_TYPE *resultMagnsData = resultMagns->Get();
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    
    for (int i = 0; i < resultMagnsSize; i++)
    {
        FLOAT_TYPE mel0 = (i - 0.5)*melCoeff;
        if (mel0 < 0.0)
            mel0 = 0.0;
        FLOAT_TYPE freq0 = MelToFreq(mel0);
        FLOAT_TYPE id0 = freq0*idCoeff;
        if (id0 < 0.0)
            id0 = 0.0;
        if ((int)id0 >= magnsSize)
            continue;
        
        FLOAT_TYPE mel1 = (i + 0.5)*melCoeff;
        if (mel1 < 0.0)
            mel1 = 0.0;
        FLOAT_TYPE freq1 = MelToFreq(mel1);
        FLOAT_TYPE id1 = freq1*idCoeff;
        if ((int)id1 >= magnsSize)
            id1 = magnsSize - 1;
        
        FLOAT_TYPE magn = 0.0;
        
        if (id1 - id0 > 1.0)
            // Scaling down
            // Scaling from biggest scale => must take several values
        {
            // Compute simple average
            FLOAT_TYPE avg = 0.0;
            int numValues = 0;
            for (int k = (int)id0; k <= (int)id1; k++)
            {
                avg += magnsData[k];
                numValues++;
            }
            if (numValues > 0)
                avg /= numValues;
            
            magn = avg;
        }
        else
        {
            // Scaling up
            // Scaling from smallest scale => must interpolate on the distination
            FLOAT_TYPE mel = i*melCoeff;
            
            FLOAT_TYPE freq = MelToFreq(mel);
            
            if (maxFreq < BL_EPS)
                return;
            
            FLOAT_TYPE i0 = freq*idCoeff;
            
            // Optim
            // (cast from FLOAT_TYPE to int is costly)
            int ii0 = (int)i0;
            
            FLOAT_TYPE t = i0 - ii0;
            
            if (ii0 >= magnsSize)
                continue;
            
            // NOTE: this optim doesn't compute exactly the same thing than the original version
            int i1 = ii0 + 1;
            if (i1 >= magnsSize)
                continue;
            
            FLOAT_TYPE magn0 = magnsData[ii0];
            FLOAT_TYPE magn1 = magnsData[i1];
            
            magn = (1.0 - t)*magn0 + t*magn1;
        }
        
        resultMagnsData[i] = magn;
    }
}
template void BLUtils::FreqsToMelNorm2(WDL_TypedBuf<float> *resultMagns,
                                       const WDL_TypedBuf<float> &magns,
                                       float hzPerBin,
                                       float zeroValue);
template void BLUtils::FreqsToMelNorm2(WDL_TypedBuf<double> *resultMagns,
                                       const WDL_TypedBuf<double> &magns,
                                       double hzPerBin,
                                       double zeroValue);

// Ok
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::MelNormIdToFreq(FLOAT_TYPE idx, FLOAT_TYPE hzPerBin, int bufferSize)
{
    //FLOAT_TYPE maxFreq = hzPerBin*(bufferSize - 1);
    FLOAT_TYPE maxFreq = hzPerBin*(bufferSize/2);
    
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    FLOAT_TYPE mel = (idx/(bufferSize/2))*maxMel;
    
    FLOAT_TYPE result = MelToFreq(mel);
    
    return result;
}
template float BLUtils::MelNormIdToFreq(float idx, float hzPerBin, int bufferSize);
template double BLUtils::MelNormIdToFreq(double idx, double hzPerBin, int bufferSize);

// GOOD !
template <typename FLOAT_TYPE>
int
BLUtils::FreqIdToMelNormId(int idx, FLOAT_TYPE hzPerBin, int bufferSize)
{
    FLOAT_TYPE maxFreq = hzPerBin*(bufferSize/2);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    FLOAT_TYPE freq = hzPerBin*idx;
    FLOAT_TYPE mel = FreqToMel(freq);
        
    FLOAT_TYPE resultId = (mel/maxMel)*(bufferSize/2);
    
    resultId = bl_round(resultId);
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}
template int BLUtils::FreqIdToMelNormId(int idx, float hzPerBin, int bufferSize);
template int BLUtils::FreqIdToMelNormId(int idx, double hzPerBin, int bufferSize);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FreqIdToMelNormIdF(FLOAT_TYPE idx, FLOAT_TYPE hzPerBin, int bufferSize)
{
    FLOAT_TYPE maxFreq = hzPerBin*(bufferSize/2);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    FLOAT_TYPE freq = hzPerBin*idx;
    FLOAT_TYPE mel = FreqToMel(freq);
    
    FLOAT_TYPE resultId = (mel/maxMel)*(bufferSize/2);
    
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}
template float BLUtils::FreqIdToMelNormIdF(float idx, float hzPerBin,
                                           int bufferSize);
template double BLUtils::FreqIdToMelNormIdF(double idx, double hzPerBin,
                                            int bufferSize);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FreqToMelNormId(FLOAT_TYPE freq, FLOAT_TYPE hzPerBin, int bufferSize)
{
    FLOAT_TYPE maxFreq = hzPerBin*(bufferSize/2);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    FLOAT_TYPE mel = FreqToMel(freq);
    
    FLOAT_TYPE resultId = (mel/maxMel)*(bufferSize/2);
    
    resultId = bl_round(resultId);
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}
template float BLUtils::FreqToMelNormId(float freq, float hzPerBin,
                                        int bufferSize);
template double BLUtils::FreqToMelNormId(double freq, double hzPerBin,
                                         int bufferSize);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::FreqToMelNorm(FLOAT_TYPE freq, FLOAT_TYPE hzPerBin, int bufferSize)
{
    FLOAT_TYPE maxFreq = hzPerBin*(bufferSize/2);
    FLOAT_TYPE maxMel = FreqToMel(maxFreq);
    
    FLOAT_TYPE mel = FreqToMel(freq);
    
    FLOAT_TYPE result = mel/maxMel;
    
    return result;
}
template float BLUtils::FreqToMelNorm(float freq, float hzPerBin, int bufferSize);
template double BLUtils::FreqToMelNorm(double freq, double hzPerBin, int bufferSize);

template <typename FLOAT_TYPE>
void
BLUtils::MixParamToCoeffs(FLOAT_TYPE mix, FLOAT_TYPE *coeff0, FLOAT_TYPE *coeff1)
{
    //mix = (mix - 0.5)*2.0;
    
    if (mix <= 0.0)
    {
        *coeff0 = 1.0;
        *coeff1 = 1.0 + mix;
    }
    else if (mix > 0.0)
    {
        *coeff0 = 1.0 - mix;
        *coeff1 = 1.0;
    }
}
template void BLUtils::MixParamToCoeffs(float mix, float *coeff0, float *coeff1);
template void BLUtils::MixParamToCoeffs(double mix, double *coeff0, double *coeff1);

template <typename FLOAT_TYPE>
void
BLUtils::SmoothDataWin(WDL_TypedBuf<FLOAT_TYPE> *ioData,
                       const WDL_TypedBuf<FLOAT_TYPE> &win)
{
    WDL_TypedBuf<FLOAT_TYPE> data = *ioData;
    SmoothDataWin(ioData, data, win);
}
template void BLUtils::SmoothDataWin(WDL_TypedBuf<float> *ioData,
                                     const WDL_TypedBuf<float> &win);
template void BLUtils::SmoothDataWin(WDL_TypedBuf<double> *ioData,
                                     const WDL_TypedBuf<double> &win);

template <typename FLOAT_TYPE>
void
BLUtils::SmoothDataWin(WDL_TypedBuf<FLOAT_TYPE> *result,
                       const WDL_TypedBuf<FLOAT_TYPE> &data,
                       const WDL_TypedBuf<FLOAT_TYPE> &win)
{
    result->Resize(data.GetSize());
    
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    int winSize = win.GetSize();
    FLOAT_TYPE *winData = win.Get();
    FLOAT_TYPE *dataData = data.Get();

    int halfWinSize = winSize/2;
    
    for (int i = 0; i < resultSize; i++)
    {
        FLOAT_TYPE sumVal = 0.0;
        FLOAT_TYPE sumCoeff = 0.0;
        for (int j = 0; j < winSize; j++)
        {
            //int idx = i + j - winSize/2;
            int idx = i + j - halfWinSize;
            
            if ((idx < 0) || (idx > resultSize - 1))
                continue;
            
            FLOAT_TYPE val = dataData[idx];
            FLOAT_TYPE coeff = winData[j];
            
            sumVal += val*coeff;
            sumCoeff += coeff;
        }
        
        if (sumCoeff > BL_EPS)
        {
            sumVal /= sumCoeff;
        }
        
        resultData[i] = sumVal;
    }
}
template void BLUtils::SmoothDataWin(WDL_TypedBuf<float> *result,
                                     const WDL_TypedBuf<float> &data,
                                     const WDL_TypedBuf<float> &win);
template void BLUtils::SmoothDataWin(WDL_TypedBuf<double> *result,
                                     const WDL_TypedBuf<double> &data,
                                     const WDL_TypedBuf<double> &win);

template <typename FLOAT_TYPE>
void
BLUtils::SmoothDataPyramid(WDL_TypedBuf<FLOAT_TYPE> *result,
                           const WDL_TypedBuf<FLOAT_TYPE> &data,
                           int maxLevel)
{
    WDL_TypedBuf<FLOAT_TYPE> xCoordinates;
    xCoordinates.Resize(data.GetSize());
    
    int dataSize = data.GetSize();
    FLOAT_TYPE *xCoordinatesData = xCoordinates.Get();
    
    for (int i = 0; i < dataSize; i++)
    {
        xCoordinatesData[i] = i;
    }
    
    WDL_TypedBuf<FLOAT_TYPE> yCoordinates = data;
    
    int level = 1;
    while(level < maxLevel)
    {
        WDL_TypedBuf<FLOAT_TYPE> newXCoordinates;
        WDL_TypedBuf<FLOAT_TYPE> newYCoordinates;
        
        // Comppute new size
        FLOAT_TYPE newSize = xCoordinates.GetSize()/2.0;
        newSize = ceil(newSize);
        
        // Prepare
        newXCoordinates.Resize((int)newSize);
        newYCoordinates.Resize((int)newSize);
        
        // Copy the last value (in case of odd size)
        newXCoordinates.Get()[(int)newSize - 1] =
        xCoordinates.Get()[xCoordinates.GetSize() - 1];
        newYCoordinates.Get()[(int)newSize - 1] =
        yCoordinates.Get()[yCoordinates.GetSize() - 1];
        
        // Divide by 2
        
        int xCoordinatesSize = xCoordinates.GetSize();
        FLOAT_TYPE *xCoordinatesData2 = xCoordinates.Get();
        int yCoordinatesSize = yCoordinates.GetSize();
        FLOAT_TYPE *yCoordinatesData = yCoordinates.Get();
        FLOAT_TYPE *newXCoordinatesData = newXCoordinates.Get();
        FLOAT_TYPE *newYCoordinatesData = newYCoordinates.Get();
        
        for (int i = 0; i < xCoordinatesSize; i += 2)
        {
            // x
            FLOAT_TYPE x0 = xCoordinatesData2[i];
            FLOAT_TYPE x1 = x0;
            if (i + 1 < xCoordinatesSize)
                x1 = xCoordinatesData2[i + 1];
            
            // y
            FLOAT_TYPE y0 = yCoordinatesData[i];
            
            FLOAT_TYPE y1 = y0;
            if (i + 1 < yCoordinatesSize)
                y1 = yCoordinatesData[i + 1];
            
            // new
            FLOAT_TYPE newX = (x0 + x1)/2.0;
            FLOAT_TYPE newY = (y0 + y1)/2.0;
            
            // add
            newXCoordinatesData[i/2] = newX;
            newYCoordinatesData[i/2] = newY;
        }
        
        xCoordinates = newXCoordinates;
        yCoordinates = newYCoordinates;
        
        level++;
    }
    
    // Prepare the result
    result->Resize(data.GetSize());
    BLUtils::FillAllZero(result);
    
    // Put the low LOD values into the result
    
    int xCoordinatesSize = xCoordinates.GetSize();
    FLOAT_TYPE *xCoordinatesData3 = xCoordinates.Get();
    FLOAT_TYPE *yCoordinatesData = yCoordinates.Get();
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    for (int i = 0; i < xCoordinatesSize; i++)
    {
        FLOAT_TYPE x = xCoordinatesData3[i];
        FLOAT_TYPE y = yCoordinatesData[i];
        
        x = bl_round(x);
        
        // check bounds
        if (x < 0.0)
            x = 0.0;
        if (x > resultSize - 1)
            x = resultSize - 1;
        
        resultData[(int)x] = y;
    }
    
    // And complete the values that remained zero
    BLUtils::FillMissingValues(result, false, (FLOAT_TYPE)0.0);
}
template void BLUtils::SmoothDataPyramid(WDL_TypedBuf<float> *result,
                                         const WDL_TypedBuf<float> &data,
                                         int maxLevel);
template void BLUtils::SmoothDataPyramid(WDL_TypedBuf<double> *result,
                                         const WDL_TypedBuf<double> &data,
                                         int maxLevel);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyWindowMin(WDL_TypedBuf<FLOAT_TYPE> *values, int winSize)
{
    WDL_TypedBuf<FLOAT_TYPE> result;
    result.Resize(values->GetSize());
    
    int resultSize = result.GetSize();
    FLOAT_TYPE *resultData = result.Get();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < resultSize; i++)
    {
        FLOAT_TYPE minVal = INF;
        for (int j = 0; j < winSize; j++)
        {
            int idx = i + j - winSize/2;
            
            if ((idx < 0) || (idx > resultSize - 1))
                continue;
            
            FLOAT_TYPE val = valuesData[idx];
            if (val < minVal)
                minVal = val;
        }
        
        resultData[i] = minVal;
    }
    
    *values = result;
}
template void BLUtils::ApplyWindowMin(WDL_TypedBuf<float> *values, int winSize);
template void BLUtils::ApplyWindowMin(WDL_TypedBuf<double> *values, int winSize);

template <typename FLOAT_TYPE>
void
BLUtils::ApplyWindowMax(WDL_TypedBuf<FLOAT_TYPE> *values, int winSize)
{
    WDL_TypedBuf<FLOAT_TYPE> result;
    result.Resize(values->GetSize());
    
    int resultSize = result.GetSize();
    FLOAT_TYPE *resultData = result.Get();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < resultSize; i++)
    {
        FLOAT_TYPE maxVal = -INF;
        for (int j = 0; j < winSize; j++)
        {
            int idx = i + j - winSize/2;
            
            if ((idx < 0) || (idx > resultSize - 1))
                continue;
            
            FLOAT_TYPE val = valuesData[idx];
            if (val > maxVal)
                maxVal = val;
        }
        
        resultData[i] = maxVal;
    }
    
    *values = result;
}
template void BLUtils::ApplyWindowMax(WDL_TypedBuf<float> *values, int winSize);
template void BLUtils::ApplyWindowMax(WDL_TypedBuf<double> *values, int winSize);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMean(const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    if (values.GetSize() == 0)
        return 0.0;
    
    // SIMD
    
    //int valuesSize = values.GetSize();
    //FLOAT_TYPE *valuesData = values.Get();
    
    FLOAT_TYPE sum = ComputeSum(values);
    
    //FLOAT_TYPE sum = 0.0;
    //for (int i = 0; i < valuesSize; i++)
    //{
    //    FLOAT_TYPE val = valuesData[i];
    //
    //    sum += val;
    //}
    
    FLOAT_TYPE result = sum/values.GetSize();
    
    return result;
}
template float BLUtils::ComputeMean(const WDL_TypedBuf<float> &values);
template double BLUtils::ComputeMean(const WDL_TypedBuf<double> &values);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeSigma(const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    if (values.GetSize() == 0)
        return 0.0;
    
    FLOAT_TYPE mean = ComputeMean(values);
    
    FLOAT_TYPE sum = 0.0;
    
    int valuesSize = values.GetSize();
    FLOAT_TYPE *valuesData = values.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        FLOAT_TYPE diff = std::fabs(val - mean);
        
        sum += diff;
    }
    
    FLOAT_TYPE result = sum/values.GetSize();
    
    return result;
}
template float BLUtils::ComputeSigma(const WDL_TypedBuf<float> &values);
template double BLUtils::ComputeSigma(const WDL_TypedBuf<double> &values);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeMean(const vector<FLOAT_TYPE> &values)
{
    if (values.empty())
        return 0.0;
    
    FLOAT_TYPE sum = 0.0;
    for (int i = 0; i < values.size(); i++)
    {
        FLOAT_TYPE val = values[i];
        
        sum += val;
    }
    
    FLOAT_TYPE result = sum/values.size();
    
    return result;
}
template float BLUtils::ComputeMean(const vector<float> &values);
template double BLUtils::ComputeMean(const vector<double> &values);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeSigma(const vector<FLOAT_TYPE> &values)
{
    if (values.size())
        return 0.0;
    
    FLOAT_TYPE mean = ComputeMean(values);
    
    FLOAT_TYPE sum = 0.0;
    for (int i = 0; i < values.size(); i++)
    {
        FLOAT_TYPE val = values[i];
        
        FLOAT_TYPE diff = std::fabs(val - mean);
        
        sum += diff;
    }
    
    FLOAT_TYPE result = sum/values.size();
    
    return result;
}
template float BLUtils::ComputeSigma(const vector<float> &values);
template double BLUtils::ComputeSigma(const vector<double> &values);

template <typename FLOAT_TYPE>
void
BLUtils::GetMinMaxFreqAxisValues(FLOAT_TYPE *minHzValue, FLOAT_TYPE *maxHzValue,
                                 int bufferSize, FLOAT_TYPE sampleRate)
{
    FLOAT_TYPE hzPerBin = sampleRate/bufferSize;
    
    *minHzValue = 1*hzPerBin;
    *maxHzValue = (bufferSize/2)*hzPerBin;
}
template void BLUtils::GetMinMaxFreqAxisValues(float *minHzValue, float *maxHzValue,
                                               int bufferSize, float sampleRate);
template void BLUtils::GetMinMaxFreqAxisValues(double *minHzValue, double *maxHzValue,
                                               int bufferSize, double sampleRate);

template <typename FLOAT_TYPE>
void
BLUtils::GenNoise(WDL_TypedBuf<FLOAT_TYPE> *ioBuf)
{
    int ioBufSize = ioBuf->GetSize();
    FLOAT_TYPE *ioBufData = ioBuf->Get();
    
    for (int i = 0; i < ioBufSize; i++)
    {
        FLOAT_TYPE noise = ((FLOAT_TYPE)std::rand())/RAND_MAX;
        ioBufData[i] = noise;
    }
}
template void BLUtils::GenNoise(WDL_TypedBuf<float> *ioBuf);
template void BLUtils::GenNoise(WDL_TypedBuf<double> *ioBuf);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::ComputeDist(FLOAT_TYPE p0[2], FLOAT_TYPE p1[2])
{
    FLOAT_TYPE a = p0[0] - p1[0];
    FLOAT_TYPE b = p0[1] - p1[1];
    
    FLOAT_TYPE d2 = a*a + b*b;
    FLOAT_TYPE dist = std::sqrt(d2);
    
    return dist;
}
template float BLUtils::ComputeDist(float p0[2], float p1[2]);
template double BLUtils::ComputeDist(double p0[2], double p1[2]);

template <typename FLOAT_TYPE>
void
BLUtils::CutHalfSamples(WDL_TypedBuf<FLOAT_TYPE> *samples, bool cutDown)
{
    // FIX: fixes scrolling and spreading the last value when stop playing
    // - UST: feed clipper also when not playing
    // => the last value scrolls (that makes a line)
#define FIX_SPREAD_LAST_VALUE 1
    
    // Find first value
    FLOAT_TYPE firstValue = 0.0;
    
    for (int i = 0; i < samples->GetSize(); i++)
    {
        FLOAT_TYPE samp = samples->Get()[i];
        if ((samp >= 0.0) && cutDown)
        {
            firstValue = samp;
            break;
        }
        
        if ((samp <= 0.0) && !cutDown)
        {
            firstValue = samp;
            break;
        }
    }
    
    // Cut half, and replace undefined values by prev valid value
    FLOAT_TYPE prevValue = firstValue;
    for (int i = 0; i < samples->GetSize(); i++)
    {
        FLOAT_TYPE samp = samples->Get()[i];
        
#if !FIX_SPREAD_LAST_VALUE
        if ((samp >= 0.0) && !cutDown)
#else
            if ((samp > 0.0) && !cutDown)
#endif
            {
                samp = prevValue;
            
                samples->Get()[i] = samp;
            
                continue;
            }

#if !FIX_SPREAD_LAST_VALUE
        if ((samp <= 0.0) && cutDown)
#else
            if ((samp < 0.0) && cutDown)
#endif
            {
                samp = prevValue;
            
                samples->Get()[i] = samp;
            
                continue;
            }
        
        prevValue = samp;
    }
}
template void BLUtils::CutHalfSamples(WDL_TypedBuf<float> *samples, bool cutDown);
template void BLUtils::CutHalfSamples(WDL_TypedBuf<double> *samples, bool cutDown);

template <typename FLOAT_TYPE>
bool
BLUtils::IsMono(const WDL_TypedBuf<FLOAT_TYPE> &leftSamples,
                const WDL_TypedBuf<FLOAT_TYPE> &rightSamples,
                FLOAT_TYPE eps)
{
    if (leftSamples.GetSize() != rightSamples.GetSize())
        return false;
    
#if USE_SIMD
    int bufferSize = leftSamples.GetSize();
    const FLOAT_TYPE *buf0Data = leftSamples.Get();
    const FLOAT_TYPE *buf1Data = rightSamples.Get();
    
    if (_useSimd && (bufferSize % SIMD_PACK_SIZE == 0))
    {
        for (int i = 0; i < bufferSize; i += SIMD_PACK_SIZE)
        {
            simdpp::float64<SIMD_PACK_SIZE> v0 = simdpp::load(buf0Data);
            simdpp::float64<SIMD_PACK_SIZE> v1 = simdpp::load(buf1Data);
            
            simdpp::float64<SIMD_PACK_SIZE> d = v1 - v0;
            
            simdpp::float64<SIMD_PACK_SIZE> a = simdpp::abs(d);
            
            FLOAT_TYPE r = simdpp::reduce_max(a);
            if (r > eps)
                return false;
            
            buf0Data += SIMD_PACK_SIZE;
            buf1Data += SIMD_PACK_SIZE;
        }
        
        // Finished
        return true;
    }
#endif
    
    for (int i = 0; i < leftSamples.GetSize(); i++)
    {
        FLOAT_TYPE left = leftSamples.Get()[i];
        FLOAT_TYPE right = rightSamples.Get()[i];
        
        if (std::fabs(left - right) > eps)
            return false;
    }
    
    return true;
}
template bool BLUtils::IsMono(const WDL_TypedBuf<float> &leftSamples,
                              const WDL_TypedBuf<float> &rightSamples,
                              float eps);
template bool BLUtils::IsMono(const WDL_TypedBuf<double> &leftSamples,
                              const WDL_TypedBuf<double> &rightSamples,
                              double eps);

template <typename FLOAT_TYPE>
void
BLUtils::Smooth(WDL_TypedBuf<FLOAT_TYPE> *ioCurrentValues,
                WDL_TypedBuf<FLOAT_TYPE> *ioPrevValues,
                FLOAT_TYPE smoothFactor)
{
    if (ioCurrentValues->GetSize() != ioPrevValues->GetSize())
        return;
    
#if USE_SIMD_OPTIM
    int nFrames = ioCurrentValues->GetSize();
    FLOAT_TYPE *currentBuf = ioCurrentValues->Get();
    FLOAT_TYPE *prevBuf = ioPrevValues->Get();
    Mix(currentBuf, currentBuf, prevBuf, nFrames, smoothFactor);
    
    // *ioPrevValues = *ioCurrentValues;
#else
    for (int i = 0; i < ioCurrentValues->GetSize(); i++)
    {
        FLOAT_TYPE val = ioCurrentValues->Get()[i];
        FLOAT_TYPE prevVal = ioPrevValues->Get()[i];
        
        FLOAT_TYPE newVal = smoothFactor*prevVal + (1.0 - smoothFactor)*val;
        
        ioCurrentValues->Get()[i] = newVal;
    }
#endif
    
    // Warinig, this line ins important!
    *ioPrevValues = *ioCurrentValues;
}
template void BLUtils::Smooth(WDL_TypedBuf<float> *ioCurrentValues,
                              WDL_TypedBuf<float> *ioPrevValues,
                              float smoothFactor);
template void BLUtils::Smooth(WDL_TypedBuf<double> *ioCurrentValues,
                              WDL_TypedBuf<double> *ioPrevValues,
                              double smoothFactor);


template <typename FLOAT_TYPE>
void
BLUtils::Smooth(vector<WDL_TypedBuf<FLOAT_TYPE> > *ioCurrentValues,
                vector<WDL_TypedBuf<FLOAT_TYPE> > *ioPrevValues,
                FLOAT_TYPE smoothFactor)
{
    if (ioCurrentValues->size() != ioPrevValues->size())
        return;
    
    for (int i = 0; i < ioCurrentValues->size(); i++)
    {
        WDL_TypedBuf<FLOAT_TYPE> &current = (*ioCurrentValues)[i];
        //WDL_TypedBuf<FLOAT_TYPE> &prev = (*ioCurrentValues)[i];
        WDL_TypedBuf<FLOAT_TYPE> &prev = (*ioPrevValues)[i];
    
        if (current.GetSize() != prev.GetSize())
            return; // abort
        
#if !USE_SIMD_OPTIM
        for (int j = 0; j < current.GetSize(); j++)
        {
            FLOAT_TYPE val = current.Get()[j];
            FLOAT_TYPE prevVal = prev.Get()[j];
        
            FLOAT_TYPE newVal = smoothFactor*prevVal + (1.0 - smoothFactor)*val;
        
            current.Get()[j] = newVal;
        }
#else
        Smooth(&current, &prev, smoothFactor);
#endif
    }
    
    *ioPrevValues = *ioCurrentValues;
}
template void BLUtils::Smooth(vector<WDL_TypedBuf<float> > *ioCurrentValues,
                              vector<WDL_TypedBuf<float> > *ioPrevValues,
                              float smoothFactor);
template void BLUtils::Smooth(vector<WDL_TypedBuf<double> > *ioCurrentValues,
                              vector<WDL_TypedBuf<double> > *ioPrevValues,
                              double smoothFactor);

template <typename FLOAT_TYPE>
void
BLUtils::SmoothMax(WDL_TypedBuf<FLOAT_TYPE> *ioCurrentValues,
                   WDL_TypedBuf<FLOAT_TYPE> *ioPrevValues,
                   FLOAT_TYPE smoothFactor)
{
    if (ioCurrentValues->GetSize() != ioPrevValues->GetSize())
        return;
    
#if !USE_SIMD_OPTIM
    for (int i = 0; i < ioCurrentValues->GetSize(); i++)
    {
        FLOAT_TYPE val = ioCurrentValues->Get()[i];
        FLOAT_TYPE prevVal = ioPrevValues->Get()[i];
        
        if (val > prevVal)
            prevVal = val;
        
        FLOAT_TYPE newVal = smoothFactor*prevVal + (1.0 - smoothFactor)*val;
        
        ioCurrentValues->Get()[i] = newVal;
    }
    
    *ioPrevValues = *ioCurrentValues;
#else
    ComputeMax(ioPrevValues, *ioCurrentValues);
    Smooth(ioCurrentValues, ioPrevValues, smoothFactor);
#endif
}
template void BLUtils::SmoothMax(WDL_TypedBuf<float> *ioCurrentValues,
                                 WDL_TypedBuf<float> *ioPrevValues,
                                 float smoothFactor);
template void BLUtils::SmoothMax(WDL_TypedBuf<double> *ioCurrentValues,
                                 WDL_TypedBuf<double> *ioPrevValues,
                                 double smoothFactor);

// Detect all maxima in the data
// NOTE: for the moment take 0 for the minimal value
template <typename FLOAT_TYPE>
void
BLUtils::FindMaxima(const WDL_TypedBuf<FLOAT_TYPE> &data,
                    WDL_TypedBuf<FLOAT_TYPE> *result)
{
    result->Resize(data.GetSize());
    BLUtils::FillAllZero(result);
    
    FLOAT_TYPE prevValue = 0.0;
    for (int i = 0; i < data.GetSize(); i++)
    {
        FLOAT_TYPE currentValue = data.Get()[i];
        
        FLOAT_TYPE nextValue = 0.0;
        if (i < data.GetSize() - 1)
            nextValue = data.Get()[i + 1];
        
        if ((currentValue > prevValue) && (currentValue > nextValue))
        {
            result->Get()[i] = currentValue;
        }
        
        prevValue = currentValue;
    }
}
template void BLUtils::FindMaxima(const WDL_TypedBuf<float> &data,
                                  WDL_TypedBuf<float> *result);
template void BLUtils::FindMaxima(const WDL_TypedBuf<double> &data,
                                  WDL_TypedBuf<double> *result);

// Fill with 1.0 where there is a maxima, 0.0 elsewhere
template <typename FLOAT_TYPE>
void
BLUtils::FindMaxima2(const WDL_TypedBuf<FLOAT_TYPE> &data,
                     WDL_TypedBuf<FLOAT_TYPE> *result)
{
    result->Resize(data.GetSize());
    BLUtils::FillAllZero(result);
    
    FLOAT_TYPE prevValue = 0.0;
    for (int i = 0; i < data.GetSize(); i++)
    {
        FLOAT_TYPE currentValue = data.Get()[i];
        
        FLOAT_TYPE nextValue = 0.0;
        if (i < data.GetSize() - 1)
            nextValue = data.Get()[i + 1];
        
        if ((currentValue > prevValue) && (currentValue > nextValue))
        {
            result->Get()[i] = 1.0;
        }
        
        prevValue = currentValue;
    }
}
template void BLUtils::FindMaxima2(const WDL_TypedBuf<float> &data,
                                   WDL_TypedBuf<float> *result);
template void BLUtils::FindMaxima2(const WDL_TypedBuf<double> &data,
                                   WDL_TypedBuf<double> *result);

template <typename FLOAT_TYPE>
void
BLUtils::FindMaxima(WDL_TypedBuf<FLOAT_TYPE> *ioData)
{
    WDL_TypedBuf<FLOAT_TYPE> data = *ioData;
    FindMaxima(data, ioData);
}
template void BLUtils::FindMaxima(WDL_TypedBuf<float> *ioData);
template void BLUtils::FindMaxima(WDL_TypedBuf<double> *ioData);

// Fill with 1.0 or max value where there is a maxima, 0.0 elsewhere
// If keepMaxValue is true, fill with max value instead of 1.0
template <typename FLOAT_TYPE>
void
BLUtils::FindMaxima2D(int width, int height,
                      const WDL_TypedBuf<FLOAT_TYPE> &data,
                      WDL_TypedBuf<FLOAT_TYPE> *result,
                      bool keepMaxValue)
{
    result->Resize(data.GetSize());
    BLUtils::FillAllZero(result);
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            FLOAT_TYPE currentValue = data.Get()[i + j*width];
            
            bool maximumFound = true;
            for (int i0 = i - 1; i0 <= i + 1; i0++)
            {
                if (!maximumFound)
                    break;
                
                for (int j0 = j - 1; j0 <= j + 1; j0++)
                {
                    if ((i0 >= 0) && (i0 < width) &&
                        (j0 >= 0) && (j0 < height))
                    {
                        FLOAT_TYPE val = data.Get()[i0 + j0*width];
                        
                        if (val > currentValue)
                        {
                            maximumFound = false;
                            break;
                        }
                    }
                }
            }
            
            if (maximumFound)
            {
                FLOAT_TYPE val = 1.0;
                if (keepMaxValue)
                    val = currentValue;
                
                result->Get()[i + j*width] = val;
            }
        }
    }
}
template void BLUtils::FindMaxima2D(int width, int height,
                                    const WDL_TypedBuf<float> &data,
                                    WDL_TypedBuf<float> *result,
                                    bool keepMaxValue);
template void BLUtils::FindMaxima2D(int width, int height,
                                    const WDL_TypedBuf<double> &data,
                                    WDL_TypedBuf<double> *result,
                                    bool keepMaxValue);

template <typename FLOAT_TYPE>
void
BLUtils::FindMaxima2D(int width, int height, WDL_TypedBuf<FLOAT_TYPE> *ioData,
                      bool keepMaxValue)
{
    WDL_TypedBuf<FLOAT_TYPE> data = *ioData;
    FindMaxima2D(width, height, data, ioData, keepMaxValue);
}
template void BLUtils::FindMaxima2D(int width, int height,
                                    WDL_TypedBuf<float> *ioData,
                                    bool keepMaxValue);
template void BLUtils::FindMaxima2D(int width, int height,
                                    WDL_TypedBuf<double> *ioData,
                                    bool keepMaxValue);

template <typename FLOAT_TYPE>
void
BLUtils::FindMinima(const WDL_TypedBuf<FLOAT_TYPE> &data,
                    WDL_TypedBuf<FLOAT_TYPE> *result,
                    FLOAT_TYPE infValue)
{
    result->Resize(data.GetSize());
    BLUtils::FillAllValue(result, infValue);
    
    FLOAT_TYPE prevValue = infValue;
    for (int i = 0; i < data.GetSize(); i++)
    {
        FLOAT_TYPE currentValue = data.Get()[i];
        
        FLOAT_TYPE nextValue = infValue;
        if (i < data.GetSize() - 1)
            nextValue = data.Get()[i + 1];
        
        if ((currentValue < prevValue) && (currentValue < nextValue))
        {
            result->Get()[i] = currentValue;
        }
        
        prevValue = currentValue;
    }
}
template void BLUtils::FindMinima(const WDL_TypedBuf<float> &data,
                                  WDL_TypedBuf<float> *result,
                                  float infValue);
template void BLUtils::FindMinima(const WDL_TypedBuf<double> &data,
                                  WDL_TypedBuf<double> *result,
                                  double infValue);

template <typename FLOAT_TYPE>
void
BLUtils::FindMinima(const WDL_TypedBuf<FLOAT_TYPE> &data,
                    WDL_TypedBuf<FLOAT_TYPE> *result)
{
    if (data.GetSize() == 0)
        return;
    
    result->Resize(data.GetSize());
    BLUtils::FillAllValue(result, (FLOAT_TYPE)1.0);
    
    FLOAT_TYPE prevValue = data.Get()[0];
    for (int i = 0; i < data.GetSize(); i++)
    {
        FLOAT_TYPE currentValue = data.Get()[i];
        
        FLOAT_TYPE nextValue = 1.0;
        if (i < data.GetSize() - 1)
            nextValue = data.Get()[i + 1];
        
        if ((currentValue < prevValue) && (currentValue < nextValue))
        {
            result->Get()[i] = currentValue;
        }
        
        prevValue = currentValue;
    }
}
template void BLUtils::FindMinima(const WDL_TypedBuf<float> &data,
                                  WDL_TypedBuf<float> *result);
template void BLUtils::FindMinima(const WDL_TypedBuf<double> &data,
                                  WDL_TypedBuf<double> *result);

// Fill with 0.0 where there is a minima, 0.0 elsewhere
template <typename FLOAT_TYPE>
void
BLUtils::FindMinima2(const WDL_TypedBuf<FLOAT_TYPE> &data,
                     WDL_TypedBuf<FLOAT_TYPE> *result)
{
    if (data.GetSize() == 0)
        return;
    
    result->Resize(data.GetSize());
    BLUtils::FillAllValue(result, (FLOAT_TYPE)1.0);
    
    FLOAT_TYPE prevValue = data.Get()[0];
    for (int i = 0; i < data.GetSize(); i++)
    {
        FLOAT_TYPE currentValue = data.Get()[i];
        
        FLOAT_TYPE nextValue = 1.0;
        if (i < data.GetSize() - 1)
            nextValue = data.Get()[i + 1];
        
        if ((currentValue < prevValue) && (currentValue < nextValue))
        {
            result->Get()[i] = 0.0;
        }
        
        prevValue = currentValue;
    }
}
template void BLUtils::FindMinima2(const WDL_TypedBuf<float> &data,
                                   WDL_TypedBuf<float> *result);
template void BLUtils::FindMinima2(const WDL_TypedBuf<double> &data,
                                   WDL_TypedBuf<double> *result);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeDerivative(const WDL_TypedBuf<FLOAT_TYPE> &values,
                           WDL_TypedBuf<FLOAT_TYPE> *result)
{
    if (values.GetSize() == 0)
        return;
    
    result->Resize(values.GetSize());
    
    int size = values.GetSize();
    FLOAT_TYPE *valuesBuf = values.Get();
    FLOAT_TYPE *resultBuf = result->Get();
    
    FLOAT_TYPE prevValue = values.Get()[0];
    for (int i = 0; i < size; i++)
    {
        FLOAT_TYPE val = valuesBuf[i];
        FLOAT_TYPE diff = val - prevValue;
        
        resultBuf[i] = diff;
        
        prevValue = val;
    }
}
template void BLUtils::ComputeDerivative(const WDL_TypedBuf<float> &values,
                                         WDL_TypedBuf<float> *result);
template void BLUtils::ComputeDerivative(const WDL_TypedBuf<double> &values,
                                         WDL_TypedBuf<double> *result);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeDerivative(WDL_TypedBuf<FLOAT_TYPE> *ioValues)
{
    WDL_TypedBuf<FLOAT_TYPE> values = *ioValues;
    ComputeDerivative(values, ioValues);
}
template void BLUtils::ComputeDerivative(WDL_TypedBuf<float> *ioValues);
template void BLUtils::ComputeDerivative(WDL_TypedBuf<double> *ioValues);

/*void
  BLUtils::SwapColors(LICE_MemBitmap *bmp)
  {
  int w = bmp->getWidth();
  int h = bmp->getHeight();
    
  LICE_pixel *buf = bmp->getBits();
    
  for (int i = 0; i < w*h; i++)
  {
  LICE_pixel &pix = buf[i];
        
  int r = LICE_GETR(pix);
  int g = LICE_GETG(pix);
  int b = LICE_GETB(pix);
  int a = LICE_GETA(pix);
        
  pix = LICE_RGBA(b, g, r, a);
  }
  }*/

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::BinaryImageMatch(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image0,
                          const vector<WDL_TypedBuf<FLOAT_TYPE> > &image1)
{
    FLOAT_TYPE matchScore0 = 0.0; //BinaryImageMatchAux(image0, image1);
    FLOAT_TYPE matchScore1 = BinaryImageMatchAux(image1, image0);
    
    FLOAT_TYPE matchScore = (matchScore0 + matchScore1)/2.0;
    
    return matchScore;
}
template
float BLUtils::BinaryImageMatch(const vector<WDL_TypedBuf<float> > &image0,
                                const vector<WDL_TypedBuf<float> > &image1);
template double
BLUtils::BinaryImageMatch(const vector<WDL_TypedBuf<double> > &image0,
                          const vector<WDL_TypedBuf<double> > &image1);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::BinaryImageMatchAux(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image0,
                             const vector<WDL_TypedBuf<FLOAT_TYPE> > &image1)
{
    int numHit = 0;
    int numMiss = 0;
    int numForeground = 0;
    
    for (int i = 0; i < image0.size(); i++)
    {
        for (int j = 0; j < image0[i].GetSize(); j++)
        {
            FLOAT_TYPE val0 = image0[i].Get()[j];
            FLOAT_TYPE val1 = image1[i].Get()[j];
            
            if (val0 > 0.0)
            {
                numForeground++;
                
                if (val1 > 0.0)
                {
                    numHit++;
                }
                else
                {
                    numMiss++;
                }
            }
        }
    }
    
    FLOAT_TYPE matchScore = 0.0;
    if (numForeground > 0)
    {
        matchScore = ((FLOAT_TYPE)(numHit - numMiss))/numForeground;
    }
    
    return matchScore;
}
template float
BLUtils::BinaryImageMatchAux(const vector<WDL_TypedBuf<float> > &image0,
                             const vector<WDL_TypedBuf<float> > &image1);
template double
BLUtils::BinaryImageMatchAux(const vector<WDL_TypedBuf<double> > &image0,
                             const vector<WDL_TypedBuf<double> > &image1);

template <typename FLOAT_TYPE>
void
BLUtils::BuildMinMaxMapHoriz(vector<WDL_TypedBuf<FLOAT_TYPE> > *values)
{
    //const FLOAT_TYPE undefineValue = -1e15;
    const FLOAT_TYPE undefineValue = -1.0;
    
    if (values->empty())
        return;
    
    // Init
    vector<WDL_TypedBuf<FLOAT_TYPE> > minMaxMap = *values;
    for (int i = 0; i < minMaxMap.size(); i++)
    {
        BLUtils::FillAllValue(&minMaxMap[i], undefineValue);
    }
    
    // Iterate over lines
    for (int i = 0; i < (*values)[0].GetSize(); i++)
    {
        // Generate the source line
        WDL_TypedBuf<FLOAT_TYPE> line;
        line.Resize(values->size());
        for (int j = 0; j < values->size(); j++)
        {
            line.Get()[j] = (*values)[j].Get()[i];
        }
        
        // Process
        WDL_TypedBuf<FLOAT_TYPE> minima;
        FindMinima2(line, &minima);
        
        WDL_TypedBuf<FLOAT_TYPE> maxima;
        FindMaxima2(line, &maxima);
        
        WDL_TypedBuf<FLOAT_TYPE> newLine;
        newLine.Resize(line.GetSize());
        BLUtils::FillAllValue(&newLine, undefineValue);
        
        for (int j = 0; j < line.GetSize(); j++)
        {
            FLOAT_TYPE mini = minima.Get()[j];
            FLOAT_TYPE maxi = maxima.Get()[j];
            
            if (mini < 1.0)
                newLine.Get()[j] = 0.0;
            
            if (maxi > 0.0)
                newLine.Get()[j] = 1.0;
        }
        
        bool extendBounds = true;
        BLUtils::FillMissingValues3(&newLine, extendBounds, undefineValue);
        
        // Copy the result line
        for (int j = 0; j < values->size(); j++)
        {
            minMaxMap[j].Get()[i] = newLine.Get()[j];
        }
    }
    
    // Finish
    *values = minMaxMap;
}
template void BLUtils::BuildMinMaxMapHoriz(vector<WDL_TypedBuf<float> > *values);
template void BLUtils::BuildMinMaxMapHoriz(vector<WDL_TypedBuf<double> > *values);

template <typename FLOAT_TYPE>
void
BLUtils::BuildMinMaxMapVert(vector<WDL_TypedBuf<FLOAT_TYPE> > *values)
{
    const FLOAT_TYPE undefineValue = -1.0;
    
    // Init
    vector<WDL_TypedBuf<FLOAT_TYPE> > minMaxMap = *values;
    for (int i = 0; i < minMaxMap.size(); i++)
    {
        BLUtils::FillAllValue(&minMaxMap[i], undefineValue);
    }
    
    // Iterate over lines
    for (int i = 0; i < values->size(); i++)
    {
        const WDL_TypedBuf<FLOAT_TYPE> &line = (*values)[i];
        
        WDL_TypedBuf<FLOAT_TYPE> minima;
        FindMinima2(line, &minima);
        
        WDL_TypedBuf<FLOAT_TYPE> maxima;
        FindMaxima2(line, &maxima);
        
        WDL_TypedBuf<FLOAT_TYPE> newLine;
        newLine.Resize(line.GetSize());
        BLUtils::FillAllValue(&newLine, undefineValue);
        
        for (int j = 0; j < line.GetSize(); j++)
        {
            FLOAT_TYPE mini = minima.Get()[j];
            FLOAT_TYPE maxi = maxima.Get()[j];
            
            if (mini < 1.0)
                newLine.Get()[j] = 0.0;
            
            if (maxi > 0.0)
                newLine.Get()[j] = 1.0;
        }
        
        bool extendBounds = false;
        BLUtils::FillMissingValues3(&newLine, extendBounds, undefineValue);
        
        minMaxMap[i] = newLine;
    }
    
    // Finish
    *values = minMaxMap;
}
template void BLUtils::BuildMinMaxMapVert(vector<WDL_TypedBuf<float> > *values);
template void BLUtils::BuildMinMaxMapVert(vector<WDL_TypedBuf<double> > *values);

template <typename FLOAT_TYPE>
void
BLUtils::Normalize(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    FLOAT_TYPE minimum = BLUtils::ComputeMin(*values);
    FLOAT_TYPE maximum = BLUtils::ComputeMax(*values);
    
#if !USE_SIMD_OPTIM
    for (int i = 0; i < values->GetSize(); i++)
    {
        FLOAT_TYPE val = values->Get()[i];
        
        if (std::fabs(maximum - minimum) > 0.0)
            val = (val - minimum)/(maximum - minimum);
        else
            val = 0.0;
        
        values->Get()[i] = val;
    }
#else
    FLOAT_TYPE diff = std::fabs(maximum - minimum);
    if (diff > 0.0)
    {
        AddValues(values, -minimum);
        
        FLOAT_TYPE coeff = 1.0/diff;
        MultValues(values, coeff);
    }
    else
    {
        FillAllZero(values);
    }
#endif
}
template void BLUtils::Normalize(WDL_TypedBuf<float> *values);
template void BLUtils::Normalize(WDL_TypedBuf<double> *values);

template <typename FLOAT_TYPE>
void
BLUtils::Normalize(FLOAT_TYPE *values, int numValues)
{
    FLOAT_TYPE min_val = INF;
    FLOAT_TYPE max_val = -INF;
    
    int i;
    for (i = 0; i < numValues; i++)
    {
        if (values[i] < min_val)
            min_val = values[i];
        if (values[i] > max_val)
            max_val = values[i];
    }
    
    if (max_val - min_val > BL_EPS)
    {
        for (i = 0; i < numValues; i++)
        {
            values[i] = (values[i] - min_val)/(max_val - min_val);
        }
    }
}
template void BLUtils::Normalize(float *values, int numValues);
template void BLUtils::Normalize(double *values, int numValues);

template <typename FLOAT_TYPE>
void
BLUtils::Normalize(WDL_TypedBuf<FLOAT_TYPE> *values,
                   FLOAT_TYPE minimum, FLOAT_TYPE maximum)
{
#if !USE_SIMD_OPTIM
    for (int i = 0; i < values->GetSize(); i++)
    {
        FLOAT_TYPE val = values->Get()[i];
        
        if (std::fabs(maximum - minimum) > 0.0)
            val = (val - minimum)/(maximum - minimum);
        else
            val = 0.0;
        
        values->Get()[i] = val;
    }
#else
    FLOAT_TYPE diff = std::fabs(maximum - minimum);
    if (diff > 0.0)
    {
        AddValues(values, -minimum);
        
        FLOAT_TYPE coeff = 1.0/diff;
        MultValues(values, coeff);
    }
    else
    {
        FillAllZero(values);
    }
#endif
}
template void BLUtils::Normalize(WDL_TypedBuf<float> *values,
                                 float minimum, float maximum);
template void BLUtils::Normalize(WDL_TypedBuf<double> *values,
                                 double minimum, double maximum);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtils::Normalize(FLOAT_TYPE value, FLOAT_TYPE minimum, FLOAT_TYPE maximum)
{
    FLOAT_TYPE result = 0.0;
    if (std::fabs(maximum - minimum) > 0.0)
        result = (value - minimum)/(maximum - minimum);
    
    return result;
}
template float BLUtils::Normalize(float value, float minimum, float maximum);
template double BLUtils::Normalize(double value, double minimum, double maximum);

template <typename FLOAT_TYPE>
void
BLUtils::Normalize(deque<WDL_TypedBuf<FLOAT_TYPE> > *values)
{
    FLOAT_TYPE minValue = INF;
    FLOAT_TYPE maxValue = -INF;
    
    for (int i = 0; i < values->size(); i++)
    {
        for (int j = 0; j < (*values)[i].GetSize(); j++)
        {
            FLOAT_TYPE val = (*values)[i].Get()[j];
            
            if (val < minValue)
                minValue = val;
            
            if (val > maxValue)
                maxValue = val;
        }
    }
    
    for (int i = 0; i < values->size(); i++)
    {
        for (int j = 0; j < (*values)[i].GetSize(); j++)
        {
            FLOAT_TYPE val = (*values)[i].Get()[j];
            
            if (maxValue - minValue > BL_EPS)
                val = (val - minValue)/(maxValue - minValue);
            
            (*values)[i].Get()[j] = val;
        }
    }
}
template void BLUtils::Normalize(deque<WDL_TypedBuf<float> > *values);
template void BLUtils::Normalize(deque<WDL_TypedBuf<double> > *values);

void
BLUtils::NormalizeMagns(WDL_TypedBuf<WDL_FFT_COMPLEX> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        WDL_FFT_COMPLEX &val = values->Get()[i];
        double magn = COMP_MAGN(val);
        if (magn > 0.0)
        {
            val.re /= magn;
            val.im /= magn;
        }
    }
}

void
BLUtils::NormalizeMagnsF(WDL_TypedBuf<WDL_FFT_COMPLEX> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        WDL_FFT_COMPLEX &val = values->Get()[i];
        float magn = COMP_MAGNF(val);
        if (magn > 0.0)
        {
            val.re /= magn;
            val.im /= magn;
        }
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::Normalize(WDL_TypedBuf<FLOAT_TYPE> *values,
                   FLOAT_TYPE *minVal, FLOAT_TYPE *maxVal)
{
    *minVal = BLUtils::ComputeMin(*values);
    *maxVal = BLUtils::ComputeMax(*values);
    
    if (std::fabs(*maxVal - *minVal) > BL_EPS)
    {
        for (int i = 0; i < values->GetSize(); i++)
        {
            FLOAT_TYPE val = values->Get()[i];
            
            val = (val - *minVal)/(*maxVal - *minVal);
            
            values->Get()[i] = val;
        }
    }
}
template void BLUtils::Normalize(WDL_TypedBuf<float> *values,
                                 float *minVal, float *maxVal);
template void BLUtils::Normalize(WDL_TypedBuf<double> *values,
                                 double *minVal, double *maxVal);

template <typename FLOAT_TYPE>
void
BLUtils::DeNormalize(WDL_TypedBuf<FLOAT_TYPE> *values,
                     FLOAT_TYPE minVal, FLOAT_TYPE maxVal)
{
    if (std::fabs(maxVal - minVal) > BL_EPS)
    {
        for (int i = 0; i < values->GetSize(); i++)
        {
            FLOAT_TYPE val = values->Get()[i];
            
            val = val*(maxVal - minVal) + minVal;
            
            values->Get()[i] = val;
        }
    }
}
template void BLUtils::DeNormalize(WDL_TypedBuf<float> *values,
                                   float minVal, float maxVal);
template void BLUtils::DeNormalize(WDL_TypedBuf<double> *values,
                                   double minVal, double maxVal);

template <typename FLOAT_TYPE>
void
BLUtils::NormalizeFilter(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    FLOAT_TYPE sum = BLUtils::ComputeSum(*values);
    
    if (std::fabs(sum) < BL_EPS)
        return;
    
    FLOAT_TYPE sumInv = 1.0/sum;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        FLOAT_TYPE val = values->Get()[i];
        
        val *= sumInv;
        
        values->Get()[i] = val;
    }
}
template void BLUtils::NormalizeFilter(WDL_TypedBuf<float> *values);
template void BLUtils::NormalizeFilter(WDL_TypedBuf<double> *values);

// Chroma
template <typename FLOAT_TYPE>
static FLOAT_TYPE
ComputeC0Freq(FLOAT_TYPE aTune)
{
    FLOAT_TYPE AMinus1 = aTune/32.0;
    
    FLOAT_TYPE toneMult = std::pow((FLOAT_TYPE)2.0, (FLOAT_TYPE)(1.0/12.0));
    
    FLOAT_TYPE ASharpMinus1 = AMinus1*toneMult;
    FLOAT_TYPE BMinus1 = ASharpMinus1*toneMult;
    
    FLOAT_TYPE C0 = BMinus1*toneMult;
    
    return C0;
}
template float ComputeC0Freq(float aTune);
template double ComputeC0Freq(double aTune);

template <typename FLOAT_TYPE>
void
BLUtils::BinsToChromaBins(int numBins, WDL_TypedBuf<FLOAT_TYPE> *chromaBins,
                          FLOAT_TYPE sampleRate, FLOAT_TYPE aTune)
{
    chromaBins->Resize(numBins);
    
    if (numBins == 0)
        return;
    
    // Corresponding to A 440
    //#define C0_TONE 16.35160
    
    FLOAT_TYPE c0Freq = ComputeC0Freq(aTune);
    
    FLOAT_TYPE toneMult = std::pow((FLOAT_TYPE)2.0, (FLOAT_TYPE)(1.0/12.0));
    
    FLOAT_TYPE hzPerBin = sampleRate/(numBins*2);
    
    // Do not take 0Hz!
    if (chromaBins->GetSize() > 0)
        chromaBins->Get()[0] = 0.0;
    
    FLOAT_TYPE c0FreqInv = 1.0/c0Freq; //
    FLOAT_TYPE logToneMultInv = 1.0/std::log(toneMult); //
    FLOAT_TYPE inv12 = 1.0/12.0; //
    for (int i = 1; i < chromaBins->GetSize(); i++)
    {
        FLOAT_TYPE freq = i*hzPerBin;
        
        // See: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
        //FLOAT_TYPE fRatio = freq / c0Freq;
        FLOAT_TYPE fRatio = freq*c0FreqInv;
        //FLOAT_TYPE tone = std::log(fRatio)/std::log(toneMult);
        FLOAT_TYPE tone = std::log(fRatio)*logToneMultInv;
        
        // Shift by one (strange...)
        tone += 1.0;
        
        // Adjust to the axis labels
        tone -= (1.0/12.0);
        
        tone = fmod(tone, (FLOAT_TYPE)12.0);
        
        //FLOAT_TYPE toneNorm = tone/12.0;
        FLOAT_TYPE toneNorm = tone*inv12;
        
        FLOAT_TYPE binNumF = toneNorm*chromaBins->GetSize();
        
        //int binNum = bl_round(binNumF);
        FLOAT_TYPE binNum = binNumF; // Keep float
        
        if ((binNum >= 0) && (binNum < chromaBins->GetSize()))
            chromaBins->Get()[i] = binNum;
    }
}
template void BLUtils::BinsToChromaBins(int numBins, WDL_TypedBuf<float> *chromaBins,
                                        float sampleRate, float aTune);
template void BLUtils::BinsToChromaBins(int numBins, WDL_TypedBuf<double> *chromaBins,
                                        double sampleRate, double aTune);

template <typename FLOAT_TYPE>
void
BLUtils::ComputeSamplesByFrequency(WDL_TypedBuf<FLOAT_TYPE> *freqSamples,
                                   FLOAT_TYPE sampleRate,
                                   const WDL_TypedBuf<FLOAT_TYPE> &magns,
                                   const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                   int tBinNum)
{
    freqSamples->Resize(magns.GetSize());
 
    FLOAT_TYPE t = ((FLOAT_TYPE)tBinNum*2.0)/sampleRate;
    
    WDL_TypedBuf<FLOAT_TYPE> freqs;
    BLUtilsFft::FftFreqs(&freqs, magns.GetSize(), sampleRate);
    
    for (int i = 0; i < magns.GetSize(); i++)
    {
        FLOAT_TYPE freq = freqs.Get()[i];
        
        FLOAT_TYPE magn = magns.Get()[i];
        FLOAT_TYPE phase = phases.Get()[i];
        
        FLOAT_TYPE samp = magn*std::sin((FLOAT_TYPE)(2.0*M_PI*freq*t + phase));
        
        freqSamples->Get()[i] = samp;
    }
}
template void BLUtils::ComputeSamplesByFrequency(WDL_TypedBuf<float> *freqSamples,
                                                 float sampleRate,
                                                 const WDL_TypedBuf<float> &magns,
                                                 const WDL_TypedBuf<float> &phases,
                                                 int tBinNum);
template void BLUtils::ComputeSamplesByFrequency(WDL_TypedBuf<double> *freqSamples,
                                                 double sampleRate,
                                                 const WDL_TypedBuf<double> &magns,
                                                 const WDL_TypedBuf<double> &phases,
                                                 int tBinNum);

template <typename FLOAT_TYPE>
void
BLUtils::SeparatePeaks2D(int width, int height,
                         WDL_TypedBuf<FLOAT_TYPE> *ioData,
                         bool keepMaxValue)
{
    WDL_TypedBuf<FLOAT_TYPE> result = *ioData;
    
    if (!keepMaxValue)
        BLUtils::FillAllValue(&result, (FLOAT_TYPE)1.0);
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            FLOAT_TYPE currentVal = ioData->Get()[i + j*width];
            
            // Test horizontal
            if ((i > 0) && (i < width - 1))
            {
                FLOAT_TYPE val0 = ioData->Get()[i - 1 + j*width];
                FLOAT_TYPE val1 = ioData->Get()[i + 1 + j*width];
                       
                if (((currentVal < val0) && (currentVal < val1)) ||
                    ((currentVal <= val0) && (currentVal < val1)) ||
                    ((currentVal < val0) && (currentVal <= val1)) ||
                    ((currentVal <= val0) && (currentVal <= val1))) //
                {
                    result.Get()[i + j*width] = 0.0;
                    
                    continue;
                }
            }
            
            // Test vertical
            if ((j > 0) && (j < height - 1))
            {
                FLOAT_TYPE val0 = ioData->Get()[i + (j - 1)*width];
                FLOAT_TYPE val1 = ioData->Get()[i + (j + 1)*width];
                
                if (((currentVal < val0) && (currentVal < val1)) ||
                    ((currentVal <= val0) && (currentVal < val1)) ||
                    ((currentVal < val0) && (currentVal <= val1)) ||
                    ((currentVal <= val0) && (currentVal <= val1))) //
                {
                    result.Get()[i + j*width] = 0.0;
                    
                    continue;
                }
            }
            
            // Test oblic (anti-slash)
            if ((i > 0) && (j > 0) &&
                ((i < width - 1) && (j < height - 1)))
            {
                FLOAT_TYPE val0 = ioData->Get()[(i - 1) + (j - 1)*width];
                FLOAT_TYPE val1 = ioData->Get()[(i + 1) + (j + 1)*width];
                
                if (((currentVal < val0) && (currentVal < val1)) ||
                    ((currentVal <= val0) && (currentVal < val1)) ||
                    ((currentVal < val0) && (currentVal <= val1)) ||
                    ((currentVal <= val0) && (currentVal <= val1))) //
                {
                    result.Get()[i + j*width] = 0.0;
                    
                    continue;
                }
            }
            
            // Test oblic (slash)
            if ((i > 0) && (j > 0) &&
                ((i < width - 1) && (j < height - 1)))
            {
                FLOAT_TYPE val0 = ioData->Get()[(i - 1) + (j + 1)*width];
                FLOAT_TYPE val1 = ioData->Get()[(i + 1) + (j - 1)*width];
                
                if (((currentVal < val0) && (currentVal < val1)) ||
                    ((currentVal <= val0) && (currentVal < val1)) ||
                    ((currentVal < val0) && (currentVal <= val1)) ||
                    ((currentVal <= val0) && (currentVal <= val1))) //
                {
                    result.Get()[i + j*width] = 0.0;
                    
                    continue;
                }
            }
        }
    }
    
    *ioData = result;
}
template void BLUtils::SeparatePeaks2D(int width, int height,
                                       WDL_TypedBuf<float> *ioData,
                                       bool keepMaxValue);
template void BLUtils::SeparatePeaks2D(int width, int height,
                                       WDL_TypedBuf<double> *ioData,
                                       bool keepMaxValue);

template <typename FLOAT_TYPE>
void
BLUtils::SeparatePeaks2D2(int width, int height,
                          WDL_TypedBuf<FLOAT_TYPE> *ioData,
                          bool keepMaxValue)
{
#define WIN_SIZE 3
#define HALF_WIN_SIZE 1 //WIN_SIZE*0.5
    
    WDL_TypedBuf<FLOAT_TYPE> result = *ioData;
    
    if (!keepMaxValue)
        BLUtils::FillAllValue(&result, (FLOAT_TYPE)1.0);
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            FLOAT_TYPE currentVal = ioData->Get()[i + j*width];
            
            bool minimumFound = false;
            
            // Test horizontal
            for (int i0 = i - HALF_WIN_SIZE; i0 <= i + HALF_WIN_SIZE; i0++)
            {
                if (minimumFound)
                    break;
                
                for (int i1 = i - HALF_WIN_SIZE; i1 <= i + HALF_WIN_SIZE; i1++)
                {
                    int j0 = j - HALF_WIN_SIZE;
                    int j1 = j + HALF_WIN_SIZE;
                    
                    if ((i0 >= 0) && (i0 < width) &&
                        (i1 >= 0) && (i1 < width) &&
                        (j0 >= 0) && (j0 < height) &&
                        (j1 >= 0) && (j1 < height))
                    {
                        FLOAT_TYPE val0 = ioData->Get()[i0 + j0*width];
                        FLOAT_TYPE val1 = ioData->Get()[i1 + j1*width];
                        
                        if (((currentVal < val0) && (currentVal < val1))) // ||
                            //((currentVal <= val0) && (currentVal < val1)) ||
                            //((currentVal < val0) && (currentVal <= val1)))
                        {
                            result.Get()[i + j*width] = 0.0;
                            
                            minimumFound = true;
                            
                            break;
                        }
                    }
                }
            }
            
            // Test vertical
            for (int j0 = j - HALF_WIN_SIZE; j0 <= j + HALF_WIN_SIZE; j0++)
            {
                if (minimumFound)
                    break;
                
                for (int j1 = j - HALF_WIN_SIZE; j1 <= j + HALF_WIN_SIZE; j1++)
                {
                    int i0 = i - HALF_WIN_SIZE;
                    int i1 = i + HALF_WIN_SIZE;
                    
                    if ((i0 >= 0) && (i0 < width) &&
                        (i1 >= 0) && (i1 < width) &&
                        (j0 >= 0) && (j0 < height) &&
                        (j1 >= 0) && (j1 < height))
                    {
                        FLOAT_TYPE val0 = ioData->Get()[i0 + j0*width];
                        FLOAT_TYPE val1 = ioData->Get()[i1 + j1*width];
                        
                        if (((currentVal < val0) && (currentVal < val1))) // ||
                            //((currentVal <= val0) && (currentVal < val1)) ||
                            //((currentVal < val0) && (currentVal <= val1)))
                        {
                            result.Get()[i + j*width] = 0.0;
                            
                            minimumFound = true;
                            
                            break;
                        }
                    }
                }
            }

        }
    }
    
    *ioData = result;
}
template void BLUtils::SeparatePeaks2D2(int width, int height,
                                        WDL_TypedBuf<float> *ioData,
                                        bool keepMaxValue);
template void BLUtils::SeparatePeaks2D2(int width, int height,
                                        WDL_TypedBuf<double> *ioData,
                                        bool keepMaxValue);

void
BLUtils::ConvertToGUIFloatType(WDL_TypedBuf<BL_GUI_FLOAT> *dst,
                               const WDL_TypedBuf<float> &src)
{
    if (sizeof(BL_FLOAT) == sizeof(float))
    {
        *((WDL_TypedBuf<float> *)dst) = src;
        
        return;
    }
    
    dst->Resize(src.GetSize());
    for (int i = 0; i < src.GetSize(); i++)
    {
        dst->Get()[i] = src.Get()[i];
    }
}

void
BLUtils::ConvertToGUIFloatType(WDL_TypedBuf<BL_GUI_FLOAT> *dst,
                               const WDL_TypedBuf<double> &src)
{
    if (sizeof(BL_GUI_FLOAT) == sizeof(double))
    {
        *((WDL_TypedBuf<double> *)dst) = src;
        return;
    }
    
    dst->Resize(src.GetSize());
    for (int i = 0; i < src.GetSize(); i++)
    {
        dst->Get()[i] = src.Get()[i];
    }
}

void
BLUtils::ConvertToFloatType(WDL_TypedBuf<BL_FLOAT> *dst,
                            const WDL_TypedBuf<float> &src)
{
    if (sizeof(BL_FLOAT) == sizeof(float))
    {
        *((WDL_TypedBuf<float> *)dst) = src;
        return;
    }
    
    dst->Resize(src.GetSize());
    for (int i = 0; i < src.GetSize(); i++)
    {
        dst->Get()[i] = src.Get()[i];
    }
}

void
BLUtils::ConvertToFloatType(WDL_TypedBuf<BL_FLOAT> *dst,
                            const WDL_TypedBuf<double> &src)
{
    if (sizeof(BL_FLOAT) == sizeof(double))
    {
        *((WDL_TypedBuf<double> *)dst) = src;
        
        return;
    }
                
    dst->Resize(src.GetSize());
    for (int i = 0; i < src.GetSize(); i++)
    {
        dst->Get()[i] = src.Get()[i];
    }
}

template <typename FLOAT_TYPE>
void
BLUtils::ConvertFromFloat(WDL_TypedBuf<FLOAT_TYPE> *dst,
                          const WDL_TypedBuf<float> &src)
{
    if (sizeof(FLOAT_TYPE) == sizeof(float))
    {
        *((WDL_TypedBuf<float> *)dst) = src;
        return;
    }
    
    dst->Resize(src.GetSize());

    int size = src.GetSize();
    FLOAT_TYPE *dstBuf = dst->Get();
    float *srcBuf = src.Get();
    
    for (int i = 0; i < size; i++)
    {
        //dst->Get()[i] = src.Get()[i];
        dstBuf[i] = srcBuf[i];
    }
}
template void BLUtils::ConvertFromFloat(WDL_TypedBuf<float> *dst,
                                        const WDL_TypedBuf<float> &src);
template void BLUtils::ConvertFromFloat(WDL_TypedBuf<double> *dst,
                                        const WDL_TypedBuf<float> &src);

template <typename FLOAT_TYPE>
void
BLUtils::ConvertToFloat(WDL_TypedBuf<float> *dst,
                        const WDL_TypedBuf<FLOAT_TYPE> &src)
{
    if (sizeof(FLOAT_TYPE) == sizeof(float))
    {
        *((WDL_TypedBuf<FLOAT_TYPE> *)dst) = src;
        return;
    }
    
    dst->Resize(src.GetSize());

    int size = src.GetSize();
    float *dstBuf = dst->Get();
    FLOAT_TYPE *srcBuf = src.Get();
    
    for (int i = 0; i < size; i++)
    {
        //dst->Get()[i] = src.Get()[i];
        dstBuf[i] = srcBuf[i];
    }
}
template void BLUtils::ConvertToFloat(WDL_TypedBuf<float> *dst,
                                      const WDL_TypedBuf<float> &src);
template void BLUtils::ConvertToFloat(WDL_TypedBuf<float> *dst,
                                      const WDL_TypedBuf<double> &src);

void
BLUtils::FixDenormal(WDL_TypedBuf<BL_FLOAT> *data)
{
    for (int i = 0; i < data->GetSize(); i++)
    {
        BL_FLOAT &val = data->Get()[i];
        FIX_FLT_DENORMAL(val);
    }
}

void
BLUtils::AddIntermediatePoints(const WDL_TypedBuf<BL_FLOAT> &x,
                               const WDL_TypedBuf<BL_FLOAT> &y,
                               WDL_TypedBuf<BL_FLOAT> *newX,
                               WDL_TypedBuf<BL_FLOAT> *newY)
{
    if (x.GetSize() != y.GetSize())
        return;
    
    if (x.GetSize() < 2)
        return;
    
    //
    newX->Resize(x.GetSize()*2);
    newY->Resize(y.GetSize()*2);
    
    for (int i = 0; i < newX->GetSize() - 1; i++)
    {
        if (i % 2 == 0)
        {
            // Simply copy the value
            newX->Get()[i] = x.Get()[i/2];
            newY->Get()[i] = y.Get()[i/2];
            
            continue;
        }
        
        // Take care of last bound
        if (i/2 + 1 >= x.GetSize())
            break;
        
        // Take the middle
        newX->Get()[i] = (x.Get()[i/2] + x.Get()[i/2 + 1])*0.5;
        newY->Get()[i] = (y.Get()[i/2] + y.Get()[i/2 + 1])*0.5;
    }
    
    // Last value
    newX->Get()[newX->GetSize() - 1] = x.Get()[x.GetSize() - 1];
    newY->Get()[newY->GetSize() - 1] = y.Get()[y.GetSize() - 1];
}

long int
BLUtils::GetTimeMillis()
{
    // Make the demo flag to blink
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    
    return ms;
}

double
BLUtils::GetTimeMillisF()
{
    double ms = UpTime::GetUpTimeF();
    
    return ms;
}

template <typename FLOAT_TYPE>
void
BLUtils::FastQueueToBuf(const WDL_TypedFastQueue<FLOAT_TYPE> &q,
                        WDL_TypedBuf<FLOAT_TYPE> *buf,
                        int numToCopy)
{
    if (numToCopy == -1)
        numToCopy = q.Available();

    if (numToCopy > q.Available())
        numToCopy = q.Available();
    
    buf->Resize(numToCopy);
    
    q.GetToBuf(0, buf->Get(), numToCopy);
}
template void BLUtils::FastQueueToBuf(const WDL_TypedFastQueue<float> &q,
                                      WDL_TypedBuf<float> *buf,
                                      int numToCopy);
template void BLUtils::FastQueueToBuf(const WDL_TypedFastQueue<double> &q,
                                      WDL_TypedBuf<double> *buf,
                                      int numToCopy);

template <typename FLOAT_TYPE>
void
BLUtils::FastQueueToBuf(const WDL_TypedFastQueue<FLOAT_TYPE> &q,
                        int queueOffset,
                        WDL_TypedBuf<FLOAT_TYPE> *buf,
                        int numToCopy)
{
    if (numToCopy == -1)
        numToCopy = (q.Available() - queueOffset);

    if (numToCopy + queueOffset > q.Available())
        numToCopy = q.Available() - queueOffset;
    
    buf->Resize(numToCopy);
    
    q.GetToBuf(queueOffset, buf->Get(), numToCopy);
}
template void BLUtils::FastQueueToBuf(const WDL_TypedFastQueue<float> &q,
                                      int queueOffset,
                                      WDL_TypedBuf<float> *buf,
                                      int numToCopy);
template void BLUtils::FastQueueToBuf(const WDL_TypedFastQueue<double> &q,
                                      int queueOffset,
                                      WDL_TypedBuf<double> *buf,
                                      int numToCopy);

template <typename FLOAT_TYPE>
void
BLUtils::BufToFastQueue(const WDL_TypedBuf<FLOAT_TYPE> &buf,
                        WDL_TypedFastQueue<FLOAT_TYPE> *q)
{
    int numToAdd = buf.GetSize() - q->Available();
    if (numToAdd > 0)
        q->Add(0, numToAdd);
    else if (numToAdd < 0)
    {
        q->Clear();
        q->Add(0, buf.GetSize());
    }

    q->SetFromBuf(0, buf.Get(), buf.GetSize());
}
template void BLUtils::BufToFastQueue(const WDL_TypedBuf<float> &buf,
                                      WDL_TypedFastQueue<float> *q);
template void BLUtils::BufToFastQueue(const WDL_TypedBuf<double> &buf,
                                      WDL_TypedFastQueue<double> *q);

template <typename FLOAT_TYPE>
void
BLUtils::Replace(WDL_TypedFastQueue<FLOAT_TYPE> *dst,
                 int startIdx,
                 const WDL_TypedBuf<FLOAT_TYPE> &src)
{
    int numToReplace = src.GetSize();
    if (startIdx + numToReplace > dst->Available())
        numToReplace = dst->Available() - startIdx;

    dst->SetFromBuf(startIdx, src.Get(), numToReplace);
}
template void BLUtils::Replace(WDL_TypedFastQueue<float> *dst, int startIdx,
                               const WDL_TypedBuf<float> &src);
template void BLUtils::Replace(WDL_TypedFastQueue<double> *dst, int startIdx,
                               const WDL_TypedBuf<double> &src);

template <typename FLOAT_TYPE>
void BLUtils::ConsumeLeft(WDL_TypedFastQueue<FLOAT_TYPE> *ioBuffer,
                          int numToConsume)
{
    if (numToConsume > ioBuffer->Available())
        numToConsume = ioBuffer->Available();

    ioBuffer->Advance(numToConsume);
}
template void BLUtils::ConsumeLeft(WDL_TypedFastQueue<float> *ioBuffer,
                                   int numToConsume);
template void BLUtils::ConsumeLeft(WDL_TypedFastQueue<double> *ioBuffer,
                                   int numToConsume);

template <typename FLOAT_TYPE>
void
BLUtils::ResizeFillZeros(WDL_TypedFastQueue<FLOAT_TYPE> *q, int newSize)
{
    int numToAdd = newSize - q->Available();
    if (numToAdd > 0)
        q->Add(0, numToAdd);
    else if (numToAdd < 0)
    {
        q->Clear();
        q->Add(0, newSize);
    }
}
template void BLUtils::ResizeFillZeros(WDL_TypedFastQueue<float> *q, int newSize);
template void BLUtils::ResizeFillZeros(WDL_TypedFastQueue<double> *q, int newSize);

template <typename FLOAT_TYPE>
void
BLUtils::ConsumeLeft(const WDL_TypedBuf<FLOAT_TYPE> &inBuffer,
                     WDL_TypedBuf<FLOAT_TYPE> *outBuffer,
                     int numToConsume)
{
    int newSize = inBuffer.GetSize() - numToConsume;
    if (newSize <= 0)
    {
        outBuffer->Resize(0);
        
        return;
    }

    outBuffer->Resize(newSize);
    memcpy(outBuffer->Get(),
           &inBuffer.Get()[numToConsume],
           newSize*sizeof(FLOAT_TYPE));
}
template void BLUtils::ConsumeLeft(const WDL_TypedBuf<float> &inBuffer,
                                   WDL_TypedBuf<float> *outBuffer,
                                   int numToConsume);
template void BLUtils::ConsumeLeft(const WDL_TypedBuf<double> &inBuffer,
                                   WDL_TypedBuf<double> *outBuffer,
                                   int numToConsume);

template <typename FLOAT_TYPE>
void
BLUtils::SetBufResize(WDL_TypedBuf<FLOAT_TYPE> *dstBuffer,
                      const WDL_TypedBuf<FLOAT_TYPE> &srcBuffer,
                      int srcOffset, int numToCopy)
{
    if (numToCopy == -1)
        numToCopy = srcBuffer.GetSize();
    // FIX: fixed here
    if (numToCopy + srcOffset > srcBuffer.GetSize())
        numToCopy = srcBuffer.GetSize() - srcOffset;

    // FIX: fix crash Ghost (SA, load file, zoom -> crash)
    if (numToCopy < 0)
    {
        //numToCopy = 0;
        
        return;
    }
    
    dstBuffer->Resize(numToCopy);
    memcpy(dstBuffer->Get(),
           &srcBuffer.Get()[srcOffset],
           numToCopy*sizeof(FLOAT_TYPE));
}
template void BLUtils::SetBufResize(WDL_TypedBuf<float> *dstBuffer,
                                    const WDL_TypedBuf<float> &srcBuffer,
                                    int srcOffset, int numToCopy);
template void BLUtils::SetBufResize(WDL_TypedBuf<double> *dstBuffer,
                                    const WDL_TypedBuf<double> &srcBuffer,
                                    int srcOffset, int numToCopy);

template <typename FLOAT_TYPE>
void
BLUtils::SetBuf(WDL_TypedBuf<FLOAT_TYPE> *dstBuffer,
                const WDL_TypedBuf<FLOAT_TYPE> &srcBuffer)
{
    int numToCopy = srcBuffer.GetSize();
    if (dstBuffer->GetSize() < numToCopy)
        numToCopy = dstBuffer->GetSize();
    
    memcpy(dstBuffer->Get(), srcBuffer.Get(), numToCopy*sizeof(FLOAT_TYPE));
}
template void BLUtils::SetBuf(WDL_TypedBuf<float> *dstBuffer,
                              const WDL_TypedBuf<float> &srcBuffer);
template void BLUtils::SetBuf(WDL_TypedBuf<double> *dstBuffer,
                              const WDL_TypedBuf<double> &srcBuffer);
template void BLUtils::SetBuf(WDL_TypedBuf<WDL_FFT_COMPLEX> *dstBuffer,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &srcBuffer);

// NOTE; this has not been tested at all...
template <typename FLOAT_TYPE>
static void ShiftBuffer(WDL_TypedBuf<FLOAT_TYPE> *buffer, int numToShift)
{
    if (numToShift > 0)
    {
        for (int i = buffer->GetSize() - 1; i > 0; i++)
        {
            if (i <= numToShift)
                buffer->Get()[i] = 0.0;
            else
            {
                buffer->Get()[i] = buffer->Get()[i - numToShift];
            }
        }
    }
    else if (numToShift < 0)
    {
        for (int i = 0; i < buffer->GetSize(); i++)
        {
            if (i >= buffer->GetSize() - numToShift)
                buffer->Get()[i] = 0.0;
            else
            {
                buffer->Get()[i] = buffer->Get()[i + numToShift];
            }
        }
    }   
}
template void ShiftBuffer(WDL_TypedBuf<float> *buffer, int numToShift);
template void ShiftBuffer(WDL_TypedBuf<double> *buffer, int numToShift);
