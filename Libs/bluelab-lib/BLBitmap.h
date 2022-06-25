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
 
#ifndef BL_BITMAP_H
#define BL_BITMAP_H

#include <stdlib.h>

#include "IPlug_include_in_plug_hdr.h"

#ifdef IGRAPHICS_NANOVG

class BLBitmap
{
public:
    BLBitmap(int width, int height, int bpp,
             unsigned char *data = NULL);

    BLBitmap(const BLBitmap &other);
    
    virtual ~BLBitmap();

    void Clear();
    
    int GetWidth() const;
    int GetHeight() const;
    int GetBpp() const;
    
    const unsigned char *GetData() const;
    
    // Load from file
    static BLBitmap *Load(const char *fileName);
    
    // Save
    static void Save(const BLBitmap *bmp, const char *fileName);
    
    static void FillAlpha(BLBitmap *bmp, unsigned char val);
    
    static void Blit(BLBitmap *dst, const BLBitmap *src);
    
    static void ScaledBlit(BLBitmap *dest, const BLBitmap *src,
                           int dstx, int dsty, int dstw, int dsth,
                           float srcx, float srcy, float srcw, float srch);

protected:
    int mWidth;
    int mHeight;
    int mBpp;
    
    unsigned char *mData;
};

#endif

#endif
