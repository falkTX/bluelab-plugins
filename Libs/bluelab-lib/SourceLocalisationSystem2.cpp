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
//  SourceLocalisationSystem2.cpp
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

// For iPlug2, commented lice code
#define DISABLE_LICE 1
#if !DISABLE_LICE
#include <lice.h>
#endif

#include <DelayLinePhaseShift.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <PPMFile.h>

#include "SourceLocalisationSystem2.h"

// Test validated !
#define DEBUG_DELAY_LINE 0 //1

// Process only before 6KHz
#define CUTOFF_FREQ 6000.0

#define INTER_MIC_DISTANCE 0.2 // 20cm
#define SOUND_SPEED 340.0

#define USE_STENCIL 0 //1

// Original algo
#define ALGO_LIU 0

// Far better:
// - avoids a constant fake source at the center
// - more accurate
//
#define ALGO_NIKO 1


SourceLocalisationSystem2::SourceLocalisationSystem2(int bufferSize,
                                                   BL_FLOAT sampleRate,
                                                   int numBands)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mNumBands = numBands;
    
    Init();
}

SourceLocalisationSystem2::~SourceLocalisationSystem2()
{
    DeleteDelayLines();
}

void
SourceLocalisationSystem2::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    Init();
}

void
SourceLocalisationSystem2::AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2])
{
#if DEBUG_DELAY_LINE
    DelayLinePhaseShift::DBG_Test(mBufferSize, mSampleRate, samples[0]);
#endif
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> samples0[2] = { samples[0], samples[1] };
    samples0[0].Resize(numBins);
    samples0[1].Resize(numBins);
    
    // Compute coincidence
    vector<WDL_TypedBuf<BL_FLOAT> > coincidence;
    ComputeCoincidences(samples0, &coincidence);
    
    //DBG_DumpCoincidence("co.ppm", coincidence, 10000.0);
    //DBG_DumpCoincidenceLine("co.txt", 20, coincidence);
    //DBG_DumpCoincidence("coincidence", coincidence, 10000.0);
    
#if 0
    // DEBUG
    WDL_TypedBuf<BL_FLOAT> magns0;
    BLUtils::ComplexToMagn(&magns0, samples[0]);
    BLDebug::DumpData("magns0.txt", magns0);
    WDL_TypedBuf<BL_FLOAT> magns1;
    BLUtils::ComplexToMagn(&magns1, samples[1]);
    BLDebug::DumpData("magns1.txt", magns1);
#endif
    
#if ALGO_LIU
    // NOTE: with FindMinima() + Threshold(), there is always
    // a detected source at the middle (azimuth 0)
    
    FindMinima(&coincidence);
    
    TimeIntegrate(&coincidence);
    
    Threshold(&coincidence);
    
#if !USE_STENCIL
    FreqIntegrate(coincidence, &mCurrentLocalization);
#else
    FreqIntegrateStencil(coincidence, &mCurrentLocalization);
#endif
#endif
    
#if ALGO_NIKO
    TimeIntegrate(&coincidence);
    
    //FindMinima(&coincidence);
    
    //Threshold(&coincidence);
    
#if !USE_STENCIL
    FreqIntegrate(coincidence, &mCurrentLocalization);
#else
    FreqIntegrateStencil(coincidence, &mCurrentLocalization);
#endif
    
    //FindMinima2(&mCurrentLocalization);
    FindMinima3(&mCurrentLocalization);
#endif
    
    // DEBUG
#if 0
    WDL_TypedBuf<BL_FLOAT> testLocalization;
    FreqIntegrate(coincidence, &testLocalization);
    
    BLDebug::DumpData("test.txt", testLocalization);
    BLDebug::DumpData("stencil.txt", mCurrentLocalization);
#endif

    
    // TEST
    //InvertValues(&mCurrentLocalization);
}

void
SourceLocalisationSystem2::GetLocalization(WDL_TypedBuf<BL_FLOAT> *localization)
{
    *localization = mCurrentLocalization;
}

void
SourceLocalisationSystem2::Init()
{ 
    DeleteDelayLines();
    
    mDelayLines[0].clear();
    mDelayLines[1].clear();
    
    BL_FLOAT maxDelay = 0.5*INTER_MIC_DISTANCE/SOUND_SPEED;
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < numBins/*mBufferSize/2*/; i++)
        {
            vector<DelayLinePhaseShift *> bandLines;
            for (int j = 0; j < mNumBands; j++)
            {
                //BL_FLOAT delay = maxDelay*sin((((BL_FLOAT)(j - 1))/(mNumBands - 1))*M_PI - M_PI/2.0);
                BL_FLOAT delay = maxDelay*sin((((BL_FLOAT)j)/(mNumBands - 1))*M_PI - M_PI/2.0);
                
                if (k == 1)
                {
                    delay = -delay;
                }
                
                DelayLinePhaseShift *line = new DelayLinePhaseShift(mSampleRate, mBufferSize,
                                                                    i, maxDelay*2.0, delay);
        
                bandLines.push_back(line);
            }
        
            mDelayLines[k].push_back(bandLines);
        }
    }
}

void
SourceLocalisationSystem2::ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                                              vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    if (samples[0].GetSize() != numBins/*mBufferSize/2*/)
        return;
    if (samples[1].GetSize() != numBins/*mBufferSize/2*/)
        return;
    
    // Allocate delay result
    vector<vector<WDL_FFT_COMPLEX> > delayedSamples[2];
    for (int k = 0; k < 2; k++)
    {
        vector<WDL_FFT_COMPLEX> samps;
        samps.resize(mNumBands);
        
        for (int i = 0; i < numBins/*mBufferSize/2*/; i++)
        {
            delayedSamples[k].push_back(samps);
        }
    }
    
    // Apply delay
    for (int k = 0; k < 2; k++)
    {
        // Bands
        for (int i = 0; i < mDelayLines[k].size(); i++)
        {
            // Buffer size
            for (int j = 0; j < mDelayLines[k][i].size(); j++)
            {
                DelayLinePhaseShift *line = mDelayLines[k][i][j];
                
                WDL_FFT_COMPLEX del = line->ProcessSample(samples[k].Get()[i]);
                
                delayedSamples[k][i][j] = del;
            }
        }
    }
    
    // Substract
    //
    coincidence->resize(0);
    
    for (int j = 0; j < mNumBands; j++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> diffSamps;
        diffSamps.Resize(mBufferSize/2);
        
        ComputeDiff(&diffSamps, delayedSamples[0], delayedSamples[1], j);
        
        WDL_TypedBuf<BL_FLOAT> diffMagn;
        BLUtilsComp::ComplexToMagn(&diffMagn, diffSamps);
        
        coincidence->push_back(diffMagn);
    }
}

void
SourceLocalisationSystem2::ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
                                      const vector<vector<WDL_FFT_COMPLEX> > &buf0,
                                      const vector<vector<WDL_FFT_COMPLEX> > &buf1,
                                      int index)
{
    if (buf0.empty())
        return;
    
    if (buf0.size() != buf1.size())
        return;
    
    resultDiff->Resize(buf0.size());
    
    for (int i = 0; i < buf0.size(); i++)
    {
        WDL_FFT_COMPLEX val0 = buf0[i][index];
        WDL_FFT_COMPLEX val1 = buf1[i][index];
            
        WDL_FFT_COMPLEX d;
        
#if 1 // ORIG
        d.re = val0.re - val1.re;
        d.im = val0.im - val1.im;
#endif
        
#if 0 // TEST
        BL_FLOAT magn0 = COMP_MAGN(val0);
        BL_FLOAT magn1 = COMP_MAGN(val1);
        
        BL_FLOAT diff = std::fabs(magn0 - magn1);
        
        d.re = diff;
        d.im = 0.0;
#endif
        
        resultDiff->Get()[i] = d;
    }
}

void
SourceLocalisationSystem2::FindMinima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
#define INF 1e15;
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    for (int i = 0; i < numBins/*mBufferSize/2*/; i++)
    {
        int minIndex = 0;
        BL_FLOAT minVal = INF;
        
        for (int j = 0; j < mNumBands; j++)
        {
            BL_FLOAT co = (*coincidence)[j].Get()[i];
            
            if (co < minVal)
            {
                minVal = co;
                minIndex = j;
            }
        }
        
        for (int j = 0; j < mNumBands; j++)
        {
            BL_FLOAT co = (*coincidence)[j].Get()[i];
            if (co <= minVal)
                (*coincidence)[j].Get()[i] = 1.0;
            else
                (*coincidence)[j].Get()[i] = 0.0;
            
#if 0
            if (j != minIndex)
                (*coincidence)[j].Get()[i] = 0.0;
            else
                (*coincidence)[j].Get()[i] = 1.0;
#endif
        }
    }
}

// Find only 1 minimum
void
SourceLocalisationSystem2::FindMinima2(WDL_TypedBuf<BL_FLOAT> *coincidence)
{
#define INF 1e15;
    
    BL_FLOAT minVal = INF;
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT co = coincidence->Get()[i];
            
        if (co < minVal)
        {
            minVal = co;
        }
    }
        
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT co = coincidence->Get()[i];
        if (co <= minVal)
            coincidence->Get()[i] = 1.0;
        else
            coincidence->Get()[i] = 0.0;
        
    }
}

// Find several minima
void
SourceLocalisationSystem2::FindMinima3(WDL_TypedBuf<BL_FLOAT> *coincidence)
{
#define THRS 1e10
    
    WDL_TypedBuf<BL_FLOAT> minima;
    BLUtils::FindMinima(*coincidence, &minima, (BL_FLOAT)(THRS*2.0));
    
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT mini = minima.Get()[i];
        
        if (mini < THRS)
            coincidence->Get()[i] = 1.0;
        else
            coincidence->Get()[i] = 0.0;
        
    }
}

#if 0
void
SourceLocalisationSystem2::FindMaxima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
#define INF 1e15;
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    for (int i = 0; i < numBins/*mBufferSize/2*/; i++)
    {
        int maxIndex = 0;
        BL_FLOAT maxVal = -INF;
        
        for (int j = 0; j < mNumBands; j++)
        {
            BL_FLOAT co = (*coincidence)[j].Get()[i];
            
            if (co > maxVal)
            {
                maxVal = co;
                maxIndex = j;
            }
        }
        
        for (int j = 0; j < mNumBands; j++)
        {
            if (j != maxIndex)
                (*coincidence)[j].Get()[i] = 0.0;
            else
                (*coincidence)[j].Get()[i] = 1.0;
        }
    }
}
#endif

void
SourceLocalisationSystem2::TimeIntegrate(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence)
{
#define TIME_SMOOTH_FACTOR 0.98
    
    if (mPrevCoincidence.size() != ioCoincidence->size())
        mPrevCoincidence = *ioCoincidence;
    
    BLUtils::Smooth(ioCoincidence, &mPrevCoincidence, (BL_FLOAT)TIME_SMOOTH_FACTOR);
}

void
SourceLocalisationSystem2::Threshold(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence)
{
    // Threshold is set greater than or equal to zero.
    // A greater value of threshold can remove phantom coincidences
#define THRESHOLD 1.0 //0.0 // 1.0
    
    for (int i = 0; i < ioCoincidence->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &coincidence = (*ioCoincidence)[i];
        
        for (int j = 0; j < coincidence.GetSize(); j++)
        {
            BL_FLOAT val = coincidence.Get()[j];
            if (val < THRESHOLD)
                val = 0.0;
            
            coincidence.Get()[j] = val;
        }
    }
}

void
SourceLocalisationSystem2::FreqIntegrate(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                        WDL_TypedBuf<BL_FLOAT> *localization)
{
    localization->Resize(coincidence.size());
    
    for (int i = 0; i < coincidence.size(); i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &freqs = coincidence[i];
        
        BL_FLOAT sumFreqs = BLUtils::ComputeSum(freqs);
        
        localization->Get()[i] = sumFreqs;
    }
}

void
SourceLocalisationSystem2::FreqIntegrateStencil(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                                WDL_TypedBuf<BL_FLOAT> *localization)
{
#define DEBUG_MASK 0 //1
    
    localization->Resize((int)coincidence.size());
    
    BL_FLOAT ITDmax = 0.5*INTER_MIC_DISTANCE/SOUND_SPEED;
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
#if 0
    // Num points used in the summation for each azimuth
    vector<int> numPoints;
    numPoints.resize(coincidence.size());
    for (int i = 0; i < numPoints.size(); i++)
        numPoints[i] = 0;
#endif
    
    for (int i = 0; i < coincidence.size(); i++)
    {
#if DEBUG_MASK
        WDL_TypedBuf<BL_FLOAT> mask;
        mask.Resize(coincidence[0].GetSize()*coincidence.size());
        BLUtils::FillAllZero(&mask);
#endif
        
        // Num points used in the summation for each azimuth
        int numPoints = 0;
        BL_FLOAT sum = 0.0;
        
        BL_FLOAT azimuth = (((BL_FLOAT)i)/(coincidence.size() - 1))*M_PI - M_PI/2.0;
        
        const WDL_TypedBuf<BL_FLOAT> &freqs = coincidence[i];
        
        for (int j = 1/*0*/; j < freqs.GetSize(); j++)
        {
            BL_FLOAT fm = j*hzPerBin;
            
            BL_FLOAT gammaMinf = -ITDmax*fm*(1.0 + std::sin(azimuth));
            BL_FLOAT gammaMaxf = ITDmax*fm*(1.0 - std::sin(azimuth));
            
            int gammaMin = std::ceil(gammaMinf);
            int gammaMax = std::floor(gammaMaxf);
            
            for (int k = gammaMin; k <= gammaMax; k++)
            {
	      BL_FLOAT x0 = std::sin(azimuth) + ((BL_FLOAT)k)/(ITDmax*fm);
                
	      if (std::fabs(x0) > 1.0)
                    continue;
                
	      BL_FLOAT xf = std::asin(x0);
                
                // 
                xf = (xf/M_PI) + 0.5;
                
                //
                xf = xf*(coincidence.size() - 1);
                
                int x = bl_round(xf);
                
#if DEBUG_MASK
                // Choose an azimuth
                //if (i == coincidence.size()/2)
                {
                    int y = freqs.GetSize() - j - 1;
                
                    mask.Get()[x + y*coincidence.size()] = 1.0;
                }
#endif
                
                BL_FLOAT val = coincidence[x].Get()[j];
                
                sum += val;
                numPoints++;
            }
        }
        
        if (numPoints > 0)
            sum /= numPoints;
        
        // Re-normalized, to be consistent with the version
        // of FreqIntegrate() without stencil
        sum *= coincidence.size();
        // Hack
        sum *= 6.0;
        
        localization->Get()[i] = sum;
        
#if DEBUG_MASK
        char fileName[255];
        sprintf(fileName, "mask-%d.ppm", i);
        
        PPMFile::SavePPM(fileName, mask.Get(),
                         coincidence.size(),
                         coincidence[0].GetSize(),
                         1, 256.0);
#endif
    }
}

void
SourceLocalisationSystem2::InvertValues(WDL_TypedBuf<BL_FLOAT> *localization)
{
    //BL_FLOAT maxVal = 1.0;
    BL_FLOAT maxVal = BLUtils::ComputeMax(*localization);
    
    for (int i = 0; i < localization->GetSize(); i++)
    {
        BL_FLOAT val = localization->Get()[i];
        
        val = maxVal - val;
        if (val < 0.0)
            val = 0.0;
        
        localization->Get()[i] = val;
    }
}
    
void
SourceLocalisationSystem2::DeleteDelayLines()
{
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < mDelayLines[k].size(); i++)
        {
            vector<DelayLinePhaseShift *> &bandLines = mDelayLines[k][i];
            
            for (int j = 0; j < bandLines.size(); j++)
            {
                DelayLinePhaseShift *line = bandLines[j];
                delete line;
            }
        }
    }
}

void
SourceLocalisationSystem2::DBG_DumpCoincidence(const char *fileName,
                                              const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                              BL_FLOAT colorCoeff)
{
    int width = coincidence.size();
    if (width == 0)
        return;
    
    int height = coincidence[0].GetSize();
    
#define SUMMATION_BAND 1
#define SUMMATION_BAND_SIZE 20
    
#if SUMMATION_BAND
    WDL_TypedBuf<BL_FLOAT> summationBand;
    BLUtils::ResizeFillZeros(&summationBand, width);
    
    height = height + SUMMATION_BAND_SIZE;
#endif
    
    BL_FLOAT *image = new BL_FLOAT[width*height];
    for (int j = 0; j < height; j++)
    {
#if SUMMATION_BAND
        if (j >= coincidence[0].GetSize())
            continue;
#endif
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT val = coincidence[i].Get()[j];
        
            image[i + (height - j - 1)*width] = val;
            
#if SUMMATION_BAND
            summationBand.Get()[i] += val;
#endif
        }
    }
    
#if SUMMATION_BAND
    BLUtils::MultValues(&summationBand, (BL_FLOAT)0.005);
    
    for (int j = height - SUMMATION_BAND_SIZE; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT val = summationBand.Get()[i];
            
            image[i + (height - j - 1)*width] = val;
        }
    }
#endif
    
#if 0
    char fileNamePPM[256];
    sprintf(fileNamePPM, "%s.ppm", fileName);
    PPMFile::SavePPM(fileNamePPM, image, width, height, 1, 256.0*colorCoeff);
#else

#if !DISABLE_LICE
    // #bl-iplug2
    //LICE_IBitmap *bmp = new LICE_MemBitmap(width, height, 1);
    LICE_IBitmap *bmp = NULL;
    
    LICE_pixel *pixels = bmp->getBits();
    for (int i = 0; i < width*height; i++)
    {
        BL_FLOAT val = image[i];
        int pix = val*255*colorCoeff;
        if (pix > 255)
            pix = 255;
        
        pixels[i] = LICE_RGBA(pix, pix, pix, 255);
    }
#endif
    
    char fileNamePNG[256];
    sprintf(fileNamePNG, "%s.png", fileName);
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", fileNamePNG);
    
    // #bl-iplug2
#if 0
    LICE_WritePNG(fullFilename, bmp, true);
#endif

#if !DISABLE_LICE
    delete bmp;
#endif
    
#endif
}

void
SourceLocalisationSystem2::DBG_DumpCoincidenceLine(const char *fileName,
                                                  int index,
                                                  const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence)
{
    int height = coincidence.size();
    if (height == 0)
        return;
    
    WDL_TypedBuf<BL_FLOAT> data;
    data.Resize(height);
    for (int i = 0; i < height; i++)
    {
        data.Get()[i] = coincidence[i].Get()[index];
    }
    
    BLDebug::DumpData(fileName, data);
}
