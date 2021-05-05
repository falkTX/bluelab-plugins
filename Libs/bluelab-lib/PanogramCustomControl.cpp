//
//  PanogramCustomControl.cpp
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtilsAlgo.h>

//#include <Panogram.h>
#include <SpectrogramDisplayScroll4.h>

#include "PanogramCustomControl.h"

// 8 pixels
#define SELECTION_BORDER_SIZE 8


PanogramCustomControl::PanogramCustomControl(PlaySelectPluginInterface *plug)
{
    mPlug = plug;
    
    mSpectroDisplay = NULL;

    mViewOrientation = HORIZONTAL;
    
    Reset();
}

// NEW
void
PanogramCustomControl::Reset()
{
    mPrevMouseDrag = false;
    
    mStartDrag[0] = 0;
    mStartDrag[1] = 0;
    
    mPrevMouseY = 0.0;
    
    mPrevMouseInsideSelect = false;
    
    mSelectionActive = false;
    
    for (int i = 0; i < 4; i++)
        mSelection[i] = 0.0;
    
    for (int i = 0; i < 4; i++)
        mBorderSelected[i] = false;
    
    mPrevMouseDown = false;
    
    // Plug
    mPlug->SetBarActive(false);
    mPlug->SetSelectionActive(false);
    mPlug->ClearBar();
    mPlug->ResetPlayBar();
}

void
PanogramCustomControl::Resize(int prevWidth, int prevHeight,
                              int newWidth, int newHeight)
{
    mSelection[0] = (((BL_FLOAT)mSelection[0])/prevWidth)*newWidth;
    mSelection[1] = (((BL_FLOAT)mSelection[1])/prevHeight)*newHeight;
    mSelection[2] = (((BL_FLOAT)mSelection[2])/prevWidth)*newWidth;
    mSelection[3] = (((BL_FLOAT)mSelection[3])/prevHeight)*newHeight;
}

void
PanogramCustomControl::
SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
PanogramCustomControl::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    if (mViewOrientation == VERTICAL)
    {
        int width;
        int height;
        mPlug->GetGraphSize(&width, &height);
    
        BLUtilsAlgo::Rotate90(width, height, &x, &y, true, false);
    }
    
    mPrevMouseDown = true;
    mPrevMouseY = y;
    
    if (mod.Cmd)
        // Command pressed (or Control on Windows)
    {
        // We are dragging the spectrogram,
        // do not change the bar position
        
        return;
    }
    
    mPrevMouseDrag = false;
    
    
    mStartDrag[0] = x;
    mStartDrag[1] = y;
        
    // Check that if is the beginning of a drag from inside the selection.
    // In this case, move the selection.
    mPrevMouseInsideSelect = InsideSelection(x, y);
        
    // Try to select borders
    SelectBorders(x, y);
}

void
PanogramCustomControl::OnMouseUp(float x, float y, const IMouseMod &mod)
{
    if (mViewOrientation == VERTICAL)
    {
        int width;
        int height;
        mPlug->GetGraphSize(&width, &height);
    
        BLUtilsAlgo::Rotate90(width, height, &x, &y, true, false);
    }
    
    // FIX: click on the resize button, to a bigger size, then the
    // mouse up is detected inside the graph, without previous mouse down
    // (would put the bar in incorrect position)
    if (!mPrevMouseDown)
        return;
    mPrevMouseDown = false;
    
    if (mod.Cmd)
        // Command pressed (or control on Windows)
    {
        // We are dragging the spectrogram,
        // do not change the bar position
        
        return;
    }
    
    int width;
    int height;
    mPlug->GetGraphSize(&width, &height);
    
    if (!mPrevMouseDrag)
        // Pure mouse up
    {
        mPlug->SetBarActive(true);
        mPlug->SetBarPos(x);
        mPlug->SetSelectionActive(false); //
        
        // Set the play bar to origin
        mPlug->ResetPlayBar();
            
        mSelectionActive = false;
    }
    
    for (int i = 0; i < 4; i++)
        mBorderSelected[i] = false;
}

void
PanogramCustomControl::OnMouseDrag(float x, float y,
                                   float dX, float dY, const IMouseMod &mod)
{
    if (mViewOrientation == VERTICAL)
    {
        int width;
        int height;
        mPlug->GetGraphSize(&width, &height);
    
        BLUtilsAlgo::Rotate90(width, height, &x, &y, true, false);
        BLUtilsAlgo::Rotate90Delta(width, height, &dX, &dY, true, false);
    }
    
    bool beginDrag = !mPrevMouseDrag;
    
    mPrevMouseDrag = true;
    
    int width;
    int height;
    
    mPlug->GetGraphSize(&width, &height);
    
    if (!mPrevMouseInsideSelect && !BorderSelected())
    {
        mSelection[0] = mStartDrag[0];
        mSelection[1] = mStartDrag[1];
        mSelection[2] = x;
        mSelection[3] = y;
            
        // Swap if necessary
        if (mSelection[0] > mSelection[2])
        {
            // Swap
            int tmp = mSelection[0];
            mSelection[0] = mSelection[2];
            mSelection[2] = tmp;
        }
            
        if (mSelection[1] > mSelection[3])
        {
            // Swap
            int tmp = mSelection[1];
            mSelection[1] = mSelection[3];
            mSelection[3] = tmp;
        }
            
        // Selection is active !
        // (could have been disactivated by putting the bar)
        mSelectionActive = true;
        
        mPlug->UpdateSelection(mSelection[0], mSelection[1],
                               mSelection[2], mSelection[3],
                               false, true);
            
        mPlug->ClearBar();
            
        // Set the play bar at the beginning of the new selection
        if (beginDrag)
            mPlug->ResetPlayBar();
    }
    else
    {
        if (!BorderSelected())
        {
            // Move the selection
            mSelection[0] += dX;
            mSelection[1] += dY;
                
            mSelection[2] += dX;
            mSelection[3] += dY;
        }
        else
            // Modify the selection
        {
            if (mBorderSelected[0])
                mSelection[0] += dX;
                
            if (mBorderSelected[1])
                mSelection[1] += dY;
                
            if (mBorderSelected[2])
                mSelection[2] += dX;
                
            if (mBorderSelected[3])
                mSelection[3] += dY;
        }
        
        mPlug->UpdateSelection(mSelection[0], mSelection[1],
                               mSelection[2], mSelection[3],
                               false, false);
            
        // Set the play bar at the beginning of the new selection
        if (beginDrag)
            mPlug->ResetPlayBar();
    }
    
    // NEW: capture playbar inside selection
    if (mPlug->PlayBarOutsideSelection())
    {
        // Play bar is outside the selection,
        // rest it to the beginning of the selection
        //
        // This allows moving the selection, with play bar
        // "captured" inside the selection, on both sides
        mPlug->ResetPlayBar();
    }
}

bool
PanogramCustomControl::InsideSelection(int x, int y)
{
    if (!mSelectionActive)
        return false;
    
    if (x < mSelection[0])
        return false;
    
    if (y < mSelection[1])
        return false;
    
    if (x > mSelection[2])
        return false;
    
    if (y > mSelection[3])
        return false;
    
    return true;
}

void
PanogramCustomControl::SetViewOrientation(ViewOrientation orientation)
{
    mViewOrientation = orientation;
}

void
PanogramCustomControl::SelectBorders(int x, int y)
{
    // FIX: avoid selecting selection rectangle border when it is not visible !
    // Do not try to select a border if only the play bar is visible
    bool barActive = mPlug->IsBarActive();
    if (barActive)
        return;
    
    for (int i = 0; i < 4; i++)
        mBorderSelected[i] = false;
    
    int dist0 = abs(x - (int)mSelection[0]);
    if ((dist0 < SELECTION_BORDER_SIZE) && // near x
        (y > mSelection[1]) && (y < mSelection[3])) // on the interval of y
        mBorderSelected[0] = true;
    
    int dist1 = abs(y - (int)mSelection[1]);
    if ((dist1 < SELECTION_BORDER_SIZE) && // near y
        (x > mSelection[0]) && (x < mSelection[2])) // on the interval of x
        mBorderSelected[1] = true;
    
    int dist2 = abs(x - (int)mSelection[2]);
    if ((dist2 < SELECTION_BORDER_SIZE) && // near x
        (y > mSelection[1]) && (y < mSelection[3])) // on the interval of y
        mBorderSelected[2] = true;
    
    int dist3 = abs(y - (int)mSelection[3]);
    if ((dist3 < SELECTION_BORDER_SIZE) && // near y
        (x > mSelection[0]) && (x < mSelection[2])) // on the interval of x
        mBorderSelected[3] = true;
}

bool
PanogramCustomControl::BorderSelected()
{
    for (int i = 0; i < 4; i++)
    {
        if (mBorderSelected[i])
            return true;
    }
    
    return false;
}

void
PanogramCustomControl::GetSelection(BL_FLOAT selection[4])
{
    for (int i = 0; i < 4; i++)
        selection[i] = mSelection[i];
}

void
PanogramCustomControl::SetSelection(BL_FLOAT x0, BL_FLOAT y0,
                                 BL_FLOAT x1, BL_FLOAT y1)
{
    mSelection[0] = x0;
    mSelection[1] = y0;
    mSelection[2] = x1;
    mSelection[3] = y1;
}

void
PanogramCustomControl::SetSelectionActive(bool flag)
{
    mSelectionActive = flag;
}

#endif // IGRAPHICS_NANOVG

