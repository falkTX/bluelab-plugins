//
//  BLVumeterControl.hpp
//  UST-macOS
//
//  Created by applematuer on 9/30/20.
//
//

#ifndef BLVumeterControl_h
#define BLVumeterControl_h

#include <IGraphics.h>
#include <IControl.h>
#include <IGraphicsStructs.h>
#include <IPlugConstants.h>

using namespace iplug;
using namespace iplug::igraphics;

// Vector vumeter
class BLVumeterControl : public IControl
{
public:
    BLVumeterControl(IRECT &rect,
                     const IColor &color,
                     int paramIdx = kNoParameter);
    
    virtual ~BLVumeterControl();
    
    void Draw(IGraphics& g) override;
    
protected:
    IColor mColor;
};

#endif /* BLVumeterControl_hpp */
