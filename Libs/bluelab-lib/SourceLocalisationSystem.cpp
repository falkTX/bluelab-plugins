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
//  SourceLocalisationSystem.cpp
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#include <DualDelayLine.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <PPMFile.h>

#include "SourceLocalisationSystem.h"

#define HACK_DELAY_FACTOR 1 //0

SourceLocalisationSystem::SourceLocalisationSystem(int bufferSize,
                                                   BL_FLOAT sampleRate,
                                                   int numBands)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mNumBands = numBands;
    
    Init();
}

SourceLocalisationSystem::~SourceLocalisationSystem()
{
    DeleteDelayLines();
}

void
SourceLocalisationSystem::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    Init();
}

void
SourceLocalisationSystem::AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2])
{
    // Compute coincidence
    vector<WDL_TypedBuf<BL_FLOAT> > coincidence;
    ComputeCoincidences(samples, &coincidence);
    
    //DBG_DumpCoincidence("co.ppm", coincidence, 10000.0);
    
    //DBG_DumpCoincidenceLine("co.txt", 20, coincidence);
    
    //FindMinima(&coincidence);
    
    //TimeIntegrate(&coincidence);
    
    //Threshold(&coincidence);
    
    //DBG_DumpCoincidence("co2.ppm", coincidence, 10000.0);
    
    FreqIntegrate(coincidence, &mCurrentLocalization);
}

void
SourceLocalisationSystem::GetLocalization(WDL_TypedBuf<BL_FLOAT> *localization)
{
    *localization = mCurrentLocalization;
}

void
SourceLocalisationSystem::Init()
{
#define INTER_MIC_DISTANCE 0.2 // 20cm
#define SOUND_SPEED 340.0
 
    DeleteDelayLines();
    
    mDelayLines[0].clear();
    mDelayLines[1].clear();
    
    BL_FLOAT maxDelay = 0.5*INTER_MIC_DISTANCE/SOUND_SPEED;
    
#if HACK_DELAY_FACTOR
    //BL_FLOAT delayFactor = mBufferSize/4.0;
    BL_FLOAT delayFactor = 100.0;
    //BL_FLOAT delayFactor = 10.0;
    maxDelay *= delayFactor;
#endif
    
    //WDL_TypedBuf<BL_FLOAT> dbgDelays;
    //dbgDelays.Resize(mNumBands);
    
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < mBufferSize/2; i++)
        {
            vector<DualDelayLine *> bandLines;
            for (int j = 0; j < mNumBands; j++)
            {
                //BL_FLOAT delay = maxDelay*sin((((BL_FLOAT)(j - 1))/(mNumBands - 1))*M_PI - M_PI/2.0);
                BL_FLOAT delay = maxDelay*sin((((BL_FLOAT)j)/(mNumBands - 1))*M_PI - M_PI/2.0);
                
                if (k == 1)
                {
                    //delay = maxDelay - delay;
                    
                    delay = -delay;
                }
                
                DualDelayLine *line = new DualDelayLine(mSampleRate, maxDelay*2.0, delay);
        
                bandLines.push_back(line);
                
                //if ((k == 0) && (i == 0))
                //    dbgDelays.Get()[j] = delay + maxDelay;
            }
        
            mDelayLines[k].push_back(bandLines);
        }
    }
    
    //BLDebug::DumpData("delays.txt", dbgDelays);
}

void
SourceLocalisationSystem::ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                                              vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
    if (samples[0].GetSize() != mBufferSize/2)
        return;
    if (samples[1].GetSize() != mBufferSize/2)
        return;
    
    // Allocate delay result
    vector<vector<WDL_FFT_COMPLEX> > delayedSamples[2];
    for (int k = 0; k < 2; k++)
    {
        vector<WDL_FFT_COMPLEX> samps;
        samps.resize(mNumBands);
        
        for (int i = 0; i < mBufferSize/2; i++)
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
                DualDelayLine *line = mDelayLines[k][i][j];
                
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
SourceLocalisationSystem::ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
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
SourceLocalisationSystem::FindMinima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
#define INF 1e15;
    
    for (int i = 0; i < mBufferSize/2; i++)
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
            if (j != minIndex)
                (*coincidence)[j].Get()[i] = 0.0;
            else
                (*coincidence)[j].Get()[i] = 1.0;
        }
    }
}

void
SourceLocalisationSystem::FindMaxima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
#define INF 1e15;
    
    for (int i = 0; i < mBufferSize/2; i++)
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

void
SourceLocalisationSystem::TimeIntegrate(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence)
{
#define TIME_SMOOTH_FACTOR 0.9
    
    if (mPrevCoincidence.size() != ioCoincidence->size())
        mPrevCoincidence = *ioCoincidence;
    
    BLUtils::Smooth(ioCoincidence, &mPrevCoincidence, (BL_FLOAT)TIME_SMOOTH_FACTOR);
}

void
SourceLocalisationSystem::Threshold(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence)
{
    // Threshold is set greater than or equal to zero.
    // A greater value of threshold can remove phantom coincidences
#define THRESHOLD 0.0 // 1.0
    
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
SourceLocalisationSystem::FreqIntegrate(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
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
SourceLocalisationSystem::DeleteDelayLines()
{
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < mDelayLines[k].size(); i++)
        {
            vector<DualDelayLine *> &bandLines = mDelayLines[k][i];
            
            for (int j = 0; j < bandLines.size(); j++)
            {
                DualDelayLine *line = bandLines[j];
                delete line;
            }
        }
    }
}

void
SourceLocalisationSystem::DBG_DumpCoincidence(const char *fileName,
                                              const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                              BL_FLOAT colorCoeff)
{
    int height = coincidence.size();
    if (height == 0)
        return;
    
    int width = coincidence[0].GetSize();
    
    BL_FLOAT *image = new BL_FLOAT[width*height];
    for (int i = 0; i < height; i++)
    {
        memcpy(&image[i*width], coincidence[i].Get(), width*sizeof(BL_FLOAT));
    }
    
    PPMFile::SavePPM(fileName, image, width, height, 1, 256.0*colorCoeff);
}

void
SourceLocalisationSystem::DBG_DumpCoincidenceLine(const char *fileName,
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
