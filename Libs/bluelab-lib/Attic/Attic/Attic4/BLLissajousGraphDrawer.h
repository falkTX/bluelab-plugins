//
//  BLLissajousGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__BLLissajousGraphDrawer__
#define __BL_StereoWidth__BLLissajousGraphDrawer__

#include <GraphControl11.h>

// From StereoWidthGraphDrawer2
// BLLissajousGraphDrawer: from USTLissajousGraphDrawer
//
class BLLissajousGraphDrawer : public GraphCustomDrawer
{
public:
    BLLissajousGraphDrawer(BL_FLOAT scale, const char *title = NULL);
    
    virtual ~BLLissajousGraphDrawer();
    
    virtual void PreDraw(NVGcontext *vg, int width, int height);
    
protected:
    BL_FLOAT mScale;
    
    bool mTitleSet;
    char mTitleText[256];
};

#endif /* defined(__BL_StereoWidth__BLLissajousGraphDrawer__) */
