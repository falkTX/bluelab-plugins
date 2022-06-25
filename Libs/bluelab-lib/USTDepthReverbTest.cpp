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
//  USTDepthReverbTest.cpp
//  BL-ReverbDepth
//
//  Created by applematuer on 8/31/20.
//
//

#include <USTDepthProcess4.h>

#include "USTDepthReverbTest.h"

USTDepthReverbTest::USTDepthReverbTest(BL_FLOAT sampleRate)
{
    mDepthProcess = new USTDepthProcess4(sampleRate);
}

USTDepthReverbTest::USTDepthReverbTest(const USTDepthReverbTest &other)
{
    mDepthProcess = new USTDepthProcess4(*other.mDepthProcess);
}

USTDepthReverbTest::~USTDepthReverbTest()
{
    delete mDepthProcess;
}

BLReverb *
USTDepthReverbTest::Clone() const
{
    USTDepthReverbTest *clone = new USTDepthReverbTest(*this);
    
    return clone;
}

void
USTDepthReverbTest::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mDepthProcess->Reset(sampleRate, blockSize);
}

void
USTDepthReverbTest::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                            WDL_TypedBuf<BL_FLOAT> *outputL,
                            WDL_TypedBuf<BL_FLOAT> *outputR)
{
    mDepthProcess->Process(input, outputL, outputR);
}

void
USTDepthReverbTest::Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                            WDL_TypedBuf<BL_FLOAT> *outputL,
                            WDL_TypedBuf<BL_FLOAT> *outputR)
{
    mDepthProcess->Process(inputs, outputL, outputR);
}

void
USTDepthReverbTest::SetUseReverbTail(bool flag)
{
    mDepthProcess->SetUseReverbTail(flag);
}

void
USTDepthReverbTest::SetDry(BL_FLOAT dry)
{
    mDepthProcess->SetDry(dry);
}

void
USTDepthReverbTest::SetWet(BL_FLOAT wet)
{
    mDepthProcess->SetWet(wet);
}

void
USTDepthReverbTest::SetRoomSize(BL_FLOAT roomSize)
{
    mDepthProcess->SetRoomSize(roomSize);
}

void
USTDepthReverbTest::SetWidth(BL_FLOAT width)
{
    mDepthProcess->SetWidth(width);
}

void
USTDepthReverbTest::SetDamping(BL_FLOAT damping)
{
    mDepthProcess->SetDamping(damping);
}

void
USTDepthReverbTest::SetUseFilter(bool flag)
{
    mDepthProcess->SetUseFilter(flag);
}

void
USTDepthReverbTest::SetUseEarlyReflections(bool flag)
{
    mDepthProcess->SetUseEarlyReflections(flag);
}

void
USTDepthReverbTest::SetEarlyRoomSize(BL_FLOAT roomSize)
{
    mDepthProcess->SetEarlyRoomSize(roomSize);
}

void
USTDepthReverbTest::SetEarlyIntermicDist(BL_FLOAT dist)
{
    mDepthProcess->SetEarlyIntermicDist(dist);
}

void
USTDepthReverbTest::SetEarlyNormDepth(BL_FLOAT depth)
{
    mDepthProcess->SetEarlyNormDepth(depth);
}

void
USTDepthReverbTest::SetEarlyOrder(int order)
{
    mDepthProcess->SetEarlyOrder(order);
}

void
USTDepthReverbTest::SetEarlyReflectCoeff(BL_FLOAT reflectCoeff)
{
    mDepthProcess->SetEarlyReflectCoeff(reflectCoeff);
}
