//
//  ImageDisplayColor.cpp
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include "ImageDisplayColor.h"

#include "resource.h"

// Image
ImageDisplayColor::ImageDisplayColor(NVGcontext *vg, Mode mode)
{
    mVg = vg;
    
    mMode = mode;
    
    // Image
    mNvgImage = 0;
    mMustUpdateImage = false;
    mMustUpdateImageData = false;
    
    mShowImage = false;
    
    mImageAlpha = 1.0;
    
    mImageWidth = 0;
    mImageHeight = 0;
    
    mPrevImageSize = 0;

    for (int i = 0; i < 4; i++)
        mImageBounds[i] = 0.0;
}

ImageDisplayColor::~ImageDisplayColor()
{
    if (mNvgImage != 0)
        nvgDeleteImage(mVg, mNvgImage);
}

void
ImageDisplayColor::SetNvgContext(NVGcontext *vg)
{
    mVg = vg;
}

void
ImageDisplayColor::ResetGL()
{
    if (mNvgImage != 0)
        nvgDeleteImage(mVg, mNvgImage);
    mNvgImage = 0;
    
    mVg = NULL;
    
    // Force re-creating the nvg images
    mImageData.Resize(0);
    
    mMustUpdateImageData = true;
    
    // FIX(2/2): fixes Logic, close plug window, re-open => the graph control view was blank
#if 1
    mMustUpdateImage = true;
#endif
}

void
ImageDisplayColor::Reset()
{
    mImageData.Resize(0);
    
    mMustUpdateImage = true;
    
    mMustUpdateImageData = true;
}

void
ImageDisplayColor::SetImage(int width, int height, const WDL_TypedBuf<unsigned char> &data)
{
    mImageWidth = width;
    mImageHeight = height;
    
    mImageData.Resize(data.GetSize());
    
    memcpy(mImageData.Get(), data.Get(), mImageWidth*mImageHeight*4*sizeof(unsigned char));
    
    mMustUpdateImage = true;
    mMustUpdateImageData = true;
}

bool
ImageDisplayColor::DoUpdateImage()
{
    // Update first, before displaying
    if (!mMustUpdateImage)
        return false;
    
    //int w = mImage->GetMaxNumCols();
    int w = mImageWidth;
    int h = mImageHeight;
    
#if 1 // Avoid white image when there is no data
    if (w == 0)
    {
        w = 1;
        int imageSize = w*h*4;
        
        mImageData.Resize(imageSize);
        memset(mImageData.Get(), 0, imageSize);
        
        // Image image
        if (mNvgImage != 0)
            nvgDeleteImage(mVg, mNvgImage);
        
        int nearestFlag = 0;
        if (mMode == MODE_NEAREST)
        {
            nearestFlag = NVG_IMAGE_NEAREST;
        }
        
        mNvgImage = nvgCreateImageRGBA(mVg,
                                       w, h,
                                       nearestFlag,
                                       mImageData.Get());
        
        mMustUpdateImage = false;
        mMustUpdateImageData = false;
        
        return true;
    }
#endif
    
    if (mMustUpdateImageData)
    {
        if (mImageData.GetSize() != mPrevImageSize)
        {
            mPrevImageSize = mImageData.GetSize();
            
            // Image image
            if (mNvgImage != 0)
                nvgDeleteImage(mVg, mNvgImage);
            
            int nearestFlag = 0;
            if (mMode == MODE_NEAREST)
            {
                nearestFlag = NVG_IMAGE_NEAREST;
            }
            
            mNvgImage = nvgCreateImageRGBA(mVg,
                                           w, h,
                                           nearestFlag,
                                           mImageData.Get());
            
			// No need since it has been better fixed in nanovg
#ifdef WIN32 // Hack: after having created the image, update it again
			 // FIX: Image blinking between random pixels and correct pixels  
			//nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());
#endif
        }
        else
        {
            //memset(mImageData.Get(), 0, imageSize);
            
            // Image image
            nvgUpdateImage(mVg, mNvgImage, mImageData.Get());
        }
    }
    
    mMustUpdateImage = false;
    mMustUpdateImageData = false;
    
    return true;
}

void
ImageDisplayColor::DrawImage(int width, int height)
{
    if (!mShowImage)
        return;
    
    // Draw Image first
    nvgSave(mVg);
    
    //
    // Image image
    //
    
    // Display the rightmost par in case of zoom
    //NVGpaint imgPaint = nvgImagePattern(mVg, 0.0, 0.0, width, height,
    //                                    0.0, mNvgSpectroImage, mImageAlpha);
    
    NVGpaint imgPaint = nvgImagePattern(mVg,
                                        mImageBounds[0]*width,
                                        mImageBounds[1]*height,
                                        (mImageBounds[2] - mImageBounds[0])*width,
                                        (mImageBounds[3] - mImageBounds[1])*height,
                                        0.0, mNvgImage, mImageAlpha);
    
    BL_FLOAT b0Yf = mImageBounds[1]*height;
    BL_FLOAT b1Yf = (mImageBounds[3] - mImageBounds[1])*height;
#if GRAPH_CONTROL_FLIP_Y
    b0Yf = height - b0Yf;
    b1Yf = height - b1Yf;
#endif
    
    nvgBeginPath(mVg);
    nvgRect(mVg,
            mImageBounds[0]*width,
            b0Yf,
            (mImageBounds[2] - mImageBounds[0])*width,
            b1Yf);
    
    
    nvgFillPaint(mVg, imgPaint);
    nvgFill(mVg);
    
    nvgRestore(mVg);
}

void
ImageDisplayColor::SetBounds(BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom)
{
    mImageBounds[0] = left;
    mImageBounds[1] = top;
    mImageBounds[2] = right;
    mImageBounds[3] = bottom;
    
    mShowImage = true;
    
    // Be sure to create the texture image in the right thread
    UpdateImage(true);
}


void
ImageDisplayColor::Show(bool flag)
{
    mShowImage = flag;
}

void
ImageDisplayColor::UpdateImage(bool updateData)
{
    mMustUpdateImage = true;
    
    if (!mMustUpdateImageData)
        mMustUpdateImageData = updateData;
}

void
ImageDisplayColor::SetAlpha(BL_FLOAT alpha)
{
    mImageAlpha = alpha;
}

#endif // IGRAPHICS_NANOVG
