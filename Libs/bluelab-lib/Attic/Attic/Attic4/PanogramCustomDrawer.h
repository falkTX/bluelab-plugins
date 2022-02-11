//
//  PanogramCustomDrawer.h
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifndef __BL_Panogram__PanogramCustomDrawer__
#define __BL_Panogram__PanogramCustomDrawer__

class Panogram;

class PanogramCustomDrawer : public GraphCustomDrawer
{
public:
    PanogramCustomDrawer(Plugin *plug,
                         BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    virtual ~PanogramCustomDrawer() {}
    
    void Reset();
    
    // Implement one of the two methods, depending on when
    // you whant to draw
    
    // Draw before everything
    void PreDraw(NVGcontext *vg, int width, int height) {}
    
    // Draw after everything
    void PostDraw(NVGcontext *vg, int width, int height);
    
    //
    void ClearBar();
    
    void ClearSelection();
    
    void SetBarPos(BL_FLOAT pos);
    BL_FLOAT GetBarPos();
    
    void SetBarActive(bool flag);
    bool IsBarActive();
    
    void SetSelection(BL_FLOAT x0, BL_FLOAT y0,
                      BL_FLOAT x1, BL_FLOAT y1);
    void SetSelectionActive(bool flag);
    
    void GetSelection(BL_FLOAT *x0, BL_FLOAT *y0,
                      BL_FLOAT *x1, BL_FLOAT *y1);
    
    bool IsSelectionActive();
    
    BL_FLOAT GetPlayBarPos();
    void SetPlayBarPos(BL_FLOAT pos, bool activate);
    
    bool IsPlayBarActive();
    void SetPlayBarActive(bool flag);
    
    // Normalized inside selection
    void SetSelPlayBarPos(BL_FLOAT pos);
    
protected:
    void DrawBar(NVGcontext *vg, int width, int height);
    void DrawSelection(NVGcontext *vg, int width, int height);
    void DrawPlayBar(NVGcontext *vg, int width, int height);
    
    Plugin *mPlug;
    
    bool mBarActive;
    BL_FLOAT mBarPos;
    
    bool mSelectionActive;
    BL_FLOAT mSelection[4];
    
    bool mPlayBarActive;
    BL_FLOAT mPlayBarPos;
    
    BL_FLOAT mBounds[4];
};

#endif /* defined(__BL_Panogram__PanogramCustomDrawer__) */
