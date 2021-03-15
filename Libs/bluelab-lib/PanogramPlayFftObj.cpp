//
//  PanogramPlayFftObj.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <HistoMaskLine2.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "PanogramPlayFftObj.h"

// BAD: doesn't work well
// Align better the sound played with the grapahics
// PROBLEM: no sound with thin selections
#define FIX_SHIFT_LINE_COUNT 0 //1

// At the right of the panogram, just before the play bar
// rewinds, the last line is played several times, making a garbage sound
#define FIX_LAST_LINES_BAD_SOUND 1

// GOOD
// New method => works better
// Align better the sound with selection and play bar
#define SHIFT_X_SELECTION 1

// With Reaper and SyncroTest (sine):
// Use a project freezed by default
// de-freeze, play host, stop host, freeze, then try to play the panogram => no sound
#define FIX_NO_SOUND_FIRST_DE_FREEZE 1


PanogramPlayFftObj::PanogramPlayFftObj(int bufferSize, int oversampling, int freqRes,
                                       BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mMode = RECORD;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
    
    mDataSelection[0] = 0.0;
    mDataSelection[1] = 0.0;
    mDataSelection[2] = 0.0;
    mDataSelection[3] = 0.0;
    
    mNumCols = 0;
    
    mIsPlaying = false;
    
    mHostIsPlaying = false;
}

PanogramPlayFftObj::~PanogramPlayFftObj() {}

void
PanogramPlayFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    if (mMode == BYPASS)
        return;
    
    if (mMode == PLAY)
    {
        if (mSelectionEnabled)
        {
            BL_FLOAT x1 = mDataSelection[2];
            if (mLineCount > x1)
                mSelectionPlayFinished = true;
        }
        else if (mLineCount >= mNumCols)
        {
            mSelectionPlayFinished = true;
        }
        
        WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf0;
        magns.Resize(ioBuffer->GetSize()/2);
        
        WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf1;
        phases.Resize(ioBuffer->GetSize()/2);
        
        if (!mSelectionPlayFinished)
        {
            //GetDataLine(mCurrentMagns, &magns);
            //GetDataLine(mCurrentPhases, &phases);
            
            int lineCount = mLineCount;
            
            bool inside = true;
            
#if FIX_SHIFT_LINE_COUNT
            inside = ShiftXPlayBar(&lineCount);
            
#if !FIX_LAST_LINES_BAD_SOUND
            inside = true;
#endif

#endif
            
            if (inside)
            {
                GetDataLineMask(mCurrentMagns, &magns, lineCount);
                GetDataLineMask(mCurrentPhases, &phases, lineCount);
            }
            else
            {
                // Outside selection => silence !
                BLUtils::FillAllZero(&magns);
                BLUtils::FillAllZero(&phases);
            }
        }
        
        // Avoid a residual play on all the frequencies
        // when looping, at the end of the loop
        if (mSelectionPlayFinished)
        {
            BLUtils::FillAllZero(&magns);
            BLUtils::FillAllZero(&phases);
        }
        
        BLUtilsComp::MagnPhaseToComplex(ioBuffer, magns, phases);
        
        BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
        BLUtilsFft::FillSecondFftHalf(ioBuffer);
        
        if (mIsPlaying)
        {
            // Increment
            mLineCount++;
        }
    }
    
    if (mMode == RECORD)
    {
        //WDL_TypedBuf<WDL_FFT_COMPLEX> ioBuffer0 = *ioBuffer;
        //BLUtils::TakeHalf(&ioBuffer0);

        WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer0 = mTmpBuf2;
        BLUtils::TakeHalf(*ioBuffer, &ioBuffer0);
        
        WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf3;
        WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf4;
        BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioBuffer0);

        if (mCurrentMagns.size() != mNumCols)
        {
            mCurrentMagns.push_back(magns);
            if (mCurrentMagns.size() > mNumCols)
                mCurrentMagns.pop_front();
        }
        else
        {
            mCurrentMagns.freeze();
            mCurrentMagns.push_pop(magns);
        }

        if (mCurrentPhases.size() != mNumCols)
        {
            mCurrentPhases.push_back(phases);
            if (mCurrentPhases.size() > mNumCols)
                mCurrentPhases.pop_front();
        }
        else
        {
            mCurrentPhases.freeze();
            mCurrentPhases.push_pop(phases);
        }
        
        // Play only inside selection
        //
        if (mHostIsPlaying && mSelectionEnabled)
        {
            WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf5;
            magns.Resize(ioBuffer->GetSize()/2);
            
            WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf6;
            phases.Resize(ioBuffer->GetSize()/2);
            
            //
            int lineCount = (mDataSelection[0] + mDataSelection[2])/2.0;
            
#if FIX_SHIFT_LINE_COUNT
            ShiftXPlayBar(&lineCount);
#endif
            
            GetDataLineMask(mCurrentMagns, &magns, lineCount);
            GetDataLineMask(mCurrentPhases, &phases, lineCount);

            //BLUtils::MagnPhaseToComplex(ioBuffer, magns, phases); 
            //BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
            //BLUtils::FillSecondFftHalf(ioBuffer);

            BLUtilsComp::MagnPhaseToComplex(&ioBuffer0, magns, phases);
            BLUtils::SetBuf(ioBuffer, ioBuffer0);
            BLUtilsFft::FillSecondFftHalf(ioBuffer);
        }
    }
}

void
PanogramPlayFftObj::Reset(int bufferSize, int oversampling,
                          int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
}

void
PanogramPlayFftObj::SetMode(Mode mode)
{
    mMode = mode;
}

void
PanogramPlayFftObj::SetNormSelection(BL_FLOAT x0, BL_FLOAT y0,
                                     BL_FLOAT x1, BL_FLOAT y1)
{
    // Swap
    BL_FLOAT newY0 = 1.0 - y1;
    BL_FLOAT newY1 = 1.0 - y0;
    y0 = newY0;
    y1 = newY1;
    
    mDataSelection[0] = x0*mNumCols;
    mDataSelection[1] = y0*mBufferSize/2;
    mDataSelection[2] = x1*mNumCols;
    mDataSelection[3] = y1*mBufferSize/2;
    
#if SHIFT_X_SELECTION
    ShiftXSelection(&mDataSelection[0]);
    ShiftXSelection(&mDataSelection[2]);
#endif
    
    mSelectionEnabled = true;
}

void
PanogramPlayFftObj::GetNormSelection(BL_FLOAT selection[4])
{
    int lineSize = mBufferSize/2;
    
    selection[0] = mDataSelection[0]/mNumCols;
    selection[1] = mDataSelection[1]/lineSize;
    
    selection[2] = mDataSelection[2]/mNumCols;
    selection[3] = mDataSelection[3]/lineSize;
}

void
PanogramPlayFftObj::SetSelectionEnabled(bool flag)
{
    mSelectionEnabled = flag;
}

void
PanogramPlayFftObj::RewindToStartSelection()
{
    // Rewind to the beginning of the selection
    mLineCount = mDataSelection[0];
    
    mSelectionPlayFinished = false;
}

void
PanogramPlayFftObj::RewindToNormValue(BL_FLOAT value)
{
    mLineCount = value*mNumCols;
    mLineCount = 0;
    
    mSelectionPlayFinished = false;
}

bool
PanogramPlayFftObj::SelectionPlayFinished()
{
    return mSelectionPlayFinished;
}

BL_FLOAT
PanogramPlayFftObj::GetPlayPosition()
{
    int lineCount = mLineCount;
    
    BL_FLOAT res = ((BL_FLOAT)lineCount)/mNumCols;
    
    return res;
}

BL_FLOAT
PanogramPlayFftObj::GetSelPlayPosition()
{
    int lineCount = mLineCount;
    
#if SHIFT_X_SELECTION
    lineCount -= 4;
    if (lineCount < mDataSelection[0] + 1 /*+ 2*/)
        lineCount = mDataSelection[0] + 1 /*+ 2*/;
#endif
    
    BL_FLOAT res = ((BL_FLOAT)(lineCount - mDataSelection[0]))/
                          (mDataSelection[2] - mDataSelection[0]);
    
    return res;
}

void
PanogramPlayFftObj::SetNumCols(int numCols)
{
    if (numCols != mNumCols)
    {
        mNumCols = numCols;
    
        mCurrentMagns.resize(mNumCols);
        mCurrentMagns.freeze();
        
        mCurrentPhases.resize(mNumCols);
        mCurrentPhases.freeze();
        
        WDL_TypedBuf<BL_FLOAT> &zeros = mTmpBuf7;
        zeros.Resize(mBufferSize/2);
        BLUtils::FillAllZero(&zeros);
    
        for (int i = 0; i < mNumCols; i++)
        {
            mCurrentMagns[i] = zeros;
            mCurrentPhases[i] = zeros;
        }
        
#if FIX_NO_SOUND_FIRST_DE_FREEZE
        /*mMaskLines.clear();
        
          HistoMaskLine2 maskLine(mBufferSize);
          for (int i = 0; i < mNumCols; i++)
          {
          mMaskLines.push_back(maskLine);
          }*/

        HistoMaskLine2 maskLine(mBufferSize);
        mMaskLines.resize(mNumCols);
        mMaskLines.clear(maskLine);
#endif
    }
}

void
PanogramPlayFftObj::SetIsPlaying(bool flag)
{
    mIsPlaying = flag;
}

void
PanogramPlayFftObj::SetHostIsPlaying(bool flag)
{
    mHostIsPlaying = flag;
}

void
PanogramPlayFftObj::AddMaskLine(const HistoMaskLine2 &maskLine)
{
    if (mMode == RECORD)
    {
        mMaskLines.push_back(maskLine);
        if (mMaskLines.size() > mNumCols)
            mMaskLines.pop_front();
    }
}

void
PanogramPlayFftObj::GetDataLine(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &inData,
                                WDL_TypedBuf<BL_FLOAT> *data,
                                int lineCount)
{
    BL_FLOAT x0 = mDataSelection[0];
    BL_FLOAT x1 = mDataSelection[2];
    
    // Here, we manage data on x
    if ((lineCount < x0) || (lineCount > x1) ||
        (lineCount >= mNumCols))
    {
        // Set to 0 outside x selection
        BLUtils::FillAllZero(data);
    }
    else
    {
        // Here, we manage data on y
        if (lineCount < mNumCols)
        {
            // Original line
            *data = inData[lineCount];
            
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
PanogramPlayFftObj::GetDataLineMask(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &inData,
                                    WDL_TypedBuf<BL_FLOAT> *data,
                                    int lineCount)
{
    BL_FLOAT x0 = mDataSelection[0];
    BL_FLOAT x1 = mDataSelection[2];
    
    // Here, we manage data on x
    if ((lineCount < x0) || (lineCount > x1) ||
        (lineCount >= mNumCols))
    {
        // Set to 0 outside x selection
        BLUtils::FillAllZero(data);
    }
    else
    {
        // Here, we manage data on y
        if ((lineCount > 0) &&(lineCount < mNumCols))
        {
            // Original line
            *data = inData[lineCount];
            
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
            
            // Mask
            if (lineCount < mMaskLines.size())
            {
                HistoMaskLine2 &maskLine = mMaskLines[lineCount];
                maskLine.Apply(data, y0, y1);
            }
        }
        else
        {
            BLUtils::FillAllZero(data);
        }
    }
}

// For play bar
bool
PanogramPlayFftObj::ShiftXPlayBar(int *xValue)
{
    // Shift a little, to be exactly at the middle of the selection
    *xValue += mOverlapping*2;
    if (*xValue >= mNumCols)
    {
        *xValue = mNumCols - 1;
        
        return false;
    }
    
    return true;
}

#if 0
bool
PanogramPlayFftObj::ShiftX(BL_FLOAT *xValue)
{
    int xValueInt = *xValue;
    bool result = ShiftX(&xValueInt);
    *xValue = xValueInt;
    
    return result;
}
#endif

// For selection
bool
PanogramPlayFftObj::ShiftXSelection(BL_FLOAT *xValue)
{
    // Shift a little, to be exactly at the middle of the selection
    *xValue += 2;
    if (*xValue >= mNumCols)
    {
        *xValue = mNumCols - 1;
        
        return false;
    }
    
    return true;
}
