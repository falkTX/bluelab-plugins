//
//  USTLissajousGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__USTLissajousGraphDrawer__
#define __BL_StereoWidth__USTLissajousGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

// From StereoWidthGraphDrawer2
class USTLissajousGraphDrawer : public GraphCustomDrawer
{
public:
    USTLissajousGraphDrawer(BL_FLOAT scale);
    
    virtual ~USTLissajousGraphDrawer();
    
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
protected:
    BL_FLOAT mScale;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__USTLissajousGraphDrawer__) */
