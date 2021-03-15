//
//  WavetableSynth.cpp
//  BL-SASViewer
//
//  Created by applematuer on 3/1/19.
//
//

#include <math.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "WavetableSynth.h"


#define EPS 1e-15


WavetableSynth::WavetableSynth(int bufferSize,
                               int overlapping,
                               BL_FLOAT sampleRate,
                               int precision,
                               BL_FLOAT minFreq)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mSampleRate = sampleRate;
    mPrecision = precision;
    mMinFreq = minFreq;
    
    ComputeTables();
}

WavetableSynth::~WavetableSynth() {}

void
WavetableSynth::Reset(BL_FLOAT sampleRate)
{
    if (sampleRate != mSampleRate)
    {
        mSampleRate = sampleRate;
    
        ComputeTables();
    }
}

void
WavetableSynth::GetSamplesNearest(WDL_TypedBuf<BL_FLOAT> *buffer,
                                  BL_FLOAT freq, BL_FLOAT amp)
{
    // For the moment, get the nearest
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    BL_FLOAT tableNum = ((freq - mMinFreq)/hzPerBin)*mPrecision;
    tableNum = bl_round(tableNum);
    
    if (tableNum < 0.0)
        tableNum = 0.0;
    if (tableNum > mTables.size() - 1)
        tableNum = mTables.size() - 1;
    
    const Table &table = mTables[tableNum];
    
    int numSamples = buffer->GetSize();
    if (numSamples > mBufferSize)
        numSamples = mBufferSize;
    
    memcpy(buffer->Get(),
           &table.mBuffer.Get()[(int)table.mCurrentPos],
           numSamples*sizeof(BL_FLOAT));
    
    // Apply amplitude if necessary
    if (std::fabs(amp - 1.0) > EPS)
    {
        for (int i = 0; i < buffer->GetSize(); i++)
        {
            BL_FLOAT val = buffer->Get()[i];
            
            val *= amp;
            
            buffer->Get()[i] = val;
        }
    }
}

void
WavetableSynth::GetSamplesLinear(WDL_TypedBuf<BL_FLOAT> *buffer,
                                 BL_FLOAT freq, BL_FLOAT amp)
{
    // For the moment, get the nearest
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    BL_FLOAT tableNum = ((freq - mMinFreq)/hzPerBin)*mPrecision;
    int tn0 = tableNum;
    int tn1 = tableNum + 1;
    if (tn1 > mTables.size() - 1)
        tn1 = mTables.size() - 1;
    
    BL_FLOAT t = tableNum - (int)tableNum;
    
    const Table &table0 = mTables[tn0];
    const Table &table1 = mTables[tn1];
    
    int numSamples = buffer->GetSize();
    if (numSamples > mBufferSize)
        numSamples = mBufferSize;
    
    WDL_TypedBuf<BL_FLOAT> buf0;
    buf0.Resize(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> buf1;
    buf1.Resize(numSamples);
    
    memcpy(buf0.Get(),
           &table0.mBuffer.Get()[(int)table0.mCurrentPos],
           numSamples*sizeof(BL_FLOAT));
    
    memcpy(buf1.Get(),
           &table1.mBuffer.Get()[(int)table1.mCurrentPos],
           numSamples*sizeof(BL_FLOAT));
    
    for (int i = 0; i < buffer->GetSize(); i++)
    {
        BL_FLOAT val0 = buf0.Get()[i];
        BL_FLOAT val1 = buf1.Get()[i];
        
        BL_FLOAT val = (1.0 - t)*val0 + t*val1;
        buffer->Get()[i] = val;
    }
    
    // Apply amplitude if necessary
    if (std::fabs(amp - 1.0) > EPS)
    {
        for (int i = 0; i < buffer->GetSize(); i++)
        {
            BL_FLOAT val = buffer->Get()[i];
            
            val *= amp;
            
            buffer->Get()[i] = val;
        }
    }
}

BL_FLOAT
WavetableSynth::GetSampleNearest(int idx, BL_FLOAT freq, BL_FLOAT amp)
{
    // For the moment, get the nearest
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    BL_FLOAT tableNum = ((freq - mMinFreq)/hzPerBin)*mPrecision;
    tableNum = bl_round(tableNum);
    
    if (tableNum < 0.0)
        tableNum = 0.0;
    if (tableNum > mTables.size() - 1)
        tableNum = mTables.size() - 1;
    
    const Table &table = mTables[tableNum];
    
    BL_FLOAT sample = table.mBuffer.Get()[(int)table.mCurrentPos + idx];
    
    // Apply amplitude if necessary
    if (std::fabs(amp - 1.0) > EPS)
    {
        sample *= amp;
    }
    
    return sample;
}

BL_FLOAT
WavetableSynth::GetSampleLinear(int idx, BL_FLOAT freq, BL_FLOAT amp)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    BL_FLOAT tableNum = ((freq - mMinFreq)/hzPerBin)*mPrecision;
    int tn0 = tableNum;
    int tn1 = tableNum + 1;
    if (tn1 > mTables.size() - 1)
        tn1 = mTables.size() - 1;
    
    BL_FLOAT t = tableNum - (int)tableNum;
    
    const Table &table0 = mTables[tn0];
    const Table &table1 = mTables[tn1];
    
    BL_FLOAT sample0 = table0.mBuffer.Get()[(int)table0.mCurrentPos + idx];
    BL_FLOAT sample1 = table1.mBuffer.Get()[(int)table1.mCurrentPos + idx];
    
    BL_FLOAT sample = (1.0 - t)*sample0 + t*sample1;;
    
    // Apply amplitude if necessary
    if (std::fabs(amp - 1.0) > EPS)
    {
        sample *= amp;
    }
    
    return sample;
}

void
WavetableSynth::NextBuffer()
{
    int tableSize = (((BL_FLOAT)mSampleRate)/mMinFreq)*2 + 1;
    
    for (int i = 0; i < mTables.size(); i++)
    {
        Table &table = mTables[i];
        
        // Increase to the next pos
        table.mCurrentPos += mBufferSize/mOverlapping;
        
        // Make a modulo to return back,
        // at a position with the same "phase"
        BL_FLOAT periodSamples = mSampleRate/table.mFrequency;
        periodSamples = bl_round(periodSamples);
        if (periodSamples < 0.0)
            periodSamples = 0.0;
        
        // WARNING: can surely make discontinuities
        // (because cycle length is not exactly a multiple of the period)
        //table.mCurrentPos = table.mCurrentPos % (int)periodSamples;
        table.mCurrentPos = std::fmod(table.mCurrentPos, periodSamples);
        if (table.mCurrentPos > tableSize)
        {
            // Error, table is too small
            table.mCurrentPos = 0.0;
        }
    }
}

// TODO: optimize memory:
// - varying tables size (big size for low freqs, little size for high freqs)
void
WavetableSynth::ComputeTables()
{
    mTables.clear();
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    //int tableSize = (((BL_FLOAT)mBufferSize)/mMinFreq)*2;
    int tableSize = (((BL_FLOAT)mSampleRate)/mMinFreq)*2 + 1;
    
    int numTables = (mBufferSize/2)*mPrecision;
    
    for (int i = 0; i < numTables; i++)
    {
        Table table;
        
        BL_FLOAT freq = (((BL_FLOAT)i)/mPrecision)*hzPerBin + mMinFreq;
        
        table.mFrequency = freq;
        table.mCurrentPos = 0.0;
        table.mBuffer.Resize(tableSize);
        
        for (int j = 0; j < table.mBuffer.GetSize(); j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/mSampleRate;
            
            BL_FLOAT val = std::sin(2.0*M_PI*freq*t);
            
            table.mBuffer.Get()[j] = val;
        }
        
        mTables.push_back(table);
    }
}
