//
//  PanogramGraphDrawer.h
//  BL-Pano
//
//  Created by applematuer on 8/9/19.
//
//

#ifndef __BL_Pano__PanogramGraphDrawer__
#define __BL_Pano__PanogramGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

class PanogramGraphDrawer : public GraphCustomDrawer
{
public:
    enum ViewOrientation
    {
        HORIZONTAL = 0,
        VERTICAL
    };
    
    PanogramGraphDrawer();
    
    virtual ~PanogramGraphDrawer();

    // The graph will destroy it automatically
    bool IsOwnedByGraph() override { return true; }
    
    // Draw after everything
    void PostDraw(NVGcontext *vg, int width, int height) override;

    void SetViewOrientation(ViewOrientation orientation);

protected:
    ViewOrientation mViewOrientation;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Pano__PanogramGraphDrawer__) */
