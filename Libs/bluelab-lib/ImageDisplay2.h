//
//  ImageDisplay.h
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Ghost__ImageDisplay2__
#define __BL_Ghost__ImageDisplay2__

#include <GraphControl12.h>

#include "IPlug_include_in_plug_hdr.h"

class NVGcontext;
class BLImage;
class ColorMap4;
class ImageDisplay2 : public GraphCustomDrawer
{
public:
    enum Mode
    {
        MODE_NEAREST,
        MODE_LINEAR
    };
    
    ImageDisplay2(Mode mode = MODE_LINEAR);
    
    virtual ~ImageDisplay2();

    void Reset();
    
    void SetBounds(BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom);
    void GetBounds(BL_FLOAT *left, BL_FLOAT *top, BL_FLOAT *right, BL_FLOAT *bottom);

    void PreDraw(NVGcontext *vg, int width, int height) override;
    bool IsOwnedByGraph() override { return true; }
    bool NeedRedraw() override;
    
    void SetImage(BLImage *image);
    
    void ShowImage(bool flag);
    
    //
    bool NeedUpdateImage();
    bool DoUpdateImage();
    void UpdateImage(bool updateData = true);
    
    void UpdateColormap(bool flag);
    
protected:
    // NanoVG
    NVGcontext *mVg;
    
    // Image
    BL_FLOAT mImageBounds[4];
    
    BLImage *mImage;
    
    int mNvgImage;
    
    // Floats stocked as unsichend char buffer
    WDL_TypedBuf<unsigned char> mImageData;
    
    bool mNeedUpdateImage;
    bool mNeedUpdateImageData;
    bool mNeedUpdateColormapData;
    
    bool mNeedRedraw;
    
    //
    bool mShowImage;
    
    // Colormap
    int mNvgColormapImage;
    WDL_TypedBuf<unsigned int> mColormapImageData;
    
    //
    int mPrevImageSize;
    
    //
    Mode mMode;
};

#endif /* defined(__BL_Ghost__ImageDisplay__) */
