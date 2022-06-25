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
//  USTDepthReverbTest.h
//  BL-ReverbDepth
//
//  Created by applematuer on 8/31/20.
//
//

#ifndef __BL_ReverbDepth__USTDepthReverbTest__
#define __BL_ReverbDepth__USTDepthReverbTest__

#include <BLTypes.h>

#include <BLReverb.h>

// Test class, to setup USTDepthProcess
// (includes JReverb + additional filters)
class USTDepthProcess4;
class USTDepthReverbTest : public BLReverb
{
public:
    USTDepthReverbTest(BL_FLOAT sampleRate);
    
    USTDepthReverbTest(const USTDepthReverbTest &other);
    
    virtual ~USTDepthReverbTest();
    
    virtual BLReverb *Clone() const override;

    virtual void Reset(BL_FLOAT sampleRate, int blockSize) override;

    // Mono
    virtual void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR) override;

    // Stereo
    virtual void Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR) override;
    
    //
    void SetUseReverbTail(bool flag);
    
    void SetDry(BL_FLOAT dry);
    void SetWet(BL_FLOAT wet);
    void SetRoomSize(BL_FLOAT roomSize);
    void SetWidth(BL_FLOAT width);
    void SetDamping(BL_FLOAT damping);
    
    void SetUseFilter(bool flag);
    
    //
    void SetUseEarlyReflections(bool flag);
    
    void SetEarlyRoomSize(BL_FLOAT roomSize);
    void SetEarlyIntermicDist(BL_FLOAT dist);
    void SetEarlyNormDepth(BL_FLOAT depth);
    
    void SetEarlyOrder(int order);
    void SetEarlyReflectCoeff(BL_FLOAT reflectCoeff);
    
protected:
    USTDepthProcess4 *mDepthProcess;
};

#endif /* defined(__BL_ReverbDepth__USTDepthReverbTest__) */
