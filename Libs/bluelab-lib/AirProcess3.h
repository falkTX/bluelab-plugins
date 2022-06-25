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
//  AirProcess3.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Air__AirProcess3__
#define __BL_Air__AirProcess3__

#include <FftProcessObj16.h>
#include <PartialTracker3.h>

// From AirProcess
// - code clean (removed transient stuff)
//
// AirProcess3: use SoftMaskingComp4 (and use only a single one)

class PartialTracker5;
class SoftMaskingComp4;
class AirProcess3 : public ProcessObj
{
public:
    AirProcess3(int bufferSize,
                BL_FLOAT overlapping, BL_FLOAT oversampling,
                BL_FLOAT sampleRate);
    
    virtual ~AirProcess3();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate) override;
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void SetThreshold(BL_FLOAT threshold);
    void SetMix(BL_FLOAT mix);

    void SetUseSoftMasks(bool flag);
    
    int GetLatency();
    
    void GetNoise(WDL_TypedBuf<BL_FLOAT> *magns);
    void GetHarmo(WDL_TypedBuf<BL_FLOAT> *magns);
    void GetSum(WDL_TypedBuf<BL_FLOAT> *magns);

    void SetEnableSum(bool flag);
    
protected:
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases);

    // Compute mask for s0, from s0 and s1
    void ComputeMask(const WDL_TypedBuf<BL_FLOAT> &s0Buf,
                     const WDL_TypedBuf<BL_FLOAT> &s1Buf,
                     WDL_TypedBuf<BL_FLOAT> *s0Mask);
        
    //
    //int mBufferSize;
    //BL_FLOAT mOverlapping;
    //BL_FLOAT mOversampling;
    //BL_FLOAT mSampleRate;
    
    PartialTracker5 *mPartialTracker;
    
    BL_FLOAT mMix;

    bool mUseSoftMasks;
    
    SoftMaskingComp4 *mSoftMaskingComp;
    
    WDL_TypedBuf<BL_FLOAT> mNoise;
    WDL_TypedBuf<BL_FLOAT> mHarmo;
    WDL_TypedBuf<BL_FLOAT> mSum;

    bool mEnableComputeSum;
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    //WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    //WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf10[2];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf11;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf12;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf13;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf14;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf15;
    //WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf16;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf17;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf18;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf19;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf20;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf21;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf22;
};

#endif /* defined(__BL_Air__AirProcess3__) */
