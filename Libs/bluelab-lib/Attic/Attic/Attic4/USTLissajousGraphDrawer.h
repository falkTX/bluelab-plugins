//
//  USTLissajousGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__USTLissajousGraphDrawer__
#define __BL_StereoWidth__USTLissajousGraphDrawer__

#include <GraphControl11.h>

// From StereoWidthGraphDrawer2
class USTLissajousGraphDrawer : public GraphCustomDrawer
{
public:
    USTLissajousGraphDrawer(BL_FLOAT scale);
    
    virtual ~USTLissajousGraphDrawer();
    
    virtual void PreDraw(NVGcontext *vg, int width, int height);
    
protected:
    BL_FLOAT mScale;
};

#endif /* defined(__BL_StereoWidth__USTLissajousGraphDrawer__) */
