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
//  AirProcess2.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Air__AirProcess2__
#define __BL_Air__AirProcess2__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include <PartialTracker3.h>

//#include "FftProcessObj15.h"
#include "FftProcessObj16.h"

#define AIR_PROCESS_PROFILE 0

#if AIR_PROCESS_PROFILE
#include <BlaTimer.h>
#endif

// From AirProcess
// - code clean (removed transient stuff)

//class PartialTracker3;
//class PartialTracker4;
class PartialTracker5;
class SoftMaskingComp3;
class AirProcess2 : public ProcessObj
{
public:
    AirProcess2(int bufferSize,
                BL_FLOAT overlapping, BL_FLOAT oversampling,
                BL_FLOAT sampleRate);
    
    virtual ~AirProcess2();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate) override;
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void SetThreshold(BL_FLOAT threshold);
    void SetMix(BL_FLOAT mix);
    
    int GetLatency();
    
    void GetNoise(WDL_TypedBuf<BL_FLOAT> *magns);
    void GetHarmo(WDL_TypedBuf<BL_FLOAT> *magns);
    void GetSum(WDL_TypedBuf<BL_FLOAT> *magns);
    
protected:
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases);
    
    /*int mBufferSize;
      BL_FLOAT mOverlapping;
      BL_FLOAT mOversampling;
      BL_FLOAT mSampleRate; */
    
    //PartialTracker3 *mPartialTracker;
    //PartialTracker4 *mPartialTracker;
    PartialTracker5 *mPartialTracker;
    
    BL_FLOAT mMix;
    BL_FLOAT mTransientSP;
    
    bool mDebugFreeze;
    
    // First is noise masking, second is harmo masking
    SoftMaskingComp3 *mSoftMaskingComps[2];
    
    WDL_TypedBuf<BL_FLOAT> mNoise;
    WDL_TypedBuf<BL_FLOAT> mHarmo;
    WDL_TypedBuf<BL_FLOAT> mSum;
    
#if AIR_PROCESS_PROFILE
    BlaTimer mTimer;
    long mCount;
#endif

private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf10[2];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf11;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf12;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf13;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf14;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf15;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf16;
};

#endif /* defined(__BL_Air__AirProcess2__) */
