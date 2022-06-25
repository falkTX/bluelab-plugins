/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  BLVumeterNeedleControl.hpp
//  UST-macOS
//
//  Created by applematuer on 9/30/20.
//
//

#ifndef BLVumeterNeedleControl_h
#define BLVumeterNeedleControl_h

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
                           float needleDepth,
                           int paramIdx = kNoParameter);
    
    virtual ~BLVumeterNeedleControl();
    
    void Draw(IGraphics& g) override;
    
protected:
    IColor mColor;
    
    // Needle depth, in pixels
    float mNeedleDepth;
};

#endif /* BLVumeterNeedleControl_hpp */
