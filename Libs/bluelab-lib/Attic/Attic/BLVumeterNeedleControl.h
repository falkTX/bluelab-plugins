//
//  BLVumeterNeedleControl.hpp
//  UST-macOS
//
//  Created by applematuer on 9/30/20.
//
//

#ifndef BLVumeterNeedleControl_h
#define BLVumeterNeedleControl_h

//#include "IGraphics_include_in_plug_hdr.h"
#include <IGraphics.h>
#include <IControl.h>
#include <IGraphicsStructs.h>
#include <IPlugConstants.h>

using namespace iplug;
using namespace iplug::igraphics;

// Vector vumeter
class BLVumeterNeedleControl : public IControl
{
public:
    BLVumeterNeedleControl(IRECT &rect,
                           const IColor &color,
                           const IColor &bgColor,
                           float needleDepth,
                           int paramIdx = kNoParameter);
    
    virtual ~BLVumeterNeedleControl();
    
    void Draw(IGraphics& g) override;
    
protected:
    IColor mColor;
    IColor mBgColor;
    
    float mNeedleDepth;
};

#endif /* BLVumeterNeedleControl_hpp */
