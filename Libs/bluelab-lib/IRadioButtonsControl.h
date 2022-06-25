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
//  IRadioButtonControls.h
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#ifndef IRadioButtonControls_h
#define IRadioButtonControls_h

#include <stdlib.h>

#include <vector>
using namespace std;

#include <IControl.h>
#include <IPlugConstants.h>

using namespace iplug;
using namespace iplug::igraphics;

class IRadioButtonsControl : public IControl
{
public:
    IRadioButtonsControl(IRECT pR, int paramIdx,
                         int nButtons, const IBitmap &bitmap,
                         EDirection direction = EDirection::Vertical,
                         bool reverse = false);
    virtual ~IRadioButtonsControl() {}
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    
    virtual void Draw(IGraphics &g) override;
    
    virtual void GetRects(vector<IRECT> *rects)
    {
        *rects = mRECTs;
    }
    
protected:
    vector<IRECT> mRECTs;
    IBitmap mBitmap;
};

#endif /* IRadioButtonControls_h */
