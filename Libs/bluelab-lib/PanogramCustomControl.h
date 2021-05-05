//
//  PanogramCustomControl.h
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifndef __BL_Panogram__PanogramCustomControl__
#define __BL_Panogram__PanogramCustomControl__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

#include <PlaySelectPluginInterface.h>

class SpectrogramDisplayScroll4;
class PanogramCustomControl : public GraphCustomControl
{
public:
    enum ViewOrientation
    {
        HORIZONTAL = 0,
        VERTICAL
    };
    
    PanogramCustomControl(PlaySelectPluginInterface *plug);
    
    virtual ~PanogramCustomControl() {}
    
    void Reset();
    
    void Resize(int prevWidth, int prevHeight,
                int newWidth, int newHeight);
    
    void SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay);
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod);
    virtual void OnMouseUp(float x, float y, const IMouseMod &mod);
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &mod);
    
    void GetSelection(BL_FLOAT selection[4]);
    void SetSelection(BL_FLOAT x0, BL_FLOAT y0,
                      BL_FLOAT x1, BL_FLOAT y1);
    
    void SetSelectionActive(bool flag);

    void SetViewOrientation(ViewOrientation orientation);
    
protected:
    bool InsideSelection(int x, int y);
    
    void SelectBorders(int x, int y);
    bool BorderSelected();
    
    //
    PlaySelectPluginInterface *mPlug;
    
    // Used to detect pure mouse up, without drag
    bool mPrevMouseDrag;
    
    int mStartDrag[2];
    
    BL_FLOAT mPrevMouseY;
    
    bool mSelectionActive;
    BL_FLOAT mSelection[4];
    
    bool mPrevMouseInsideSelect;
    
    bool mBorderSelected[4];
    
    SpectrogramDisplayScroll4 *mSpectroDisplay;
    
    // Detect if we actually made the mouse down insde the spectrogram
    // (FIXES: mouse up on resize button to a bigger size, and then the mouse up
    // is inside the graph at the end (without previous mouse down inside)
    bool mPrevMouseDown;

    ViewOrientation mViewOrientation;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Panogram__PanogramCustomControl__) */
