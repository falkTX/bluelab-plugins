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

#include <BLTypes.h>
#include <BLUtils.h>

#include "ColorMap2.h"

// Avoid having a range of 1 => would made binary image
#define MAX_RANGE 0.996

ColorMap2::ColorMap2(const unsigned char startColor[3],
                    const unsigned char endColor[3])
{
    for (int k = 0; k < 4; k++)
        mStartColor[k] = startColor[k];
    
    for (int k = 0; k < 4; k++)
        mEndColor[k] = endColor[k];
    
    mRange = 0.0;
    mContrast = 0.5;
    
    Generate(startColor, endColor, &mColors);
    
    mSize = mColors.GetSize();
    mBuf = mColors.Get();
}

ColorMap2::~ColorMap2() {}

void
ColorMap2::SetRange(BL_FLOAT range)
{
    mRange = range*MAX_RANGE;
    
    Generate(mStartColor, mEndColor, &mColors);
}

void
ColorMap2::SetContrast(BL_FLOAT contrast)
{
    mContrast = contrast;
    
    Generate(mStartColor, mEndColor, &mColors);
}

void
ColorMap2::GetColor(BL_FLOAT t, CmColor *color)
{
    if (t < 0.0)
        t = 0.0;
    
    if (t > 1.0)
        t = 1.0;
    
    int colIndex = t*(mSize - 1);
    
    *color = mBuf[colIndex];
}

void
ColorMap2::SavePPM(const char *fileName)
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
    
    //Data
    for (int i = 0; i < mColors.GetSize() ; i++)
    {
        CmColor &col = mColors.Get()[i];
        
        unsigned char *rgb = (unsigned char *)&col;
        
        fprintf(file, "%d %d %d\n", rgb[0], rgb[1], rgb[2]);
        
        fprintf(file, "\n");
    }

    fclose(file);
}

void
ColorMap2::Generate(const unsigned char startColor[4],
                    const unsigned char endColor[4],
                    WDL_TypedBuf<CmColor> *result)
{
    result->Resize(256*256);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/result->GetSize();
        
        BL_FLOAT t2[2] = { t, t };
        
        // Apply gamma
        t2[0] = std::pow(t2[0], 1.0/((1.0 - mContrast)*COLORMAP_GAMMA));
        
        for (int k = 0; k < 2; k++)
        {
            // Apply range
            if (mRange > 0.0)
            {
                BL_FLOAT t0 = 0.0;
                BL_FLOAT t1 = 1.0 - mRange;
            
                t2[k] = (t2[k] - t0)/(t1 - t0);
            } else // range >= 0.0
            {
                BL_FLOAT t0 = -mRange;
                BL_FLOAT t1 = 1.0;
            
                t2[k] = (t2[k] - t0)/(t1 - t0);
            }
        
            if (t2[k] < 0.0)
                t2[k] = 0.0;
            if (t2[k] > 1.0)
                t2[k] = 1.0;
            
            // Hack (maybe)
            // This is better like this, since later we multiply lightness
            // then t would have been take two times
            t2[k] = std::sqrt(t2[k]);
        }
        
        // Result
        int col[4];
        
        // Hue
        BL_FLOAT th = t2[1];
        for (int k = 0; k < 4; k++)
        {
            col[k] = (1.0 - th)*startColor[k] + th*endColor[k];
            if (col[k] < 0)
                col[k] = 0;
            if (col[k] > 255)
                col[k] = 255;
        }
        
        // Lightness
        BL_FLOAT tl = t2[0];
        for (int k = 0; k < 3; k++)
        {
            col[k] = (int)(tl*col[k]);
        }
        
        unsigned char col8[4] = { (unsigned char)col[0],
                                  (unsigned char)col[1],
                                  (unsigned char)col[2],
                                  (unsigned char)col[3] };
        
        result->Get()[i] = *((CmColor *)&col8);
    }
    
    // DEBUG
    //SavePPM("colormap.ppm");
}
