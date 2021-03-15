//
//  SpectroEditFftObj3.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "SpectroEditFftObj3.h"

SpectroEditFftObj3::SpectroEditFftObj3(int bufferSize, int oversampling,
                                       int freqRes, BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);

    mSamples = NULL;
    mSamplesPos = 0.0;
    
    mMode = BYPASS;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
    
    mDataSelection[0] = 0.0;
    mDataSelection[1] = 0.0;
    mDataSelection[2] = 0.0;
    mDataSelection[3] = 0.0;
}

SpectroEditFftObj3::~SpectroEditFftObj3() {}

void
SpectroEditFftObj3::SetSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    mSamples = samples;
}

void
SpectroEditFftObj3::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                            const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    if ((mMode == PLAY) || (mMode == PLAY_RENDER) || (mMode == GEN_DATA))
    {
        if (mMode != PLAY_RENDER)
            BLUtils::FillAllZero(ioBuffer);
        
        if (mSamples != NULL)
        {            
            // Manages zeros if we get out of bounds
            if ((mSamplesPos < 0) ||
                (mSamplesPos + mBufferSize >= mSamples->GetSize()))
                // Out of bounds
                // We are processing one of the borders of the spectrogram
            {
                // Generate a buffer filled of zeros
                //BLUtils::ResizeFillZeros(ioBuffer, mBufferSize);

                // Fill with exactly the right number of zeros
                SpectroEditFftObj3::FillFromSelection(ioBuffer,
                                                      *mSamples,
                                                      mSamplesPos,
                                                      mBufferSize);
            }
            else // We are in bounds
            {
                BLUtils::SetBufResize(ioBuffer, *mSamples, mSamplesPos, mBufferSize);
            }
        }
    }
}

void
SpectroEditFftObj3::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf0;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioBuffer);
 
    if (mMode == BYPASS)
    {
        BLUtils::FillAllZero(&magns);
    }
    
    if ((mMode == PLAY) || (mMode == PLAY_RENDER))
    {
        if (mSamples != NULL)
        {
            if (mSelectionEnabled)
            {
                BL_FLOAT x1 = mDataSelection[2];
                if (mSamplesPos > x1)
                    mSelectionPlayFinished = true;
            }
            else if (mSamplesPos >= mSamples->GetSize())
            {
                mSelectionPlayFinished = true;
            }
        
            if (!mSelectionPlayFinished)
            {
                WDL_TypedBuf<BL_FLOAT> &newMagns = mTmpBuf3;
                GetData(magns, &newMagns);
                magns = newMagns;
            
                WDL_TypedBuf<BL_FLOAT> &newPhases = mTmpBuf4;
                GetData(phases, &newPhases);
                phases = newPhases;
            }
        
            // If mode is PLAY_RENDER, avoid writing zeros when
            // we have finished playing the spectrogram
            //
            // Will avoid erasing the remaining of the track when
            // "render tracks" on Reaper
            if (mMode != PLAY_RENDER)
            {
                // Avoid a residual play on all the frequencies
                // when looping, at the end of the loop
                if (mSelectionPlayFinished)
                {
                    BLUtils::FillAllZero(&magns);
                    BLUtils::FillAllZero(&phases);
                }
            }
        }
        
        if (mMode != PLAY_RENDER)
        {
            if (mSamples == NULL)
            {
                // Same as above
                BLUtils::FillAllZero(&magns);
                BLUtils::FillAllZero(&phases);
            }
        }
    }
    
    if (mMode == GEN_DATA)
    {
        // Accumulate the data only if we are into selection bounds
        if ((mSamplesPos >= mDataSelection[0]) &&
            (mSamplesPos < mDataSelection[2]))
        {
            mCurrentMagns.push_back(magns);
            mCurrentPhases.push_back(phases);
        }
        else // New version, manages zeros when out of bounds
        {
            WDL_TypedBuf<BL_FLOAT> zeros;
            BLUtils::ResizeFillZeros(&zeros, magns.GetSize());
            
            mCurrentMagns.push_back(zeros);
            mCurrentPhases.push_back(zeros);
        }
    }
    
    if (mMode == REPLACE_DATA)
    {
        if (!mCurrentReplaceMagns.empty())
        {
            // Replace the data only if we are into selection bounds
            if ((mSamplesPos >= mDataSelection[0]) &&
                (mSamplesPos < mDataSelection[2]))
            {
                magns = mCurrentReplaceMagns[0];
                phases = mCurrentReplacePhases[0];

#if 0 // Set to 1 to DEBUG
                BLUtils::FillAllZero(&magns);
                BLUtils::FillAllZero(&phases);
#endif
                // NOTE: this is not optimal for memory
                BLUtils::ConsumeLeft(&mCurrentReplaceMagns);
                BLUtils::ConsumeLeft(&mCurrentReplacePhases);
            }
            // Nothing to do if we are out of bounds
            // Simply don't touch the spectrogram
        }
    }
    
    // Incremenent only when playing
    // in order to have the playbar not moving
    // when we don't play !
    if ((mMode == PLAY) ||
        (mMode == PLAY_RENDER) ||
        (mMode == GEN_DATA) ||
        (mMode == REPLACE_DATA))
    {
        mSamplesPos += mBufferSize/mOverlapping;
    }

    WDL_TypedBuf<WDL_FFT_COMPLEX> &result = mTmpBuf5;
    BLUtilsComp::MagnPhaseToComplex(&result, magns, phases);

    BLUtils::SetBuf(ioBuffer0, result);

    BLUtilsFft::FillSecondFftHalf(ioBuffer0);
}

void
SpectroEditFftObj3::Reset(int bufferSize, int oversampling,
                          int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);

    mSamplesPos = 0.0;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj3::SetMode(Mode mode)
{
    mMode = mode;
}

SpectroEditFftObj3::Mode
SpectroEditFftObj3::GetMode()
{
    return mMode;
}

void
SpectroEditFftObj3::SetDataSelection(BL_FLOAT x0, BL_FLOAT y0,
                                     BL_FLOAT x1, BL_FLOAT y1)
{
    mDataSelection[0] = x0;
    mDataSelection[1] = y0;
    mDataSelection[2] = x1;
    mDataSelection[3] = y1;
    
    mSelectionEnabled = true;
}

void
SpectroEditFftObj3::SetSelectionEnabled(bool flag)
{
    mSelectionEnabled = flag;
}

bool
SpectroEditFftObj3::IsSelectionEnabled()
{
    return mSelectionEnabled;
}

void
SpectroEditFftObj3::GetNormSelection(BL_FLOAT selection[4])
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
    {
        selection[0] = 0.0;
        selection[1] = 0.0;
        selection[2] = 1.0;
        selection[3] = 1.0;
        
        return;
    }
    
    BL_FLOAT numSamples = mSamples->GetSize();
    int lineSize = mBufferSize/2;
    
    selection[0] = mDataSelection[0]/numSamples;
    selection[1] = mDataSelection[1]/lineSize;
    
    selection[2] = mDataSelection[2]/numSamples;
    selection[3] = mDataSelection[3]/lineSize;
}

void
SpectroEditFftObj3::SetNormSelection(const BL_FLOAT selection[4])
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return;
    
    BL_FLOAT numSamples = mSamples->GetSize();
    int lineSize = mBufferSize/2;
    
    mDataSelection[0] = selection[0]*numSamples;
    mDataSelection[1] = selection[1]*lineSize;
    mDataSelection[2] = selection[2]*numSamples;
    mDataSelection[3] = selection[3]*lineSize;
}

void
SpectroEditFftObj3::RewindToStartSelection()
{
    // Rewind to the beginning of the selection
    mSamplesPos = mDataSelection[0];
    
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj3::RewindToNormValue(BL_FLOAT value)
{
    if (mSamples == NULL)
        mSamplesPos =  0.0;
    else
        mSamplesPos = value*mSamples->GetSize();
    
    mSelectionPlayFinished = false;
}

bool
SpectroEditFftObj3::SelectionPlayFinished()
{
    return mSelectionPlayFinished;
}

BL_FLOAT
SpectroEditFftObj3::GetPlayPosition()
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    BL_FLOAT numSamples = mSamples->GetSize();
    BL_FLOAT res = mSamplesPos/numSamples;
    
    return res;
}

BL_FLOAT
SpectroEditFftObj3::GetSelPlayPosition()
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    BL_FLOAT res = (mSamplesPos - mDataSelection[0])/
    (mDataSelection[2] - mDataSelection[0]);
    
    return res;
}

void
SpectroEditFftObj3::GetGeneratedData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                                     vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    *magns = mCurrentMagns;
    *phases = mCurrentPhases;
}

void
SpectroEditFftObj3::ClearGeneratedData()
{
    mCurrentMagns.clear();
    mCurrentPhases.clear();
}

// Just for principle
// In reality, these data should be consumed when playing
void
SpectroEditFftObj3::ClearReplaceData()
{
    mCurrentReplaceMagns.clear();
    mCurrentReplacePhases.clear();
}

void
SpectroEditFftObj3::SetReplaceData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                                   const vector<WDL_TypedBuf<BL_FLOAT> > &phases)
{
    mCurrentReplaceMagns = magns;
    mCurrentReplacePhases = phases;
}

void
SpectroEditFftObj3::GetData(const WDL_TypedBuf<BL_FLOAT> &currentData,
                            WDL_TypedBuf<BL_FLOAT> *data)
{
    if (!mSelectionEnabled)
    {
        *data = currentData;
    
        return;
    }

    BL_FLOAT x0 = mDataSelection[0];
    BL_FLOAT x1 = mDataSelection[2];
    
    // Here, we manage data on x
    if ((mSamplesPos < x0) || (mSamplesPos > x1) ||
        (mSamples == NULL) ||
        (mSamplesPos >= mSamples->GetSize()))
    {
        // Set to 0 outside x selection
        BLUtils::FillAllZero(data);
    }
    else
    {
        // Here, we manage data on y
        if (mSamplesPos < mSamples->GetSize())
        {
            // Original line
            *data = currentData;
        
            // Set to 0 outside y selection
            BL_FLOAT y0 = mDataSelection[1];
            if (y0 < 0.0)
                y0 = 0.0;

            // After Valgrind tests
            // Could happen if we dragged the whole selection outside
            if (y0 >= data->GetSize())
                y0 = data->GetSize() - 1.0;
	    
            BL_FLOAT y1 = mDataSelection[3];
            if (y1 >= data->GetSize())
                y1 = data->GetSize() - 1.0;
            
            // Can happen if we dragged the whole selection outside
            if (y1 < 0.0)
                y1 = 0.0;
            
            for (int i = 0; i <= y0; i++)
                data->Get()[i] = 0.0;
        
            for (int i = y1; i < data->GetSize(); i++)
                data->Get()[i] = 0.0;
        }
        else
        {
            BLUtils::FillAllZero(data);
        }
    }
}

void
SpectroEditFftObj3::FillFromSelection(WDL_TypedBuf<BL_FLOAT> *result,
                                      const WDL_TypedBuf<BL_FLOAT> &inBuf,
                                      int selStartSamples,
                                      int selSizeSamples)
{
    result->Resize(selSizeSamples);
    BLUtils::FillAllZero(result);
    
    for (int i = selStartSamples; i < selStartSamples + selSizeSamples; i++)
    {
        if ((i >= 0) && (i < inBuf.GetSize()))
        {
            BL_FLOAT val = inBuf.Get()[i];
            result->Get()[i - selStartSamples] = val;
        }
    }
}
