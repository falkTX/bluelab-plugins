//
//  USTProcess.h
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __UST__USTProcess__
#define __UST__USTProcess__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class USTWidthAdjuster;
class DelayObj4;

class USTProcess
{
public:
    static void ComputePolarSamples(const WDL_TypedBuf<double> samples[2],
                                    WDL_TypedBuf<double> polarSamples[2]);

    static double ComputeCorrelation(const WDL_TypedBuf<double> samples[2]);
    
    static void StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples,
                            double widthFactor);
    
    static void StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples,
                            const USTWidthAdjuster *widthAdjuster);
    
    // Correct mehod for balance (no pan law)
    static void Balance(vector<WDL_TypedBuf<double> * > *ioSamples,
                        double balance);
    
    // Balance with pan low 0dB
    static void Balance0(vector<WDL_TypedBuf<double> * > *ioSamples,
                         double balance);
    
    // Balance with pan low -3dB
    static void Balance3(vector<WDL_TypedBuf<double> * > *ioSamples,
                         double balance);
    
    static void MonoToStereo(vector<WDL_TypedBuf<double> > *samplesVec,
                             DelayObj4 *delayObj);
    
    static void StereoToMono(vector<WDL_TypedBuf<double> > *samplesVec);
    
protected:
    static void StereoWiden(double *left, double *right, double widthFactor);
    
    static double ComputeFactor(double normVal, double maxVal);
};

#endif /* defined(__UST__USTProcess__) */
