//
//  ColorMap4.cpp
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#include "resource.h"

#include <BLUtils.h>

#include "ColorMap4.h"

// Avoid having a range of 1 => would made binary image
#define MAX_RANGE 0.996


#define EPS 1e-15

#define GHOST_OPTIM 1


ColorMap4::ColorMap4(bool glsl)
{
    mRange = 0.0;
    mContrast = 0.5;
    
    mSize = 0;
    mBuf = NULL;
    
    mUseGLsl = glsl;
}

ColorMap4::~ColorMap4() {}

void
ColorMap4::AddColor(unsigned char r, unsigned char g,
                    unsigned char b, unsigned char a,
                    BL_FLOAT t)
{
    unsigned char col8[4] = { r, g, b, a };
    
    if (mUseGLsl) // FIX_STEREO_VIZ: The blue colormap was brown in StereoViz (bad color inversion)
    {
        col8[0] = b;
        col8[2] = r;
    }
    
    CmColor col = *((CmColor *)&col8);
    
    mColors.push_back(col);
    mTs.push_back(t); // t*t seems very good !
}

void
ColorMap4::Generate()
{
    Generate(&mResultColors);
    
    mSize = mResultColors.GetSize();
    mBuf = mResultColors.Get();
}

void
ColorMap4::SetRange(BL_FLOAT range)
{
#if GHOST_OPTIM
  if (std::fabs((BL_FLOAT)(mRange - range*MAX_RANGE)) < EPS)
        return;
#endif

    mRange = range*MAX_RANGE;
    
    Generate();
}

BL_FLOAT
ColorMap4::GetRange()
{
    return mRange;
}

void
ColorMap4::SetContrast(BL_FLOAT contrast)
{
#if GHOST_OPTIM
  if (std::fabs(mContrast - contrast) < EPS)
        return;
#endif
    
    mContrast = contrast;
    
    Generate();
}

BL_FLOAT
ColorMap4::GetContrast()
{
    return mContrast;
}

void
ColorMap4::GetColor(BL_FLOAT t, CmColor *color)
{
    if (t < 0.0)
        t = 0.0;
    
    if (t > 1.0)
        t = 1.0;
    
    int colIndex = t*(mSize - 1);
    
    *color = mBuf[colIndex];
}

void
ColorMap4::SavePPM(const char *fileName)
{
    if (mResultColors.GetSize() == 0)
        return;
    
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", fileName);
    
    FILE *file = fopen(fullFilename, "w");
    if (file == NULL)
        return;
    
    // Header
    fprintf(file, "P3\n");
    fprintf(file, "%d %d\n", mResultColors.GetSize(), 1);
    fprintf(file, "%d\n", 255);
    
    //Data
    for (int i = 0; i < mResultColors.GetSize() ; i++)
    {
        CmColor &col = mResultColors.Get()[i];
        
        unsigned char *rgb = (unsigned char *)&col;
        
        fprintf(file, "%d %d %d\n", rgb[0], rgb[1], rgb[2]);
        
        fprintf(file, "\n");
    }

    fclose(file);
}

void
ColorMap4::GetDataRGBA(WDL_TypedBuf<CmColor> *result)
{
    *result = mResultColors;
}

void
ColorMap4::Generate(WDL_TypedBuf<CmColor> *result)
{
    long colorMapSize = (!mUseGLsl) ? 256*256 : 4096;
    
    result->Resize(colorMapSize);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/result->GetSize();
        
        // Apply contrast/gamma
        t = std::pow(t, SAMPLE_TYPE(1.0/((1.0 - mContrast)*COLORMAP_GAMMA)));
        
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
        for (int j = 0; j < mTs.size() - 1; j++)
        {
            BL_FLOAT colT0 = mTs[j];
            BL_FLOAT colT1 = mTs[j + 1];
            
            if ((t >= colT0) && (t <= colT1))
            {
                BL_FLOAT t0 = (t - colT0)/(colT1 - colT0);
                
                const CmColor &col0 = mColors[j];
                const CmColor &col1 = mColors[j + 1];
                
                unsigned char *ucCol0 = (unsigned char *)&col0;
                unsigned char *ucCol1 = (unsigned char *)&col1;
            
                for (int k = 0; k < 4; k++)
                {
                    col[k] = (1.0 - t0)*ucCol0[k] + t0*ucCol1[k];
                    if (col[k] < 0)
                        col[k] = 0;
                    if (col[k] > 255)
                        col[k] = 255;
                }
            }
        }
        
        unsigned char col8NoGLsl[4] = { (unsigned char)col[0],
                                        (unsigned char)col[1],
                                        (unsigned char)col[2],
                                        (unsigned char)col[3] };
        // Swap colors here (instead of later)
        unsigned char col8GLsl[4] = { (unsigned char)col[2],
                                      (unsigned char)col[1],
                                      (unsigned char)col[0],
                                      (unsigned char)col[3] };


        if (mUseGLsl)
            result->Get()[i] = *((CmColor *)&col8GLsl);
        else
            result->Get()[i] = *((CmColor *)&col8NoGLsl);
    }
    
    // DEBUG
    //SavePPM("colormap.ppm");
}

