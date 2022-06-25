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
