//
//  SpectroEditFftObj2.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "SpectroEditFftObj2.h"

SpectroEditFftObj2::SpectroEditFftObj2(int bufferSize, int oversampling, int freqRes,
                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //mLineCount = 0;
    mLineCount = 0.0;
    
    mMode = BYPASS;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
    
    mDataSelection[0] = 0.0;
    mDataSelection[1] = 0.0;
    mDataSelection[2] = 0.0;
    mDataSelection[3] = 0.0;
    
    mSamples = NULL;
}

SpectroEditFftObj2::~SpectroEditFftObj2() {}

void
SpectroEditFftObj2::SetSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    mSamples = samples;
}

#if 0 // ORIGIN
void
SpectroEditFftObj2::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                            const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    if ((mMode == PLAY) || (mMode == PLAY_RENDER) || (mMode == GEN_DATA))
    {
        if (mMode != PLAY_RENDER)
            BLUtils::FillAllZero(ioBuffer);
        
        if (mSamples != NULL)
        {
            int sampleId = LineCountToSampleId(mLineCount);
           
            if ((mMode == PLAY) || (mMode == PLAY_RENDER))
                // Manage latency, from the spectrogram display to the sound played
            {
                sampleId -= mBufferSize;
            }
            
            if (mMode == GEN_DATA)
            // Manage latency, to fix a shift when editing (extract slice, then re-paste it)
            {
                // After "BL_FLOAT GetNumLines()", which fixed data selection incorrect rounding
                //
                // NOTE: previously tested with 0.75*bufferSize and 0.5*bufferSize
                
                sampleId -= mBufferSize;
            }
            
            // Manages zeros if we get out of bounds
            if ((sampleId < 0) || (sampleId + mBufferSize >= mSamples->GetSize()))
                // Out of bounds
                // We are processing one of the borders of the spectrogram
            {
                // Generate a buffer filled of zeros
                BLUtils::ResizeFillZeros(ioBuffer, mBufferSize);
            }
            else // We are in bounds
            {
                ioBuffer->Resize(0);
                ioBuffer->Add(&mSamples->Get()[sampleId], mBufferSize);
            }
        }
    }
}
#endif

#if 1 // For Fft15 and latency
void
SpectroEditFftObj2::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                            const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    if ((mMode == PLAY) || (mMode == PLAY_RENDER) || (mMode == GEN_DATA))
    {
        if (mMode != PLAY_RENDER)
            BLUtils::FillAllZero(ioBuffer);
        
        if (mSamples != NULL)
        {
            int sampleId = LineCountToSampleId(mLineCount);
            
            if ((mMode == PLAY) || (mMode == PLAY_RENDER))
                // Manage latency, from the spectrogram display to the sound played
            {
                
#if 0 // 0: GOOD for Fft15 => no shift !
                sampleId -= mBufferSize;
#endif
            }
            
            if (mMode == GEN_DATA)
                // Manage latency, to fix a shift when editing (extract slice, then re-paste it)
            {
                // After "BL_FLOAT GetNumLines()", which fixed data selection incorrect rounding
                //
                // NOTE: previously tested with 0.75*bufferSize and 0.5*bufferSize
                
#if 0 // 0: GOOD for Fft15 => no shift !
                sampleId -= mBufferSize;
#endif
            }
            
            // Manages zeros if we get out of bounds
            if ((sampleId < 0) || (sampleId + mBufferSize >= mSamples->GetSize()))
                // Out of bounds
                // We are processing one of the borders of the spectrogram
            {
                // Generate a buffer filled of zeros
                BLUtils::ResizeFillZeros(ioBuffer, mBufferSize);
            }
            else // We are in bounds
            {
                //ioBuffer->Resize(0);
                //ioBuffer->Add(&mSamples->Get()[sampleId], mBufferSize);

                /*ioBuffer->Resize(mBufferSize);
                  memcpy(ioBuffer->Get(), &mSamples->Get()[sampleId],
                  mBufferSize*sizeof(BL_FLOAT));*/

                BLUtils::SetBufResize(ioBuffer, *mSamples, sampleId, mBufferSize);
            }
        }
    }
}
#endif

void
SpectroEditFftObj2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    //BLUtils::TakeHalf(ioBuffer);
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
            int sampleId = LineCountToSampleId(mLineCount);
            if (mSelectionEnabled)
            {
                BL_FLOAT x1 = mDataSelection[2];
                if (mLineCount > x1)
                    mSelectionPlayFinished = true;
            }
            else if (sampleId >= mSamples->GetSize())
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
        if ((mLineCount >= mDataSelection[0]) &&
            (mLineCount < mDataSelection[2]))
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
            if ((mLineCount >= mDataSelection[0]) &&
                (mLineCount < mDataSelection[2]))
            {
                magns = mCurrentReplaceMagns[0];
                phases = mCurrentReplacePhases[0];

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
        mLineCount++;
    }

    WDL_TypedBuf<WDL_FFT_COMPLEX> &result = mTmpBuf5;
    BLUtilsComp::MagnPhaseToComplex(&result, magns, phases);

    //memcpy(ioBuffer0->Get(), result.Get(),
    //       result.GetSize()*sizeof(WDL_FFT_COMPLEX));
    BLUtils::SetBuf(ioBuffer0, result);
    
    //BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);

    BLUtilsFft::FillSecondFftHalf(ioBuffer0);
}

void
SpectroEditFftObj2::Reset(int bufferSize, int oversampling,
                          int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0.0;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj2::SetMode(Mode mode)
{
    mMode = mode;
}

SpectroEditFftObj2::Mode
SpectroEditFftObj2::GetMode()
{
    return mMode;
}

void
SpectroEditFftObj2::SetDataSelection(BL_FLOAT x0, BL_FLOAT y0,
                                     BL_FLOAT x1, BL_FLOAT y1)
{
    mDataSelection[0] = x0*mOverlapping;
    mDataSelection[1] = y0;
    mDataSelection[2] = x1*mOverlapping;
    mDataSelection[3] = y1;
    
    SwapSelection(mDataSelection);
    
    mSelectionEnabled = true;
}

void
SpectroEditFftObj2::SetSelectionEnabled(bool flag)
{
    mSelectionEnabled = flag;
}

bool
SpectroEditFftObj2::IsSelectionEnabled()
{
    return mSelectionEnabled;
}

void
SpectroEditFftObj2::GetNormSelection(BL_FLOAT selection[4])
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
    {
        selection[0] = 0.0;
        selection[1] = 0.0;
        selection[2] = 1.0;
        selection[3] = 1.0;
        
        return;
    }
    
    BL_FLOAT numLines = GetNumLines();
    
    int lineSize = mBufferSize/2;
    
    selection[0] = mDataSelection[0]/numLines;
    selection[1] = mDataSelection[1]/lineSize;
    
    selection[2] = mDataSelection[2]/numLines;
    selection[3] = mDataSelection[3]/lineSize;
}

void
SpectroEditFftObj2::SetNormSelection(const BL_FLOAT selection[4])
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return;
    
    BL_FLOAT numLines = GetNumLines();
    
    int lineSize = mBufferSize/2;
    
    mDataSelection[0] = selection[0]*numLines;
    mDataSelection[1] = selection[1]*lineSize;
    mDataSelection[2] = selection[2]*numLines;
    mDataSelection[3] = selection[3]*lineSize;
    
    SwapSelection(mDataSelection);
}

void
SpectroEditFftObj2::RewindToStartSelection()
{
    // Rewind to the beginning of the selection
    mLineCount = mDataSelection[0];
    
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj2::RewindToNormValue(BL_FLOAT value)
{
    BL_FLOAT numLines = GetNumLines();
    
    mLineCount = (mSamples == NULL) ? 0 : value*numLines;
    
    mSelectionPlayFinished = false;
}

bool
SpectroEditFftObj2::SelectionPlayFinished()
{
    return mSelectionPlayFinished;
}

BL_FLOAT
SpectroEditFftObj2::GetPlayPosition()
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    BL_FLOAT numLines = GetNumLines();
    //BL_FLOAT res = ((BL_FLOAT)mLineCount)/numLines;
    BL_FLOAT res = mLineCount/numLines;
    
    return res;
}

BL_FLOAT
SpectroEditFftObj2::GetSelPlayPosition()
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    //BL_FLOAT res = ((BL_FLOAT)(mLineCount - mDataSelection[0]))/
    //                      (mDataSelection[2] - mDataSelection[0]);
    BL_FLOAT res = (mLineCount - mDataSelection[0])/
    (mDataSelection[2] - mDataSelection[0]);
    
    return res;
}

BL_FLOAT
SpectroEditFftObj2::GetViewPlayPosition(BL_FLOAT startDataPos, BL_FLOAT endDataPos)
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    //BL_FLOAT res = ((BL_FLOAT)(mLineCount - startDataPos))/
    //                      (mOverlapping*(endDataPos - startDataPos));
    BL_FLOAT res = (mLineCount - startDataPos)/
    (mOverlapping*(endDataPos - startDataPos));
    
    return res;
}

BL_FLOAT/*long*/
SpectroEditFftObj2::GetLineCount()
{
    return mLineCount;
}

void
SpectroEditFftObj2::SetLineCount(BL_FLOAT/*long*/ lineCount)
{
     mLineCount = lineCount;
}

void
SpectroEditFftObj2::GetGeneratedData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                                     vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    *magns = mCurrentMagns;
    *phases = mCurrentPhases;
}

void
SpectroEditFftObj2::ClearGeneratedData()
{
    mCurrentMagns.clear();
    mCurrentPhases.clear();
}

// Just for principle
// In reality, these data should be consumed when playing
void
SpectroEditFftObj2::ClearReplaceData()
{
    mCurrentReplaceMagns.clear();
    mCurrentReplacePhases.clear();
}

void
SpectroEditFftObj2::SetReplaceData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                                   const vector<WDL_TypedBuf<BL_FLOAT> > &phases)
{
    mCurrentReplaceMagns = magns;
    mCurrentReplacePhases = phases;
}

void
SpectroEditFftObj2::GetData(const WDL_TypedBuf<BL_FLOAT> &currentData,
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
    if ((mLineCount < x0) || (mLineCount > x1) ||
        (mSamples == NULL) ||
        (mLineCount >= mOverlapping*mSamples->GetSize()/mBufferSize))
    {
        // Set to 0 outside x selection
        BLUtils::FillAllZero(data);
    }
    else
    {
        // Here, we manage data on y
        if (mLineCount < mOverlapping*mSamples->GetSize()/mBufferSize)
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

long
SpectroEditFftObj2::LineCountToSampleId(BL_FLOAT/*long*/ lineCount)
{
    long sampleId = lineCount*mBufferSize/mOverlapping;
    
    return sampleId;
}

BL_FLOAT
SpectroEditFftObj2::GetNumLines()
{
    BL_FLOAT numLines = (((BL_FLOAT)mSamples->GetSize())/mBufferSize)*mOverlapping;
    
    return numLines;
}

void
SpectroEditFftObj2::SwapSelection(BL_FLOAT selection[4])
{
#if 0 // Not needed anymore since iPlug2
    if (selection[0] > selection[2])
    {
        BL_FLOAT tmp = selection[0];
        selection[0] = selection[2];
        selection[2] = tmp;
    }
    
    if (selection[1] > selection[3])
    {
        BL_FLOAT tmp = selection[1];
        selection[1] = selection[3];
        selection[3] = tmp;
    }
#endif
}
