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
//  ColorMap.cpp
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#include <BLUtils.h>

#include "ColorMap.h"

ColorMap::ColorMap(const unsigned char startColor[3],
                   const unsigned char endColor[3])
{
    Generate(startColor, endColor, &mColors);
}

ColorMap::~ColorMap() {}

void
ColorMap::GetColor(BL_FLOAT t, unsigned char color[3])
{
    int colIndex = t*mColors.GetSize();
    
    if (colIndex < 0)
        colIndex = 0;
    
    if (colIndex > mColors.GetSize() - 1)
        colIndex = mColors.GetSize() - 1;
    
    const Color &col = mColors.Get()[colIndex];
    
    for (int k = 0; k < 3; k++)
    {
        color[k] = col.rgb[k];
    }
}

void
ColorMap::SavePPM(const char *fileName)
{
    if (mColors.GetSize() == 0)
        return;
    
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", fileName);
    
    FILE *file = fopen(fullFilename, "w");
    
    // Header
    fprintf(file, "P3\n");
    fprintf(file, "%d %d\n", mColors.GetSize(), 1);
    fprintf(file, "%d\n", 255);
    
    // Data
    for (int i = 0; i < mColors.GetSize() ; i++)
    {
        Color &col = mColors.Get()[i];
            
        fprintf(file, "%d %d %d\n",
                col.rgb[0], col.rgb[1], col.rgb[2]);
        
        fprintf(file, "\n");
    }

    fclose(file);
}

void
ColorMap::Generate(const unsigned char startColor[3],
                   const unsigned char endColor[3],
                   WDL_TypedBuf<Color> *result)
{
    result->Resize(256*256);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/result->GetSize();
        
        // Hue
        unsigned char col[3];
        for (int k = 0; k < 3; k++)
        {
            BL_FLOAT t0 = t; //t*t;
            
            col[k] = (1.0 - t0)*startColor[k] + t0*endColor[k];
        }
        
        // Lightness
        for (int k = 0; k < 3; k++)
        {
            BL_FLOAT coeff = t;
            
            coeff = std::sqrt(coeff);
            coeff = std::sqrt(coeff);
            
            col[k] = (int)(coeff*col[k]);
        }
        
        for (int k = 0; k < 3; k++)
        {
            result->Get()[i].rgb[k] = col[k];
        }
    }
    
    // DEBUG
    //SavePPM("colormap.ppm");
}
