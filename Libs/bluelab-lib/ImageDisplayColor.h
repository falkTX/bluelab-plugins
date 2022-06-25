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
