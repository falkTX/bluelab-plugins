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
//  SpectroEditFftObj2EXPE.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

//#include <TransientLib4.h>
#include <TransientLib5.h>

#include "SpectroEditFftObj2EXPE.h"

SpectroEditFftObj2EXPE::SpectroEditFftObj2EXPE(int bufferSize, int oversampling,
                                               int freqRes,
                                               BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mMode = BYPASS;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
    
    mDataSelection[0] = 0.0;
    mDataSelection[1] = 0.0;
    mDataSelection[2] = 0.0;
    mDataSelection[3] = 0.0;
    
    mSamples = NULL;
    
    mSmoothFactor = 0.5;
    mFreqAmpRatio = 0.5;
    
    mTransThreshold = 0.0;

    mTransLib = new TransientLib5();
}

SpectroEditFftObj2EXPE::~SpectroEditFftObj2EXPE()
{
    delete mTransLib;
}

void
SpectroEditFftObj2EXPE::SetSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    mSamples = samples;
}

void
SpectroEditFftObj2EXPE::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                            const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    if ((mMode == PLAY) || (mMode == GEN_DATA))
    {
        BLUtils::FillAllZero(ioBuffer);
        
        if (mSamples != NULL)
        {
            int sampleId = LineCountToSampleId(mLineCount);
           
            if (mMode == PLAY)
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

void
SpectroEditFftObj2EXPE::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
 
    if (mMode == BYPASS)
    {
        BLUtils::FillAllZero(&magns);
    }
    
    if (mMode == PLAY)
    {
        if (mSamples != NULL)
        {
            ////// EXPE
            WDL_TypedBuf<BL_FLOAT> *prevPhases = NULL;
            if (mPrevPhases.GetSize() > 0)
                prevPhases = &mPrevPhases;
            
            // TEST
            //WDL_TypedBuf<BL_FLOAT> magnsNorm = magns;
            //BL_FLOAT avg = BLUtils::ComputeAvg(magns);
            //if (avg > 0.0)
            //    BLUtils::MultValues(&magnsNorm, 1.0/avg);
            
            // NOTE: interesting if we remove the db threshold
            // in TransientLib4::ComputeTransientness2
            WDL_TypedBuf<BL_FLOAT> transientness;
            mTransLib->ComputeTransientness2(magns,
                                             phases,
                                             prevPhases,
                                             mFreqAmpRatio,
                                             mSmoothFactor,
                                             &transientness);
            
            mPrevPhases = phases;
            
            //
            WDL_TypedBuf<int> samplesIds;
            BLUtilsFft::FftIdsToSamplesIds(phases, &samplesIds);
            
            WDL_TypedBuf<BL_FLOAT> sampleTrans = transientness;
            
            BLUtils::Permute(&sampleTrans, samplesIds, false/*true*/);
            //
            
            //BLUtils::MultValues(&magns, sampleTrans); // TEST (seems to work better, not sure)
                              
            for (int i = 0; i < magns.GetSize(); i++)
            {
                BL_FLOAT magn = magns.Get()[i];
                BL_FLOAT trans = sampleTrans.Get()[i];
                
                if (trans < mTransThreshold)
                    magn = 0.0;
                
                magns.Get()[i] = magn;
            }
            ////////
            
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
                WDL_TypedBuf<BL_FLOAT> newMagns;
                GetData(magns, &newMagns);
                magns = newMagns;
            
                WDL_TypedBuf<BL_FLOAT> newPhases;
                GetData(phases, &newPhases);
                phases = newPhases;
            }
        
            // Avoid a residual play on all the frequencies
            // when looping, at the end of the loop
            if (mSelectionPlayFinished)
            {
                BLUtils::FillAllZero(&magns);
                BLUtils::FillAllZero(&phases);
            }
        }
        
        if (mSamples == NULL)
        {
            // Same as above
            BLUtils::FillAllZero(&magns);
            BLUtils::FillAllZero(&phases);
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
        (mMode == GEN_DATA) ||
        (mMode == REPLACE_DATA))
    {
        mLineCount++;
    }
    
    BLUtilsComp::MagnPhaseToComplex(ioBuffer, magns, phases);
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

void
SpectroEditFftObj2EXPE::Reset(int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(mBufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj2EXPE::SetMode(Mode mode)
{
    mMode = mode;
}

SpectroEditFftObj2EXPE::Mode
SpectroEditFftObj2EXPE::GetMode()
{
    return mMode;
}

void
SpectroEditFftObj2EXPE::SetDataSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1)
{
    mDataSelection[0] = x0*mOverlapping;
    mDataSelection[1] = y0;
    mDataSelection[2] = x1*mOverlapping;
    mDataSelection[3] = y1;
    
    mSelectionEnabled = true;
}

void
SpectroEditFftObj2EXPE::SetSelectionEnabled(bool flag)
{
    mSelectionEnabled = flag;
}

bool
SpectroEditFftObj2EXPE::IsSelectionEnabled()
{
    return mSelectionEnabled;
}

void
SpectroEditFftObj2EXPE::GetNormSelection(BL_FLOAT selection[4])
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
SpectroEditFftObj2EXPE::SetNormSelection(const BL_FLOAT selection[4])
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return;
    
    BL_FLOAT numLines = GetNumLines();
    
    int lineSize = mBufferSize/2;
    
    mDataSelection[0] = selection[0]*numLines;
    mDataSelection[1] = selection[1]*lineSize;
    mDataSelection[2] = selection[2]*numLines;
    mDataSelection[3] = selection[3]*lineSize;
}

void
SpectroEditFftObj2EXPE::RewindToStartSelection()
{
    // Rewind to the beginning of the selection
    mLineCount = mDataSelection[0];
    
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj2EXPE::RewindToNormValue(BL_FLOAT value)
{
    BL_FLOAT numLines = GetNumLines();
    
    mLineCount = (mSamples == NULL) ? 0 : value*numLines;
    
    mSelectionPlayFinished = false;
}

bool
SpectroEditFftObj2EXPE::SelectionPlayFinished()
{
    return mSelectionPlayFinished;
}

BL_FLOAT
SpectroEditFftObj2EXPE::GetPlayPosition()
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    BL_FLOAT numLines = GetNumLines();
    BL_FLOAT res = ((BL_FLOAT)mLineCount)/numLines;
    
    return res;
}

BL_FLOAT
SpectroEditFftObj2EXPE::GetSelPlayPosition()
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    BL_FLOAT res = ((BL_FLOAT)(mLineCount - mDataSelection[0]))/
                          (mDataSelection[2] - mDataSelection[0]);
    
    return res;
}

BL_FLOAT
SpectroEditFftObj2EXPE::GetViewPlayPosition(BL_FLOAT startDataPos, BL_FLOAT endDataPos)
{
    if ((mSamples == NULL) || (mSamples->GetSize() == 0))
        return 0.0;
    
    BL_FLOAT res = ((BL_FLOAT)(mLineCount - startDataPos))/
                          (mOverlapping*(endDataPos - startDataPos));
    
    return res;
}

long
SpectroEditFftObj2EXPE::GetLineCount()
{
    return mLineCount;
}

void
SpectroEditFftObj2EXPE::SetLineCount(long lineCount)
{
     mLineCount = lineCount;
}

void
SpectroEditFftObj2EXPE::GetGeneratedData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                                     vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    *magns = mCurrentMagns;
    *phases = mCurrentPhases;
}

void
SpectroEditFftObj2EXPE::ClearGeneratedData()
{
    mCurrentMagns.clear();
    mCurrentPhases.clear();
}

// Just for principle
// In reality, these data should be consumed when playing
void
SpectroEditFftObj2EXPE::ClearReplaceData()
{
    mCurrentReplaceMagns.clear();
    mCurrentReplacePhases.clear();
}

// EXPE
void
SpectroEditFftObj2EXPE::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
}

void
SpectroEditFftObj2EXPE::SetFreqAmpRatio(BL_FLOAT ratio)
{
    mFreqAmpRatio = ratio;
}

void
SpectroEditFftObj2EXPE::SetTransThreshold(BL_FLOAT thrs)
{
    mTransThreshold = thrs;
}

void
SpectroEditFftObj2EXPE::SetReplaceData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                                   const vector<WDL_TypedBuf<BL_FLOAT> > &phases)
{
    mCurrentReplaceMagns = magns;
    mCurrentReplacePhases = phases;
}

void
SpectroEditFftObj2EXPE::GetData(const WDL_TypedBuf<BL_FLOAT> &currentData,
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
SpectroEditFftObj2EXPE::LineCountToSampleId(long lineCount)
{
    long sampleId = lineCount*mBufferSize/mOverlapping;
    
    return sampleId;
}

BL_FLOAT
SpectroEditFftObj2EXPE::GetNumLines()
{
    BL_FLOAT numLines = (((BL_FLOAT)mSamples->GetSize())/mBufferSize)*mOverlapping;
    
    return numLines;
}
