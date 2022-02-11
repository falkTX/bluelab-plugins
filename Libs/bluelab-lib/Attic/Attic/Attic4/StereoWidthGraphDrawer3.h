//
//  StereoWidthGraphDrawer3.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__StereoWidthGraphDrawer3__
#define __BL_StereoWidth__StereoWidthGraphDrawer3__

#include <GraphControl11.h>

// From StereoWidthGraphDrawer2
// BLCircleGraphDrawer: from USTCircleGraphDrawer
//
// StereoWidthGraphDrawer3 from BLCircleGraphDrawer (to get the same style)
//
class StereoWidthGraphDrawer3 : public GraphCustomDrawer
{
public:
    StereoWidthGraphDrawer3(const char *title = NULL);
    
    virtual ~StereoWidthGraphDrawer3() {}
    
    virtual void PreDraw(NVGcontext *vg, int width, int height);
    
protected:
    bool mTitleSet;
    char mTitleText[256];
};

#endif /* defined(__BL_StereoWidth__StereoWidthGraphDrawer3__) */
