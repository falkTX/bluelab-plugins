//
//  USTCircleGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__USTCircleGraphDrawer__
#define __BL_StereoWidth__USTCircleGraphDrawer__

#include <GraphControl11.h>

// From StereoWidthGraphDrawer2
class USTCircleGraphDrawer : public GraphCustomDrawer
{
public:
    USTCircleGraphDrawer() {}
    
    virtual ~USTCircleGraphDrawer() {}
    
    virtual void PreDraw(NVGcontext *vg, int width, int height);
};

#endif /* defined(__BL_StereoWidth__USTCircleGraphDrawer__) */
