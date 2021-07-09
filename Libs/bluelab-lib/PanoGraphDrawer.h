//
//  PanoGraphDrawer.h
//  BL-Pano
//
//  Created by applematuer on 8/9/19.
//
//

#ifndef __BL_Pano__PanoGraphDrawer__
#define __BL_Pano__PanoGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl11.h>

class PanoGraphDrawer : public GraphCustomDrawer
{
public:
    PanoGraphDrawer();
    
    virtual ~PanoGraphDrawer();
    
    // Draw after everything
    void PostDraw(NVGcontext *vg, int width, int height) override;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Pano__PanoGraphDrawer__) */
