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

#include "BLUtilsFft.h"

#define USE_SIMD_OPTIM 1

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsFft::FftBinToFreq(int binNum, int numBins, FLOAT_TYPE sampleRate)
{
    if (binNum > numBins/2)
        // Second half => not relevant
        return -1.0;
    
    // Problem here ?
    return binNum*sampleRate/(numBins /*2.0*/); // Modif for Zarlino
}
template float BLUtilsFft::FftBinToFreq(int binNum, int numBins, float sampleRate);
template double BLUtilsFft::FftBinToFreq(int binNum, int numBins, double sampleRate);

// Fixed version
// In the case we want to fill a BUFFER_SIZE/2 array, as it should be
// (for stereo phase correction)
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsFft::FftBinToFreq2(int binNum, int numBins, FLOAT_TYPE sampleRate)
{
    if (binNum > numBins)
        // Second half => not relevant
        return -1.0;
    
    // Problem here ?
    return binNum*sampleRate/(numBins*2.0);
}
template float BLUtilsFft::FftBinToFreq2(int binNum, int numBins, float sampleRate);
template double BLUtilsFft::FftBinToFreq2(int binNum, int numBins, double sampleRate);

// This version may be false
template <typename FLOAT_TYPE>
int
BLUtilsFft::FreqToFftBin(FLOAT_TYPE freq, int numBins,
                         FLOAT_TYPE sampleRate, FLOAT_TYPE *t)
{
    FLOAT_TYPE fftBin = (freq*numBins/*/2.0*/)/sampleRate; // Modif for Zarlino
    
    // Round with 1e-10 precision
    // This is necessary otherwise we will take the wrong
    // bin when there are rounding errors like "80.999999999"
    fftBin = BLUtilsMath::Round(fftBin, 10);
    
    if (t != NULL)
    {
        FLOAT_TYPE freq0 = FftBinToFreq(fftBin, numBins, sampleRate);
        FLOAT_TYPE freq1 = FftBinToFreq(fftBin + 1, numBins, sampleRate);
        
        *t = (freq - freq0)/(freq1 - freq0);
    }
    
    return fftBin;
}
template int BLUtilsFft::FreqToFftBin(float freq, int numBins,
                                      float sampleRate, float *t);
template int BLUtilsFft::FreqToFftBin(double freq, int numBins,
                                      double sampleRate, double *t);

template <typename FLOAT_TYPE>
void
BLUtilsFft::FftFreqs(WDL_TypedBuf<FLOAT_TYPE> *freqs, int numBins,
                     FLOAT_TYPE sampleRate)
{
    freqs->Resize(numBins);
    FLOAT_TYPE *freqsData = freqs->Get();
    
    for (int i = 0; i < numBins; i++)
    {
        FLOAT_TYPE freq = FftBinToFreq2(i, numBins, sampleRate);
        
        freqsData[i] = freq;
    }
}
template void BLUtilsFft::FftFreqs(WDL_TypedBuf<float> *freqs, int numBins,
                                   float sampleRate);
template void BLUtilsFft::FftFreqs(WDL_TypedBuf<double> *freqs, int numBins,
                                   double sampleRate);

template <typename FLOAT_TYPE>
void
BLUtilsFft::MinMaxFftBinFreq(FLOAT_TYPE *minFreq, FLOAT_TYPE *maxFreq,
                             int numBins, FLOAT_TYPE sampleRate)
{
    *minFreq = sampleRate/(numBins/2.0);
    *maxFreq = ((FLOAT_TYPE)(numBins/2.0 - 1.0)*sampleRate)/numBins;
}
template void BLUtilsFft::MinMaxFftBinFreq(float *minFreq, float *maxFreq,
                                           int numBins, float sampleRate);
template void BLUtilsFft::MinMaxFftBinFreq(double *minFreq, double *maxFreq,
                                           int numBins, double sampleRate);

template <typename FLOAT_TYPE>
void
BLUtilsFft::NormalizeFftValues(WDL_TypedBuf<FLOAT_TYPE> *magns)
{
    FLOAT_TYPE sum = 0.0;
    
    int magnsSize = magns->GetSize();
    FLOAT_TYPE *magnsData = magns->Get();
    
#if !USE_SIMD_OPTIM
    // Not test "/2"
    for (int i = 1; i < magnsSize/*/2*/; i++)
    {
        FLOAT_TYPE magn = magnsData[i];
        
        sum += magn;
    }
#else
    sum = BLUtils::ComputeSum(magnsData, magnsSize);
    if (magnsSize > 0)
        sum -= magnsData[0];
#endif
    
    sum /= magns->GetSize()/*/2*/ - 1;
    
    magns->Get()[0] = sum;
    magns->Get()[magns->GetSize() - 1] = sum;
}
template void BLUtilsFft::NormalizeFftValues(WDL_TypedBuf<float> *magns);
template void BLUtilsFft::NormalizeFftValues(WDL_TypedBuf<double> *magns);

void
BLUtilsFft::FillSecondFftHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    if (ioBuffer->GetSize() < 2)
        return;
    
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    
    int ioBufferSize2 = ioBuffer->GetSize()/2;
    WDL_FFT_COMPLEX *ioBufferData = ioBuffer->Get();
    
    for (int i = 1; i < ioBufferSize2; i++)
    {
        int id0 = i + ioBufferSize2;
        
#if 1 // ORIG
        // Orig, bug...
        // doesn't fill exactly the symetry (the last value differs)
        // but WDL generates ffts like that
        int id1 = ioBufferSize2 - i;
#endif
        
#if 1 // FIX: fill the value at the middle
      // (which was not filled, and could be undefined if not filled outside the function)
      //
      // NOTE: added for Rebalance, to fix a bug:
      // - waveform values like 1e+250
      //
      // NOTE: quick fix, better solution could be found, by
      // comparing with WDL fft
      //
      // NOTE: could fix many plugins, like for example StereoViz
      //
        ioBufferData[ioBufferSize2].re = 0.0;
        ioBufferData[ioBufferSize2].im = 0.0;
#endif
        
#if 0 // Bug fix (but strange WDL behaviour)
        // Really symetric version
        // with correct last value
        // But if we apply to just generate WDL fft, the behaviour becomes different
        int id1 = ioBufferSize2 - i - 1;
#endif
        
        ioBufferData[id0].re = ioBufferData[id1].re;
        
        // Complex conjugate
        ioBufferData[id0].im = -ioBufferData[id1].im;
    }
}

template <typename FLOAT_TYPE>
void
BLUtilsFft::FillSecondFftHalf(WDL_TypedBuf<FLOAT_TYPE> *ioMagns)
{
    if (ioMagns->GetSize() < 2)
        return;

    int ioMagnsSize2 = ioMagns->GetSize()/2;
    FLOAT_TYPE *ioMagnsData = ioMagns->Get();
    
    for (int i = 1; i < ioMagnsSize2; i++)
    {
        int id0 = i + ioMagnsSize2;
        
        // WARNING: doesn't fill exactly the symetry (the last value differs)
        // but WDL generates ffts like that
        int id1 = ioMagnsSize2 - i;
        
        // FIX: fill the value at the middle
        ioMagnsData[ioMagnsSize2] = 0.0;
        
        ioMagnsData[id0] = ioMagnsData[id1];
    }
}
template void BLUtilsFft::FillSecondFftHalf(WDL_TypedBuf<float> *ioMagns);
template void BLUtilsFft::FillSecondFftHalf(WDL_TypedBuf<double> *ioMagns);

void
BLUtilsFft::FillSecondFftHalf(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inHalfBuf,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *outBuf)
{
    if (inHalfBuf.GetSize() < 1)
        return;
    outBuf->Resize(inHalfBuf.GetSize()*2);

    memcpy(outBuf->Get(), inHalfBuf.Get(),
           inHalfBuf.GetSize()*sizeof(WDL_FFT_COMPLEX));
           
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    
    int ioBufferSize2 = inHalfBuf.GetSize();
    WDL_FFT_COMPLEX *ioBufferData = outBuf->Get();
    
    for (int i = 1; i < ioBufferSize2; i++)
    {
        int id0 = i + ioBufferSize2;
        
        // Orig, bug...
        // doesn't fill exactly the symetry (the last value differs)
        // but WDL generates ffts like that
        int id1 = ioBufferSize2 - i;
        
        // FIX: fill the value at the middle
        // (which was not filled, and could be undefined if not filled outside the function)
        //
        // NOTE: added for Rebalance, to fix a bug:
        // - waveform values like 1e+250
        //
        // NOTE: quick fix, better solution could be found, by
        // comparing with WDL fft
        //
        // NOTE: could fix many plugins, like for example StereoViz
        //
        ioBufferData[ioBufferSize2].re = 0.0;
        ioBufferData[ioBufferSize2].im = 0.0;
        
        ioBufferData[id0].re = ioBufferData[id1].re;
        
        // Complex conjugate
        ioBufferData[id0].im = -ioBufferData[id1].im;
    }
}

template <typename FLOAT_TYPE>
void
BLUtilsFft::FillSecondFftHalf(const WDL_TypedBuf<FLOAT_TYPE> &inHalfMagns,
                              WDL_TypedBuf<FLOAT_TYPE> *outMagns)
{
    if (inHalfMagns.GetSize() < 1)
        return;
    outMagns->Resize(inHalfMagns.GetSize()*2);
    memcpy(outMagns->Get(), inHalfMagns.Get(),
           inHalfMagns.GetSize()*sizeof(FLOAT_TYPE));
    
    int ioMagnsSize2 = outMagns->GetSize()/2;
    FLOAT_TYPE *outMagnsData = outMagns->Get();
    
    for (int i = 1; i < ioMagnsSize2; i++)
    {
        int id0 = i + ioMagnsSize2;
        
        // WARNING: doesn't fill exactly the symetry (the last value differs)
        // but WDL generates ffts like that
        int id1 = ioMagnsSize2 - i;
        
        // FIX: fill the value at the middle
        outMagnsData[ioMagnsSize2] = 0.0;
        
        outMagnsData[id0] = outMagnsData[id1];
    }
}
template void BLUtilsFft::FillSecondFftHalf(const WDL_TypedBuf<float> &inHalfMagns,
                                            WDL_TypedBuf<float> *outMagns);
template void BLUtilsFft::FillSecondFftHalf(const WDL_TypedBuf<double> &inHalfMagns,
                                            WDL_TypedBuf<double> *outMagns);


// See: http://werner.yellowcouch.org/Papers/transients12/index.html
template <typename FLOAT_TYPE>
void
BLUtilsFft::FftIdsToSamplesIds(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                               WDL_TypedBuf<int> *samplesIds)
{
    samplesIds->Resize(phases.GetSize());
    BLUtils::FillAllZero(samplesIds);
    
    int bufSize = phases.GetSize();
    FLOAT_TYPE *phasesData = phases.Get();
    int *samplesIdsData = samplesIds->Get();
    
    FLOAT_TYPE prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];
        
        FLOAT_TYPE phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // TODO: optimize this !
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        FLOAT_TYPE samplePos = ((FLOAT_TYPE)bufSize)*phaseDiff/(2.0*M_PI);
        
        samplesIdsData[i] = (int)samplePos;
    }
    
    // NOT SURE AT ALL !
    // Just like that, seems inverted
    // So we reverse back !
    //BLUtilsFft::Reverse(samplesIds);
}
template void BLUtilsFft::FftIdsToSamplesIds(const WDL_TypedBuf<float> &phases,
                                             WDL_TypedBuf<int> *samplesIds);
template void BLUtilsFft::FftIdsToSamplesIds(const WDL_TypedBuf<double> &phases,
                                             WDL_TypedBuf<int> *samplesIds);

// See: http://werner.yellowcouch.org/Papers/transients12/index.html
template <typename FLOAT_TYPE>
void
BLUtilsFft::FftIdsToSamplesIdsFloat(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                    WDL_TypedBuf<FLOAT_TYPE> *samplesIds)
{
    samplesIds->Resize(phases.GetSize());
    BLUtils::FillAllZero(samplesIds);
    
    int bufSize = phases.GetSize();
    FLOAT_TYPE *phasesData = phases.Get();
    FLOAT_TYPE *samplesIdsData = samplesIds->Get();
    
    FLOAT_TYPE prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];
        
        FLOAT_TYPE phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // TODO: optimize this !
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        FLOAT_TYPE samplePos = ((FLOAT_TYPE)bufSize)*phaseDiff/(2.0*M_PI);
        
        samplesIdsData[i] = samplePos;
    }
    
    // NOT SURE AT ALL !
    // Just like that, seems inverted
    // So we reverse back !
    //BLUtilsFft::Reverse(samplesIds);
}
template void BLUtilsFft::FftIdsToSamplesIdsFloat(const WDL_TypedBuf<float> &phases,
                                                  WDL_TypedBuf<float> *samplesIds);
template void BLUtilsFft::FftIdsToSamplesIdsFloat(const WDL_TypedBuf<double> &phases,
                                                  WDL_TypedBuf<double> *samplesIds);

template <typename FLOAT_TYPE>
void
BLUtilsFft::FftIdsToSamplesIdsSym(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                  WDL_TypedBuf<int> *samplesIds)
{
    samplesIds->Resize(phases.GetSize());
    BLUtils::FillAllZero(samplesIds);
    
    int bufSize = phases.GetSize();
    FLOAT_TYPE *phasesData = phases.Get();
    int samplesIdsSize = samplesIds->GetSize();
    int *samplesIdsData = samplesIds->Get();
    
    FLOAT_TYPE prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];
        
        FLOAT_TYPE phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        FLOAT_TYPE samplePos = ((FLOAT_TYPE)bufSize)*phaseDiff/(2.0*M_PI);
        
        // For sym...
        samplePos *= 2.0;
        
        samplePos = fmod(samplePos, samplesIdsSize);
        
        samplesIdsData[i] = (int)samplePos;
    }
}
template void BLUtilsFft::FftIdsToSamplesIdsSym(const WDL_TypedBuf<float> &phases,
                                                WDL_TypedBuf<int> *samplesIds);
template void BLUtilsFft::FftIdsToSamplesIdsSym(const WDL_TypedBuf<double> &phases,
                                                WDL_TypedBuf<int> *samplesIds);

template <typename FLOAT_TYPE>
void
BLUtilsFft::SamplesIdsToFftIds(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                               WDL_TypedBuf<int> *fftIds)
{
    fftIds->Resize(phases.GetSize());
    BLUtils::FillAllZero(fftIds);
    
    int bufSize = phases.GetSize();
    FLOAT_TYPE *phasesData = phases.Get();
    int fftIdsSize = fftIds->GetSize();
    int *fftIdsData = fftIds->Get();
    
    FLOAT_TYPE prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];
        
        FLOAT_TYPE phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        FLOAT_TYPE samplePos = ((FLOAT_TYPE)bufSize)*phaseDiff/(2.0*M_PI);
        
        int samplePosI = (int)samplePos;
        
        if ((samplePosI > 0) && (samplePosI < fftIdsSize))
            fftIdsData[samplePosI] = i;
    }
}
template void BLUtilsFft::SamplesIdsToFftIds(const WDL_TypedBuf<float> &phases,
                                             WDL_TypedBuf<int> *fftIds);
template void BLUtilsFft::SamplesIdsToFftIds(const WDL_TypedBuf<double> &phases,
                                             WDL_TypedBuf<int> *fftIds);
