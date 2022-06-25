/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  ImageDisplay2.cpp
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <ColorMap4.h>
#include <BLImage.h>

#include "ImageDisplay2.h"


ImageDisplay2::ImageDisplay2(Mode mode)
{
    mVg = NULL;
    
    mMode = mode;

    mImage = NULL;
    
    // Image
    mNvgImage = 0;
    mNeedUpdateImage = true;
    mNeedUpdateImageData = true;
    
    mNvgColormapImage = 0;
    mNeedUpdateColormapData = true;
    
    mShowImage = false;
    
    mPrevImageSize = 0;
    
    mNeedRedraw = false;

    for (int i = 0; i < 4; i++)
        mImageBounds[i] = 0.0;
}

ImageDisplay2::~ImageDisplay2()
{
    if (mVg == NULL)
      return;
  
    if (mNvgImage != 0)
        nvgDeleteImage(mVg, mNvgImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
}

void
ImageDisplay2::Reset()
{
    mImageData.Resize(0);
    
    mNeedUpdateImage = true;
    mNeedUpdateImageData = true;
    mNeedUpdateColormapData = true;
    
    mNeedRedraw = true;
}

void
ImageDisplay2::SetImage(BLImage *image)
{
  mImage = image;
  
  if (mImage == NULL)
    return;
    
  bool updated = mImage->GetImageDataFloat(&mImageData);
  if (updated)
  {
      mNeedUpdateImage = true;
      mNeedUpdateImageData = true;
      
      mNeedRedraw = true;
  }
}

bool
ImageDisplay2::NeedUpdateImage()
{
    return mNeedUpdateImage;
}

bool
ImageDisplay2::DoUpdateImage()
{
    // Update first, before displaying
    if (!mNeedUpdateImage)
        return false;
    
    int w = mImage->GetWidth();
    int h = mImage->GetHeight();
    
    int nearestFlag = 0;
    if (mMode == MODE_NEAREST)
    {
        nearestFlag = NVG_IMAGE_NEAREST;
    }
    
#if 1 // Avoid white image when there is no data
    if ((w == 0) || (mNvgImage == 0))
    {
        w = 1;
        int imageSize = w*h*4;
        
        mImageData.Resize(imageSize);
        memset(mImageData.Get(), 0, imageSize);
        
        // Image image
        if (mNvgImage != 0)
            nvgDeleteImage(mVg, mNvgImage);
        
        mNvgImage = nvgCreateImageRGBA(mVg,
                                       w, h,
                                       nearestFlag |
                                       NVG_IMAGE_ONE_FLOAT_FORMAT,
                                       mImageData.Get());
        
        mNeedUpdateImage = false;
        mNeedUpdateImageData = false;
        
        mNeedRedraw = true;
        
        return true;
    }
#endif
    
    if (mNeedUpdateImageData || (mNvgImage == 0))
    {
        if (mImageData.GetSize() != mPrevImageSize)
        {
            mPrevImageSize = mImageData.GetSize();
            
            // Image image
            if (mNvgImage != 0)
                nvgDeleteImage(mVg, mNvgImage);
            
            int iw = mImage->GetWidth();
            int ih = mImage->GetHeight();
            mNvgImage = nvgCreateImageRGBA(mVg,
                                           iw, ih,
                                           nearestFlag |
                                           NVG_IMAGE_ONE_FLOAT_FORMAT,
                                           mImageData.Get());
        }
        else
        {          
            // Image image
            nvgUpdateImage(mVg, mNvgImage, mImageData.Get());
        }
    }
    
    if (mNeedUpdateColormapData || (mNvgColormapImage == 0))
    {
        // Colormap
        WDL_TypedBuf<unsigned int> colorMapData;
        bool updated = mImage->GetColormapImageDataRGBA(&colorMapData);
        if (updated || (mNvgColormapImage == 0))
        {
            if ((colorMapData.GetSize() != mColormapImageData.GetSize()) ||
                (mNvgColormapImage == 0))
            {
                mColormapImageData = colorMapData;
                
                if (mNvgColormapImage != 0)
                    nvgDeleteImage(mVg, mNvgColormapImage);
                
                mNvgColormapImage =
                            nvgCreateImageRGBA(mVg,
                                               mColormapImageData.GetSize(),
                                               1, NVG_IMAGE_NEAREST,
                                               (unsigned char *)mColormapImageData.Get());
            }
            else
            {
                mColormapImageData = colorMapData;
                
                nvgUpdateImage(mVg, mNvgColormapImage,
                               (unsigned char *)mColormapImageData.Get());
            }
        }
    }
    
    mNeedUpdateImage = false;
    mNeedUpdateImageData = false;
    mNeedUpdateColormapData = false;
    
    mNeedRedraw = true;
    
    return true;
}

void
ImageDisplay2::PreDraw(NVGcontext *vg, int width, int height)
{
  mVg = vg;

  bool updated = DoUpdateImage();
  
  if (!mShowImage)
  {
      mNeedRedraw = false;
      
      return;
  }
    
  // Draw Image first
  nvgSave(mVg);

  // New: set colormap only in the Image state
  nvgSetColormap(mVg, mNvgColormapImage);
    
  //
  // Image image
  //
  
  // Display the rightmost par in case of zoom
  BL_FLOAT alpha = mImage->GetAlpha();
    
  NVGpaint imgPaint = nvgImagePattern(mVg,
                                      mImageBounds[0]*width,
                                      mImageBounds[1]*height,
                                      (mImageBounds[2] - mImageBounds[0])*width,
                                      (mImageBounds[3] - mImageBounds[1])*height,
                                      0.0, mNvgImage, alpha);
  
  BL_FLOAT b0Yf = mImageBounds[1]*height;
  BL_FLOAT b1Yf = (mImageBounds[3] - mImageBounds[1])*height;
    
  nvgBeginPath(mVg);
  nvgRect(mVg,
          mImageBounds[0]*width,
          b0Yf,
          (mImageBounds[2] - mImageBounds[0])*width,
          b1Yf);
  
  nvgFillPaint(mVg, imgPaint);
  nvgFill(mVg);
  
  nvgRestore(mVg);
    
  mNeedRedraw = updated;
}

bool
ImageDisplay2::NeedRedraw()
{
    return mNeedRedraw;
}

void
ImageDisplay2::SetBounds(BL_FLOAT left, BL_FLOAT top,
                         BL_FLOAT right, BL_FLOAT bottom)
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
ImageDisplay2::GetBounds(BL_FLOAT *left, BL_FLOAT *top,
                         BL_FLOAT *right, BL_FLOAT *bottom)
{
    *left = mImageBounds[0];
    *top = mImageBounds[1];
    *right = mImageBounds[2];
    *bottom = mImageBounds[3];
}

void
ImageDisplay2::ShowImage(bool flag)
{
    mShowImage = flag;
}

void
ImageDisplay2::UpdateImage(bool updateData)
{
    mNeedUpdateImage = true;
    
    if (!mNeedUpdateImageData)
        mNeedUpdateImageData = updateData;
}

void
ImageDisplay2::UpdateColormap(bool flag)
{
    mNeedUpdateImage = true;
    
    if (!mNeedUpdateColormapData)
    {
        mNeedUpdateColormapData = flag;
    }
}


#endif // IGRAPHICS_NANOVG
