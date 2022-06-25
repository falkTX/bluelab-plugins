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
 
#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <CMA2Smoother.h>

#include "BLUtilsEnvelope.h"

template <typename FLOAT_TYPE>
void
BLUtilsEnvelope::ShiftSamples(const WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                              int shiftSize)
{
    if (shiftSize < 0)
        shiftSize += ioSamples->GetSize();
    
    WDL_TypedBuf<FLOAT_TYPE> copySamples = *ioSamples;
    
    int ioSamplesSize = ioSamples->GetSize();
    FLOAT_TYPE *ioSamplesData = ioSamples->Get();
    FLOAT_TYPE *copySamplesData = copySamples.Get();
    
    for (int i = 0; i < ioSamplesSize; i++)
    {
        int srcIndex = i;
        int dstIndex = (srcIndex + shiftSize) % ioSamplesSize;
        
        ioSamplesData[dstIndex] = copySamplesData[srcIndex];
    }
}
template void BLUtilsEnvelope::ShiftSamples(const WDL_TypedBuf<float> *ioSamples,
                                            int shiftSize);
template void BLUtilsEnvelope::ShiftSamples(const WDL_TypedBuf<double> *ioSamples,
                                            int shiftSize);

template <typename FLOAT_TYPE>
void
BLUtilsEnvelope::ComputeEnvelope(const WDL_TypedBuf<FLOAT_TYPE> &samples,
                                 WDL_TypedBuf<FLOAT_TYPE> *envelope,
                                 bool extendBoundsValues)
{
    WDL_TypedBuf<FLOAT_TYPE> maxValues;
    maxValues.Resize(samples.GetSize());
    BLUtils::FillAllZero(&maxValues);
    
    // First step: put the maxima in the array
    FLOAT_TYPE prevSamples[3] = { 0.0, 0.0, 0.0 };
    bool zeroWasCrossed = false;
    FLOAT_TYPE prevValue = 0.0;
    
    int samplesSize = samples.GetSize();
    FLOAT_TYPE *samplesData = samples.Get();
    int maxValuesSize = maxValues.GetSize();
    FLOAT_TYPE *maxValuesData = maxValues.Get();
    
    for (int i = 0; i < samplesSize; i++)
    {
        FLOAT_TYPE sample = samplesData[i];
        
        // Wait for crossing the zero line a first time
        if (i == 0)
        {
            prevValue = sample;
            
            continue;
        }
        
        if (!zeroWasCrossed)
        {
            if (prevValue*sample < 0.0)
            {
                zeroWasCrossed = true;
            }
            
            prevValue = sample;
            
            // Before first zero cross, we don't take the maximum
            continue;
        }
        
        sample = std::fabs(sample);
        
        prevSamples[0] = prevSamples[1];
        prevSamples[1] = prevSamples[2];
        prevSamples[2] = sample;
        
        if ((prevSamples[1] >= prevSamples[0]) &&
            (prevSamples[1] >= prevSamples[2]))
            // Local maximum
        {
            int idx = i - 1;
            if (idx < 0)
                idx = 0;
            maxValuesData[idx] = prevSamples[1];
        }
    }
    
    // Suppress the last maximum until zero is crossed
    // (avoids finding maxima from edges of truncated periods)
    FLOAT_TYPE prevValue2 = 0.0;
    for (int i = samplesSize - 1; i > 0; i--)
    {
        FLOAT_TYPE sample = samplesData[i];
        if (prevValue2*sample < 0.0)
            // Zero is crossed !
        {
            break;
        }
        
        prevValue2 = sample;
        
        // Suppress potential false maxima
        maxValuesData[i] = 0.0;
    }
    
    // Should be defined to 1 !
#if 0 // TODO: check it and validate it (code factoring :) ) !
    *envelope = maxValues;
    
    FillMissingValues(envelope, extendBoundsValues);
    
#else
    if (extendBoundsValues)
        // Extend the last maximum to the end
    {
        // Find the last max
        int lastMaxIndex = samplesSize - 1;
        FLOAT_TYPE lastMax = 0.0;
        for (int i = samplesSize - 1; i > 0; i--)
        {
            FLOAT_TYPE val = maxValuesData[i];
            if (val > 0.0)
            {
                lastMax = val;
                lastMaxIndex = i;
            
                break;
            }
        }
    
        // Fill the last values with last max
        for (int i = samplesSize - 1; i > lastMaxIndex; i--)
        {
            maxValuesData[i] = lastMax;
        }
    }
    
    // Second step: fill the holes by linear interpolation
    //envelope->Resize(samples.GetSize());
    //BLUtilsEnvelope::FillAllZero(envelope);
    *envelope = maxValues;
    
    FLOAT_TYPE startVal = 0.0;
    
    // First, find start val
    for (int i = 0; i < maxValuesSize; i++)
    {
        FLOAT_TYPE val = maxValuesData[i];
        if (val > 0.0)
        {
            startVal = val;
        }
    }
    
    int loopIdx = 0;
    int startIndex = 0;
    //FLOAT_TYPE lastValidVal = 0.0;
    
    // Then start the main loop
    
    int envelopeSize = envelope->GetSize();
    FLOAT_TYPE *envelopeData = envelope->Get();
    
    while(loopIdx < envelopeSize)
    {
        FLOAT_TYPE val = maxValuesData[loopIdx];
        
        if (val > 0.0)
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
            if (!extendBoundsValues &&
                (loopIdx == 0))
                startVal = 0.0;
            
            // Find how many missing values we have
            int endIndex = startIndex + 1;
            FLOAT_TYPE endVal = 0.0;
            bool defined = false;
            
            while(endIndex < maxValuesSize)
            {
                if (endIndex < maxValuesSize)
                    endVal = maxValuesData[endIndex];
                
                defined = (endVal > 0.0);
                if (defined)
                    break;
                
                endIndex++;
            }
    
#if 0 // Make problems with envelopes ending with zeros
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
                FLOAT_TYPE t = ((FLOAT_TYPE)(i - startIndex))/
                    (endIndex - startIndex - 1);
                
                FLOAT_TYPE newVal = (1.0 - t)*startVal + t*endVal;
                envelopeData[i] = newVal;
            }
            
            startIndex = endIndex;
            loopIdx = endIndex;
        }
    }
#endif
}
template void BLUtilsEnvelope::ComputeEnvelope(const WDL_TypedBuf<float> &samples,
                                               WDL_TypedBuf<float> *envelope,
                                               bool extendBoundsValues);
template void BLUtilsEnvelope::ComputeEnvelope(const WDL_TypedBuf<double> &samples,
                                               WDL_TypedBuf<double> *envelope,
                                               bool extendBoundsValues);

// Smooth, then compute envelope
///template <typename FLOAT_TYPE>
void
BLUtilsEnvelope::ComputeEnvelopeSmooth(CMA2Smoother *smoother,
                                       const WDL_TypedBuf<BL_FLOAT> &samples,
                                       WDL_TypedBuf<BL_FLOAT> *envelope,
                                       BL_FLOAT smoothCoeff,
                                       bool extendBoundsValues)
{
    WDL_TypedBuf<BL_FLOAT> smoothedSamples;
    smoothedSamples.Resize(samples.GetSize());
                           
    BL_FLOAT cmaCoeff = smoothCoeff*samples.GetSize();
    
    WDL_TypedBuf<BL_FLOAT> samplesAbs = samples;
    BLUtils::ComputeAbs(&samplesAbs);
    
    smoother->ProcessOne(samplesAbs.Get(), smoothedSamples.Get(),
                         samplesAbs.GetSize(), cmaCoeff);
    
    
    // Restore the sign, for envelope computation
    
    int samplesSize = samples.GetSize();
    BL_FLOAT *samplesData = samples.Get();
    BL_FLOAT *smoothedSamplesData = smoothedSamples.Get();
    
    for (int i = 0; i < samplesSize; i++)
    {
        BL_FLOAT sample = samplesData[i];
        
        if (sample < 0.0)
            smoothedSamplesData[i] *= -1.0;
    }
    
    ComputeEnvelope(smoothedSamples, envelope, extendBoundsValues);
}
/*template void BLUtilsEnvelope::ComputeEnvelopeSmooth(const WDL_TypedBuf<float> &samples,
  WDL_TypedBuf<float> *envelope,
  float smoothCoeff,
  bool extendBoundsValues);
  template void BLUtilsEnvelope::ComputeEnvelopeSmooth(const WDL_TypedBuf<double> &samples,
  WDL_TypedBuf<double> *envelope,
  double smoothCoeff,
  bool extendBoundsValues);
*/

// Compute an envelope by only smoothing
//template <typename FLOAT_TYPE>
void
BLUtilsEnvelope::ComputeEnvelopeSmooth2(CMA2Smoother *smoother,
                                        const WDL_TypedBuf<BL_FLOAT> &samples,
                                        WDL_TypedBuf<BL_FLOAT> *envelope,
                                        BL_FLOAT smoothCoeff)
{
    envelope->Resize(samples.GetSize());
    
    BL_FLOAT cmaCoeff = smoothCoeff*samples.GetSize();
    
    WDL_TypedBuf<BL_FLOAT> samplesAbs = samples;
    BLUtils::ComputeAbs(&samplesAbs);
    
    smoother->ProcessOne(samplesAbs.Get(), envelope->Get(),
                         samplesAbs.GetSize(), cmaCoeff);
    
    // Normalize
    // Because CMA2Smoother reduce the values
    
    BL_FLOAT maxSamples = BLUtils::ComputeMax(samples.Get(),
                                              samples.GetSize());
    BL_FLOAT maxEnvelope = BLUtils::ComputeMax(envelope->Get(),
                                               envelope->GetSize());
    
    if (maxEnvelope > BL_EPS)
    {
        BL_FLOAT coeff = maxSamples/maxEnvelope;
        BLUtils::MultValues(envelope, coeff);
    }
}
/*template void BLUtilsEnvelope::ComputeEnvelopeSmooth2(const WDL_TypedBuf<float> &samples,
  WDL_TypedBuf<float> *envelope,
  float smoothCoeff);
  template void BLUtilsEnvelope::ComputeEnvelopeSmooth2(const WDL_TypedBuf<double> &samples,
  WDL_TypedBuf<double> *envelope,
  double smoothCoeff);
*/

template <typename FLOAT_TYPE>
void
BLUtilsEnvelope::ZeroBoundEnvelope(WDL_TypedBuf<FLOAT_TYPE> *envelope)
{
    if (envelope->GetSize() == 0)
        return;
    
    envelope->Get()[0] = 0.0;
    envelope->Get()[envelope->GetSize() - 1] = 0.0;
}
template void BLUtilsEnvelope::ZeroBoundEnvelope(WDL_TypedBuf<float> *envelope);
template void BLUtilsEnvelope::ZeroBoundEnvelope(WDL_TypedBuf<double> *envelope);

template <typename FLOAT_TYPE>
void
BLUtilsEnvelope::CorrectEnvelope(WDL_TypedBuf<FLOAT_TYPE> *samples,
                                 const WDL_TypedBuf<FLOAT_TYPE> &envelope0,
                                 const WDL_TypedBuf<FLOAT_TYPE> &envelope1)
{
    int samplesSize = samples->GetSize();
    FLOAT_TYPE *samplesData = samples->Get();
    FLOAT_TYPE *envelope0Data = envelope0.Get();
    FLOAT_TYPE *envelope1Data = envelope1.Get();
    
    for (int i = 0; i < samplesSize; i++)
    {
        FLOAT_TYPE sample = samplesData[i];
        
        FLOAT_TYPE env0 = envelope0Data[i];
        FLOAT_TYPE env1 = envelope1Data[i];
        
        FLOAT_TYPE coeff = 0.0; //
        
        if ((env0 > BL_EPS) && (env1 > BL_EPS))
            coeff = env0/env1;
        
        sample *= coeff;
        
        // Just in case
        if (sample > 1.0)
            sample = 1.0;
        if (sample < -1.0)
            sample = -1.0;
        
        samplesData[i] = sample;
    }
}
template void BLUtilsEnvelope::CorrectEnvelope(WDL_TypedBuf<float> *samples,
                                               const WDL_TypedBuf<float> &envelope0,
                                               const WDL_TypedBuf<float> &envelope1);
template void BLUtilsEnvelope::CorrectEnvelope(WDL_TypedBuf<double> *samples,
                                               const WDL_TypedBuf<double> &envelope0,
                                               const WDL_TypedBuf<double> &envelope1);

template <typename FLOAT_TYPE>
int
BLUtilsEnvelope::GetEnvelopeShift(const WDL_TypedBuf<FLOAT_TYPE> &envelope0,
                                  const WDL_TypedBuf<FLOAT_TYPE> &envelope1,
                                  int precision)
{
    int max0 = BLUtils::FindMaxIndex(envelope0);
    int max1 = BLUtils::FindMaxIndex(envelope1);
    
    int shift = max1 - max0;
    
    if (precision > 1)
    {
        FLOAT_TYPE newShift = ((FLOAT_TYPE)shift)/precision;
        newShift = bl_round(newShift);
        
        newShift *= precision;
        
        shift = newShift;
    }
    
    return shift;
}
template int BLUtilsEnvelope::GetEnvelopeShift(const WDL_TypedBuf<float> &envelope0,
                                               const WDL_TypedBuf<float> &envelope1,
                                               int precision);
template int BLUtilsEnvelope::GetEnvelopeShift(const WDL_TypedBuf<double> &envelope0,
                                               const WDL_TypedBuf<double> &envelope1,
                                               int precision);
