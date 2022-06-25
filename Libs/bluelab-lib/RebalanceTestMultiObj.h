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
//  RebalanceTestMultiObj.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#ifndef RebalanceTestMultiObj_hpp
#define RebalanceTestMultiObj_hpp

#include <FftProcessObj16.h>

#include "IPlug_include_in_plug_hdr.h"

#if TEST_DNN_INPUT

class RebalanceDumpFftObj;
class RebalanceTestMultiObj : public MultichannelProcess
{
public:
    RebalanceTestMultiObj(RebalanceDumpFftObj *rebalanceDumpFftObjs[4],
                          double sampleRate);
    
    virtual ~RebalanceTestMultiObj();
    
    void Reset() override;
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, double sampleRate) override;
    
    void
    ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                     const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) override;
    
protected:
    double mSampleRate;
    
    RebalanceDumpFftObj *mRebalanceDumpFftObjs[4];
    
    int mDumpCount;
};

#endif

#endif /* RebalanceTestMultiObj_hpp */
