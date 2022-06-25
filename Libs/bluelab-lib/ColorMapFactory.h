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
//  ColorMapFactory.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/10/20.
//
//

#ifndef __BL_SoundMetaViewer__ColorMapFactory__
#define __BL_SoundMetaViewer__ColorMapFactory__

class ColorMap4;
class ColorMapFactory
{
public:
    enum ColorMap
    {
      COLORMAP_BLUE = 0,
      COLORMAP_DAWN,
      COLORMAP_DAWN_FIXED, //
      COLORMAP_RAINBOW,
      COLORMAP_RAINBOW_LINEAR,
      COLORMAP_PURPLE,
      COLORMAP_SWEET,
      COLORMAP_OCEAN,
      COLORMAP_WASP,
        
      COLORMAP_GREEN,
      COLORMAP_GREY,
      COLORMAP_GREEN_RED,
      COLORMAP_SWEET2,
      COLORMAP_SKY,
      COLORMAP_LANDSCAPE,
      COLORMAP_FIRE,
      COLORMAP_PURPLE2
    };
    
    ColorMapFactory(bool useGLSL, bool transparentColorMap);
    
    virtual ~ColorMapFactory();
    
    ColorMap4 *CreateColorMap(ColorMap colorMapId);
    
protected:
    bool mUseGLSL;
    bool mTransparentColorMap;
};

#endif /* defined(__BL_SoundMetaViewer__ColorMapFactory__) */
