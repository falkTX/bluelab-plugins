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
 
#include <stdio.h>
#include <math.h>

#include <FftProcessObj16.h>
#include <PostTransientFftObj3.h>
#include <PitchShiftFftObj3.h>
#include <PhasesDiff.h>
#include <StereoPhasesProcess.h>

#include <BLUtilsPlug.h>

#include "PitchShifterBLSmb.h"

// With 1024, we miss some frequencies
#define BUFFER_SIZE 2048

// OVERSAMPLING 2 is not good ...
// ... and the author of real freqencies computation adviced 4
#define OVERSAMPLING_0 4
#define OVERSAMPLING_1 8
#define OVERSAMPLING_2 16
#define OVERSAMPLING_3 32

#define FREQ_RES 1 //4 seems to improve transients with 4

// Doesn't change anything, at least for small pitches
// Indeed, we only "shift" the mangnitude bins, we don't
// increase or decrease, add or remove.
//
// (but if set to 1, internally, the transient boost won't use it
// when boosting the transients)
#define KEEP_SYNTHESIS_ENERGY 0


// When set to 0, we are accurate for frequencies
#define VARIABLE_HANNING 0

// GOOD
// Avoid phasing effect when stereo processing
//
// BAD: Finally, when tested on Protools on real stereo file
// (Midning extract), it wobbles a little, the sound is less clear
// (compared with prev version 5.0.2: it has these defects too)
//
// That seem to only improved with fake stereo i.e a duplicated
// mono channel
#define ADJUST_STEREO_PHASES 0 //1


#define USE_VARIABLE_BUFFER_SIZE 1

// Fix bad sound in mono
// (due to try to fix phases between two stereo channels)
#define FIX_BAD_SOUND_MONO 1

//
PitchShifterBLSmb::PitchShifterBLSmb()
{
    mFftObj = NULL;
    mPhasesProcess = NULL;
    
    mPitchObjs[0] = NULL;
    mPitchObjs[1] = NULL;
    
    //
    mOversampling = OVERSAMPLING_0;
    
    mSampleRate = 44100.0;
    mFactor = 1.0;

    InitFft(mSampleRate);
}

PitchShifterBLSmb::~PitchShifterBLSmb()
{
    if (mFftObj != NULL)
        delete mFftObj;
    
    if (mPhasesProcess != NULL)
        delete mPhasesProcess;
    
    for (int i = 0; i < 2; i++)
    {
        if (mPitchObjs[i] != NULL)
            delete mPitchObjs[i];
    }
}

void
PitchShifterBLSmb::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;

    // Sample rate has changed, and we can have variable buffer size
    InitFft(mSampleRate);

    //
    int bufferSize = BUFFER_SIZE;
  
#if USE_VARIABLE_BUFFER_SIZE
    bufferSize = BLUtilsPlug::PlugComputeBufferSize(BUFFER_SIZE, sampleRate);
#endif

    // Called when we restart the playback
    // The cursor position may have changed
    // Then we must reset

    if (mFftObj != NULL)
        mFftObj->Reset(bufferSize, mOversampling, FREQ_RES, sampleRate);
  
    for (int i = 0; i < 2; i++)
    {
        if (mPitchObjs[i] != NULL)
            mPitchObjs[i]->Reset(bufferSize, mOversampling, FREQ_RES, sampleRate);
    }
  
    if (mPhasesProcess != NULL)
        mPhasesProcess->Reset(bufferSize, mOversampling, FREQ_RES, sampleRate);
}

void
PitchShifterBLSmb::Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                           vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    if (in.empty())
        return;

#if FIX_BAD_SOUND_MONO
    // FIX: bad sound in mono
    if (in.size() < 2)
    {
        // Mono, disable to avoid bad sound
        if (mPhasesProcess != NULL)
            mPhasesProcess->SetActive(false);
    }
    else
    {
        // Stero: enable to fix phase diff problems
        if (mPhasesProcess != NULL)
            mPhasesProcess->SetActive(true);
    }
#endif

    vector<WDL_TypedBuf<BL_FLOAT> > dummyScIn;
    mFftObj->Process(in, dummyScIn, out);
}
    
void
PitchShifterBLSmb::SetFactor(BL_FLOAT factor)
{
    mFactor = factor;

    for (int i = 0; i < 2; i++)
        mPitchObjs[i]->SetFactor(mFactor);
}

void
PitchShifterBLSmb::SetQuality(int quality)
{
    switch(quality)
    {
        case 0:
            mOversampling = 4;
            break;
            
        case 1:
            mOversampling = 8;
            break;
            
        case 2:
            mOversampling = 16;
            break;
            
        case 3:
            mOversampling = 32;
            break;
            
        default:
            break;
    }

    InitFft(mSampleRate);
}

void
PitchShifterBLSmb::SetTransBoost(BL_FLOAT transBoost)
{
#if !DEBUG_PITCH_OBJ
    mFftObj->SetTransBoost(transBoost);
#endif
}

int
PitchShifterBLSmb::ComputeLatency(int blockSize)
{
    int latency = mFftObj->ComputeLatency(blockSize);

    return latency;
}

void
PitchShifterBLSmb::InitFft(BL_FLOAT sampleRate)
{
    int bufferSize = BUFFER_SIZE;
  
#if USE_VARIABLE_BUFFER_SIZE
    bufferSize = BLUtilsPlug::PlugComputeBufferSize(BUFFER_SIZE, sampleRate);
  
    // OLD (failed)
    //BLUtilsPlug::PlugUpdateLatency(this, BUFFER_SIZE, PLUG_LATENCY, sampleRate);
#endif
  
    if (mFftObj == NULL)
    {      
        // Must use a second array
        // because the heritage doesn't convert autmatically from
        // WDL_TypedBuf<PostTransientFftObj2 *> to WDL_TypedBuf<ProcessObj *>
        // (tested with std vector too)
        vector<ProcessObj *> processObjs;
        for (int i = 0; i < 2; i++)
        {
            mPitchObjs[i] = new PitchShiftFftObj3(bufferSize, mOversampling,
                                                  FREQ_RES, mSampleRate);
      
            processObjs.push_back(mPitchObjs[i]);
        }
      
        int numChannels = 2;
        int numScInputs = 0;
      
#if DEBUG_PITCH_OBJ
        mFftObj = new FftProcessObj16(processObjs,
                                      numChannels, numScInputs,
                                      BUFFER_SIZE, mOversampling, FREQ_RES,
                                      mSampleRate);
#else
        mFftObj = new PostTransientFftObj3(processObjs,
                                           numChannels, numScInputs,
                                           bufferSize, mOversampling, FREQ_RES,
                                           mSampleRate);
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
    
        // Moreover, this seems to avoids phase drift !
#if ADJUST_STEREO_PHASES
        mPhasesProcess = new StereoPhasesProcess(bufferSize);
        mFftObj->AddMultichannelProcess(mPhasesProcess);
#endif
    }
    else
    {
        mFftObj->Reset(bufferSize, mOversampling, FREQ_RES, sampleRate);
    }
}
