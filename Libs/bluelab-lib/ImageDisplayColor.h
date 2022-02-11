//
//  ImageDisplayColor.h
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Ghost__ImageDisplayColor__
#define __BL_Ghost__ImageDisplayColor__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "resource.h"

class NVGcontext;

// ImageDisplayColor: from ImageDisplay
// - do not use colormap, use RGB already in data
// (ImageDisplay was float gray pixels + colormap) 
class ImageDisplayColor
{
public:
    enum Mode
    {
        MODE_NEAREST,
        MODE_LINEAR
    };
    
    ImageDisplayColor(NVGcontext *vg, Mode mode = MODE_LINEAR);
    
    virtual ~ImageDisplayColor();
    
    void SetNvgContext(NVGcontext *vg);
    void ResetGL();
                      
    void Reset();
    
    void SetBounds(BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom);
    
    void SetImage(int width, int height, const WDL_TypedBuf<unsigned char> &data);
    
    void Show(bool flag);
    void SetAlpha(BL_FLOAT alpha);
    
    //
    bool DoUpdateImage();
    void UpdateImage(bool updateData = true);
    
    //
    void DrawImage(int width, int height);
    
protected:
    // NanoVG
    NVGcontext *mVg;
    
    int mImageWidth;
    int mImageHeight;
    
    // Image
    BL_FLOAT mImageBounds[4];
    
    int mNvgImage;
    
    // Floats stocked as unsichend char buffer
    WDL_TypedBuf<unsigned char> mImageData;
    
    bool mMustUpdateImage;
    bool mMustUpdateImageData;
    
    BL_FLOAT mImageAlpha;
    
    //
    bool mShowImage;
    
    //
    int mPrevImageSize;
    
    Mode mMode;
};

#endif /* defined(__BL_Ghost__ImageDisplayColor__) */
