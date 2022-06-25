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
//  ImageDisplay.cpp
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <ColorMap4.h>

#include "ImageDisplay.h"

#include "resource.h"

// Image
#define GLSL_COLORMAP 1

ImageDisplay::ImageDisplay(NVGcontext *vg, Mode mode)
{
    mVg = vg;
    
    mMode = mode;
    
    // Image
    mNvgImage = 0;
    mNeedUpdateImage = false;
    mNeedUpdateImageData = false;
    mNvgColormapImage = 0;
    
    mShowImage = false;
    
    mImageAlpha = 1.0;
    
    mImageWidth = 0;
    mImageHeight = 0;
    
    mPrevImageSize = 0;
    
    // Colormap
    mRange = 0.0;
    mContrast = 0.5;
    
    mColorMap = NULL;

    for (int i = 0; i < 4; i++)
        mImageBounds[i] = 0.0;
    
    SetColorMap(0);
}

ImageDisplay::~ImageDisplay()
{
    if (mNvgImage != 0)
        nvgDeleteImage(mVg, mNvgImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
    
    if (mColorMap != NULL)
        delete mColorMap;
}

void
ImageDisplay::SetNvgContext(NVGcontext *vg)
{
    mVg = vg;
}

void
ImageDisplay::ResetGL()
{
    if (mNvgImage != 0)
        nvgDeleteImage(mVg, mNvgImage);
    mNvgImage = 0;
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
    mNvgColormapImage = 0;
    
    mVg = NULL;
    
    // Force re-creating the nvg images
    mImageData.Resize(0);
    mColormapImageData.Resize(0);
    
    mNeedUpdateImageData = true;
    
    // FIX(2/2): fixes Logic, close plug window, re-open => the graph control view was blank
#if 1
    mNeedUpdateImage = true;
#endif
}

void
ImageDisplay::Reset()
{
    mImageData.Resize(0);
    
    mNeedUpdateImage = true;
    
    mNeedUpdateImageData = true;
}

void
ImageDisplay::SetRange(BL_FLOAT range)
{
    mRange = range;
    
    // NEW
    mColorMap->SetRange(mRange);
    mColorMap->Generate();
}

void
ImageDisplay::SetContrast(BL_FLOAT contrast)
{
    mContrast = contrast;
    
    // NEW
    mColorMap->SetContrast(mContrast);
    mColorMap->Generate();
}


void
ImageDisplay::SetImage(int width, int height, const WDL_TypedBuf<BL_FLOAT> &data)
{
    mImageWidth = width;
    mImageHeight = height;
    
    mImageData.Resize(data.GetSize()*4);
    for (int i = 0; i < data.GetSize(); i++)
    {
        BL_FLOAT val = data.Get()[i];
        //val *= 255;
        //if (val > 255)
        //    val = 255;
        //if (val < 0)
        //    val = 0;
        
        ((float *)mImageData.Get())[i] = (float)val;
    }
    
    mNeedUpdateImage = true;
    mNeedUpdateImageData = true;
}

bool
ImageDisplay::NeedUpdateImage()
{
    return mNeedUpdateImage;
}

bool
ImageDisplay::DoUpdateImage()
{
    // Update first, before displaying
    if (!mNeedUpdateImage)
        return false;
    
    //int w = mImage->GetMaxNumCols();
    int w = mImageWidth;
    int h = mImageHeight;
    
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
#if GLSL_COLORMAP
                                       NVG_IMAGE_ONE_FLOAT_FORMAT,
#else
                                       0,
#endif
                                       mImageData.Get());
        
        mNeedUpdateImage = false;
        mNeedUpdateImageData = false;
        
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
            
            mNvgImage = nvgCreateImageRGBA(mVg,
                                           w, h,
                                           nearestFlag |
#if GLSL_COLORMAP
                                           NVG_IMAGE_ONE_FLOAT_FORMAT,
#else
                                           0,
#endif
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
    
    // Colormap
    WDL_TypedBuf<unsigned int> colorMapData;
    mColorMap->GetDataRGBA(&colorMapData);
    
    if ((colorMapData.GetSize() != mColormapImageData.GetSize()) ||
        (mNvgColormapImage == 0))
        
    {
        mColormapImageData = colorMapData;
        
        if (mNvgColormapImage != 0)
            nvgDeleteImage(mVg, mNvgColormapImage);
        
        mNvgColormapImage = nvgCreateImageRGBA(mVg,
                                               mColormapImageData.GetSize(), 1, NVG_IMAGE_NEAREST /*0*/,
                                               (unsigned char *)mColormapImageData.Get());
    } else
    {
        mColormapImageData = colorMapData;
        
        nvgUpdateImage(mVg, mNvgColormapImage, (unsigned char *)mColormapImageData.Get());
    }
    
    mNeedUpdateImage = false;
    mNeedUpdateImageData = false;
    
    return true;
}

void
ImageDisplay::DrawImage(int width, int height)
{
    if (!mShowImage)
        return;
    
    // Draw Image first
    nvgSave(mVg);
    
    // New
    mColorMap->SetRange(mRange);
    mColorMap->SetContrast(mContrast);
    
    // New: set colormap only in the Image state
#if GLSL_COLORMAP
    nvgSetColormap(mVg, mNvgColormapImage);
#endif
    
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
ImageDisplay::SetBounds(BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom)
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
ImageDisplay::GetBounds(BL_FLOAT *left, BL_FLOAT *top, BL_FLOAT *right, BL_FLOAT *bottom)
{
    *left = mImageBounds[0];
    *top = mImageBounds[1];
    *right = mImageBounds[2];
    *bottom = mImageBounds[3];
}

void
ImageDisplay::Show(bool flag)
{
    mShowImage = flag;
}

void
ImageDisplay::UpdateImage(bool updateData)
{
    mNeedUpdateImage = true;
    
    if (!mNeedUpdateImageData)
        mNeedUpdateImageData = updateData;
}

void
ImageDisplay::SetAlpha(BL_FLOAT alpha)
{
    mImageAlpha = alpha;
}

void
ImageDisplay::SetColorMap(int colorMapNum)
{
    if (mColorMap != NULL)
        delete mColorMap;
    
    switch(colorMapNum)
    {
        case 0:
        {
            // Blue and dark pink
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(68, 22, 68, 255, 0.25);
            mColorMap->AddColor(32, 122, 190, 255, 0.5);
            mColorMap->AddColor(172, 212, 250, 255, 0.75);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
            break;
            
        case 1:
        {
            // Green
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(32, 42, 26, 255, 0.0);
            mColorMap->AddColor(66, 108, 60, 255, 0.25);
            mColorMap->AddColor(98, 150, 82, 255, 0.5);
            mColorMap->AddColor(166, 206, 148, 255, 0.75);
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
            break;
            
        case 2:
        {
            // Grey
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(128, 128, 128, 255, 0.5);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
            break;
            
        case 3:
        {
            // Cyan and Pink
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(0, 150, 150, 255, 0.2);//
            mColorMap->AddColor(0, 255, 255, 255, 0.5/*0.2*/);
            mColorMap->AddColor(255, 0, 255, 255, 0.8);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
            break;
            
            
        case 4:
        {
            // Green and red
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(0, 255, 0, 255, 0.25);
            mColorMap->AddColor(255, 0, 0, 255, 1.0);
        }
            break;
            
        case 5:
        {
            // Multicolor ("jet")
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 128, 255, 0.0);
            mColorMap->AddColor(0, 0, 246, 255, 0.1);
            mColorMap->AddColor(0, 77, 255, 255, 0.2);
            mColorMap->AddColor(0, 177, 255, 255, 0.3);
            mColorMap->AddColor(38, 255, 209, 255, 0.4);
            mColorMap->AddColor(125, 255, 122, 255, 0.5);
            mColorMap->AddColor(206, 255, 40, 255, 0.6);
            mColorMap->AddColor(255, 200, 0, 255, 0.7);
            mColorMap->AddColor(255, 100, 0, 255, 0.8);
            mColorMap->AddColor(241, 8, 0, 255, 0.9);
            mColorMap->AddColor(132, 0, 0, 255, 1.0);
        }
            break;
            
        case 6:
        {
            // Cyan to orange
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(0, 255, 255, 255, 0.25);
            //mColorMap->AddColor(255, 128, 0, 255, 1.0);
            mColorMap->AddColor(255, 128, 0, 255, 0.75);
            mColorMap->AddColor(255, 228, 130, 255, 1.0);
        }
            break;
            
        case 7:
        {
            // Cyan and Orange (Wasp ?)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(22, 35, 68, 255, 0.25);
            mColorMap->AddColor(190, 90, 32, 255, 0.5);
            mColorMap->AddColor(250, 220, 96, 255, 0.75);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
            break;
            
        case 8:
        {
#if 0
            // Sky (parula / ice)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(62, 38, 168, 255, 0.004);
            
            mColorMap->AddColor(46, 135, 247, 255, 0.25);
            mColorMap->AddColor(11, 189, 189, 255, 0.5);
            mColorMap->AddColor(157, 201, 67, 255, 0.75);
            
            mColorMap->AddColor(249, 251, 21, 255, 1.0);
#endif
            
            // Not so bad, quite similar to original ice
            //
            // Sky (ice)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(4, 6, 19, 255, 0.0);
            
            mColorMap->AddColor(58, 61, 126, 255, 0.25);
            mColorMap->AddColor(67, 126, 184, 255, 0.5);
            
            //mColorMap->AddColor(112, 182, 205, 255, 0.75);
            mColorMap->AddColor(73, 173, 208, 255, 0.75);
            
            mColorMap->AddColor(232, 251, 252, 255, 1.0);
        }
            break;
            
        case 9:
        {
            // Dawn (thermal...)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(4, 35, 53, 255, 0.0);
            
            mColorMap->AddColor(90, 61, 154, 255, 0.25);
            mColorMap->AddColor(185, 97, 125, 255, 0.5);
            
            mColorMap->AddColor(245, 136, 71, 255, 0.75);
            
            mColorMap->AddColor(233, 246, 88, 255, 1.0);
            
        }
            break;
            
            // See: https://matplotlib.org/3.1.1/tutorials/colors/colormaps.html
        case 10:
        {
            // Rainbow2 (gist_rainbow)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(255, 48, 80, 255, 0.0);
            mColorMap->AddColor(255, 112, 0, 255, 0.114);
            mColorMap->AddColor(255, 245, 0, 255, 0.212);
            mColorMap->AddColor(115, 255, 0, 255, 0.318);
            mColorMap->AddColor(0, 255, 26, 255, 0.416);
            mColorMap->AddColor(0, 255, 246, 255, 0.581);
            mColorMap->AddColor(25, 0, 255, 255, 0.787);
            mColorMap->AddColor(231, 0, 255, 255, 0.935);
            mColorMap->AddColor(255, 0, 195, 255, 1.0);
        }
            break;
            
        case 11:
        {
            // Landscape (terrain)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(51, 51, 153, 255, 0.0);
            mColorMap->AddColor(2, 148, 250, 255, 0.150);
            mColorMap->AddColor(37, 111, 109, 255, 0.286);
            mColorMap->AddColor(253, 254, 152, 255, 0.494);
            mColorMap->AddColor(128, 92, 84, 255, 0.743);
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
            break;
            
        case 12:
        {
            // Fire (fire)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(143, 0, 0, 255, 0.200);
            mColorMap->AddColor(236, 0, 0, 255, 0.337);
            mColorMap->AddColor(255, 116, 0, 255, 0.535);
            mColorMap->AddColor(255, 234, 0, 255, 0.706);
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
            break;
            
        default:
            return;
            break;
    }
    
    // NEW
    // Forward the current parameters
    mColorMap->SetRange(mRange);
    mColorMap->SetContrast(mContrast);
    
    mColorMap->Generate();
}

#endif // IGRAPHICS_NANOVG
