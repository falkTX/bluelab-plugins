//
//  SMVProcessXComputerLissajous.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputerLissajous__
#define __BL_SoundMetaViewer__SMVProcessXComputerLissajous__

#include <SMVProcessXComputer.h>

// Polar vectorscope
class SMVProcessXComputerLissajous : public SMVProcessXComputer
{
public:
    SMVProcessXComputerLissajous();
    
    virtual ~SMVProcessXComputerLissajous();
    
    virtual void Reset(double sampleRate) {};
    
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


};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerLissajous__) */
