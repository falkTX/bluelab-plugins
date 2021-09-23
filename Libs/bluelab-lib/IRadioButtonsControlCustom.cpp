//
//  IRadioButtonControls.cpp
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#include "IRadioButtonsControlCustom.h"

IRadioButtonsControlCustom::
IRadioButtonsControlCustom(IRECT pR,
                           int paramIdx, int nButtons,
                           const IBitmap bitmaps[],
                           EDirection direction, bool reverse)
:   IControl(pR, paramIdx)
{
    mBitmaps.resize(nButtons);
    for (int i = 0; i < nButtons; i++)
        mBitmaps[i] = bitmaps[i];
    
    mRECTs.resize(nButtons);

    //int h = int((float)bitmaps[0].H() / (float)bitmaps[0].N());
    float hf = bitmaps[0].H();
    if (bitmaps[0].N() > 0)
        hf /= bitmaps[0].N();
    int h = (int)hf;
    
    if (reverse)
    {
        if (direction == EDirection::Horizontal)
        {
            int dX = int((float) (pR.W() - nButtons * bitmaps[0].W()) /
                         (float) (nButtons - 1));
            int x = mRECT.R - bitmaps[0].W() - dX;
            int y = mRECT.T;
            
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmaps[0].W(), y + h);
                x -= bitmaps[0].W() + dX;
            }
        }
        else
        {
            int dY = int((float) (pR.H() - nButtons * h) /  (float) (nButtons - 1));
            int x = mRECT.L;
            int y = mRECT.B - h - dY;
            
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmaps[0].W(), y + h);
                y -= h + dY;
            }
        }
        
    }
    else
    {
        int x = mRECT.L, y = mRECT.T;
        
        if (direction == EDirection::Horizontal)
        {
            int dX = int((float) (pR.W() - nButtons * bitmaps[0].W()) /
                         (float) (nButtons - 1));
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmaps[0].W(), y + h);
                x += bitmaps[0].W() + dX;
            }
        }
        else
        {
            int dY = int((float) (pR.H() - nButtons * h) /  (float) (nButtons - 1));
            for (int i = 0; i < nButtons; ++i)
            {
                mRECTs[i] = IRECT(x, y, x + bitmaps[0].W(), y + h);
                y += h + dY;
            }
        }
    }

    mMouseOver.resize(nButtons);
    for (int i = 0; i < nButtons; i++)
        mMouseOver[i] = false;
}

void IRadioButtonsControlCustom::OnMouseDown(float x, float y, const IMouseMod &mod)
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
IRadioButtonsControlCustom::OnMouseOver(float x, float y, const IMouseMod &mod)
{
    for (int i = 0; i < mMouseOver.size(); i++)
    {
        bool over = false;
        if (mRECTs[i].Contains(x, y))
            over = true;
        
        mMouseOver[i] = over;
    }

    SetDirty();
}

void
IRadioButtonsControlCustom::OnMouseOut()
{
    bool prevMouseIn = false;
    for (int i = 0; i < mMouseOver.size(); i++)
    {
        if (mMouseOver[i])
        {
            prevMouseIn = true;
            break;
        }
    }
    if (!prevMouseIn)
        return;
    
    for (int i = 0; i < mMouseOver.size(); i++)
        mMouseOver[i] = false;

    SetDirty();
}
    
void
IRadioButtonsControlCustom::Draw(IGraphics &g)
{
    int n = (int)mRECTs.size();
    int active = int(0.5 + GetValue() * (float) (n - 1));
    
    if (active < 0)
        active = 0;
    if (active > n - 1)
        active = n - 1;
    
    for (int i = 0; i < n; ++i)
    {
        float h = mBitmaps[i].H();
        if (mBitmaps[i].N() > 0)
            h /= mBitmaps[i].N();

        if (i == active)
        {
            // Last one
            g.DrawBitmap(mBitmaps[i], mRECTs[i], 0.0,
                         mBitmaps[i].H() - h,
                         &mBlend);
        }
        else
        {
            // 0 or 1
            if (!mMouseOver[i])
                g.DrawBitmap(mBitmaps[i], mRECTs[i], 0.0,
                             0.0, // 0
                             &mBlend);
            else
                g.DrawBitmap(mBitmaps[i], mRECTs[i], 0.0,
                             h, // 1
                             &mBlend);
        }
    }
}
