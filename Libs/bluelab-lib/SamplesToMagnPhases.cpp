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
 
#include <FftProcessObj16.h>
#include <SpectroEditFftObj3.h>
//#include <SamplesPyramid2.h>
#include <SamplesPyramid3.h>

#include <BLUtils.h>
#include <BLUtilsFade.h>

#include <BLDebug.h>

#include "SamplesToMagnPhases.h"

// NOTE: cppcheck warans about out of bounds values
// (don't know why...)

SamplesToMagnPhases::SamplesToMagnPhases(vector<WDL_TypedBuf<BL_FLOAT> > *samples,
                                         FftProcessObj16 *fftObj,
                                         SpectroEditFftObj3 *spectroEditObjs[2],
                                         SamplesPyramid3 *samplesPyramid)
{
    mSamples = samples;
    
    mFftObj = fftObj;
    for (int i = 0; i < 2; i++)
        mSpectroEditObjs[i] = spectroEditObjs[i];

    mSamplesPyramid = samplesPyramid;

    mStep = 1.0;

    mForceMono = false;
}

SamplesToMagnPhases::~SamplesToMagnPhases() {}

void
SamplesToMagnPhases::SetSamples(vector<WDL_TypedBuf<BL_FLOAT> > *samples)
{
    mSamples = samples;
        
    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] != NULL)
        {
            if (i < samples->size())
                mSpectroEditObjs[i]->SetSamples(&(*samples)[i]);
            else
                mSpectroEditObjs[i]->SetSamples(NULL);
            
            mSpectroEditObjs[i]->SetSamplesForMono(mSamples);
        }
    }
}

void
SamplesToMagnPhases::SetForceMono(bool flag)
{
    mForceMono = flag;

    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] != NULL)
            mSpectroEditObjs[i]->SetForceMono(flag);
    }
}

void
SamplesToMagnPhases::ReadSpectroDataSlice(vector<WDL_TypedBuf<BL_FLOAT> > magns[2],
                                          vector<WDL_TypedBuf<BL_FLOAT> > phases[2],
                                          BL_FLOAT minXNorm,
                                          BL_FLOAT maxXNorm)
{
    // Check arguments
    if (mSpectroEditObjs[0] == NULL)
        return;
    if (mFftObj == NULL)
        return;

    int numChannels = (int)mSamples->size();
    if (numChannels == 0)        
        return;

    // Save state
    ReadWriteSliceState state;
    ReadWriteSliceSaveState(&state);
    
    // Process
    //
    
    //Empty the result
    for (int i = 0; i < 2; i++)
    {
        magns[i].clear();
        phases[i].clear();
    }
    
    // Normalized to samples
    BL_FLOAT minDataXSamples;
    BL_FLOAT maxDataXSamples;
    NormalizedPosToSamplesPos(minXNorm, maxXNorm,
                              &minDataXSamples, &maxDataXSamples);

    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] != NULL)
            mSpectroEditObjs[i]->SetMode(SpectroEditFftObj3::GEN_DATA);
    }

    // Adjust selection for latency etc.
    int bufferSize = mFftObj->GetBufferSize();
    int overlapping = mFftObj->GetOverlapping();
    
    //
    int bufStepSize = bufferSize/overlapping;
    
    // Latency of fft obj
    // The magnitudes are not computed until number of input samples is >= latency
    int latency = bufferSize;
    // Must process more, so we have enough magnitudes on the right
    int rightOverfeed = bufferSize;
    
    SetDataSelection(minDataXSamples - latency, maxDataXSamples + rightOverfeed);

    // Generate data
    int currentX = minDataXSamples;
    while(currentX < maxDataXSamples + latency + rightOverfeed)
    {
        // in
        vector<WDL_TypedBuf<BL_FLOAT> > &in = mTmpBuf0;
        in.resize(numChannels);
        for (int i = 0; i < numChannels; i++)
        {
            in[i].Resize(bufStepSize);
            BLUtils::FillAllZero(&in[i]);
        }

        // sc
        vector<WDL_TypedBuf<BL_FLOAT> > dummyScIn;

        // Generate magns and phases    
        mFftObj->Process(in, dummyScIn, NULL);
            
        // Retrieve the computed magns and phases
        for (int i = 0; i < numChannels; i++)
        {
            // There are 4 magns and phase arrays for oversampling 4
            vector<WDL_TypedBuf<BL_FLOAT> > &magns0 = mTmpBuf3;
            vector<WDL_TypedBuf<BL_FLOAT> > &phases0 = mTmpBuf4;
            if (mSpectroEditObjs[i] != NULL)
            {
                mSpectroEditObjs[i]->GetGeneratedData(&magns0, &phases0);

                // Update the result
                for (int j = 0; j < magns0.size(); j++)
                {
                    magns[i].push_back(magns0[j]);
                    phases[i].push_back(phases0[j]);
                }
                
                mSpectroEditObjs[i]->ClearGeneratedData();
            }
        }
        
        currentX += bufStepSize*mStep;

        // Check bounds
        if (mSpectroEditObjs[0]->SelectionPlayFinished())
            break;
    }
    
    // Restore state
    ReadWriteSliceRestoreState(state);
}

void
SamplesToMagnPhases::
WriteSpectroDataSlice(vector<WDL_TypedBuf<BL_FLOAT> > magns[2],
                      vector<WDL_TypedBuf<BL_FLOAT> > phases[2],
                      BL_FLOAT minXNorm,
                      BL_FLOAT maxXNorm,
                      int fadeNumSamples,
                      vector<WDL_TypedBuf<BL_FLOAT> > *outSamples)
{
    // Check arguments
    if (mSpectroEditObjs[0] == NULL)
        return;
    if (mFftObj == NULL)
        return;

    int numChannels = (int)mSamples->size();
    if (numChannels == 0)        
        return;

    // Save state
    ReadWriteSliceState state;
    ReadWriteSliceSaveState(&state);
    
    // Process
    //
    
    // Normalized to samples
    BL_FLOAT minDataXSamples;
    BL_FLOAT maxDataXSamples;
    NormalizedPosToSamplesPos(minXNorm, maxXNorm,
                              &minDataXSamples, &maxDataXSamples);

    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] != NULL)
            mSpectroEditObjs[i]->SetMode(SpectroEditFftObj3::REPLACE_DATA);
    }
    
    // Set all the magns and phases to replace, it will be consumed progressively
    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] != NULL)
        {
            mSpectroEditObjs[i]->ClearReplaceData();
            
            // Set all the data, will be consummed progressively by the Fft obj
            mSpectroEditObjs[i]->SetReplaceData(magns[i], phases[i]);
        }
    }
    
    // Must use tmp channels, for skeezing buffers for latency
    vector<WDL_TypedBuf<BL_FLOAT> > tmpBuf;
    vector<WDL_TypedBuf<BL_FLOAT> > *tmpOutChannels = &tmpBuf;
    if (outSamples != NULL)
    {
        // Clear, just in case
        outSamples->resize(0);
        
        tmpOutChannels = outSamples;
    }
    
    tmpOutChannels->resize(numChannels);

    // Setup selection for latency etc.
    //
    int bufferSize = mFftObj->GetBufferSize();
    int bufStepSize = bufferSize/mFftObj->GetOverlapping();

    // Latency of the fft obj
    // The fft obj starts by generating "latency" number of zeros
    // We will remove correspnding samples on the left
    int latency = bufferSize;
    // Feed more on the left with samples, to avoid that the
    // first samples have a windowing fade effect
    // We will remove correspnding samples on the left
    int leftOverfeed = bufferSize;
    // Continue more on the right, to compensate for latency
    int rightOverfeed = latency;

    SetDataSelection(minDataXSamples, maxDataXSamples + rightOverfeed);
    
    // Generate data
    
    int currentX = minDataXSamples - leftOverfeed;
    while(currentX < maxDataXSamples + rightOverfeed)
    {
        // in
        vector<WDL_TypedBuf<BL_FLOAT> > &in = mTmpBuf1;
        in.resize(numChannels);
        for (int i = 0; i < numChannels; i++)
        {
            // Fill input with samples
            // => so the first chunk won't have the windowing effect
            
            // Fill some parts with zeros if selection is partially out of bounds
            SpectroEditFftObj3::FillFromSelection(&in[i], (*mSamples)[i],
                                                  (int)currentX, bufStepSize);
        }
        
        // cc
        vector<WDL_TypedBuf<BL_FLOAT> > dummyScIn;
        
        // out
        vector<WDL_TypedBuf<BL_FLOAT> > &out = mTmpBuf2;
        out.resize(numChannels);
        for (int i = 0; i < numChannels; i++)
        {
            out[i].Resize(bufStepSize);
            BLUtils::FillAllZero(&out[i]);
        }

        // Process
        mFftObj->Process(in, dummyScIn, &out);
            
        for (int i = 0; i < numChannels; i++)
            (*tmpOutChannels)[i].Add(out[i].Get(), out[i].GetSize());
        
        currentX += bufStepSize*mStep;

        // Check bounds
        if (mSpectroEditObjs[0]->SelectionPlayFinished())
            break;
    }
            
    // Adjust for latency  
    for (int i = 0; i < tmpOutChannels->size(); i++)
    {
        BLUtils::ConsumeLeft(&(*tmpOutChannels)[i], latency);
    }

    // Adjust for left overfeed
    for (int i = 0; i < tmpOutChannels->size(); i++)
    {
        BLUtils::ConsumeLeft(&(*tmpOutChannels)[i], leftOverfeed);
    }
 
    // Adjust the right remaining samples
    // so the samples on the right will be exactly adjusted to selection
    // (when reading, we use ceil(), to have at least the requires magns,
    // or just a bit more)
    //
    // Recompute the right bound, because we had adjusted it to buffer size
    BL_FLOAT realMaxDataXSamples = maxXNorm*(*mSamples)[0].GetSize();
    BL_FLOAT selSize = realMaxDataXSamples - minDataXSamples;
    for (int i = 0; i < tmpOutChannels->size(); i++)
    {
        if ((*tmpOutChannels)[i].GetSize() > selSize)
            (*tmpOutChannels)[i].Resize(selSize);
    }
    
    for (int i = 0; i < numChannels; i++)
    {
        // Copy/Fade

        if (fadeNumSamples != 0)
            // Must consider fading
        {
            // Test if we have a really small selection compared to the fade size
            // In this case, adjust the fade size
#define FADE_MIN_RATIO 0.25
            if ((*tmpOutChannels)[i].GetSize()*FADE_MIN_RATIO < fadeNumSamples*2)
            {
                // Take 25% of fade on the left, 25% fade on the right
                // and 50% not faded at the center
                fadeNumSamples = (*tmpOutChannels)[i].GetSize()*FADE_MIN_RATIO;
            }
            
            // Fade
            int bufSize = (*tmpOutChannels)[i].GetSize();
            BL_FLOAT fadeStartPos = 0.0;
            BL_FLOAT fadeEndPos = ((BL_FLOAT)fadeNumSamples)/bufSize;
            
            const BL_FLOAT *src = &(*mSamples)[i].Get()[(int)minDataXSamples];
            BL_FLOAT *dst = (*tmpOutChannels)[i].Get();

            //TestSigmoid::RunTest();

            // 0.5 is line
            // 0.2 is a bit flag
            // 0.05 is more flat => better fades!
#define SIGMO_A 0.05
            
            // Check bounds for fading
            //

            // Left fade
            int leftFadeMin = minDataXSamples;
            int leftFadeMax = minDataXSamples + fadeNumSamples;
            if ((leftFadeMin >= 0) &&
                (leftFadeMax < (*mSamples)[i].GetSize()))
            {
                BLUtilsFade::Fade2Left(dst, src, bufSize,
                                       fadeStartPos, fadeEndPos,
                                       (BL_FLOAT)1.0, (BL_FLOAT)0.0,
                                       (BL_FLOAT)SIGMO_A);
            }

            // Right fade
            int rightFadeMin = minDataXSamples + bufSize - fadeNumSamples;
            int rightFadeMax = minDataXSamples + bufSize;
            if ((rightFadeMin >= 0) &&
                (rightFadeMax < (*mSamples)[i].GetSize()))
            {
                BLUtilsFade::Fade2Right(dst, src, bufSize,
                                        fadeStartPos, fadeEndPos,
                                        (BL_FLOAT)1.0, (BL_FLOAT)0.0,
                                        (BL_FLOAT)SIGMO_A);
            }
            
        }

        // Crop if selection is out of bounds
        //
        
        // Adjust left
        int minDataXSamples0 = minDataXSamples;
        if (minDataXSamples0 < 0)
        {
            int numToCut = -minDataXSamples0;
            BLUtils::ConsumeLeft(&(*tmpOutChannels)[i], numToCut);

            minDataXSamples0 = 0;
        }

        // Adjust right
        if (minDataXSamples0 + (*tmpOutChannels)[i].GetSize() >
            (*mSamples)[i].GetSize())
        {
            int numToCut = (minDataXSamples0 + (*tmpOutChannels)[i].GetSize()) -
            (*mSamples)[i].GetSize();
            
            BLUtils::ConsumeRight(&(*tmpOutChannels)[i], numToCut);
        }
        
        if ((i == 0) &&
            (outSamples == NULL)) // We don't modify mSamples
        {
            if (mSamplesPyramid != NULL)
            {
                if (tmpOutChannels->size() == 1)
                    // Normal behaviour
                {
                    mSamplesPyramid->ReplaceValues(minDataXSamples0,
                                                   (*tmpOutChannels)[i]);
                }
                else if (tmpOutChannels->size() >= 2)
                {
                    // Stereo to mono
                    // NOTE: thing to change in Ghost too
                    WDL_TypedBuf<BL_FLOAT> mono;
                    BLUtils::StereoToMono(&mono,
                                          (*tmpOutChannels)[0],
                                          (*tmpOutChannels)[1]);
                
                    mSamplesPyramid->ReplaceValues(minDataXSamples0, mono);
                }
            }
        }

        if (outSamples == NULL)
        {
            // Simple copy
            BL_FLOAT *src = (*tmpOutChannels)[i].Get();
            BL_FLOAT *dst = &(*mSamples)[i].Get()[(int)minDataXSamples0];
            memcpy(dst, src, (*tmpOutChannels)[i].GetSize()*sizeof(BL_FLOAT));
        }
    }
    
    // Restore state
    ReadWriteSliceRestoreState(state);
}

void
SamplesToMagnPhases::ReadSelectedSamples(vector<WDL_TypedBuf<BL_FLOAT> > *samples,
                                         BL_FLOAT minXNorm, BL_FLOAT maxNormX)
{
    vector<WDL_TypedBuf<BL_FLOAT> > magns[2];
    vector<WDL_TypedBuf<BL_FLOAT> > phases[2];
    ReadSpectroDataSlice(magns, phases, minXNorm, maxNormX);

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < magns[i].size(); j++)
        {
            if (mSpectroEditObjs[0] != NULL)
            {
                mSpectroEditObjs[0]->ApplyYSelection(&magns[i][j]);
                mSpectroEditObjs[0]->ApplyYSelection(&phases[i][j]);
            }
        }
    }
    
    WriteSpectroDataSlice(magns, phases, minXNorm, maxNormX, 0, samples);
}

void
SamplesToMagnPhases::SetStep(BL_FLOAT step)
{
    mStep = step;

    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] != NULL)
            mSpectroEditObjs[i]->SetStep(mStep);
    }
}

void
SamplesToMagnPhases::ReadWriteSliceSaveState(ReadWriteSliceState *state)
{
    if (mFftObj != NULL)
        mFftObj->Reset();
    
    state->mSpectroEditMode = mSpectroEditObjs[0]->GetMode();
    state->mSpectroEditSelectionEnabled = mSpectroEditObjs[0]->IsSelectionEnabled();

    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] == NULL)
            continue;
        
        mSpectroEditObjs[i]->GetNormSelection(state->mSpectroEditSels[i]);
    }
}

void
SamplesToMagnPhases::ReadWriteSliceRestoreState(const ReadWriteSliceState &state)
{
    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] != NULL)
        {
            mSpectroEditObjs[i]->SetNormSelection(state.mSpectroEditSels[i]);
            mSpectroEditObjs[i]->RewindToStartSelection();
        
            mSpectroEditObjs[i]->SetMode(state.mSpectroEditMode);

            mSpectroEditObjs[i]->
            SetSelectionEnabled(state.mSpectroEditSelectionEnabled);
        }
    }

    // Reset to avoid playing a small part of sound just after
    // in ProcessDoubleReplacing()
    mFftObj->Reset();
}

void
SamplesToMagnPhases::AlignSamplePosToFftBuffers(BL_FLOAT *pos)
{
    if (mFftObj == NULL)
        return;
    
    // Align to the SpectroEdit2 data (it has 1 line every 512 samples, for os=4)
    int bufferSize = mFftObj->GetBufferSize();
    int overlapping = mFftObj->GetOverlapping();
    
    BL_FLOAT result = *pos/(bufferSize/overlapping);
    result = ceil(result); // Take more!
    result *= bufferSize/overlapping;
    
    *pos = result;
}

void
SamplesToMagnPhases::SetDataSelection(BL_FLOAT minXSamples, BL_FLOAT maxXSamples)
{
    int bufferSize = mFftObj->GetBufferSize();
    
    for (int i = 0; i < 2; i++)
    {
        if (mSpectroEditObjs[i] == NULL)
            continue;
        
        mSpectroEditObjs[i]->SetDataSelection(minXSamples, 0,
                                              maxXSamples, bufferSize/2);
        
        mSpectroEditObjs[i]->RewindToStartSelection();
    }
}

void
SamplesToMagnPhases::NormalizedPosToSamplesPos(BL_FLOAT minXNorm,
                                               BL_FLOAT maxXNorm,
                                               BL_FLOAT *minXSamples,
                                               BL_FLOAT *maxXSamples)
{
    *minXSamples = minXNorm*(*mSamples)[0].GetSize();
    *maxXSamples = maxXNorm*(*mSamples)[0].GetSize();

    // Align to integer sample
    *minXSamples = (int)*minXSamples;
    *maxXSamples = (int)*maxXSamples;

    // Align size to buffers
    // So the left pos is exactly aligned to samples
    // and the right pos is aligned to buffers using ceil()
    BL_FLOAT dataXSize = *maxXSamples - *minXSamples;
    AlignSamplePosToFftBuffers(&dataXSize);
    *maxXSamples = *minXSamples + dataXSize;
}
