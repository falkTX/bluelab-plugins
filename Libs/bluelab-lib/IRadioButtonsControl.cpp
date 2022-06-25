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
//  IRadioButtonControls.cpp
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#include "IRadioButtonsControl.h"

IRadioButtonsControl::IRadioButtonsControl(IRECT pR,
                                           int paramIdx, int nButtons,
                                           const IBitmap &bitmap,
                                           EDirection direction, bool reverse)
:   IControl(pR, paramIdx), mBitmap(bitmap)
{
    mRECTs.resize(nButtons);
    //int h = int((float) bitmap.H() / (float) bitmap.N());
    int hf = bitmap.H();
    if (bitmap.N() > 0)
        hf /= bitmap.N();
    int h = (int)hf;

    if (reverse)
    {
        if (direction == EDirection::Horizontal)
        {
            int dX = int((float) (pR.W() - nButtons * bitmap.W()) / (float) (nButtons - 1));
            int x = mRECT.R - bitmap.W() - dX;
            int y = mRECT.T;
            
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmap.W(), y + h);
                x -= bitmap.W() + dX;
            }
        }
        else
        {
            int dY = int((float) (pR.H() - nButtons * h) /  (float) (nButtons - 1));
            int x = mRECT.L;
            int y = mRECT.B - h - dY;
            
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmap.W(), y + h);
                y -= h + dY;
            }
        }
        
    }
    else
    {
        int x = mRECT.L, y = mRECT.T;
        
        if (direction == EDirection::Horizontal)
        {
            int dX = int((float) (pR.W() - nButtons * bitmap.W()) / (float) (nButtons - 1));
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmap.W(), y + h);
                x += bitmap.W() + dX;
            }
        }
        else
        {
            int dY = int((float) (pR.H() - nButtons * h) /  (float) (nButtons - 1));
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmap.W(), y + h);
                y += h + dY;
            }
        }
    }
}

void IRadioButtonsControl::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    IControl::OnMouseDown(x, y, mod);
    
    if (mod.A) // Previously reset, we are done !
        return;
    
#ifdef PROTOOLS
    if (mod.A)
    {
        if (mDefaultValue >= 0.0)
        {
            mValue = mDefaultValue;
            SetDirty();
            return;
        }
    }
    else
#endif
        
    if (mod.R)
    {
        PromptUserInput();
        return;
    }
    
    int i, n = mRECTs.size();
    
    for (i = 0; i < n; ++i)
    {
        if (mRECTs[i].Contains(x, y))
        {
            SetValue((float) i / (float) (n - 1));
            break;
        }
    }
    
    SetDirty();
}

void
IRadioButtonsControl::Draw(IGraphics &g)
{
    int n = (int)mRECTs.size();
    int active = int(0.5 + GetValue() * (float) (n - 1));
    
    if (active < 0)
        active = 0;
    if (active > n - 1)
        active = n - 1;
    
    for (int i = 0; i < n; ++i)
    {
        if (i == active)
        {
            // 2
            g.DrawBitmap(mBitmap, mRECTs[i], 0.0, mBitmap.H()/2, &mBlend);
        }
        else
        {
            // 1
            g.DrawBitmap(mBitmap, mRECTs[i], 0.0, 0.0, &mBlend);
        }
    }
}
