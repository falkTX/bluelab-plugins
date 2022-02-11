//
//  StereoWidthGraphDrawer.h
//  BL-StereoWidth
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__StereoWidthGraphDrawer2__
#define __BL_StereoWidth__StereoWidthGraphDrawer2__

#include <GraphControl11.h>

class StereoWidthGraphDrawer2 : public GraphCustomDrawer
{
public:
    StereoWidthGraphDrawer2() {}
    
    virtual ~StereoWidthGraphDrawer2() {}
    
    virtual void PostDraw(NVGcontext *vg, int width, int height);
};

#endif /* defined(__BL_StereoWidth__StereoWidthGraphDrawer2__) */
