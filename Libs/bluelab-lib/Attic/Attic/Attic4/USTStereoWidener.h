//
//  USTStereoWidener.h
//  UST
//
//  Created by applematuer on 1/3/20.
//
//

#ifndef __UST__USTStereoWidener__
#define __UST__USTStereoWidener__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

class USTWidthAdjuster9;

class USTStereoWidener
{
public:
    USTStereoWidener();
    
    virtual ~USTStereoWidener();
    
    void StereoWiden(BL_FLOAT *l, BL_FLOAT *r, BL_FLOAT widthFactor) const;
    
    void StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                     BL_FLOAT widthFactor) const;
    
    void StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                     USTWidthAdjuster9 *widthAdjuster) const;
    
protected:
    BL_FLOAT ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal) const;
    
    void  StereoWiden(BL_FLOAT *left, BL_FLOAT *right, BL_FLOAT widthFactor,
                      const WDL_FFT_COMPLEX &angle0,
                      const WDL_FFT_COMPLEX &angle1) const;
    
    static WDL_FFT_COMPLEX ComputeAngle0();
    static WDL_FFT_COMPLEX ComputeAngle1();
    
    
    WDL_FFT_COMPLEX mAngle0;
    WDL_FFT_COMPLEX mAngle1;
};

#endif /* defined(__UST__USTStereoWidener__) */
