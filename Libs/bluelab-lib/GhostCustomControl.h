#ifndef GHOST_CUSTOM_CONTROL_H
#define GHOST_CUSTOM_CONTROL_H

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

#include <GhostPluginInterface.h>

//class SpectrogramDisplay2;
class SpectrogramDisplay3;
class MiniView2;
class GhostCustomControl : public GraphCustomControl
{
public:
    GhostCustomControl(GhostPluginInterface *plug);
    
    virtual ~GhostCustomControl() {}
    
    void Resize(int prevWidth, int prevHeight,
                int newWidth, int newHeight);
    
    //void SetSpectrogramDisplay(SpectrogramDisplay2 *spectroDisplay);
    void SetSpectrogramDisplay(SpectrogramDisplay3 *spectroDisplay);
    void SetMiniView(MiniView2 *miniView);
    
    void SetSelectionType(GhostPluginInterface::SelectionType selectionType);
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &pMod) override;
    virtual void OnMouseUp(float x, float y, const IMouseMod &pMod) override;
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &pMod) override;
    virtual /*bool*/ void OnMouseDblClick(float x, float y, const IMouseMod &pMod) override;
    virtual void OnMouseWheel(float x, float y, const IMouseMod &pMod, float d) override;
    virtual bool OnKeyDown(float x, float y,
                           const IKeyPress &key) override { return false; }
    
    virtual void OnMouseOver(float x, float y, const IMouseMod &pMod) override;
    virtual void OnMouseOut() override {}
    
    //void UpdateZoomSelection(BL_FLOAT zoomChange);
    void UpdateZoom(BL_FLOAT zoomChange);
    
    void GetSelection(BL_FLOAT selection[4]);
    void SetSelection(BL_FLOAT x0, BL_FLOAT y0,
                      BL_FLOAT x1, BL_FLOAT y1);
    
    void SetSelectionActive(bool flag);
    // NOTE: does not totally work, must click a second time for it to works 
    bool GetSelectionActive();
    
    // Refresh the selection (for example after zoom)
    void UpdateSelection(bool updateCenterPos);
    
protected:
    bool InsideSelection(int x, int y);
    
    void SelectBorders(int x, int y);
    bool BorderSelected();
    
    // Update the selection depending on the selection type
    void UpdateSelectionType();
    
    GhostPluginInterface *mPlug;
    
    // Used to detect pure mouse up, without drag
    bool mPrevMouseDrag;
    
    int mStartDrag[2];
  
    BL_FLOAT mPrevMouseY;
    
    bool mSelectionActive;
    BL_FLOAT mSelection[4];
    
    bool mPrevMouseInsideSelect;
    
    bool mBorderSelected[4];
    
    // Inside or outside spectro ?
    bool mPrevMouseDownInsideSpectro;
    bool mPrevMouseDownInsideMiniView;
    
    //SpectrogramDisplay2 *mSpectroDisplay;
    SpectrogramDisplay3 *mSpectroDisplay;
    
    GhostPluginInterface::SelectionType mSelectionType;
    
    // Detect if we actually made the mouse down insde the spectrogram
    // (FIXES: mouse up on resize button to a bigger size, and then the mouse up
    // is inside the graph at the end (without previous mouse down inside)
    bool mPrevMouseDown;
    
    MiniView2 *mMiniView;
};

#endif

#endif
