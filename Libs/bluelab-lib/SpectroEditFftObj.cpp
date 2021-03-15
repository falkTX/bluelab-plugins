//
//  SpectroEditFftObj.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "SpectroEditFftObj.h"

SpectroEditFftObj::SpectroEditFftObj(int bufferSize, int oversampling, int freqRes,
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
}

SpectroEditFftObj::~SpectroEditFftObj() {}

void
SpectroEditFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
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
    
    if (mMode == RECORD)
    {
        mLines.push_back(magns);
        mPhases.push_back(phases);
    }
    
    if (mMode == PLAY)
    {
        if (mSelectionEnabled)
        {
            BL_FLOAT x1 = mDataSelection[2];
            if (mLineCount > x1)
                mSelectionPlayFinished = true;
        }
        else if (mLineCount >= mLines.size())
        {
            mSelectionPlayFinished = true;
        }
        
        if (!mSelectionPlayFinished)
        {
            GetData(mLines, &magns);
            GetData(mPhases, &phases);
        }
    }
    
    // Incremenent only when playing
    // in order to have the playbar not moving
    // when we don't play !
    if (mMode == PLAY)
        mLineCount++;
    
    BLUtilsComp::MagnPhaseToComplex(ioBuffer, magns, phases);
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

void
SpectroEditFftObj::Reset(int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(mBufferSize, oversampling, freqRes, sampleRate);
    
    //mLines.clear();
    mLineCount = 0;
    
    mSelectionEnabled = false;
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj::ClearData()
{
    mLines.clear();
    mPhases.clear();
}

void
SpectroEditFftObj::SetMode(Mode mode)
{
    mMode = mode;
}

SpectroEditFftObj::Mode
SpectroEditFftObj::GetMode()
{
    return mMode;
}

void
SpectroEditFftObj::SetDataSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1)
{
    mDataSelection[0] = x0;
    mDataSelection[1] = y0;
    mDataSelection[2] = x1;
    mDataSelection[3] = y1;
    
    mSelectionEnabled = true;
}

void
SpectroEditFftObj::SetSelectionEnabled(bool flag)
{
    mSelectionEnabled = flag;
}

bool
SpectroEditFftObj::IsSelectionEnabled()
{
    return mSelectionEnabled;
}

void
SpectroEditFftObj::GetNormSelection(BL_FLOAT selection[4])
{
    if (mLines.empty())
    {
        selection[0] = 0.0;
        selection[1] = 0.0;
        selection[2] = 1.0;
        selection[3] = 1.0;
        
        return;
    }
    
    selection[0] = mDataSelection[0]/mLines.size();
    selection[1] = mDataSelection[1]/mLines[0].GetSize();
    
    selection[2] = mDataSelection[2]/mLines.size();
    selection[3] = mDataSelection[3]/mLines[0].GetSize();
}

void
SpectroEditFftObj::SetNormSelection(const BL_FLOAT selection[4])
{
    if (mLines.empty())
        return;
    
    mDataSelection[0] = selection[0]*mLines.size();
    mDataSelection[1] = selection[1]*mLines[0].GetSize();
    mDataSelection[2] = selection[2]*mLines.size();
    mDataSelection[3] = selection[3]*mLines[0].GetSize();
}

void
SpectroEditFftObj::RewindToStartSelection()
{
    // Rewind to the beginning of the selection
    mLineCount = mDataSelection[0];
    
    mSelectionPlayFinished = false;
}

void
SpectroEditFftObj::RewindToNormValue(BL_FLOAT value)
{
    if ((value < 0.0) || (value > 1.0))
        return;
    
    mLineCount = value*mLines.size();
    
    mSelectionPlayFinished = false;
}

bool
SpectroEditFftObj::SelectionPlayFinished()
{
    return mSelectionPlayFinished;
}

BL_FLOAT
SpectroEditFftObj::GetPlayPosition()
{
    if (mLines.empty())
        return 0.0;
    
    BL_FLOAT res = ((BL_FLOAT)mLineCount)/mLines.size();
    
    return res;
}

BL_FLOAT
SpectroEditFftObj::GetSelPlayPosition()
{
    if (mLines.empty())
        return 0.0;
    
    BL_FLOAT res = ((BL_FLOAT)(mLineCount - mDataSelection[0]))/
                          (mDataSelection[2] - mDataSelection[0]);
    
    return res;
}

// BUGGY ?
BL_FLOAT
SpectroEditFftObj::GetViewPlayPosition(BL_FLOAT startDataPos, BL_FLOAT endDataPos)
{
    if (mLines.empty())
        return 0.0;
    
    BL_FLOAT res = ((BL_FLOAT)(mLineCount - startDataPos))/
                          (mOverlapping*(endDataPos - startDataPos));
    
    return res;
}

void
SpectroEditFftObj::GetFullData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                               vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    *magns = mLines;
    *phases = mPhases;
}

void
SpectroEditFftObj::SetFullData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                               const vector<WDL_TypedBuf<BL_FLOAT> > &phases)
{
    mLines = magns;
    mPhases = phases;
}

long
SpectroEditFftObj::GetLineCount()
{
    return mLineCount;
}

void
SpectroEditFftObj::SetLineCount(long lineCount)
{
    mLineCount = lineCount;
}

void
SpectroEditFftObj::GetData(const vector<WDL_TypedBuf<BL_FLOAT> > &fullData,
                           WDL_TypedBuf<BL_FLOAT> *data)
{
    if (!mSelectionEnabled)
    {
        *data = fullData[mLineCount];
    
        return;
    }
    
    BL_FLOAT x0 = mDataSelection[0];
    BL_FLOAT x1 = mDataSelection[2];
    
    if ((mLineCount < x0) || (mLineCount > x1) ||
        (mLineCount >= fullData.size()))
    {
        // Set to 0 outside x selection
        BLUtils::FillAllZero(data);
    }
    else
    {
        if (mLineCount < fullData.size())
        {
            // Original line
            *data = fullData[mLineCount];
        
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
