//
//  USTPseudoStereoObj2.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__USTPseudoStereoObj2__
#define __UST__USTPseudoStereoObj2__

#include "IPlug_include_in_plug_hdr.h"

#define DEFAULT_WIDTH 1.0 //2.0 //1.0

// Implementation of
// "Downmix-compatible conversion from mono to stereo
// in time and frequency-domain" - Marco Fink
// See: https://www.researchgate.net/publication/283008516_Downmix-compatible_conversion_from_mono_to_stereo_in_time-_and_frequency-domain
//
// Renamed PseudoStereoObj2 => USTPseudoStereoObj

//
// USTPseudoStereoObj2: from USTPseudoStereoObj2
// - use FastRTConvolver2
//
class FastRTConvolver2;
class DelayObj4;

class USTPseudoStereoObj2
{
public:
    USTPseudoStereoObj2(double sampleRate, double width = DEFAULT_WIDTH);
    
    virtual ~USTPseudoStereoObj2();
    
    void Reset(double sampleRate, int blockSize);
    
    int GetLatency();
    
    void SetWidth(double width);
    
    void ProcessSamples(const WDL_TypedBuf<double> &sampsIn,
                        WDL_TypedBuf<double> *sampsOut0,
                        WDL_TypedBuf<double> *sampsOut1);
    
    void ProcessSamples(vector<WDL_TypedBuf<double> > *samplesVec);
    
protected:
    void GenerateIR(WDL_TypedBuf<double> *ir);
    
    void SetIRSize(double sampleRate);
    
    void UpdateDelay();
    
    void NormalizeIR(WDL_TypedBuf<double> *ir);
    
    void AdjustGain(WDL_TypedBuf<double> *samples);
    
    //
    int mIRSize;
    
    double mSampleRate;
    double mWidth;
    
    FastRTConvolver2 *mConvolver;
    
    DelayObj4 *mDelayObj;
};

#endif /* defined(__UST__USTPseudoStereoObj2__) */
