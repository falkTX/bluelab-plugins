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
//  SourceLocalisationSystem2D.cpp
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#include <DelayLinePhaseShift.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <PPMFile.h>

#include "SourceLocalisationSystem2D.h"

// Test validated !
#define DEBUG_DELAY_LINE 0 //1

// Process only before 6KHz
#define CUTOFF_FREQ 6000.0

SourceLocalisationSystem2D::SourceLocalisationSystem2D(int bufferSize,
                                                       BL_FLOAT sampleRate,
                                                       int numBands)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mNumBands = numBands;
    
    Init();
}

SourceLocalisationSystem2D::~SourceLocalisationSystem2D()
{
    DeleteDelayLines();
}

void
SourceLocalisationSystem2D::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    Init();
}

void
SourceLocalisationSystem2D::AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                                       const WDL_TypedBuf<WDL_FFT_COMPLEX> samplesY[2])
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
    vector<vector<WDL_TypedBuf<BL_FLOAT> > > coincidence;
    ComputeCoincidences(samples0, samplesY, &coincidence);
    
    //DBG_DumpCoincidence("co.ppm", coincidence, 10000.0);
    //DBG_DumpCoincidenceLine("co.txt", 20, coincidence);
    
    FindMinima(&coincidence);
    
    TimeIntegrate(&coincidence);
    
    Threshold(&coincidence);
    
    FreqIntegrate(coincidence, &mCurrentLocalization);
}

void
SourceLocalisationSystem2D::GetLocalization(vector<WDL_TypedBuf<BL_FLOAT> > *localization)
{
    *localization = mCurrentLocalization;
}

void
SourceLocalisationSystem2D::Init()
{
#define INTER_MIC_DISTANCE 0.2 // 20cm
#define SOUND_SPEED 340.0
 
    DeleteDelayLines();
    
    mDelayLines[0].clear();
    mDelayLines[1].clear();
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    // Allocate
    mDelayLines[0].resize(mNumBands);
    mDelayLines[1].resize(mNumBands);
    for (int i = 0; i < mNumBands; i++)
    {
        mDelayLines[0][i].resize(mNumBands);
        mDelayLines[1][i].resize(mNumBands);
    
        for (int j = 0; j < mNumBands; j++)
        {
            mDelayLines[0][i][j].resize(numBins);
            mDelayLines[1][i][j].resize(numBins);
        }
    }
    
    //
    BL_FLOAT maxDelay = 0.5*INTER_MIC_DISTANCE/SOUND_SPEED;
    
    // Channels
    for (int k = 0; k < 2; k++)
    {
        // X bands
        for (int i = 0; i < mNumBands; i++)
        {
            BL_FLOAT delayX = maxDelay*sin((((BL_FLOAT)i)/(mNumBands - 1))*M_PI - M_PI/2.0);
            
            // Y bands
            for (int j = 0; j < mNumBands; j++)
            {
                BL_FLOAT delayY = maxDelay*sin((((BL_FLOAT)j)/(mNumBands - 1))*M_PI - M_PI/2.0);
                
                // Freqs
                for (int l = 0; l < numBins; l++)
                {
                    DelayLinePhaseShift *lineX = new DelayLinePhaseShift(mSampleRate, mBufferSize,
                                                                         l, maxDelay*2.0, delayX);
                    DelayLinePhaseShift *lineY = new DelayLinePhaseShift(mSampleRate, mBufferSize,
                                                                         l, maxDelay*2.0, delayY);
                    
                    DelayLinePhaseShiftXY line;
                    line.mDelayLineX = lineX;
                    line.mDelayLineY = lineY;
                    
                    mDelayLines[k][i][j][l] = line;
                }
            }
        }
    }
}

void
SourceLocalisationSystem2D::ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                                                const WDL_TypedBuf<WDL_FFT_COMPLEX> samplesY[2],
                                                vector<vector<WDL_TypedBuf<BL_FLOAT> > > *coincidence)
{
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    if (samples[0].GetSize() != numBins)
        return;
    if (samples[1].GetSize() != numBins)
        return;
    
    // Allocate delay result
    vector<vector<vector<WDL_FFT_COMPLEX> > > delayedSamples[2];
    for (int k = 0; k < 2; k++)
    {
        delayedSamples[k].resize(mNumBands);
        
        for (int i = 0; i < mNumBands; i++)
        {
            delayedSamples[k][i].resize(mNumBands);
            
            for (int j = 0; j < mNumBands; j++)
            {
                delayedSamples[k][i][j].resize(numBins);
            }
        }
    }
    
    vector<vector<vector<WDL_FFT_COMPLEX> > > delayedSamplesX[2] = { delayedSamples[0], delayedSamples[1] };
    vector<vector<vector<WDL_FFT_COMPLEX> > > delayedSamplesY[2] = { delayedSamples[0], delayedSamples[1] };
    
    // Apply delay
    for (int k = 0; k < 2; k++)
    {
        // Bands
        for (int i = 0; i < mDelayLines[k].size(); i++)
        {
            // Bands
            for (int j = 0; j < mDelayLines[k][i].size(); j++)
            {
                // Buffer size
                for (int l = 0; l < mDelayLines[k][i][j].size(); l++)
                {
                    DelayLinePhaseShiftXY line = mDelayLines[k][i][j][l];
                
                    WDL_FFT_COMPLEX delX = line.mDelayLineX->ProcessSample(samples[k].Get()[i]);
                    WDL_FFT_COMPLEX delY = line.mDelayLineY->ProcessSample(samplesY[k].Get()[i]);
                
                    //WDL_FFT_COMPLEX res;
                    //COMP_ADD(delX, delY, res); // NOTE: is it correct ????
                    //delayedSamples[k][i][j][l] = res;
                    
                    delayedSamplesX[k][i][j][l] = delX;
                    delayedSamplesY[k][i][j][l] = delY;
                }
            }
        }
    }
    
    // Substract
    //
    coincidence->clear();
    coincidence->resize(mNumBands);
    
    for (int i = 0; i < mNumBands; i++)
    {
        for (int j = 0; j < mNumBands; j++)
        {
            // X
            WDL_TypedBuf<WDL_FFT_COMPLEX> diffSampsX;
            diffSampsX.Resize(mBufferSize/2);
        
            ComputeDiff(&diffSampsX, delayedSamplesX[0][i], delayedSamplesX[1][i], j);
        
            WDL_TypedBuf<BL_FLOAT> diffMagnsX;
            BLUtilsComp::ComplexToMagn(&diffMagnsX, diffSampsX);
        
            // Y
            WDL_TypedBuf<WDL_FFT_COMPLEX> diffSampsY;
            diffSampsY.Resize(mBufferSize/2);
            
            ComputeDiff(&diffSampsY, delayedSamplesY[0][i], delayedSamplesY[1][i], j);
            
            WDL_TypedBuf<BL_FLOAT> diffMagnsY;
            BLUtilsComp::ComplexToMagn(&diffMagnsY, diffSampsY);
            
            //
            WDL_TypedBuf<BL_FLOAT> diffMagns = diffMagnsX;
            BLUtils::AddValues(&diffMagns, diffMagns);
            
            (*coincidence)[i].push_back(diffMagns);
        }
    }
}

void
SourceLocalisationSystem2D::ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
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
SourceLocalisationSystem2D::FindMinima(vector<vector<WDL_TypedBuf<BL_FLOAT> > > *coincidence)
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
            for (int l = 0; l < mNumBands; l++)
            {
                BL_FLOAT co = (*coincidence)[j][l].Get()[i];
            
                if (co < minVal)
                {
                    minVal = co;
                    minIndex = j;
                }
            }
        }
        
        for (int j = 0; j < mNumBands; j++)
        {
            for (int l = 0; l < mNumBands; l++)
            {
                BL_FLOAT co = (*coincidence)[j][l].Get()[i];
                if (co <= minVal)
                    (*coincidence)[j][l].Get()[i] = 1.0;
                else
                    (*coincidence)[j][l].Get()[i] = 0.0;
            }
        }
    }
}

void
SourceLocalisationSystem2D::TimeIntegrate(vector<vector<WDL_TypedBuf<BL_FLOAT> > > *ioCoincidence)
{
#define TIME_SMOOTH_FACTOR 0.98
    
    if (mPrevCoincidence.size() != ioCoincidence->size())
        mPrevCoincidence = *ioCoincidence;
    
    for (int i = 0; i < mNumBands; i++)
        BLUtils::Smooth(&(*ioCoincidence)[i], &mPrevCoincidence[i], (BL_FLOAT)TIME_SMOOTH_FACTOR);
}

void
SourceLocalisationSystem2D::Threshold(vector<vector<WDL_TypedBuf<BL_FLOAT> > > *ioCoincidence)
{
    // Threshold is set greater than or equal to zero.
    // A greater value of threshold can remove phantom coincidences
#define THRESHOLD 1.0 //0.0 // 1.0
    
    for (int i = 0; i < ioCoincidence->size(); i++)
    {
        vector<WDL_TypedBuf<BL_FLOAT> > &coincidence = (*ioCoincidence)[i];
        
        for (int j = 0; j < coincidence.size(); j++)
        {
            for (int l = 0; l < coincidence[j].GetSize(); l++)
            {
                BL_FLOAT val = coincidence[j].Get()[l];
                if (val < THRESHOLD)
                    val = 0.0;
            
                coincidence[j].Get()[l] = val;
            }
        }
    }
}

void
SourceLocalisationSystem2D::FreqIntegrate(const vector<vector<WDL_TypedBuf<BL_FLOAT> > > &coincidence,
                                          vector<WDL_TypedBuf<BL_FLOAT> > *localization)
{
    localization->resize(coincidence.size());
    
    for (int i = 0; i < coincidence.size(); i++)
    {
        const vector<WDL_TypedBuf<BL_FLOAT> > &co = coincidence[i];
        (*localization)[i].Resize(co.size());
        
        for (int j = 0; j < co.size(); j++)
        {
            const WDL_TypedBuf<BL_FLOAT> &freqs = co[j];
        
            BL_FLOAT sumFreqs = BLUtils::ComputeSum(freqs);
        
            (*localization)[i].Get()[j] = sumFreqs;
        }
    }
}

void
SourceLocalisationSystem2D::DeleteDelayLines()
{
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < mDelayLines[k].size(); i++)
        {
            vector<vector<DelayLinePhaseShiftXY> > &bandLines = mDelayLines[k][i];
            
            for (int j = 0; j < bandLines.size(); j++)
            {
                vector<DelayLinePhaseShiftXY> &bandLine = bandLines[j];
                
                for (int j2 = 0; j2 < bandLine.size(); j2++)
                {
                    DelayLinePhaseShiftXY &line = bandLine[j2];
                    delete line.mDelayLineX;
                    delete line.mDelayLineY;
                }
            }
        }
    }
}

void
SourceLocalisationSystem2D::DBG_DumpCoincidence(const char *fileName,
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
SourceLocalisationSystem2D::DBG_DumpCoincidenceLine(const char *fileName,
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
