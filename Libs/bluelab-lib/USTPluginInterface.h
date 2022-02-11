//
//  USTPluginInterface.h
//  Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#ifndef USTPluginInterface_h
#define USTPluginInterface_h

#include <BLTypes.h>

class USTPluginInterface
{
public:
    //virtual void UpdateUpmixPanDepth(newPan, newDepth) = 0;
    virtual void UpdatePanDepthCB(BL_FLOAT pan, BL_FLOAT depth) = 0;
    //virtual void UpdateUpmixGain(drag) = 0;
    virtual void UpdateWidthCB(BL_FLOAT dWidth) = 0;
    
    virtual void UpdateVectorscopePanWidth(BL_FLOAT dpan, BL_FLOAT dwidth) = 0;
};

#endif /* USTPluginInterface_h */
