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
//  ColorMap3.cpp
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#include <BLUtils.h>

#include "ColorMap3.h"

#define GLSL_COLORMAP 1

// Avoid having a range of 1 => would made binary image
#define MAX_RANGE 0.996

#if !GLSL_COLORMAP
// Ok for soft colormap
#define COLORMAP_SIZE 256*256
#else
#define COLORMAP_SIZE 4096
#endif

ColorMap3::ColorMap3(const unsigned char startColor[3],
                     const unsigned char endColor[3],
                     BL_FLOAT blackLimit, BL_FLOAT whiteLimit)
{
    for (int k = 0; k < 4; k++)
        mStartColor[k] = startColor[k];
    
    for (int k = 0; k < 4; k++)
        mEndColor[k] = endColor[k];
    
    mRange = 0.0;
    mContrast = 0.5;
    
    mBlackLimit = blackLimit;
    mWhiteLimit = whiteLimit;
    
    Generate3(startColor, endColor, &mColors);
    
    mSize = mColors.GetSize();
    mBuf = mColors.Get();
}

ColorMap3::~ColorMap3() {}

void
ColorMap3::SetRange(BL_FLOAT range)
{
    mRange = range*MAX_RANGE;
    
    Generate3(mStartColor, mEndColor, &mColors);
}

void
ColorMap3::SetContrast(BL_FLOAT contrast)
{
    mContrast = contrast;
    
    Generate3(mStartColor, mEndColor, &mColors);
}

void
ColorMap3::GetColor(BL_FLOAT t, CmColor *color)
{
    if (t < 0.0)
        t = 0.0;
    
    if (t > 1.0)
        t = 1.0;
    
    int colIndex = t*(mSize - 1);
    
    *color = mBuf[colIndex];
}

void
ColorMap3::SavePPM(const char *fileName)
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
ColorMap3::GetDataRGBA(WDL_TypedBuf<CmColor> *result)
{
    *result = mColors;
}

void
ColorMap3::Generate(const unsigned char startColor[4],
                    const unsigned char endColor[4],
                    WDL_TypedBuf<CmColor> *result)
{
    result->Resize(COLORMAP_SIZE);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/result->GetSize();
        
        //t = BLUtils::AmpToDBNorm(t, 1e-15, -120.0);
        //t = BLUtils::DBToAmpNorm(t, 1e-15, -120.0);
        
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
            //t2[k] = std::sqrt(t2[k]);
        }
        
        // Lightness
        t2[0] = std::sqrt(t2[0]);
        
        // Hue
        t2[1] = std::sqrt(t2[1]);
        
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
        
#if !GLSL_COLORMAP
        unsigned char col8[4] = { (unsigned char)col[0],
                                  (unsigned char)col[1],
                                  (unsigned char)col[2],
                                  (unsigned char)col[3] };
#else
        // Swap colors here (instead of later)
        unsigned char col8[4] = { (unsigned char)col[2],
                                  (unsigned char)col[1],
                                  (unsigned char)col[0],
                                  (unsigned char)col[3] };
#endif
        
        result->Get()[i] = *((CmColor *)&col8);
    }
    
    // DEBUG
    //SavePPM("colormap.ppm");
}

void
ColorMap3::Generate2(const unsigned char startColor[4],
                     const unsigned char endColor[4],
                     WDL_TypedBuf<CmColor> *result)
{
//#define BLACK_LIMIT 0.5

    result->Resize(COLORMAP_SIZE);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/result->GetSize();
        
        // Apply contrast/gamma
        t = std::pow(t, 1.0/((1.0 - mContrast)*COLORMAP_GAMMA));
        
        // Apply range
        if (mRange > 0.0)
        {
            BL_FLOAT t0 = 0.0;
            BL_FLOAT t1 = 1.0 - mRange;
                
            t = (t - t0)/(t1 - t0);
        } else // range >= 0.0
        {
            BL_FLOAT t0 = -mRange;
            BL_FLOAT t1 = 1.0;
                
            t = (t - t0)/(t1 - t0);
        }
            
        if (t < 0.0)
            t = 0.0;
        if (t > 1.0)
            t = 1.0;
            
        // This seems to look better like this
        t = std::sqrt(t);
        
        // Result
        int col[4];
        
        if (t < mBlackLimit) //BLACK_LIMIT)
        {
            // From black to first color
            BL_FLOAT t0 = t/mBlackLimit/*BLACK_LIMIT*/;
            
            unsigned char blackColor[4] = { 0, 0, 0, 255 };
            
            for (int k = 0; k < 4; k++)
            {
                col[k] = (1.0 - t0)*blackColor[k] + t0*startColor[k];
                if (col[k] < 0)
                    col[k] = 0;
                if (col[k] > 255)
                    col[k] = 255;
            }
        }
        else
        {
            // From first color to second color
            BL_FLOAT t0 = (t - mBlackLimit/*BLACK_LIMIT*/)/(1.0 - mBlackLimit/*BLACK_LIMIT*/);
            
            for (int k = 0; k < 4; k++)
            {
                col[k] = (1.0 - t0)*startColor[k] + t0*endColor[k];
                if (col[k] < 0)
                    col[k] = 0;
                if (col[k] > 255)
                    col[k] = 255;
            }
        }
        
#if !GLSL_COLORMAP
        unsigned char col8[4] = { (unsigned char)col[0],
                                  (unsigned char)col[1],
                                  (unsigned char)col[2],
                                  (unsigned char)col[3] };
#else
        // Swap colors here (instead of later)
        unsigned char col8[4] = { (unsigned char)col[2],
                                  (unsigned char)col[1],
                                  (unsigned char)col[0],
                                  (unsigned char)col[3] };
#endif
        
        result->Get()[i] = *((CmColor *)&col8);
    }
    
    // DEBUG
    //SavePPM("colormap.ppm");
}

void
ColorMap3::Generate3(const unsigned char startColor[4],
                     const unsigned char endColor[4],
                     WDL_TypedBuf<CmColor> *result)
{
    result->Resize(COLORMAP_SIZE);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/result->GetSize();
        
        // Apply contrast/gamma
        t = std::pow(t, 1.0/((1.0 - mContrast)*COLORMAP_GAMMA));
        
        // Apply range
        if (mRange > 0.0)
        {
            BL_FLOAT t0 = 0.0;
            BL_FLOAT t1 = 1.0 - mRange;
            
            t = (t - t0)/(t1 - t0);
        } else // range >= 0.0
        {
            BL_FLOAT t0 = -mRange;
            BL_FLOAT t1 = 1.0;
            
            t = (t - t0)/(t1 - t0);
        }
        
        if (t < 0.0)
            t = 0.0;
        if (t > 1.0)
            t = 1.0;
        
        // This seems to look better like this
        t = std::sqrt(t);
        
        // Result
        int col[4];
        
        if (t < mBlackLimit)
        {
            // From black to first color
            BL_FLOAT t0 = t/mBlackLimit;
            
            unsigned char blackColor[4] = { 0, 0, 0, 255 };
            
            for (int k = 0; k < 4; k++)
            {
                col[k] = (1.0 - t0)*blackColor[k] + t0*startColor[k];
                if (col[k] < 0)
                    col[k] = 0;
                if (col[k] > 255)
                    col[k] = 255;
            }
        }
        else
        {
            // From first color to second color
            if ((mWhiteLimit < 0.0) || (t < mWhiteLimit))
            {
                BL_FLOAT t0;
                if (mWhiteLimit < 0.0)
                // White limit not defined
                {
                    t0 = (t - mBlackLimit)/(1.0 - mBlackLimit);
                }
                else
                {
                    t0 = (t - mBlackLimit)/(mWhiteLimit - mBlackLimit);
                }
            
                for (int k = 0; k < 4; k++)
                {
                    col[k] = (1.0 - t0)*startColor[k] + t0*endColor[k];
                    if (col[k] < 0)
                        col[k] = 0;
                    if (col[k] > 255)
                        col[k] = 255;
                }
            }
            else
                // White limit defined, and t > white limit
            {
                BL_FLOAT t0 = (t - mWhiteLimit)/(1.0 - mWhiteLimit);
                
                unsigned char whiteColor[4] = { 255, 255, 255, 255 };
                
                for (int k = 0; k < 4; k++)
                {
                    col[k] = (1.0 - t0)*endColor[k] + t0*whiteColor[k];
                    if (col[k] < 0)
                        col[k] = 0;
                    if (col[k] > 255)
                        col[k] = 255;
                }
            }
        }
        
#if !GLSL_COLORMAP
        unsigned char col8[4] = { (unsigned char)col[0],
            (unsigned char)col[1],
            (unsigned char)col[2],
            (unsigned char)col[3] };
#else
        // Swap colors here (instead of later)
            unsigned char col8[4] = { (unsigned char)col[2],
            (unsigned char)col[1],
            (unsigned char)col[0],
            (unsigned char)col[3] };
#endif
        
        result->Get()[i] = *((CmColor *)&col8);
    }
    
    // DEBUG
    //SavePPM("colormap.ppm");
}

