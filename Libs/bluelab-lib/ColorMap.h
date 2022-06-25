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
//  ColorMap.h
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#ifndef __BL_Spectrogram__ColorMap__
#define __BL_Spectrogram__ColorMap__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class ColorMap
{
public:
    ColorMap(const unsigned char startColor[3],
             const unsigned char endColor[3]);
    
    virtual ~ColorMap();
    
    void GetColor(BL_FLOAT t, unsigned char color[3]);
    
    void SavePPM(const char *fileName);
    
protected:
    typedef struct
    {
        unsigned char rgb[3];
    } Color;
    
    void Generate(const unsigned char startColor[3],
                  const unsigned char endColor[3],
                  WDL_TypedBuf<Color> *result);
    
    WDL_TypedBuf<Color> mColors;
};

#endif /* defined(__BL_Spectrogram__ColorMap__) */
