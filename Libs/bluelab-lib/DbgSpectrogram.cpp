//
//  Spectrum.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

//#include "ColorMap3.h"

//#include <BLDefs.h>

#define GLSL_COLORMAP 1
#include "ColorMap4.h"

#include <BLUtils.h>
#include "PPMFile.h"

#include <BLUtilsDecim.h>

#include "DbgSpectrogram.h"


DbgSpectrogram::DbgSpectrogram(int height, int maxCols,
                               Scale::Type scale)
{
    mHeight = height;
    mMaxCols = maxCols;

    //mYLogScale = false;
    mYScale = scale;
    
    //mYLogScaleFactor = 1.0;
    
    if (mMaxCols > 0)
    {
        // At the beginning, fill with zero values
        // This avoid the effect of "reversed scrolling",
        // when the spectrogram is not totally full
        FillWithZeros();
    }
    
    mAmpDb = false;
}

DbgSpectrogram::~DbgSpectrogram() {}

#if 0
void
DbgSpectrogram::SetYLogScale(bool flag) //, BL_FLOAT factor)
{
    mYLogScale = flag;
    //mYLogScaleFactor = factor;
}
#endif

void
DbgSpectrogram::SetYScale(Scale::Type scale)
{
    mYScale = scale;
}

void
DbgSpectrogram::SetAmpDb(bool flag)
{
    mAmpDb = flag;
}

void
DbgSpectrogram::Reset()
{
    mMagns.clear();
}

void
DbgSpectrogram::Reset(int height, int maxCols)
{
    Reset();
    
    mHeight = height;
    mMaxCols = maxCols;
    
    if (mMaxCols > 0)
    {
        // At the beginning, fill with zero values
        // This avoid the effect of "reversed scrolling",
        // when the spectrogram is not totally full
        FillWithZeros();
    }
}

int
DbgSpectrogram::GetNumCols()
{
    return mMagns.size();
}

int
DbgSpectrogram::GetMaxNumCols()
{
    return mMaxCols;
}

int
DbgSpectrogram::GetHeight()
{
    return mHeight;
}

void
DbgSpectrogram::AddLine(const WDL_TypedBuf<BL_FLOAT> &magns)
{
    if (magns.GetSize() < mHeight)
    {
        return;
    }
    
    WDL_TypedBuf<BL_FLOAT> magns0 = magns;
#if 0
    if (mYLogScale)
    {
#if !USE_DEFAULT_SCALE_MEL
        Scale::ApplyScale(Scale::LOG_FACTOR, &magns0);
#else
        Scale tmpScale;
        tmpScale.ApplyScale(Scale::MEL, &magns0);
#endif
    }
#endif
    
    Scale tmpScale;
    tmpScale.ApplyScale(mYScale, &magns0);
    
    if (magns0.GetSize() > mHeight)
    {
        BLUtilsDecim::DecimateSamples(&magns0,
                                      ((BL_FLOAT)mHeight)/magns0.GetSize());
    }
    
    if (mAmpDb)
    {
        // Convert amp to dB
        // (Works like a charm !)
        WDL_TypedBuf<BL_FLOAT> dbMagns;
        BLUtils::AmpToDBNorm(&dbMagns, magns0, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
        magns0 = dbMagns;
    }
    
    mMagns.push_back(magns0);
    
    if (mMaxCols > 0)
    {
        if (mMagns.size() > mMaxCols)
        {
            mMagns.pop_front();
        }
    }
}

bool
DbgSpectrogram::GetLine(int index, WDL_TypedBuf<BL_FLOAT> *magns)
{
    if (index >= mMagns.size())
        return false;
        
    *magns = mMagns[index];
        
    return true;
}

void
DbgSpectrogram::FillWithZeros()
{
    WDL_TypedBuf<BL_FLOAT> zeros;
    zeros.Resize(mHeight);
    BLUtils::FillAllZero(&zeros);
    
    for (int i = 0; i < mMaxCols; i++)
    {
        AddLine(zeros);
    }
}

void
DbgSpectrogram::SavePPM(const char *filename, int maxValue)
{
    if (mMagns.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > phasesUnW;
        
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    
    // Header
    fprintf(file, "P3\n");
    fprintf(file, "%ld %d\n", mMagns.size(), mMagns[0].GetSize());
    fprintf(file, "%d\n", maxValue);
    
    // Data
    for (int i = mHeight - 1; i >= 0 ; i--)
    {
        for (int j = 0; j < mMagns.size(); j++)
        {
            const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
            
            BL_FLOAT magnValue = magns.Get()[i];
            
            // Increase
            //magnValue = std::sqrt(magnValue);
            //magnValue = std::sqrt(magnValue);
            
            BL_FLOAT magnColor = magnValue*(BL_FLOAT)maxValue;
            if (magnColor > maxValue)
                magnColor = maxValue;
            
            fprintf(file, "%d %d %d\n", (int)magnColor, (int)magnColor, (int)magnColor);
        }
        
        fprintf(file, "\n");
    }
    fclose(file);
}

DbgSpectrogram *
DbgSpectrogram::ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits)
{
    BL_FLOAT ppmMultiplier = is16Bits ? 65535.0 : 255.0;
    
    DbgSpectrogram *result = new DbgSpectrogram(image->h);
    for (int i = 0; i < image->w; i++)
    {
        WDL_TypedBuf<BL_FLOAT> magns;
        magns.Resize(image->h);
        
        for (int j = 0; j < image->h; j++)
        {
            BL_FLOAT magnColor;
            
            if (!is16Bits)
            {
                PPMFile::PPMPixel pix = image->data[i + (image->h - j - 1)*image->w];
                magnColor = pix.red;
            }
            else
            {
                PPMFile::PPMPixel16 pix = image->data16[i + (image->h - j - 1)*image->w];
                magnColor = pix.red;
            }
            
            // Magn
            BL_FLOAT magnValue = magnColor/ppmMultiplier;
            
            magnValue = magnValue*magnValue;
            magnValue = magnValue*magnValue;
            
            magns.Get()[j] = magnValue;
        }
        
        result->AddLine(magns);
    }
    
    free(image);
    
    return result;
}

DbgSpectrogram *
DbgSpectrogram::ImagesToSpectrogram(PPMFile::PPMImage *magnsImage)
{
    DbgSpectrogram *result = new DbgSpectrogram(magnsImage->h);
    
    for (int i = 0; i < magnsImage->w; i++)
    {
        WDL_TypedBuf<BL_FLOAT> magns;
        magns.Resize(magnsImage->h);
        
        for (int j = 0; j < magnsImage->h; j++)
        {
            float magnValue;
            
            // Magn
            PPMFile::PPMPixel16 magnPix =
                    magnsImage->data16[i + (magnsImage->h - j - 1)*magnsImage->w];
            ((short *)&magnValue)[0] = magnPix.red;
            ((short *)&magnValue)[1] = magnPix.green;
            
            magns.Get()[j] = magnValue;
        }
        
        result->AddLine(magns);
    }
    
    free(magnsImage);
    
    return result;
}

DbgSpectrogram *
DbgSpectrogram::Load(const char *fileName)
{
    // TODO
    return NULL;
}

void
DbgSpectrogram::Save(const char *filename)
{
    // Magns
    char fullFilenameMagns[MAX_PATH];
    sprintf(fullFilenameMagns, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s-magns", filename);
    
    FILE *magnsFile = fopen(fullFilenameMagns, "w");
    
    for (int j = 0; j < mMagns.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
        
        for (int i = 0; i < mHeight; i++)
        {
            fprintf(magnsFile, "%g ", magns.Get()[i]);
        }
        
        fprintf(magnsFile, "\n");
    }
    fclose(magnsFile);
}

DbgSpectrogram *
DbgSpectrogram::LoadPPM(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM(fullFilename);
    
    DbgSpectrogram *result = ImageToSpectrogram(image, false);
    
    return result;
}

void
DbgSpectrogram::SavePPM(const char *filename)
{
    SavePPM(filename, 255);
}

DbgSpectrogram *
DbgSpectrogram::LoadPPM16(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM16(fullFilename);
    
    DbgSpectrogram *result = ImageToSpectrogram(image, true);
    
    return result;
}

void
DbgSpectrogram::SavePPM16(const char *filename)
{
    SavePPM(filename, 65535);
}

DbgSpectrogram *
DbgSpectrogram::LoadPPM32(const char *filename)
{
    // Magns
    char magnsFullFilename[MAX_PATH];
    sprintf(magnsFullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s-magns.ppm", filename);
    
    PPMFile::PPMImage *magnsImage = PPMFile::ReadPPM16(magnsFullFilename);
    
    // Phases
    char phasesFullFilename[MAX_PATH];
    sprintf(phasesFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-phases.ppm", filename);
    
    DbgSpectrogram *result = ImagesToSpectrogram(magnsImage);
    
    return result;
}

void
DbgSpectrogram::SavePPM32(const char *filename)
{
    if (mMagns.empty())
        return;
    
    int maxValue = 65000;
    
    // Magns
    char magnsFullFilename[MAX_PATH];
    sprintf(magnsFullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s-magns.ppm", filename);
    FILE *magnsFile = fopen(magnsFullFilename, "w");
    
    // Header
    fprintf(magnsFile, "P3\n");
    fprintf(magnsFile, "%ld %d\n", mMagns.size(), mMagns[0].GetSize());
    fprintf(magnsFile, "%d\n", maxValue);
    
    // Data
    for (int i = mHeight - 1; i >= 0 ; i--)
    {
        for (int j = 0; j < mMagns.size(); j++)
        {
            const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
            
            float magnValue = magns.Get()[i];
            fprintf(magnsFile, "%d %d 0\n", ((short *)&magnValue)[0], ((short *)&magnValue)[1]);
        }
        
        fprintf(magnsFile, "\n");
    }
    fclose(magnsFile);
}


