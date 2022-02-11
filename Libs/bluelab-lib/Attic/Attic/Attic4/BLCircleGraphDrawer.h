//
//  BLCircleGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__BLCircleGraphDrawer__
#define __BL_StereoWidth__BLCircleGraphDrawer__

#include <GraphControl11.h>

// From StereoWidthGraphDrawer2
// BLCircleGraphDrawer: from USTCircleGraphDrawer
//
class BLCircleGraphDrawer : public GraphCustomDrawer
{
public:
    BLCircleGraphDrawer(const char *title = NULL);
    
    virtual ~BLCircleGraphDrawer() {}
    
    virtual void PreDraw(NVGcontext *vg, int width, int height);
    
protected:
    bool mTitleSet;
    char mTitleText[256];
};

#endif /* defined(__BL_StereoWidth__BLCircleGraphDrawer__) */
