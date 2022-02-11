//
//  PanogramPlugInterface.h
//  UST-macOS
//
//  Created by applematuer on 9/26/20.
//
//

#ifndef PanogramPlugInterface_h
#define PanogramPlugInterface_h

#include <BLTypes.h>

class PanogramPlugInterface
{
public:
    // Bar
    virtual void SetBarActive(bool flag) = 0;
    virtual bool IsBarActive() = 0;
    virtual void ClearBar() = 0;
    virtual void ResetPlayBar() = 0;
    virtual void SetBarPos(int x) = 0;
    virtual bool PlayBarOutsideSelection() = 0;
    
    // Selection
    virtual void SetSelectionActive(bool flag) = 0;
    
    virtual void UpdateSelection(BL_GUI_FLOAT x0, BL_GUI_FLOAT y0,
                                 BL_GUI_FLOAT x1, BL_GUI_FLOAT y1,
                                 bool updateCenterPos,
                                 bool activateDrawSelection = false,
                                 bool updateCustomControl = false) = 0;
    
    virtual void GetGraphSize(int *width, int *height) = 0;
};

#endif /* PanogramPlugInterface_h */
