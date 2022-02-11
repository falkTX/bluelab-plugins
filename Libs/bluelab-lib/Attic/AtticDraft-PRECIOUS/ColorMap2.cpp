//
//  ColorMap.cpp
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#include "ColorMap2.h"

ColorMap2::ColorMap2(const unsigned char startColor[3],
                    const unsigned char endColor[3])
{
    for (int k = 0; k < 3; k++)
        mStartColor[k] = startColor[k];
    
    for (int k = 0; k < 3; k++)
        mEndColor[k] = endColor[k];
    
    mRange = 0.0;
    mContrast = 0.5;
    
    Generate(startColor, endColor, &mColors);
    
    mSize = mColors.GetSize();
    mBuf = mColors.Get();
}

ColorMap2::~ColorMap2() {}

void
ColorMap2::SetRange(double range)
{
    mRange = range;
    
    Generate(mStartColor, mEndColor, &mColors);
}

void
ColorMap2::SetContrast(double contrast)
{
    mContrast = contrast;
    
    Generate(mStartColor, mEndColor, &mColors);
}

void
ColorMap2::GetColor(double t, CmColor *color)
{
    int colIndex = t*mSize;
    
    if (colIndex < 0)
        colIndex = 0;
    
    if (colIndex > mSize - 1)
        colIndex = mSize - 1;
    
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
ColorMap2::Generate(const unsigned char startColor[3],
                    const unsigned char endColor[3],
                    WDL_TypedBuf<CmColor> *result)
{
    result->Resize(256*256);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        double t = ((double)i)/result->GetSize();
        
        double tOrig = t;
        
#if 1
        // Apply gamma
        t = pow(t, 1.0/COLORMAP_GAMMA);
#endif
        
        // Apply range
        if (mRange > 0.0)
        {
            double t0 = 0.0;
            double t1 = 1.0 - range;
            
            t = (t - t0)/(t1 - t0);
            //t = t*(t1 - t0) + t0;
        } else // range >= 0.0
        {
            double t0 = -mRange;
            double t1 = 1.0;
            
            t = (t - t0)/(t1 - t0);
            //t = t*(t1 - t0) + t0;
        }
        
        if (t < 0.0)
            t = 0.0;
        if (t > 1.0)
            t = 1.0;
        
        // Hue
        unsigned char col[4] = { 0, 0, 0, 255 };
        for (int k = 0; k < 3; k++)
        {
            col[k] = (1.0 - tOrig)*startColor[k] + t*endColor[k];
        }
        
#if 1
        // Lightness
        for (int k = 0; k < 3; k++)
        {
            // Make less contrast
            //t = sqrt(t);
            
            col[k] = (int)(t*col[k]);
        }
#endif
        
        result->Get()[i] = *((CmColor *)&col);
    }
    
    // DEBUG
    //SavePPM("colormap.ppm");
}