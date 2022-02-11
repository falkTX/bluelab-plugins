#ifndef PLAY_SELECT_PLUGIN_INTERFACE_H
#define PLAY_SELECT_PLUGIN_INTERFACE_H

class PlaySelectPluginInterface
{
 public:
    // Play bar
    virtual void SetBarActive(bool flag) = 0;
    virtual bool IsBarActive() = 0;
    virtual void SetBarPos(BL_FLOAT x) = 0;
    virtual BL_FLOAT GetBarPos() = 0;
    virtual void ResetPlayBar() = 0;
    virtual void ClearBar() = 0;
    
    virtual void StartPlay() = 0;
    virtual void StopPlay() = 0;
    virtual bool PlayStarted() = 0;
    
    // Selection
    virtual bool IsSelectionActive() = 0;
    virtual void UpdateSelection(BL_FLOAT x0, BL_FLOAT y0,
                                 BL_FLOAT x1, BL_FLOAT y1,
                                 bool updateCenterPos,
                                 bool activateDrawSelection = false,
                                 bool updateCustomControl = false) = 0;
    virtual void SelectionChanged() = 0;
    virtual bool PlayBarOutsideSelection() = 0;

    virtual void BarSetSelection(int x) = 0;
    virtual void SetSelectionActive(bool flag) = 0;
    
    virtual void UpdatePlayBar() = 0;

    //
    virtual void GetGraphSize(int *width, int *height) = 0;
};

#endif
