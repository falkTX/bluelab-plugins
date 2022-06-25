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
//  CMASmoother.h
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#ifndef __Transient__CMASmoother__
#define __Transient__CMASmoother__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

// Central moving average smoother
class CMASmoother
{
public:
    CMASmoother(int bufferSize, int windowSize);
    
    virtual ~CMASmoother();

    void Reset();

    void Reset(int bufferSize, int windowSize);
    
    // Return true if nFrames has been returned
    bool Process(const BL_FLOAT *data, BL_FLOAT *smoothedData, int nFrames);

    // NOTE: removed the static attribute,
    // to optimize memory bey re-using buffers
    
    // Process one buffer, without managing streaming to next buffers
    // Fill the missing input data with zeros
    //template <typename FLOAT_TYPE>
    ///*static*/ bool ProcessOne(const FLOAT_TYPE *data, FLOAT_TYPE *smoothedData,
    //                           int nFrames, int windowSize);

    /*static*/ bool ProcessOne(const BL_FLOAT *data, BL_FLOAT *smoothedData,
                               int nFrames, int windowSize);
    
protected:
    bool ProcessInternal(const BL_FLOAT *data, BL_FLOAT *smoothedData, int nFrames);
    
    // Return true if something has been processed
    bool CentralMovingAverage(WDL_TypedBuf<BL_FLOAT> &inData,
                              WDL_TypedBuf<BL_FLOAT> &outData, int windowSize);
    
    // Hack to avoid bad offset at the end of a signal
    // (This was detected in Transient, when the transient vu-meters didn't goes to 0 when no signal).
    //
    // When the signal goes to 0, an offset is remaining in the Smoother
    // (due to mPrevVal and the two points, left and right, who have the same value).
    // To avoid that, detect when the data value are constant, then update mPrevVal accordingly
    void ManageConstantValues(const BL_FLOAT *data, int nFrames);

    //
    int mBufferSize;
    int mWindowSize;
    
    bool mFirstTime;
    
    BL_FLOAT mPrevVal;
    
    WDL_TypedBuf<BL_FLOAT> mInData;
    WDL_TypedBuf<BL_FLOAT> mOutData;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
};

#endif /* defined(__Transient__CMASmoother__) */
