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
//  RebalanceDumpFftObj.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#ifndef RebalanceDumpFftObj_hpp
#define RebalanceDumpFftObj_hpp

#include <deque>
using namespace std;

#include <FftProcessObj16.h>

#include <Rebalance_defs.h>

// RebalanceDumpFftObj
class RebalanceDumpFftObj : public ProcessObj
{
public:
    /*class Slice
     {
     public:
     Slice();
     
     virtual ~Slice();
     
     void SetData(const deque<WDL_TypedBuf<double> > &mixCols,
     const deque<WDL_TypedBuf<double> > &sourceCols);
     
     void GetData(vector<WDL_TypedBuf<double> > *mixCols,
     vector<WDL_TypedBuf<double> > *sourceCols) const;
     
     protected:
     vector<WDL_TypedBuf<double> > mMixCols;
     vector<WDL_TypedBuf<double> > mSourceCols;
     };*/
    
    //
    
    RebalanceDumpFftObj(int bufferSize);
    
    virtual ~RebalanceDumpFftObj();
    
    virtual void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    // Addition, for normalized masks
    
    // Get spectrogram slices
    //void GetSlices(vector<Slice> *slices);
    
    //void ResetSlices();
    
    bool HasEnoughData();
    
    void GetData(WDL_TypedBuf<double> mixCols[REBALANCE_NUM_SPECTRO_COLS],
                 WDL_TypedBuf<double> sourceCols[REBALANCE_NUM_SPECTRO_COLS]);
    
protected:
    deque<WDL_TypedBuf<double> > mMixCols;
    deque<WDL_TypedBuf<double> > mSourceCols;
    
    //vector<Slice> mSlices;
};

#endif /* RebalanceDumpFftObj_hpp */
