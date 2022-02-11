//
//  ImageDisplay.h
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Ghost__ImageDisplay__
#define __BL_Ghost__ImageDisplay__

#include "IPlug_include_in_plug_hdr.h"

#include "resource.h"

/*class*/ struct NVGcontext;
class ColorMap4;

class ImageDisplay
{
public:
    enum Mode
    {
        MODE_NEAREST,
        MODE_LINEAR
    };
    
    ImageDisplay(NVGcontext *vg, Mode mode = MODE_LINEAR);
    
    virtual ~ImageDisplay();
    
    void SetNvgContext(NVGcontext *vg);
    void ResetGL();
                      
    void Reset();
    
    void SetBounds(BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom);
    void GetBounds(BL_FLOAT *left, BL_FLOAT *top, BL_FLOAT *right, BL_FLOAT *bottom);
                   
    void SetImage(int width, int height, const WDL_TypedBuf<BL_FLOAT> &data);
    
    // Colormap
    void SetRange(BL_FLOAT range);
    void SetContrast(BL_FLOAT contrast);
    void SetColorMap(int colorMapNum);
    
    void Show(bool flag);
    void SetAlpha(BL_FLOAT alpha);
    
    //
    bool NeedUpdateImage();
    bool DoUpdateImage();
    void UpdateImage(bool updateData = true);
    
    //
    void DrawImage(int width, int height);
    
protected:
    // NanoVG
    NVGcontext *mVg;
    
    ColorMap4 *mColorMap;
    BL_FLOAT mRange;
    BL_FLOAT mContrast;
    
    int mImageWidth;
    int mImageHeight;
    
    // Image
    BL_FLOAT mImageBounds[4];
    
    int mNvgImage;
    
    // Floats stocked as unsichend char buffer
    WDL_TypedBuf<unsigned char> mImageData;
    
    bool mNeedUpdateImage;
    bool mNeedUpdateImageData;
    
    BL_FLOAT mImageAlpha;
    
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
