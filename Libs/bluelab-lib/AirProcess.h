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
//  AirProcess.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Air__AirProcess__
#define __BL_Air__AirProcess__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include <PartialTracker3.h>

#include <BlaTimer.h>

#include <FftProcessObj16.h>


class PartialTracker3;
//class TransientShaperFftObj3;
class TransientLib5;
class AirProcess : public ProcessObj
{
public:
    AirProcess(int bufferSize,
               BL_FLOAT overlapping, BL_FLOAT oversampling,
               BL_FLOAT sampleRate);
    
    virtual ~AirProcess();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void SetThreshold(BL_FLOAT threshold);
    void SetMix(BL_FLOAT mix);
    void SetTransientSP(BL_FLOAT transientSP);
    
protected:
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void ComputeTransientness(const WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    //int mBufferSize;
    //BL_FLOAT mOverlapping;
    //BL_FLOAT mOversampling;
    //BL_FLOAT mSampleRate;
    
    PartialTracker3 *mPartialTracker;
    
    //TransientShaperFftObj3 *mSTransientObj;
    //TransientShaperFftObj3 *mPTransientObj;
    
    BL_FLOAT mMix;
    BL_FLOAT mTransientSP;
    
    // Transientness
    WDL_TypedBuf<BL_FLOAT> mSPRatio;
    
    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
    
    bool mDebugFreeze;

    TransientLib5 *mTransLib;
};

#endif /* defined(__BL_Air__AirProcess__) */
