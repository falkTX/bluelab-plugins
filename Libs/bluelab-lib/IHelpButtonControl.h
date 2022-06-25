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
//  IHelpButtonControl.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#ifndef IHelpButtonControl_h
#define IHelpButtonControl_h

#include <IControl.h>

using namespace iplug::igraphics;

class IHelpButtonControl : public IBitmapControl
{
public:
    IHelpButtonControl(float x, float y, const IBitmap &bitmap, int paramIdx,
                       const char *fileName,
                       EBlend blend = EBlend::Default)
    : IBitmapControl(x, y, bitmap, paramIdx, blend)
    {
        sprintf(mFileName, "%s", fileName);
    }
    
    virtual ~IHelpButtonControl() {}
    
    void OnMouseDown(float x, float y, const IMouseMod &mod) override;

    static void ShowManual(const char *fileName);
    
protected:
    char mFileName[1024];
};


#endif /* IHelpButtonControl_h */
