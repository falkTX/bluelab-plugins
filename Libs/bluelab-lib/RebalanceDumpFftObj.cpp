//
//  RebalanceDumpFftObj.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include "RebalanceDumpFftObj.h"

#define DUMP_OVERLAP 1 // 0

// Slice
/*RebalanceDumpFftObj::Slice::Slice() {}
 
 RebalanceDumpFftObj::Slice::~Slice() {}
 
 void
 RebalanceDumpFftObj::Slice::SetData(const deque<WDL_TypedBuf<double> > &mixCols,
 const deque<WDL_TypedBuf<double> > &sourceCols)
 {
 mMixCols.resize(mixCols.size());
 for (int i = 0; i < mMixCols.size(); i++)
 mMixCols[i] = mixCols[i];
 
 mSourceCols.resize(sourceCols.size());
 for (int i = 0; i < mSourceCols.size(); i++)
 mSourceCols[i] = sourceCols[i];
 }
 
 void
 RebalanceDumpFftObj::Slice::GetData(vector<WDL_TypedBuf<double> > *mixCols,
 vector<WDL_TypedBuf<double> > *sourceCols) const
 {
 *mixCols = mMixCols;
 *sourceCols = mSourceCols;
 }
 */

RebalanceDumpFftObj::RebalanceDumpFftObj(int bufferSize)
: ProcessObj(bufferSize)
{
    // Fill with zeros at the beginning
    mMixCols.resize(REBALANCE_NUM_SPECTRO_COLS);
    for (int i = 0; i < mMixCols.size(); i++)
    {
        BLUtils::ResizeFillZeros(&mMixCols[i], REBALANCE_BUFFER_SIZE/2);
    }
    
    mSourceCols.resize(REBALANCE_NUM_SPECTRO_COLS);
    for (int i = 0; i < mSourceCols.size(); i++)
    {
        BLUtils::ResizeFillZeros(&mSourceCols[i], REBALANCE_BUFFER_SIZE/2);
    }
}

RebalanceDumpFftObj::~RebalanceDumpFftObj() {}

void
RebalanceDumpFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                      const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    if (scBuffer == NULL)
        return;
    
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    WDL_TypedBuf<double> magnsMix;
    BLUtilsComp::ComplexToMagn(&magnsMix, mixBuffer);
    
    mMixCols.push_back(magnsMix);
    //if (mMixCols.size() > NUM_INPUT_COLS)
    //    mMixCols.pop_front();
    
    // Source
    WDL_TypedBuf<WDL_FFT_COMPLEX> sourceBuffer = *scBuffer;
    BLUtils::TakeHalf(&sourceBuffer);
    
    WDL_TypedBuf<double> magnsSource;
    BLUtilsComp::ComplexToMagn(&magnsSource, sourceBuffer);
    
    mSourceCols.push_back(magnsSource);
    //if (mSourceCols.size() > NUM_INPUT_COLS)
    //    mSourceCols.pop_front();
    
#if !DUMP_OVERLAP
    // Do not take every (overlapping) steps of mix cols
    // Take non-overlapping slices of spectrogram
    if (mMixCols.size() == NUM_INPUT_COLS)
    {
        Slice slice;
        slice.SetData(mMixCols, mSourceCols);
        mSlices.push_back(slice);
        
        mMixCols.clear();
        mSourceCols.clear();
    }
#else
    //Slice slice;
    //slice.SetData(mMixCols, mSourceCols);
    //mSlices.push_back(slice);
#endif
}

/*void
 RebalanceDumpFftObj::GetSlices(vector<Slice> *slices)
 {
 *slices = mSlices;
 }
 
 void
 RebalanceDumpFftObj::ResetSlices()
 {
 mSlices.clear();
 }*/

bool
RebalanceDumpFftObj::HasEnoughData()
{
    bool hasEnoughData = (mMixCols.size() >= REBALANCE_NUM_SPECTRO_COLS);
    
    return hasEnoughData;
}

void
RebalanceDumpFftObj::GetData(WDL_TypedBuf<double> mixCols[REBALANCE_NUM_SPECTRO_COLS],
                             WDL_TypedBuf<double> sourceCols[REBALANCE_NUM_SPECTRO_COLS])
{
    for (int i = 0; i < REBALANCE_NUM_SPECTRO_COLS; i++)
    {
        mixCols[i] = mMixCols[i];
        sourceCols[i] = mSourceCols[i];
    }
    
    for (int i = 0; i < REBALANCE_NUM_SPECTRO_COLS; i++)
    {
        mMixCols.pop_front();
        mSourceCols.pop_front();
    }
}
