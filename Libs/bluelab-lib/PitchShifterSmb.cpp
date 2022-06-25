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

#include <OversampProcessObj5.h>

// TMP, later would need to refact the code a lot
//#include <smbPitchShift.h>

#include <BLUtilsPlug.h>

#include "PitchShifterSmb.h"

#define USE_OVERSAMP_OBJ 0 //1

extern void smbPitchShift(float pitchShift, long numSampsToProcess,
                          long fftFrameSize, long osamp, float sampleRate,
                          float *indata, float *outdata);

#if USE_OVERSAMP_OBJ
class PitchShiftSmbOversampObj : public OversampProcessObj5
{
public:
    PitchShiftSmbOversampObj(PitchShifterSmb *shifter, int oversampling);
    virtual ~PitchShiftSmbOversampObj();
    
    void ProcessSamplesBuffer(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                              vector<WDL_TypedBuf<BL_FLOAT> > *out);

protected:
    PitchShifterSmb *mShifter;
};

PitchShiftSmbOversampObj::PitchShiftSmbOversampObj(PitchShifterSmb *shifter,
                                                   int oversampling)
: OversampProcessObj5(oversampling, 44100.0, true)
{
    mShifter = shifter;
}

PitchShiftSmbOversampObj::~PitchShiftSmbOversampObj() {}
    
void
PitchShiftSmbOversampObj::ProcessSamplesBuffer(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                                               vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    mShifter->ProcessSamples(in, out);
}

#endif


//
PitchShifterSmb::PitchShifterSmb()
{
    mSampleRate = 44100.0;
    mSmbOversampling = 4;
    mShift = 1.0;

    mOversampling = 1;

    mOversampObj = NULL;
    
#if USE_OVERSAMP_OBJ
    mOversampling = 4; //1;
    mOversampObj = new PitchShiftSmbOversampObj(this, mOversampling);
#endif
}

PitchShifterSmb::~PitchShifterSmb()
{
#if USE_OVERSAMP_OBJ
    delete mOversampObj;
#endif
}

void
PitchShifterSmb::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;

#if USE_OVERSAMP_OBJ
    mOversampObj->Reset(sampleRate, blockSize);
#endif
}

// Very drafty implementation
// Process only 1 channel
void
PitchShifterSmb::ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                                vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    if (in.empty())
        return;

    // TODO: manage double
    // TODO: optim memory / tmp buffers
    WDL_TypedBuf<float> inFloat;
    inFloat.Resize(in[0].GetSize());
    for (int i = 0; i < inFloat.GetSize(); i++)
    {
        // TODO: optimize .Get()
        inFloat.Get()[i] = in[0].Get()[i];
    }

    WDL_TypedBuf<float> outFloat;
    outFloat.Resize(inFloat.GetSize());

    // Process
#define FFT_FRAME_SIZE 2048 //1024
    int nFrames = inFloat.GetSize();
    float *inData = inFloat.Get();
    float *outData = outFloat.Get();
    smbPitchShift(mShift, nFrames, FFT_FRAME_SIZE*mOversampling,
                  mSmbOversampling,
                  mSampleRate*mOversampling, inData, outData);

    for (int i = 0; i < inFloat.GetSize(); i++)
    {
        // TODO: optimize .Get()
        (*out)[0].Get()[i] = outFloat.Get()[i];
    }

    // Process mono for the moment (due to static variables...
    // TODO: process 2 channels
    if (out->size() > 1)
        (*out)[1] = (*out)[0];
}

void
PitchShifterSmb::Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                         vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
#if USE_OVERSAMP_OBJ
    mOversampObj->Process(in, out);
#else
    ProcessSamples(in, out);
#endif
}
    
void
PitchShifterSmb::SetFactor(BL_FLOAT factor)
{
    mShift = factor;
}

void
PitchShifterSmb::SetQuality(int quality)
{
    switch(quality)
    {
        case 0:
#if USE_OVERSAMP_OBJ
            mOversampling = 1;
#endif
            mSmbOversampling = 4;
            break;
            
        case 1:
#if USE_OVERSAMP_OBJ
            mOversampling = 4;
#endif
            mSmbOversampling = 8;
            break;
            
        case 2:
#if USE_OVERSAMP_OBJ
            mOversampling = 8;
#endif
            mSmbOversampling = 16;
            break;
            
        case 3:
#if USE_OVERSAMP_OBJ
            mOversampling = 32;
#endif
            mSmbOversampling = 32;
            break;
            
        default:
            break;
    }

#if 0 //USE_OVERSAMP_OBJ
    // TODO: manage better without re-creating the object (if required...)
    delete mOversampObj;
    mOversampObj = new PitchShiftSmbOversampObj(this, mOversampling);
#endif
}
