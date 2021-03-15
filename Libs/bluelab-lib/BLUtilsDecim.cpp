#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "BLUtilsDecim.h"

template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateValues(WDL_TypedBuf<FLOAT_TYPE> *result,
                             const WDL_TypedBuf<FLOAT_TYPE> &buf,
                             FLOAT_TYPE decFactor)
{
    if (buf.GetSize() == 0)
        return;
    
    if (decFactor >= 1.0)
    {
        *result = buf;
        
        return;
    }
    
    BLUtils::ResizeFillZeros(result, buf.GetSize()*decFactor);
    
    int resultIdx = 0;
    
    // Keep the maxima when decimating
    FLOAT_TYPE count = 0.0;
    FLOAT_TYPE maxSample = 0.0;
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE samp = bufData[i];
        if (std::fabs(samp) > std::fabs(maxSample))
            maxSample = samp;
        
        // Fix for spectrograms
        //count += 1.0/decFactor;
        count += decFactor;
        
        if (count >= 1.0)
        {
            resultData[resultIdx++] = maxSample;
            
            maxSample = 0.0;
            
            count -= 1.0;
        }
        
        if (resultIdx >=  resultSize)
            break;
    }
}
template void BLUtilsDecim::DecimateValues(WDL_TypedBuf<float> *result,
                                           const WDL_TypedBuf<float> &buf,
                                           float decFactor);
template void BLUtilsDecim::DecimateValues(WDL_TypedBuf<double> *result,
                                           const WDL_TypedBuf<double> &buf,
                                           double decFactor);


template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateValues(WDL_TypedBuf<FLOAT_TYPE> *ioValues,
                             FLOAT_TYPE decFactor)
{
    WDL_TypedBuf<FLOAT_TYPE> origSamples = *ioValues;
    DecimateValues(ioValues, origSamples, decFactor);
}
template void BLUtilsDecim::DecimateValues(WDL_TypedBuf<float> *ioValues,
                                           float decFactor);
template void BLUtilsDecim::DecimateValues(WDL_TypedBuf<double> *ioValues,
                                           double decFactor);


template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateValuesDb(WDL_TypedBuf<FLOAT_TYPE> *result,
                               const WDL_TypedBuf<FLOAT_TYPE> &buf,
                               FLOAT_TYPE decFactor, FLOAT_TYPE minValueDb)
{
    if (buf.GetSize() == 0)
        return;
    
    if (decFactor >= 1.0)
    {
        *result = buf;
        
        return;
    }
    
    BLUtils::ResizeFillZeros(result, buf.GetSize()*decFactor);
    
    int resultIdx = 0;
    
    // Keep the maxima when decimating
    FLOAT_TYPE count = 0.0;
    FLOAT_TYPE maxSample = minValueDb;
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE samp = bufData[i];
        //if (std::fabs(samp) > std::fabs(maxSample))
        if (samp > maxSample)
            maxSample = samp;
        
        // Fix for spectrograms
        //count += 1.0/decFactor;
        count += decFactor;
        
        if (count >= 1.0)
        {
            resultData[resultIdx++] = maxSample;
            
            maxSample = minValueDb;
            
            count -= 1.0;
        }
        
        if (resultIdx >= resultSize)
            break;
    }
}
template void BLUtilsDecim::DecimateValuesDb(WDL_TypedBuf<float> *result,
                                             const WDL_TypedBuf<float> &buf,
                                             float decFactor, float minValueDb);
template void BLUtilsDecim::DecimateValuesDb(WDL_TypedBuf<double> *result,
                                             const WDL_TypedBuf<double> &buf,
                                             double decFactor, double minValueDb);


// Original version: use zero crossing of the samples
//
// FIX: to avoid long series of positive values not looking like waveforms
// FIX2: improved initial fix: really avoid loosing interesting min and max
//
// BUG: last value is not well managed
// (test: series of 10 impulses, with 1 impulse at the end => it is rest to 0)
template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateSamples(WDL_TypedBuf<FLOAT_TYPE> *result,
                              const WDL_TypedBuf<FLOAT_TYPE> &buf,
                              FLOAT_TYPE decFactor)
{
    // In case buf size is odd, and result size is even,
    // be sure to manage the last buf value
#define FIX_LAST_SAMPLE 0 //1

    // Adjust the result size to ceil(), and adjust decFactor
    // (in case of odd input buffer)
#define FIX_ODD_DECIMATE 1
    
    // Avoid re-using samples several times
    // e.g: Series of 10 impulses, after a certain level,
    // impulse peaks gets duplicated
#define FIX_SAMPLES_REUSE 1

    // Be sure to keep the first sample value
    // e.g: Series of 10 impulses,
    // first sample is an impulse peak
#define FIX_FIRST_SAMPLE 1
    
    if (buf.GetSize() == 0)
        return;
    
    if (decFactor >= 1.0)
    {
        *result = buf;
        
        return;
    }

#if !FIX_ODD_DECIMATE
    int newSize = buf.GetSize()*decFactor;
#else
    int newSize = ceil(buf.GetSize()*decFactor);
    // Adjust decimation factor
    decFactor = ((FLOAT_TYPE)newSize)/buf.GetSize();
#endif
    
    BLUtils::ResizeFillZeros(result, newSize);

    // Buffers
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    int resultIdx = 0;
    
    // Keep the maxima when decimating
    FLOAT_TYPE count = 0.0;
    
    FLOAT_TYPE minSample = buf.Get()[0];
    FLOAT_TYPE maxSample = buf.Get()[0];
    FLOAT_TYPE prevSample = buf.Get()[0];
    
    // When set to true, avoid flat beginning when the first values are negative
    bool zeroCrossed = true;

    FLOAT_TYPE prevSampleUsed = 0.0;
    
#if FIX_SAMPLES_REUSE
    bool maxSampleUsed = false;
    bool minSampleUsed = false;
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE samp = bufData[i];

        // Avoid re-using same samples several times
#if FIX_SAMPLES_REUSE
        if (maxSampleUsed)
        {
            maxSample = samp;

            maxSampleUsed = false;
        }
        if (minSampleUsed)
        {
            minSample = samp;

            minSampleUsed = false;
        }
#endif
        
        if (samp > maxSample)
            maxSample = samp;
        
        if (samp < minSample)
            minSample = samp;
        
        // Optimize by removing the multiplication
        // (sometimes we run through millions of samples,
        // so it could be worth it to optimize this)
        
        if ((samp > 0.0 && prevSample < 0.0) ||
            (samp < 0.0 && prevSample > 0.0))
            zeroCrossed = true;
        
        prevSample = samp;
        
        // Fix for spectrograms
        //count += 1.0/decFactor;

        count += decFactor;

#if !FIX_LAST_SAMPLE
        if (count >= 1.0)
#else
            if ((count >= 1.0) || // Enough src samples visited
                (i == bufSize - 1)) // Last src sample
#endif            
            {
                // Take care, if we crossed zero,
                // we take alternately positive and negative samples
                // (otherwise, we could have very long series of positive samples
                // for example. And this won't look like a waveform anymore...
                FLOAT_TYPE sampleToUse;
                if (!zeroCrossed)
                {
                    // Prefer reseting only min or max, not both, to avoid loosing
                    // interesting values
                    if (prevSampleUsed >= 0.0)
                    {
                        sampleToUse = maxSample;
                    
                        // FIX: avoid segments stuck at 0 during several samples
                        maxSample = samp;

#if FIX_SAMPLES_REUSE
                        maxSampleUsed = true;
#endif
                    }
                    else
                    {
                        sampleToUse = minSample;
                        minSample = samp;

#if FIX_SAMPLES_REUSE
                        minSampleUsed = true;
#endif
                    }
                }
                else
                {
                    if (prevSampleUsed >= 0.0)
                    {
                        sampleToUse = minSample;
                        minSample = samp;

#if FIX_SAMPLES_REUSE
                        minSampleUsed = true;
#endif
                    }
                    else
                    {
                        sampleToUse = maxSample;
                        maxSample = samp;

#if FIX_SAMPLES_REUSE
                        maxSampleUsed = true;
#endif
                    }
                }
            
                resultData[resultIdx] = sampleToUse;

#if FIX_FIRST_SAMPLE
                if (resultIdx == 0)
                {
                    // Be sure that the first sample will be the same
                    // in th origin buf and in the decimated buf
                    resultData[resultIdx] = buf.Get()[0];

#if FIX_SAMPLES_REUSE
                    // Avoid re-using same value several times
                    if (std::fabs(buf.Get()[0] - minSample) < BL_EPS)
                        minSampleUsed = true;
                    else
                        maxSampleUsed = true;
#endif
                }
#endif

                resultIdx++;
            
                count -= 1.0;
            
                prevSampleUsed = sampleToUse;
                zeroCrossed = false;
            }

#if !FIX_LAST_SAMPLE
        if (resultIdx >=  resultSize)
            break;
#else
        if (resultIdx > resultSize - 1)
            resultIdx = resultSize - 1;
#endif
    }
}
template void BLUtilsDecim::DecimateSamples(WDL_TypedBuf<float> *result,
                                            const WDL_TypedBuf<float> &buf,
                                            float decFactor);
template void BLUtilsDecim::DecimateSamples(WDL_TypedBuf<double> *result,
                                            const WDL_TypedBuf<double> &buf,
                                            double decFactor);

// DOESN'T WORK...
// Incremental version
// Try to fix long sections of 0 values
template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateSamples2(WDL_TypedBuf<FLOAT_TYPE> *result,
                               const WDL_TypedBuf<FLOAT_TYPE> &buf,
                               FLOAT_TYPE decFactor)
{
    FLOAT_TYPE factor = 0.5;
    
    // Decimate progressively
    WDL_TypedBuf<FLOAT_TYPE> tmp = buf;
    while(tmp.GetSize() > buf.GetSize()*decFactor*2.0)
    {
        DecimateSamples(&tmp, factor);
    }
    
    // Last step
    DecimateSamples(&tmp, decFactor);
    
    *result = tmp;
}
template void BLUtilsDecim::DecimateSamples2(WDL_TypedBuf<float> *result,
                                             const WDL_TypedBuf<float> &buf,
                                             float decFactor);
template void BLUtilsDecim::DecimateSamples2(WDL_TypedBuf<double> *result,
                                             const WDL_TypedBuf<double> &buf,
                                             double decFactor);


// New: take the maximum spaced value (no more zero crossing
// => code a lot simpler, and result very good
//
// FIX: to avoid long series of positive values not looking like waveforms
// FIX2: improved initial fix: really avoid loosing interesting min and max
//
// FIX: last value was not well managed
// (test: series of 10 impulses, with 1 impulse at the end => it is rest to 0)
template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateSamples3(WDL_TypedBuf<FLOAT_TYPE> *result,
                               const WDL_TypedBuf<FLOAT_TYPE> &buf,
                               FLOAT_TYPE decFactor)
{
    // In case buf size is odd, and result size is even,
    // be sure to manage the last buf value
#define FIX_LAST_SAMPLE 1

    // Adjust the result size to ceil(), and adjust decFactor
    // (in case of odd input buffer)
#define FIX_ODD_DECIMATE 1

    // Be sure to keep the first sample value
    // e.g: Series of 10 impulses,
    // first sample is an impulse peak
#define FIX_FIRST_SAMPLE 1
    
    if (buf.GetSize() == 0)
        return;
    
    if (decFactor >= 1.0)
    {
        *result = buf;
        
        return;
    }

#if !FIX_ODD_DECIMATE
    int newSize = buf.GetSize()*decFactor;
#else
    int newSize = ceil(buf.GetSize()*decFactor);
    // Adjust decimation factor
    decFactor = ((FLOAT_TYPE)newSize)/buf.GetSize();
#endif
    
    BLUtils::ResizeFillZeros(result, newSize);

    // Buffers
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    int resultIdx = 0;
    
    // Keep the maxima when decimating
    FLOAT_TYPE count = 0.0;
    
    FLOAT_TYPE maxSample = buf.Get()[0];
    FLOAT_TYPE prevSampleUsed = buf.Get()[0];
        
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE samp = bufData[i];

        // Take the maximum spacing (whatever the sign
        if (std::fabs(samp - prevSampleUsed) > std::fabs(maxSample - prevSampleUsed))
            maxSample = samp;
        
        count += decFactor;

#if !FIX_LAST_SAMPLE
        if (count >= 1.0)
#else
            if ((count >= 1.0) || // Enough src samples visited
                (i == bufSize - 1)) // Last src sample
#endif            
            {
                resultData[resultIdx] = maxSample;

                prevSampleUsed = maxSample;
            
#if FIX_FIRST_SAMPLE
                if (resultIdx == 0)
                {
                    // Be sure that the first sample will be the same
                    // in th origin buf and in the decimated buf
                    resultData[resultIdx] = buf.Get()[0];
                }
#endif

                resultIdx++;
            
                count -= 1.0;

                prevSampleUsed = maxSample;
            }

        if (resultIdx >=  resultSize)
            break;
    }
}
template void BLUtilsDecim::DecimateSamples3(WDL_TypedBuf<float> *result,
                                             const WDL_TypedBuf<float> &buf,
                                             float decFactor);
template void BLUtilsDecim::DecimateSamples3(WDL_TypedBuf<double> *result,
                                             const WDL_TypedBuf<double> &buf,
                                             double decFactor);


template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateSamples(WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                              FLOAT_TYPE decFactor)
{
    WDL_TypedBuf<FLOAT_TYPE> origSamples = *ioSamples;
    DecimateSamples(ioSamples, origSamples, decFactor);
}
template void BLUtilsDecim::DecimateSamples(WDL_TypedBuf<float> *ioSamples,
                                            float decFactor);
template void BLUtilsDecim::DecimateSamples(WDL_TypedBuf<double> *ioSamples,
                                            double decFactor);

// Simply take some samples and throw out the others
// ("sparkling" when zooming in Ghost)
template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateSamplesFast(WDL_TypedBuf<FLOAT_TYPE> *result,
                                  const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                  FLOAT_TYPE decFactor)
{
    BLUtils::ResizeFillZeros(result, buf.GetSize()*decFactor);
    
    int step = (decFactor > 0) ? 1.0/decFactor : buf.GetSize();
    int resId = 0;
    
    int bufSize = buf.GetSize();
    FLOAT_TYPE *bufData = buf.Get();
    int resultSize = result->GetSize();
    FLOAT_TYPE *resultData = result->Get();
    
    for (int i = 0; i < bufSize; i+= step)
    {
        FLOAT_TYPE val = bufData[i];
        
        if (resId >= resultSize)
            break;
        
        resultData[resId++] = val;
    }
}
template void BLUtilsDecim::DecimateSamplesFast(WDL_TypedBuf<float> *result,
                                                const WDL_TypedBuf<float> &buf,
                                                float decFactor);
template void BLUtilsDecim::DecimateSamplesFast(WDL_TypedBuf<double> *result,
                                                const WDL_TypedBuf<double> &buf,
                                                double decFactor);

template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateStep(WDL_TypedBuf<FLOAT_TYPE> *ioSamples, int step)
{
    const WDL_TypedBuf<FLOAT_TYPE> &copyInBuf = *ioSamples;
    
    DecimateStep(copyInBuf, ioSamples, step);
    
    /*int numSamples = ioSamples->GetSize();
    
      WDL_TypedBuf<FLOAT_TYPE> samplesCopy = *ioSamples;
      const FLOAT_TYPE *copyBuf = samplesCopy.Get();
      
      ioSamples->Resize(ioSamples->GetSize()/step);
      int numResultSamples = ioSamples->GetSize();
      FLOAT_TYPE *resultBuf = ioSamples->Get();
      
      int resPos = 0;
      for (int i = 0; i < numSamples; i += step)
      {
      if (resPos < numResultSamples)
      {
      FLOAT_TYPE val = copyBuf[i];
      resultBuf[resPos++] = val;
      }
      }*/
}
template void BLUtilsDecim::DecimateStep(WDL_TypedBuf<float> *ioSamples, int step);
template void BLUtilsDecim::DecimateStep(WDL_TypedBuf<double> *ioSamples, int step);

template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateStep(const WDL_TypedBuf<FLOAT_TYPE> &inSamples,
                           WDL_TypedBuf<FLOAT_TYPE> *outSamples,
                           int step)
{
    int numSamples = inSamples.GetSize();
    
    //WDL_TypedBuf<FLOAT_TYPE> samplesCopy = *ioSamples;
    const FLOAT_TYPE *inBuf = inSamples.Get();
    
    outSamples->Resize(inSamples.GetSize()/step);
    int numResultSamples = outSamples->GetSize();
    FLOAT_TYPE *outBuf = outSamples->Get();
    
    int resPos = 0;
    for (int i = 0; i < numSamples; i += step)
    {
        if (resPos < numResultSamples)
        {
            FLOAT_TYPE val = inBuf[i];
            outBuf[resPos++] = val;
        }
    }
}
template void BLUtilsDecim::DecimateStep(const WDL_TypedBuf<float> &inSamples,
                                         WDL_TypedBuf<float> *outSamples,
                                         int step);
template void BLUtilsDecim::DecimateStep(const WDL_TypedBuf<double> &inSamples,
                                         WDL_TypedBuf<double> *outSamples,
                                         int step);

template <typename FLOAT_TYPE>
void
BLUtilsDecim::DecimateSamplesFast(WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                                  FLOAT_TYPE decFactor)
{
    WDL_TypedBuf<FLOAT_TYPE> buf = *ioSamples;
    DecimateSamplesFast(ioSamples, buf, decFactor);
}
template void BLUtilsDecim::DecimateSamplesFast(WDL_TypedBuf<float> *ioSamples,
                                                float decFactor);
template void BLUtilsDecim::DecimateSamplesFast(WDL_TypedBuf<double> *ioSamples,
                                                double decFactor);
