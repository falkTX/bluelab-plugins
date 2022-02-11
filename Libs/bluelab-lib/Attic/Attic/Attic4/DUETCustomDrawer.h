//
//  DUETCustomDrawer.h
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifndef __BL_Panogram__DUETCustomDrawer__
#define __BL_Panogram__DUETCustomDrawer__

class DUETCustomDrawer : public GraphCustomDrawer
{
public:
    DUETCustomDrawer();
    
    virtual ~DUETCustomDrawer() {}
    
    void Reset();
    
    // Draw after everything
    void PostDraw(NVGcontext *vg, int width, int height);
    
    void SetPickCursorActive(bool flag);
    void SetPickCursor(BL_FLOAT x, BL_FLOAT y);
    
protected:
    void DrawPickCursor(NVGcontext *vg, int width, int height);
    
    //
    bool mPickCursorActive;
    
    BL_FLOAT mPickCursorX;
    BL_FLOAT mPickCursorY;
};

#endif /* defined(__BL_Panogram__DUETCustomDrawer__) */
