//
//  StereoWidthGraphDrawer3.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__StereoWidthGraphDrawer3__
#define __BL_StereoWidth__StereoWidthGraphDrawer3__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

// From StereoWidthGraphDrawer2
// BLCircleGraphDrawer: from USTCircleGraphDrawer
//
// StereoWidthGraphDrawer3 from BLCircleGraphDrawer (to get the same style)
//
class GUIHelper12;
class StereoWidthGraphDrawer3 : public GraphCustomDrawer
{
public:
    StereoWidthGraphDrawer3(GUIHelper12 *guiHelper = NULL,
                            const char *title = NULL);
    
    virtual ~StereoWidthGraphDrawer3() {}
    
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
protected:
    bool mTitleSet;
    char mTitleText[256];

    // Style
    float mCircleLineWidth;
    IColor mLinesColor;
    IColor mTextColor;
    int mOffsetX;
    int mTitleOffsetY;
};

#endif

#endif /* defined(__BL_StereoWidth__StereoWidthGraphDrawer3__) */
