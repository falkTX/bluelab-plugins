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

// Standard Lissajous
// Can not be scaled by Y computers
class Axis3DFactory2;
class SMVProcessXComputerLissajous : public SMVProcessXComputer
{
public:
    SMVProcessXComputerLissajous(Axis3DFactory2 *axisFactory);
    
    virtual ~SMVProcessXComputerLissajous();
    
    virtual void Reset(BL_FLOAT sampleRate) {}
    
    void ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                  const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  WDL_TypedBuf<BL_FLOAT> *resultX,
                  WDL_TypedBuf<BL_FLOAT> *resultY = NULL,
                  bool *isPolar = NULL,
                  BL_FLOAT polarCenter[2] = NULL,
                  bool *isScalable = NULL);
    
    Axis3D *CreateAxis();
    
protected:
    void ComputeLissajous(const WDL_TypedBuf<BL_FLOAT> samples[2],
                          WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                          bool fitInSquare);
    
    //
    Axis3DFactory2 *mAxisFactory;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerLissajous__) */
