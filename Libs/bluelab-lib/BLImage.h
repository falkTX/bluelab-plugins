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
 
#ifndef BL_IMAGE_H
#define BL_IMAGE_H

#include <BLTypes.h>
#include <ColorMapFactory.h>

#include "IPlug_include_in_plug_hdr.h"

class ColorMap4;
class BLImage
{
 public:
  BLImage(int width, int height, bool useGLSL = true);

  virtual ~BLImage();

  void Reset();
    
  int GetWidth();
  int GetHeight();

  void GetData(WDL_TypedBuf<BL_FLOAT> *data);
  void SetData(const WDL_TypedBuf<BL_FLOAT> &data);
  void SetData(int width, int height, const WDL_TypedBuf<BL_FLOAT> &data);
  void TouchData();
  
  void SetRange(BL_FLOAT range);
  void SetContrast(BL_FLOAT contrast);

  void SetColorMap(ColorMapFactory::ColorMap colorMapId);
  void TouchColorMap();

  BL_FLOAT GetAlpha();
  void SetAlpha(BL_FLOAT alpha);

  bool GetImageDataFloat(WDL_TypedBuf<unsigned char> *buf);
  bool GetColormapImageDataRGBA(WDL_TypedBuf<unsigned int> *colormapImageData);
  
 protected:
  int mWidth;
  int mHeight;

  WDL_TypedBuf<BL_FLOAT> mData;

  BL_FLOAT mRange;
  BL_FLOAT mContrast;
    
  ColorMapFactory *mColorMapFactory;
  ColorMap4 *mColorMap;

  BL_FLOAT mAlpha;
  
  bool mDataChanged;
  bool mColormapDataChanged;
};
#endif
