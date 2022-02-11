//
//  PanoGraphDrawer.h
//  BL-Pano
//
//  Created by applematuer on 8/9/19.
//
//

#ifndef __BL_Pano__PanoGraphDrawer__
#define __BL_Pano__PanoGraphDrawer__

#include <GraphControl11.h>

class PanoGraphDrawer : public GraphCustomDrawer
{
public:
    PanoGraphDrawer();
    
    virtual ~PanoGraphDrawer();
    
    // Draw after everything
    void PostDraw(NVGcontext *vg, int width, int height);
};

#endif /* defined(__BL_Pano__PanoGraphDrawer__) */
