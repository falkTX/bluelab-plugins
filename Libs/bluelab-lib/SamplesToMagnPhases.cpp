#include <FftProcessObj16.h>
#include <SpectroEditFftObj3.h>
//#include <SamplesPyramid2.h>
#include <SamplesPyramid3.h>

#include <BLUtils.h>
#include <BLDebug.h>

#include <TestSigmoid.h>

#include "SamplesToMagnPhases.h"

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
}

SamplesToMagnPhases::~SamplesToMagnPhases() {}

void
SamplesToMagnPhases::SetSamples(vector<WDL_TypedBuf<BL_FLOAT> > *samples)
{
    mSamples = samples;
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
            BLUtils::ResizeFillZeros(&in[i], bufStepSize);

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
        
        currentX += bufStepSize;

        // Check bounds
        if (mSpectroEditObjs[0]->SelectionPlayFinished())
            break;
    }
    
    // Restore state
    ReadWriteSliceRestoreState(state);
}

void
SamplesToMagnPhases::WriteSpectroDataSlice(vector<WDL_TypedBuf<BL_FLOAT> > magns[2],
                                           vector<WDL_TypedBuf<BL_FLOAT> > phases[2],
                                           BL_FLOAT minXNorm,
                                           BL_FLOAT maxXNorm,
                                           int fadeNumSamples)
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
            
            // Set all the data, will be consummed progressively
            // by the Fft obj
            mSpectroEditObjs[i]->SetReplaceData(magns[i], phases[i]);
        }
    }
    
    // Must use tmp channels, for skeezing buffers for latency
    vector<WDL_TypedBuf<BL_FLOAT> > tmpOutChannels;
    tmpOutChannels.resize(numChannels);

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
            
            //BLUtils::SetBufResize(&in[i], mSamples[i],
            //                      (int)currentX, BUF_STEP_SIZE);
            
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
            BLUtils::ResizeFillZeros(&out[i], bufStepSize);

        // Process
        mFftObj->Process(in, dummyScIn, &out);
            
        for (int i = 0; i < numChannels; i++)
            tmpOutChannels[i].Add(out[i].Get(), out[i].GetSize());
        
        currentX += bufStepSize;

        // Check bounds
        if (mSpectroEditObjs[0]->SelectionPlayFinished())
            break;
    }
            
    // Adjust for latency  
    for (int i = 0; i < tmpOutChannels.size(); i++)
    {
        BLUtils::ConsumeLeft(&tmpOutChannels[i], latency);
    }

    // Adjust for left overfeed
    for (int i = 0; i < tmpOutChannels.size(); i++)
    {
        BLUtils::ConsumeLeft(&tmpOutChannels[i], leftOverfeed);
    }
 
    // Adjust the right remaining samples
    // so the samples on the right will be exactly adjusted to selection
    // (when reading, we use ceil(), to have at least the requires magns,
    // or just a bit more)
    //
    // Recompute the right bound, because we had adjusted it to buffer size
    BL_FLOAT realMaxDataXSamples = maxXNorm*(*mSamples)[0].GetSize();
    BL_FLOAT selSize = realMaxDataXSamples - minDataXSamples;
    for (int i = 0; i < tmpOutChannels.size(); i++)
    {
        if (tmpOutChannels[i].GetSize() > selSize)
            tmpOutChannels[i].Resize(selSize);
    }
    
    for (int i = 0; i < numChannels; i++)
    {
        // Copy/Fade

#if 0 // This is bad to use it here
      // If we load a file, and the volume is quite high, this will clip
      // after any edit command like copy-paste, cut-paste, gain
      // (not detectable with the cut command)
        
        // Clip the waveform in case it is greater than 1
        // FIX: avoids waveform that looks right and sounds right
        // in Ghost-X, but that clips after exported and imported in another
        // software
        //mPlug->ClipWaveform(&tmpOutChannels[i]);
#endif

        if (fadeNumSamples != 0)
            // Must consider fading
        {
            // Test if we have a really small selection compared to the fade size
            // In this case, adjust the fade size
#define FADE_MIN_RATIO 0.25
            if (tmpOutChannels[i].GetSize()*FADE_MIN_RATIO < fadeNumSamples*2)
            {
                // Take 25% of fade on the left, 25% fade on the right
                // and 50% not faded at the center
                fadeNumSamples = tmpOutChannels[i].GetSize()*FADE_MIN_RATIO;
            }
            
            // Fade
            int bufSize = tmpOutChannels[i].GetSize();
            BL_FLOAT fadeStartPos = 0.0;
            BL_FLOAT fadeEndPos = ((BL_FLOAT)fadeNumSamples)/bufSize;
            
            const BL_FLOAT *src = &(*mSamples)[i].Get()[(int)minDataXSamples];
            BL_FLOAT *dst = tmpOutChannels[i].Get();

            //TestSigmoid::RunTest();

            // 0.5 is line
            // 0.2 is a bit flag
            // 0.05 is more flat => better fades!
#define SIGMO_A 0.05
            //BLUtils::Fade2Double(dst, src, bufSize,
            //                     fadeStartPos, fadeEndPos, 1.0, 0.0,
            //                     SIGMO_A);

            // Check bounds for fading
            //

            // Left fade
            int leftFadeMin = minDataXSamples;
            int leftFadeMax = minDataXSamples + fadeNumSamples;
            if ((leftFadeMin >= 0) &&
                (leftFadeMax < (*mSamples)[i].GetSize()))
            {
                BLUtils::Fade2Left(dst, src, bufSize,
                                   fadeStartPos, fadeEndPos, 1.0, 0.0,
                                   SIGMO_A);
            }

            // Right fade
            int rightFadeMin = minDataXSamples + bufSize - fadeNumSamples;
            int rightFadeMax = minDataXSamples + bufSize;
            if ((rightFadeMin >= 0) &&
                (rightFadeMax < (*mSamples)[i].GetSize()))
            {
                BLUtils::Fade2Right(dst, src, bufSize,
                                    fadeStartPos, fadeEndPos, 1.0, 0.0,
                                    SIGMO_A);
            }
            
        }

        // Crop if selection is out of bounds
        //
        
        // Adjust left
        int minDataXSamples0 = minDataXSamples;
        if (minDataXSamples0 < 0)
        {
            int numToCut = -minDataXSamples0;
            BLUtils::ConsumeLeft(&tmpOutChannels[i], numToCut);

            minDataXSamples0 = 0;
        }

        // Adjust right
        if (minDataXSamples0 + tmpOutChannels[i].GetSize() > (*mSamples)[i].GetSize())
        {
            int numToCut = (minDataXSamples0 + tmpOutChannels[i].GetSize()) -
            (*mSamples)[i].GetSize();
            
            BLUtils::ConsumeRight(&tmpOutChannels[i], numToCut);
        }
        
        if (i == 0)
        {
            if (mSamplesPyramid != NULL)
                mSamplesPyramid->ReplaceValues(minDataXSamples0,
                                               tmpOutChannels[i]);
        }
        
        // Simple copy
        BL_FLOAT *src = tmpOutChannels[i].Get();
        BL_FLOAT *dst = &(*mSamples)[i].Get()[(int)minDataXSamples0];
        memcpy(dst, src, tmpOutChannels[i].GetSize()*sizeof(BL_FLOAT));
    }
    
    // Restore state
    ReadWriteSliceRestoreState(state);
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

    // Reset after having saved state
    // because Reset() also resets the selection
    //if (mFftObj != NULL)
    //    mFftObj->Reset();
}

void
SamplesToMagnPhases::ReadWriteSliceRestoreState(const ReadWriteSliceState &state)
{
    // Reset before restoring
    // because Reset() also resets the selection
    //mFftObj->Reset();
    
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
    //result = round(result);
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
