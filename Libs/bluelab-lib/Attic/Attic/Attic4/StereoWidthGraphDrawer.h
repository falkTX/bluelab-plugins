//
//  StereoWidthGraphDrawer.h
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__StereoWidthGraphDrawer__
#define __BL_StereoWidth__StereoWidthGraphDrawer__

#include <GraphControl11.h>

class StereoWidthGraphDrawer : public GraphCustomDrawer
{
public:
    StereoWidthGraphDrawer() {}
    
    virtual ~StereoWidthGraphDrawer() {}
    
    virtual void PostDraw(NVGcontext *vg, int width, int height);
};

#endif /* defined(__BL_StereoWidth__StereoWidthGraphDrawer__) */
