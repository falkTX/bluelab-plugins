//
//  BLLissajousGraphDrawer.h
//  UST
//
//  Created by Pan on 24/04/18.
//
//

#ifndef __BL_StereoWidth__BLLissajousGraphDrawer__
#define __BL_StereoWidth__BLLissajousGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

// From StereoWidthGraphDrawer2
// BLLissajousGraphDrawer: from USTLissajousGraphDrawer
//
class GUIHelper12;
class BLLissajousGraphDrawer : public GraphCustomDrawer
{
public:
    BLLissajousGraphDrawer(BL_FLOAT scale,
                           GUIHelper12 *guiHelper = NULL,
                           const char *title = NULL);
    
    virtual ~BLLissajousGraphDrawer();
    
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
protected:
    BL_FLOAT mScale;
    
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

#endif /* defined(__BL_StereoWidth__BLLissajousGraphDrawer__) */
