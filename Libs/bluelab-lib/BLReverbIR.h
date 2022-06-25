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
//  BLReverbIR.h
//  BL-Reverb
//
//  Created by applematuer on 1/17/20.
//
//

#ifndef __BL_Reverb__BLReverbIR__
#define __BL_Reverb__BLReverbIR__

class BLReverb;
class FastRTConvolver3;

class BLReverbIR
{
public:
    BLReverbIR(BLReverb *reverb, BL_FLOAT sampleRate,
               BL_FLOAT IRLengthSeconds);
    
    virtual ~BLReverbIR();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void UpdateIRs();
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &sampsIn,
                 WDL_TypedBuf<BL_FLOAT> *sampsOut0,
                 WDL_TypedBuf<BL_FLOAT> *sampsOut1);
    
protected:
    BL_FLOAT mSampleRate;
    BL_FLOAT mIRLengthSeconds;
    
    BLReverb *mReverb;
    
    FastRTConvolver3 *mConvolvers[2];
};

#endif /* defined(__BL_Reverb__BLReverbIR__) */
