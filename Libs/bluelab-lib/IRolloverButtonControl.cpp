//
//  IRolloverButtonControl.cpp
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#include "IRolloverButtonControl.h"

// When re-opening the plugin after having clicked the button,
// the button was hilighted
#define FIX_HILIGHT_REOPEN 1

void
IRolloverButtonControl::Draw(IGraphics &g)
{
    IBitmapControl::Draw(g);
    
    // Draw, then update the text color if necessary
    if (mText != NULL)
    {
        if (GetValue() <= 0.5)
            mText->SetTextColor(mTextColor);
        else
        {
            // Hilight text too when just clicking?
            //if (mToggleFlag)
            mText->SetTextColor(mTextHilightColor);
        }
    }
}

// WARNING: there was a modification in IPlugAAX for meta parameters
// (i.e the onces associated with RolloverButtons)
void
IRolloverButtonControl::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    IBitmapControl::OnMouseDown(x, y, mod);
    if (mod.A) // Previously reset, we are done!
        return;
    
    if (mToggleFlag)
    {
        // Force value change
        // (otherwise, the button would have worked only one time)
        if ((GetValue() > 0.5) && mToggleOffFlag)
        {
            // Set hilight text before value
            // because SetValueFromUserInput() can lock the GUI by
            // opening a file chooser
            //if (mText != NULL)
            //{
            //    mText->SetTextColor(mTextColor);
            //    mText->SetDirty();
            //}
            
            // Set value after hilight text
            SetValueFromUserInput(0.0);
        }
        else
        {
            //if (mText != NULL)
            //{
            //    mText->SetTextColor(mTextHilightColor);
            //    mText->SetDirty();
            //}
            
            // Special function, to hilight the button if necessary, before
            // starting doing the action
            SetValueFromUserInput(1.0);
        }
    }
    else
    {
        // Set to 1 (hilight colors)
        //if (mText != NULL)
        //{
        //    mText->SetTextColor(mTextHilightColor);
        //    mText->SetDirty();
        //}
        
        SetValueFromUserInput(1.0);

#if FIX_HILIGHT_REOPEN
        // Ensure that the button is toggled off after
        if (mToggleOffFlag)
            SetValueFromUserInput(0.0);
#endif
    }
}

void
IRolloverButtonControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
    IBitmapControl::OnMouseUp(x, y, mod);
    
    if (!mToggleFlag)
    {
        // Set to 0 (base colors)
        SetValueFromUserInput(0.0);
        
        //mText->SetTextColor(mTextColor);
        //mText->SetDirty();
    }
}

void
IRolloverButtonControl::OnMouseOver(float x, float y, const IMouseMod &mod)
{
    // 0.5 means rollover bitmap
    if (GetValue() <= 0.5)
        SetValue(0.5);
    
    mDirty = true;

    mPrevMouseOut = false;
}

void
IRolloverButtonControl::OnMouseOut()
{
    if (GetValue() <= 0.5)
        SetValue(0.0);

    // FIX: rollover buttons refreshed if we mouse wheels wherever in the gui
    // (this is because OnMouseOut() is called in many cases,
    // not only when mouse was in and goes out)
    if (!mPrevMouseOut)
        mDirty = true;

    mPrevMouseOut = true;
}

void
IRolloverButtonControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
    if (!mDisableDblClick)
        IBitmapControl::OnMouseDblClick(x, y, mod);
}

void
IRolloverButtonControl::SetDisabled(bool disable)
{
    IControl::SetDisabled(disable);
    
    if (mText != NULL)
        mText->SetDisabled(disable);
}

void
IRolloverButtonControl::LinkText(ITextControl *textControl,
                                 const IColor &color, const IColor &hilightColor)
{
    mText = textControl;
    mTextColor = color;
    mTextHilightColor = hilightColor;
}
