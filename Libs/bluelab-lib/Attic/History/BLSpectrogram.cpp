//
//  Spectrum.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#include <BLUtils.h>
#include "ColorMap.h"
#include "PPMFile.h"

#include "BLSpectrogram.h"

#define MAX_PATH 512

#define MIN_DB -120.0
#define EPS_DB 1e-15

#define DPHASE_MULT_X 0.001
#define DPHASE_MULT_Y 0.01


BLSpectrogram::BLSpectrogram(int height, int maxCols)
{
    mHeight = height;
    mMaxCols = maxCols;
    
    mMagnsMultiplier = 1.0;
    mPhasesMultiplier = 1.0;
    mPhasesMultiplier2 = 1.0;
    
    mGain = 1.0;
    
    mDisplayMagns = true;
    mYLogScale = false;
    
    mDisplayPhasesX = false;
    mDisplayPhasesY = false;
    
    mDisplayDPhases = false;
    
    unsigned char col0[3] = { 0, 255, 0 };
    
    unsigned char col1[3] = { 255, 0, 0 };
    //unsigned char col1[3] = { 255, 128, 0 };
    
    mColorMap = new ColorMap(col0, col1);
    
    //mColorMap->SavePPM("colormap.ppm");
    
    // At the beginning, fill with zero values
    // This avoid the effect of "reversed scrolling",
    // when the spectrogram is not totally full
    FillWithZeros();
    
#if CLASS_PROFILE
    mTimer.Reset();
    mTimerCount = 0;
#endif
}

BLSpectrogram::~BLSpectrogram() {}

void
BLSpectrogram::SetMultipliers(BL_FLOAT magnMult, BL_FLOAT phaseMult)
{
    mMagnsMultiplier = magnMult;
    mPhasesMultiplier = phaseMult;
}

void
BLSpectrogram::SetGain(BL_FLOAT gain)
{
    mGain = gain;
}

void
BLSpectrogram::SetDisplayMagns(bool flag)
{
    mDisplayMagns = flag;
}

void
BLSpectrogram::SetYLogScale(bool flag)
{
    mYLogScale = flag;
}

void
BLSpectrogram::SetDisplayPhasesX(bool flag)
{
    mDisplayPhasesX = flag;
}

void
BLSpectrogram::SetDisplayPhasesY(bool flag)
{
    mDisplayPhasesY = flag;
}

void
BLSpectrogram::SetDisplayDPhases(bool flag)
{
    mDisplayDPhases = flag;
}

void
BLSpectrogram::Reset()
{
    mMagns.clear();
    mPhases.clear();
}

int
BLSpectrogram::GetNumCols()
{
    return mMagns.size();
}

int
BLSpectrogram::GetMaxNumCols()
{
    return mMaxCols;
}


int
BLSpectrogram::GetHeight()
{
    return mHeight;
}

BLSpectrogram *
BLSpectrogram::Load(const char *fileName)
{
    // TODO
    return NULL;
}

void
BLSpectrogram::Save(const char *filename)
{
    // Magns
    char fullFilenameMagns[MAX_PATH];
    sprintf(fullFilenameMagns, "/Volumes/HDD/Share/%s-magns", filename);
    
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
    
    // Phases
    char fullFilenamePhases[MAX_PATH];
    sprintf(fullFilenamePhases, "/Volumes/HDD/Share/%s-phases", filename);
    
    FILE *phasesFile = fopen(fullFilenamePhases, "w");
    
    for (int j = 0; j < mPhases.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &phases = mPhases[j];
        
        for (int i = 0; i < mHeight; i++)
        {
            fprintf(phasesFile, "%g ", phases.Get()[i]);
        }
        
        fprintf(phasesFile, "\n");
    }
    
    fclose(phasesFile);
}

BLSpectrogram *
BLSpectrogram::LoadPPM(const char *filename,
                      BL_FLOAT magnsMultiplier, BL_FLOAT phasesMultiplier)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM(fullFilename);
    
    BLSpectrogram *result = ImageToSpectrogram(image, false,
                                               magnsMultiplier, phasesMultiplier);
    
    return result;
}

void
BLSpectrogram::SavePPM(const char *filename)
{
    SavePPM(filename, 255);
}

BLSpectrogram *
BLSpectrogram::LoadPPM16(const char *filename,
                         BL_FLOAT magnsMultiplier, BL_FLOAT phasesMultiplier)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM16(fullFilename);
    
    BLSpectrogram *result = ImageToSpectrogram(image, true,
                                               magnsMultiplier, phasesMultiplier);
    
    return result;
}

void
BLSpectrogram::SavePPM16(const char *filename)
{
    SavePPM(filename, 65535);
}

BLSpectrogram *
BLSpectrogram::LoadPPM32(const char *filename)
{
    // Magns
    char magnsFullFilename[MAX_PATH];
    sprintf(magnsFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-magns.ppm", filename);
    
    PPMFile::PPMImage *magnsImage = PPMFile::ReadPPM16(magnsFullFilename);
    
    // Phases
    char phasesFullFilename[MAX_PATH];
    sprintf(phasesFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-phases.ppm", filename);
    
    PPMFile::PPMImage *phasesImage = PPMFile::ReadPPM16(phasesFullFilename);
    
    BLSpectrogram *result = ImagesToSpectrogram(magnsImage, phasesImage);
    
    return result;
}

void
BLSpectrogram::SavePPM32(const char *filename)
{
    if (mMagns.empty())
        return;
    
    int maxValue = 65000;
    
    // Magns
    char magnsFullFilename[MAX_PATH];
    sprintf(magnsFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-magns.ppm", filename);
    FILE *magnsFile = fopen(magnsFullFilename, "w");
    
    // Header
    fprintf(magnsFile, "P3\n");
    fprintf(magnsFile, "%ld %d\n", mMagns.size(), mMagns[0].GetSize());
    fprintf(magnsFile, "%d\n", maxValue);
    
    // Phases
    char phasesFullFilename[MAX_PATH];
    sprintf(phasesFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-phases.ppm", filename);
    FILE *phasesFile = fopen(phasesFullFilename, "w");
    
    // Header
    fprintf(phasesFile, "P3\n");
    fprintf(phasesFile, "%ld %d\n", mPhases.size(), mPhases[0].GetSize());
    fprintf(phasesFile, "%d\n", maxValue);
    
    
    // Data
    for (int i = mHeight - 1; i >= 0 ; i--)
    {
        for (int j = 0; j < mMagns.size(); j++)
        {
            const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
            const WDL_TypedBuf<BL_FLOAT> &phases = mPhases[j];
            
            float magnValue = magns.Get()[i];
            fprintf(magnsFile, "%d %d 0\n", ((short *)&magnValue)[0], ((short *)&magnValue)[1]);
            
            float phaseValue = phases.Get()[i];
            fprintf(phasesFile, "%d %d 0\n", ((short *)&phaseValue)[0], ((short *)&phaseValue)[1]);
        }
        
        fprintf(magnsFile, "\n");
        fprintf(phasesFile, "\n");
    }
    fclose(magnsFile);
    fclose(phasesFile);
}

void
BLSpectrogram::AddLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                       const WDL_TypedBuf<BL_FLOAT> &phases)
{
    if ((magns.GetSize() < mHeight) ||
        (phases.GetSize() < mHeight))
    {
        return;
    }
    
    WDL_TypedBuf<BL_FLOAT> magns0 = magns;
    WDL_TypedBuf<BL_FLOAT> phases0 = phases;
    
    if (mYLogScale)
    {
        BLUtils::LogScaleX(&magns0, (BL_FLOAT)3.0);
        BLUtils::LogScaleX(&phases0, (BL_FLOAT)3.0);
    }
    
    if ((magns0.GetSize() > mHeight) ||
        (phases0.GetSize() > mHeight))
    {
        BLUtils::DecimateSamples(&magns0,
                               ((BL_FLOAT)mHeight)/magns0.GetSize());
        
        BLUtils::DecimateSamples(&phases0,
                               ((BL_FLOAT)mHeight)/phases0.GetSize());
    }
    
    mMagns.push_back(magns0);
    mPhases.push_back(phases0);
    
    if (mMaxCols > 0)
    {
        if (mMagns.size() > mMaxCols)
        {
            mMagns.pop_front();
        }
        
        if (mPhases.size() > mMaxCols)
        {
            mPhases.pop_front();
        }
    }
}

bool
BLSpectrogram::GetLine(int index,
                     WDL_TypedBuf<BL_FLOAT> *magns,
                     WDL_TypedBuf<BL_FLOAT> *phases)
{
    if ((index >= mMagns.size()) ||
        (index >= mPhases.size()))
        return false;
        
    *magns = mMagns[index];
    *phases = mPhases[index];
        
    return true;
}

void
BLSpectrogram::GetImageDataRGBA(int width, int height, unsigned char *buf)
{
#if CLASS_PROFILE
    mTimer.Start();
#endif
    
    // Empty the buffer
    // Because the spectrogram may be not totally full
    memset(buf, 0, width*height*4);
    
    // Set alpha to 255
    for (int i = 0; i < width*height; i++)
    {
        buf[i*4 + 3] = 255;
    }
    
    // Unwrap phases
    vector<WDL_TypedBuf<BL_FLOAT> > phasesUnW;
    UnwrapPhases3(mPhases, &phasesUnW, mDisplayPhasesX, mDisplayPhasesY);
    
    if (mDisplayDPhases)
    {
        ComputeDPhases(&phasesUnW);
        
#if 0
        // ???
        // (We have the summation, so we must derivate two times)
        ComputeDPhases(&phasesUnW);
        
        // TEST
        ComputeDPhases(&phasesUnW);
#endif
    }
    
    int maxValue = 255;
    
    // Data
    for (int j = 0; j < mMagns.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
        const WDL_TypedBuf<BL_FLOAT> &phases = phasesUnW[j];
        
        for (int i = mHeight - 1; i >= 0 ; i--)
        {
            BL_FLOAT magnValue = magns.Get()[i];
            BL_FLOAT phaseValue = phases.Get()[i];
            
            if (mColorMap == NULL)
            {
                // Increase
	      magnValue = std::sqrt(magnValue);
	      magnValue = std::sqrt(magnValue);
                
                BL_FLOAT magnColor = magnValue*mMagnsMultiplier*(BL_FLOAT)maxValue;
                if (magnColor > maxValue)
                    magnColor = maxValue;
            
                BL_FLOAT phaseColor = phaseValue*mPhasesMultiplier*(BL_FLOAT)maxValue;
                if (phaseColor > maxValue)
                    phaseColor = maxValue;
            
                int imgIdx = (j + i*height)*4;
                buf[imgIdx] = 0;
                buf[imgIdx + 1] = (int)phaseColor;
                buf[imgIdx + 2] = (int)magnColor;
                buf[imgIdx + 3] = 255;
            }
            else
                // Apply colormap
            {
                // Magns
                unsigned char magnColor[3] = { 0, 0, 0 };
                if (mDisplayMagns)
                {
                    BL_FLOAT magnMult = magnValue;
                
                    // TESTS
                    // Increase contrast
                    //magnMult = magnMult*magnMult;
                    //magnMult = magnMult*magnMult;
                
                    //magnMult = std::sqrt(magnMult);
                    //magnMult = std::sqrt(magnMult);
                
                    //magnMult = BLUtils::AmpToDB(magnMult, EPS_DB, MIN_DB);
                    //magnMult = (magnMult - MIN_DB)/(-MIN_DB);
                    
                    magnMult *= mGain;
                
                    mColorMap->GetColor(magnMult, magnColor);
                }
                
                // Phases
                BL_FLOAT phaseColor = 0.0;
                if (mDisplayPhasesX || mDisplayPhasesY)
                {
                    BL_FLOAT phaseMult;
                    if (!mDisplayDPhases)
                    {
                        phaseMult = phaseValue*mPhasesMultiplier*mPhasesMultiplier2;
                    }
                    else
                    {
                        BL_FLOAT coeff;
                        if (mDisplayPhasesX && !mDisplayPhasesY)
                            coeff = DPHASE_MULT_X;
                        else
                            if (!mDisplayPhasesX && mDisplayPhasesY)
                                coeff = DPHASE_MULT_Y;
                        else
                            // both
                        {
                            // Take the smaller
                            coeff = (DPHASE_MULT_X < DPHASE_MULT_Y) ? DPHASE_MULT_X : DPHASE_MULT_Y;
                            
                            //coeff = DPHASE_MULT_X*DPHASE_MULT_Y;
                        }
                        
                        phaseMult = phaseValue*coeff;
                    }
                    
                    phaseColor = phaseMult*(BL_FLOAT)maxValue;
                    if (phaseColor > maxValue)
                        phaseColor = maxValue;
                }
                
                int imgIdx = (j + i*height)*4;
                
                buf[imgIdx] = (int)phaseColor;;
                buf[imgIdx + 1] = magnColor[1];
                buf[imgIdx + 2] = magnColor[0];
                buf[imgIdx + 3] = 255;
            }
        }
    }
    
#if CLASS_PROFILE
    mTimer.Stop();
    mTimerCount++;
    
    if (mTimerCount % 50 == 0)
    {
        long t = mTimer.Get();
        
        char message[512];
        sprintf(message, "elapsed: %ld ms\n", t);
        BLDebug::AppendMessage("profile.txt", message);
        
        mTimer.Reset();
    }
#endif
}

// NOTE: deque is very slow for direct access to elements,
// compared to vectors
//
void
BLSpectrogram::UnwrapPhases3(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                             vector<WDL_TypedBuf<BL_FLOAT> > *outPhases,
                             bool horizontal, bool vertical)
{
    if (inPhases.empty())
        return;
    
    outPhases->clear();
    
    // Convert deque to vec
    for (int i = 0; i < inPhases.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> line = inPhases[i];
        outPhases->push_back(line);
    }
    
    
    if (horizontal && vertical)
        mPhasesMultiplier2 = 0.015;
    else
        mPhasesMultiplier2 = 1.0;
    
    if (horizontal)
    {
        // Unwrap for horizontal lines
        for (int i = 0; i < mHeight; i++)
        {
            WDL_TypedBuf<BL_FLOAT> &phases0 = (*outPhases)[0];
            BL_FLOAT prevPhase = phases0.Get()[i];
            
            while(prevPhase < 0.0)
            {
                prevPhase += 2.0*M_PI;
            }
            
            BL_FLOAT incr = 0.0;
            for (int j = 0; j < outPhases->size(); j++)
            {
                WDL_TypedBuf<BL_FLOAT> &phases = (*outPhases)[j];
                
                BL_FLOAT phase = phases.Get()[i];
                
                if (phase < prevPhase)
                    phase += incr;
                
                while(phase < prevPhase)
                {
                    phase += 2.0*M_PI;
                    incr += 2.0*M_PI;
                }
                
                phases.Get()[i] = phase;
                
                prevPhase = phase;
            }
        }
    }
    
    if (vertical)
    {
        // Unwrap for vertical lines
        for (int j = 0; j < outPhases->size(); j++)
        {
            WDL_TypedBuf<BL_FLOAT> &phases = (*outPhases)[j];
            
            BL_FLOAT prevPhase = phases.Get()[0];
            while(prevPhase < 0.0)
            {
                prevPhase += 2.0*M_PI;
            }
            
            BL_FLOAT incr = 0.0;
            for (int i = 0; i < mHeight; i++)
            {
                BL_FLOAT phase = phases.Get()[i];
                
                if (phase < prevPhase)
                    phase += incr;
                
                while(phase < prevPhase)
                {
                    phase += 2.0*M_PI;
                    
                    incr += 2.0*M_PI;
                }
                
                phases.Get()[i] = phase;
                
                prevPhase = phase;
            }
        }
    }
}

void
BLSpectrogram::FillWithZeros()
{
    WDL_TypedBuf<BL_FLOAT> zeros;
    zeros.Resize(mHeight);
    BLUtils::FillAllZero(&zeros);
    
    for (int i = 0; i < mMaxCols; i++)
    {
        AddLine(zeros, zeros);
    }
}

void
BLSpectrogram::SavePPM(const char *filename, int maxValue)
{
    if (mMagns.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > phasesUnW;
    UnwrapPhases3(mPhases, &phasesUnW, mDisplayPhasesX, mDisplayPhasesY);
    
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", filename);
    
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
            const WDL_TypedBuf<BL_FLOAT> &phases = phasesUnW[j];
            
            BL_FLOAT magnValue = magns.Get()[i];
            
            // Increase
            magnValue = std::sqrt(magnValue);
            magnValue = std::sqrt(magnValue);
            
            BL_FLOAT magnColor = magnValue*mMagnsMultiplier*(BL_FLOAT)maxValue;
            if (magnColor > maxValue)
                magnColor = maxValue;
            
            BL_FLOAT phaseValue = phases.Get()[i];
            
            BL_FLOAT phaseColor = phaseValue*mPhasesMultiplier*(BL_FLOAT)maxValue;
            if (phaseColor > maxValue)
                phaseColor = maxValue;
            
            fprintf(file, "%d %d 0\n", (int)magnColor, (int)phaseColor);
        }
        
        fprintf(file, "\n");
    }
    fclose(file);
}

BLSpectrogram *
BLSpectrogram::ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits,
                                  BL_FLOAT magnsMultiplier, BL_FLOAT phasesMultiplier)
{
    BL_FLOAT ppmMultiplier = is16Bits ? 65535.0 : 255.0;
    
    BLSpectrogram *result = new BLSpectrogram(image->h);
    result->SetMultipliers(magnsMultiplier, phasesMultiplier);
    
    for (int i = 0; i < image->w; i++)
    {
        WDL_TypedBuf<BL_FLOAT> magns;
        magns.Resize(image->h);
        
        WDL_TypedBuf<BL_FLOAT> phases;
        phases.Resize(image->h);
        
        for (int j = 0; j < image->h; j++)
        {
            BL_FLOAT magnColor;
            BL_FLOAT phaseColor;
            
            if (!is16Bits)
            {
                PPMFile::PPMPixel pix = image->data[i + (image->h - j - 1)*image->w];
                magnColor = pix.red;
                phaseColor = pix.green;
            }
            else
            {
                PPMFile::PPMPixel16 pix = image->data16[i + (image->h - j - 1)*image->w];
                magnColor = pix.red;
                phaseColor = pix.green;
            }
            
            // Magn
            BL_FLOAT magnValue = magnColor/(magnsMultiplier*ppmMultiplier);
            
            magnValue = magnValue*magnValue;
            magnValue = magnValue*magnValue;
            
            magns.Get()[j] = magnValue;
            
            // Phase
            BL_FLOAT phaseValue = phaseColor/(phasesMultiplier*ppmMultiplier);
            
            phases.Get()[j] = phaseValue;
        }
        
        result->AddLine(magns, phases);
    }
    
    free(image);
    
    return result;
}

BLSpectrogram *
BLSpectrogram::ImagesToSpectrogram(PPMFile::PPMImage *magnsImage,
                                   PPMFile::PPMImage *phasesImage)
{
    BLSpectrogram *result = new BLSpectrogram(magnsImage->h);
    
    for (int i = 0; i < magnsImage->w; i++)
    {
        WDL_TypedBuf<BL_FLOAT> magns;
        magns.Resize(magnsImage->h);
        
        WDL_TypedBuf<BL_FLOAT> phases;
        phases.Resize(magnsImage->h);
        
        for (int j = 0; j < magnsImage->h; j++)
        {
            float magnValue;
            float phaseValue;
            
            // Magn
            PPMFile::PPMPixel16 magnPix =
                    magnsImage->data16[i + (magnsImage->h - j - 1)*magnsImage->w];
            ((short *)&magnValue)[0] = magnPix.red;
            ((short *)&magnValue)[1] = magnPix.green;
            
            // Phase
            PPMFile::PPMPixel16 phasePix =
                    phasesImage->data16[i + (phasesImage->h - j - 1)*phasesImage->w];
            ((short *)&phaseValue)[0] = phasePix.red;
            ((short *)&phaseValue)[1] = phasePix.green;
            
            magns.Get()[j] = magnValue;
            phases.Get()[j] = phaseValue;
        }
        
        result->AddLine(magns, phases);
    }
    
    free(magnsImage);
    free(phasesImage);
    
    return result;
}

void
BLSpectrogram::ComputeDPhases(vector<WDL_TypedBuf<BL_FLOAT> > *phasesUnW)
{
    vector<WDL_TypedBuf<BL_FLOAT> > dPhasesX;
    if (mDisplayPhasesX)
    {
        ComputeDPhasesX(*phasesUnW, &dPhasesX);
    }
    
    vector<WDL_TypedBuf<BL_FLOAT> > dPhasesY;
    if (mDisplayPhasesY)
    {
        ComputeDPhasesY(*phasesUnW, &dPhasesY);
    }
    
    if (!dPhasesX.empty() && !dPhasesY.empty())
    {
        for (int i = 0; i < dPhasesX.size(); i++)
        {
            WDL_TypedBuf<BL_FLOAT> phasesX = dPhasesX[i];
            WDL_TypedBuf<BL_FLOAT> phasesY = dPhasesY[i];
            
#if 1
            WDL_TypedBuf<BL_FLOAT> sumPhases;
            BLUtils::ComputeSum(phasesX, phasesY, &sumPhases);
            
            (*phasesUnW)[i] = sumPhases;
#endif
     
#if 0
            WDL_TypedBuf<BL_FLOAT> prodPhases;
            BLUtils::ComputeProduct(phasesX, phasesY, &prodPhases);
            
            (*phasesUnW)[i] = prodPhases;
#endif
        }
    }
    else
    {
        if (!dPhasesX.empty())
            *phasesUnW = dPhasesX;
        else
            if (!dPhasesY.empty())
                *phasesUnW = dPhasesY;
    }
}

void
BLSpectrogram::ComputeDPhasesX(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                               vector<WDL_TypedBuf<BL_FLOAT> > *dPhases)
{
    dPhases->resize(phasesUnW.size());
    
    for (int j = 0; j < phasesUnW.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &phases = phasesUnW[j];
        
        WDL_TypedBuf<BL_FLOAT> dPhases0;
        dPhases0.Resize(phases.GetSize());
                       
        BL_FLOAT prevPhase = 0;
        for (int i = 0; i < phases.GetSize(); i++)
        {
            BL_FLOAT p = phases.Get()[i];
            
            BL_FLOAT delta = p - prevPhase;
            delta = std::fabs(delta);
            
            dPhases0.Get()[i] = delta;
            
            prevPhase = p;
        }
        
        (*dPhases)[j] = dPhases0;
    }
}

void
BLSpectrogram::ComputeDPhasesY(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                               vector<WDL_TypedBuf<BL_FLOAT> > *dPhases)
{
    if (phasesUnW.empty())
        return;
    
    // Setup
    dPhases->resize(phasesUnW.size());
    for (int i = 0; i < phasesUnW.size(); i++)
    {
        (*dPhases)[i].Resize(phasesUnW[0].GetSize());
    }
    
    // Compute
    for (int i = 0; i < phasesUnW[0].GetSize(); i++)
    {
        BL_FLOAT prevPhase = phasesUnW[0].Get()[i];
        
        for (int j = 0; j < phasesUnW.size(); j++)
        {
            const WDL_TypedBuf<BL_FLOAT> &dPhases0 = phasesUnW[j];
            
            BL_FLOAT p = dPhases0.Get()[j];
            
            BL_FLOAT delta = p - prevPhase;
            delta = std::fabs(delta);
            
            (*dPhases)[j].Get()[i] = delta;
            
            prevPhase = p;
        }
    }
}

/// GARBAGE

#if 0

void
BLSpectrogram::UnwrapPhases()
{
    UnwrapPhases(&mPhases);
}

BL_FLOAT
BLSpectrogram::MapToPi(BL_FLOAT val)
{
    /* Map delta phase into +/- Pi interval */
    val =  fmod(val, 2.0*M_PI);
    if (val <= -M_PI)
        val += 2.0*M_PI;
    if (val > M_PI)
        val -= 2.0*M_PI;
    
    return val;
}

void
BLSpectrogram::UnwrapPhases(deque<WDL_TypedBuf<BL_FLOAT> > *ioPhases)
{
    if (ioPhases->empty())
        return;
    
#if 1
    // Unwrap for vertical lines
    for (int j = 0; j < ioPhases->size(); j++)
    {
        WDL_TypedBuf<BL_FLOAT> &phases = (*ioPhases)[j];
        
        BL_FLOAT prevPhase = phases.Get()[0];
        
        while(prevPhase < 0.0)
            prevPhase += 2.0*M_PI;
        
        for (int i = 0; i < mHeight; i++)
        {
            BL_FLOAT phase = phases.Get()[i];
            
            while(phase < prevPhase)
                phase += 2.0*M_PI;
            
            phases.Get()[i] = phase;
            
            prevPhase = phase;
        }
    }
#endif
    
#if 1
    // Unwrap for horizontal lines
    for (int i = 0; i < mHeight; i++)
    {
        WDL_TypedBuf<BL_FLOAT> &phases0 = (*ioPhases)[0];
        BL_FLOAT prevPhase = phases0.Get()[i];
        
        while(prevPhase < 0.0)
            prevPhase += 2.0*M_PI;
        
        for (int j = 0; j < mPhases.size(); j++)
        {
            WDL_TypedBuf<BL_FLOAT> &phases = (*ioPhases)[j];
            
            BL_FLOAT phase = phases.Get()[i];
            
            while(phase < prevPhase)
                phase += 2.0*M_PI;
            
            phases.Get()[i] = phase;
            
            prevPhase = phase;
        }
    }
#endif
}

void
BLSpectrogram::UnwrapPhases2(deque<WDL_TypedBuf<BL_FLOAT> > *ioPhases,
                             bool horizontal, bool vertical)
{
    if (ioPhases->empty())
        return;
    
    if (horizontal && vertical)
        mPhasesMultiplier2 = 0.015;
    else
        mPhasesMultiplier2 = 1.0;
    
    if (horizontal)
    {
        // Unwrap for horizontal lines
        for (int i = 0; i < mHeight; i++)
        {
            WDL_TypedBuf<BL_FLOAT> &phases0 = (*ioPhases)[0];
            BL_FLOAT prevPhase = phases0.Get()[i];
            
            while(prevPhase < 0.0)
            {
                prevPhase += 2.0*M_PI;
            }
            
            BL_FLOAT incr = 0.0;
            for (int j = 0; j < ioPhases->size(); j++)
            {
                WDL_TypedBuf<BL_FLOAT> &phases = (*ioPhases)[j];
                
                BL_FLOAT phase = phases.Get()[i];
                
                if (phase < prevPhase)
                    phase += incr;
                
                while(phase < prevPhase)
                {
                    phase += 2.0*M_PI;
                    incr += 2.0*M_PI;
                }
                
                phases.Get()[i] = phase;
                
                prevPhase = phase;
            }
        }
    }
    
    if (vertical)
    {
        // Unwrap for vertical lines
        for (int j = 0; j < ioPhases->size(); j++)
        {
            WDL_TypedBuf<BL_FLOAT> &phases = (*ioPhases)[j];
            
            BL_FLOAT prevPhase = phases.Get()[0];
            while(prevPhase < 0.0)
            {
                prevPhase += 2.0*M_PI;
            }
            
            BL_FLOAT incr = 0.0;
            for (int i = 0; i < mHeight; i++)
            {
                BL_FLOAT phase = phases.Get()[i];
                
                if (phase < prevPhase)
                    phase += incr;
                
                while(phase < prevPhase)
                {
                    phase += 2.0*M_PI;
                    
                    incr += 2.0*M_PI;
                }
                
                phases.Get()[i] = phase;
                
                prevPhase = phase;
            }
        }
    }
}
#endif

