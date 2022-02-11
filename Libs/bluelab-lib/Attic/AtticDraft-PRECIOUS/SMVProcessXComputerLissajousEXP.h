//
//  SMVProcessXComputerLissajousEXP.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputerLissajousEXP__
#define __BL_SoundMetaViewer__SMVProcessXComputerLissajousEXP__

#include <SMVProcessXComputer.h>

// Derived from Lissajous (nut trick to get it in the frequency domain
// => Makes very nice patters with Sine + Precedence (when varying the delay)
// => Looks a bit like V-Jaying / Artistic sound visualizer
//
class SMVProcessXComputerLissajousEXP : public SMVProcessXComputer
{
public:
    SMVProcessXComputerLissajousEXP(double sampleRate);
    
    virtual ~SMVProcessXComputerLissajousEXP();
    
    virtual void Reset(double sampleRate);
    
    void ComputeX(const WDL_TypedBuf<double> samples[2],
                  const WDL_TypedBuf<double> magns[2],
                  const WDL_TypedBuf<double> phases[2],
                  WDL_TypedBuf<double> *resultX,
                  WDL_TypedBuf<double> *resultY = NULL,
                  bool *isPolar = NULL,
                  double polarCenter[2] = NULL,
                  bool *isScalable = NULL);
    
protected:
    void ComputeLissajous(const WDL_TypedBuf<double> samples[2],
                          WDL_TypedBuf<double> lissajousSamples[2],
                          bool fitInSquare);

    void ComputeLissajousFft(const WDL_TypedBuf<double> magns[2],
                             const WDL_TypedBuf<double> phases[2],
                             WDL_TypedBuf<double> lissajousSamples[2],
                             bool fitInSquare);
    
    //
    double mSampleRate;

};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerLissajousEXP__) */
