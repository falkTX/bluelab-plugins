//
//  StereoWidenProcess.h
//  Panogram
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __Panogram__StereoWidenProcess__
#define __Panogram__StereoWidenProcess__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class DelayObj4;
class ParamSmoother;

// From USTProcess
class StereoWidenProcess
{
public:
    static void StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                            BL_FLOAT widthFactor);
    
    static void StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                            ParamSmoother *widthFactorSmoother);
    
    // For SoundMetaViewer
    static void StereoWiden(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioSamples,
                            BL_FLOAT widthFactor);
    
    // Correct mehod for balance (no pan law)
    static void Balance(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                        BL_FLOAT balance);
    
    static void Balance(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                        ParamSmoother *balanceSmoother);
    
    static void MonoToStereo(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec,
                             DelayObj4 *delayObj);

protected:
    static void StereoToMono(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec);
    
    static void StereoWiden(BL_FLOAT *left, BL_FLOAT *right, BL_FLOAT widthFactor);
    
    static BL_FLOAT ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal);
    
    static BL_FLOAT ApplyAngleShape(BL_FLOAT angle, BL_FLOAT shape);
    
#if STEREO_WIDEN_COMPLEX_OPTIM
    static WDL_FFT_COMPLEX StereoWidenComputeAngle0();
    static WDL_FFT_COMPLEX StereoWidenComputeAngle1();
    
    static void StereoWiden(BL_FLOAT *left, BL_FLOAT *right, BL_FLOAT widthFactor,
                            const WDL_FFT_COMPLEX &angle0, WDL_FFT_COMPLEX &angle1);
#endif

    // For SoundMetaViewer
    static void StereoWiden(WDL_FFT_COMPLEX *left, WDL_FFT_COMPLEX *right, BL_FLOAT widthFactor);
};

#endif /* defined(__UST__USTProcess__) */
