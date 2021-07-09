//
//  BLCircleGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__BLCircleGraphDrawer__
#define __BL_StereoWidth__BLCircleGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

// From StereoWidthGraphDrawer2
// BLCircleGraphDrawer: from USTCircleGraphDrawer
//
class GUIHelper12;
class BLCircleGraphDrawer : public GraphCustomDrawer
{
public:
    BLCircleGraphDrawer(GUIHelper12 *guiHelper = NULL,
                        const char *title = NULL);
    
    virtual ~BLCircleGraphDrawer() {}
    
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
protected:
    bool mTitleSet;
    char mTitleText[256];

    // Style
    float mCircleLineWidth;
    float mLinesWidth;
    IColor mLinesColor;
    IColor mTextColor;
    int mOffsetX;
    int mTitleOffsetY;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__BLCircleGraphDrawer__) */
