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
//  SamplesToSpectrogram.cpp
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <SimpleSpectrogramFftObj.h>
#include <FftProcessObj16.h>

#include <BLUtils.h>

#include "SamplesToSpectrogram.h"

// With 1024, we miss some frequencies
#define BUFFER_SIZE 2048

// 2: Smoother (which is better than 4 => more distinct lines)
// 2: CPU: 41%
// 4: CPU: 47%
#define OVERSAMPLING 2 //4 //1 //32 //1 //4

#define FREQ_RES 1 //4 seems to improve transients with 4

#define VARIABLE_HANNING 1
#define KEEP_SYNTHESIS_ENERGY 0

#define OPTIM_SKIP_IFFT 1


SamplesToSpectrogram::SamplesToSpectrogram(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    FftProcessObj16::Init();
    
    // Play objs
    vector<ProcessObj *> processObjs;
    mSpectrogramFftObj = new SimpleSpectrogramFftObj(BUFFER_SIZE,
                                                     OVERSAMPLING,
                                                     FREQ_RES,
                                                     sampleRate);
    processObjs.push_back(mSpectrogramFftObj);
    
    int numChannels = 1;
    int numScInputs = 0;
    
    mFftObj = new FftProcessObj16(processObjs,
                                  numChannels, numScInputs,
                                  BUFFER_SIZE, OVERSAMPLING, FREQ_RES,
                                  sampleRate);
    
#if OPTIM_SKIP_IFFT
    mFftObj->SetSkipIFft(0, true);
    mFftObj->SetSkipIFft(1, true);
#endif
    
#if !VARIABLE_HANNING
    mFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                               FftProcessObj16::WindowHanning);
    mFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                FftProcessObj16::WindowHanning);
#else
    mFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                               FftProcessObj16::WindowVariableHanning);
    mFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                FftProcessObj16::WindowVariableHanning);
#endif
    
    mFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS,
                                    KEEP_SYNTHESIS_ENERGY);
    
    mSpectrogram = mSpectrogramFftObj->GetSpectrogram();
    
    mSpectrogram->SetDisplayPhasesX(false);
    mSpectrogram->SetDisplayPhasesY(false);
    mSpectrogram->SetDisplayMagns(true);
    //mSpectrogram->SetYLogScale(false); //, 1.0);
    mSpectrogram->SetYScale(Scale::LINEAR);
    mSpectrogram->SetDisplayDPhases(false);
}

SamplesToSpectrogram::~SamplesToSpectrogram()
{
    delete mFftObj;
}

void
SamplesToSpectrogram::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mFftObj->Reset(BUFFER_SIZE, OVERSAMPLING, FREQ_RES, sampleRate);
}

int
SamplesToSpectrogram::GetBufferSize()
{
    return BUFFER_SIZE;
}

void
SamplesToSpectrogram::SetSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mFftObj->Reset();
    mSpectrogram->Reset(mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> remainigSamples = samples;
    
    while(remainigSamples.GetSize() > 0)
    {
        WDL_TypedBuf<BL_FLOAT> buf;
        if (remainigSamples.GetSize() >= BUFFER_SIZE)
        {
            buf.Add(remainigSamples.Get(), BUFFER_SIZE);
            BLUtils::ConsumeLeft(&remainigSamples, BUFFER_SIZE);
        }
        else
        {
            int numToAdd = remainigSamples.GetSize();
            int numZeros = BUFFER_SIZE - numToAdd;
            
            buf.Add(remainigSamples.Get(), numToAdd);
            BLUtils::ConsumeLeft(&remainigSamples, numToAdd);
            
            WDL_TypedBuf<BL_FLOAT> zeros;
            BLUtils::ResizeFillZeros(&zeros, numZeros);
            
            buf.Add(zeros.Get(), zeros.GetSize());            
        }
        
        vector<WDL_TypedBuf<BL_FLOAT> > in;
        in.push_back(buf);
        
        vector<WDL_TypedBuf<BL_FLOAT> > scIn;
        
        // Dummy
        vector<WDL_TypedBuf<BL_FLOAT> > out = in;
        
        //
        mFftObj->Process(in, scIn, &out);
        
        if (remainigSamples.GetSize() == 0)
        {
            // Flush the remainig data
            
            WDL_TypedBuf<BL_FLOAT> zeros;
            BLUtils::ResizeFillZeros(&zeros, BUFFER_SIZE);
            
            in[0] = zeros;
            
            for (int i = 0; i < OVERSAMPLING; i++)
            {
                mFftObj->Process(in, scIn, &out);
            }
            
            break;
        }
    }
}

BLSpectrogram4 *
SamplesToSpectrogram::GetSpectrogram()
{
    return mSpectrogram;
}

#endif
