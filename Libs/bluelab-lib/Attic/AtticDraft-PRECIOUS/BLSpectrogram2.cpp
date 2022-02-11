//
//  Spectrum.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#include "Utils.h"
#include "ColorMap2.h"
#include "PPMFile.h"

#include "BLSpectrogram2.h"

#define MAX_PATH 512


#define DPHASE_MULT_X 0.001
#define DPHASE_MULT_Y 0.01


#if 0
Later: glsl colormap
TODO:
- optimize "TORGBA"
- fix colormap red strangeness
#endif


BLSpectrogram2::BLSpectrogram2(int height, int maxCols)
{
    mHeight = height;
    mMaxCols = maxCols;
    
    mGain = 1.0;
    
    mDisplayMagns = true;
    mYLogScale = false;
    
    mDisplayPhasesX = false;
    mDisplayPhasesY = false;
    
    mDisplayDPhases = false;
    
    unsigned char col0[3] = { 0, 255, 0 };
    
    unsigned char col1[3] = { 255, 0, 0 };
    //unsigned char col1[3] = { 255, 128, 0 };
    
    mColorMap = new ColorMap2(col0, col1);
    
    //mColorMap->SavePPM("colormap.ppm");
    
    // At the beginning, fill with zero values
    // This avoid the effect of "reversed scrolling",
    // when the spectrogram is not totally full
    FillWithZeros();
    
    mProgressivePhaseUnwrap = true;
}

BLSpectrogram2::~BLSpectrogram2() {}

void
BLSpectrogram2::SetGain(double gain)
{
    mGain = gain;
}

void
BLSpectrogram2::SetDisplayMagns(bool flag)
{
    mDisplayMagns = flag;
}

void
BLSpectrogram2::SetYLogScale(bool flag)
{
    mYLogScale = flag;
}

void
BLSpectrogram2::SetDisplayPhasesX(bool flag)
{
    if (mDisplayPhasesX != flag)
    {
        mUnwrappedPhases = mPhases;
        UnwrapAllPhases(&mUnwrappedPhases, flag, mDisplayPhasesY);
    }
    
    mDisplayPhasesX = flag;
}

void
BLSpectrogram2::SetDisplayPhasesY(bool flag)
{
    if (mDisplayPhasesY != flag)
    {
        mUnwrappedPhases = mPhases;
        UnwrapAllPhases(&mUnwrappedPhases, mDisplayPhasesX, flag);
    }
    
    mDisplayPhasesY = flag;
}

void
BLSpectrogram2::SetDisplayDPhases(bool flag)
{
    mDisplayDPhases = flag;
}

void
BLSpectrogram2::Reset()
{
    mMagns.clear();
    mPhases.clear();
    mUnwrappedPhases.clear();
}

int
BLSpectrogram2::GetNumCols()
{
    return mMagns.size();
}

int
BLSpectrogram2::GetMaxNumCols()
{
    return mMaxCols;
}

int
BLSpectrogram2::GetHeight()
{
    return mHeight;
}

void
BLSpectrogram2::AddLine(const WDL_TypedBuf<double> &magns,
                        const WDL_TypedBuf<double> &phases)
{
    if ((magns.GetSize() < mHeight) ||
        (phases.GetSize() < mHeight))
    {
        return;
    }
    
    WDL_TypedBuf<double> magns0 = magns;
    WDL_TypedBuf<double> phases0 = phases;
    
    if (mYLogScale)
    {
        Utils::LogScaleX(&magns0, 3.0);
        Utils::LogScaleX(&phases0, 3.0);
    }
    
    if ((magns0.GetSize() > mHeight) ||
        (phases0.GetSize() > mHeight))
    {
        Utils::DecimateSamples(&magns0,
                               ((double)mHeight)/magns0.GetSize());
        
        Utils::DecimateSamples(&phases0,
                               ((double)mHeight)/phases0.GetSize());
    }
    
    mMagns.push_back(magns0);
    mPhases.push_back(phases0);
    
    if (mProgressivePhaseUnwrap &&
        (mDisplayPhasesX || mDisplayPhasesY))
    {
        if (mDisplayPhasesX)
            UnwrapLineX(&phases0);
        
        if (mDisplayPhasesY)
            UnwrapLineY(&phases0);
        
        mUnwrappedPhases.push_back(phases0);
    }
    
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
        
        if (mUnwrappedPhases.size() > mMaxCols)
        {
            mUnwrappedPhases.pop_front();
        }
    }
}

bool
BLSpectrogram2::GetLine(int index,
                        WDL_TypedBuf<double> *magns,
                        WDL_TypedBuf<double> *phases)
{
    if ((index >= mMagns.size()) ||
        (index >= mPhases.size()))
        return false;
        
    *magns = mMagns[index];
    *phases = mPhases[index];
        
    return true;
}

void
BLSpectrogram2::GetImageDataRGBA(int width, int height, unsigned char *buf)
{
    // Empty the buffer
    // Because the spectrogram may be not totally full
    memset(buf, 0, width*height*4);
    
    // Unwrap phases ?
    vector<WDL_TypedBuf<double> > phasesUnW;
    
    if (!mProgressivePhaseUnwrap)
    {
        mUnwrappedPhases = mPhases;
        UnwrapAllPhases(mUnwrappedPhases, &phasesUnW, mDisplayPhasesX, mDisplayPhasesY);
    }
    else
        PhasesToStdVector(mUnwrappedPhases, &phasesUnW);
    
    if (mDisplayDPhases)
    {
        ComputeDPhases(&phasesUnW);
        
#if 0 // Derivate several times ?
        // (We have the summation, so we must derivate two times)
        ComputeDPhases(&phasesUnW);
        
        // Once again ?
        ComputeDPhases(&phasesUnW);
#endif
    }
    
    int maxPixValue = 255;
    double maxPhaseValue = ComputeMaxPhaseValue(phasesUnW);
    
    // Phases
    double phaseCoeff = 0.0;
    if (mDisplayPhasesX || mDisplayPhasesY)
    {
        if (!mDisplayDPhases)
        {
            phaseCoeff = (maxPhaseValue > 0.0) ? 1.0/maxPhaseValue : 1.0;
        }
        else
        {
            if (mDisplayPhasesX && !mDisplayPhasesY)
                phaseCoeff = DPHASE_MULT_X;
            else
                if (!mDisplayPhasesX && mDisplayPhasesY)
                    phaseCoeff = DPHASE_MULT_Y;
                else
                    // both
                {
                    // Take the smaller
                    phaseCoeff = (DPHASE_MULT_X < DPHASE_MULT_Y) ?
                    DPHASE_MULT_X : DPHASE_MULT_Y;
                }
        }
    }
    
    // Data
    for (int j = 0; j < mMagns.size(); j++)
    {
        const WDL_TypedBuf<double> &magns = mMagns[j];
        const WDL_TypedBuf<double> &phases = phasesUnW[j];
        
        for (int i = mHeight - 1; i >= 0 ; i--)
        {
            double magnValue = magns.Get()[i];
            double phaseValue = phases.Get()[i];
            
            if (mColorMap == NULL)
            {
                // Increase
                magnValue = sqrt(magnValue);
                magnValue = sqrt(magnValue);
                
                double magnColor = magnValue*(double)maxPixValue;
                if (magnColor > maxPixValue)
                    magnColor = maxPixValue;
            
                double phaseColor = phaseValue*(double)maxPixValue;
                if (phaseColor > maxPixValue)
                    phaseColor = maxPixValue;
            
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
                ColorMap2::CmColor magnColor = 0;
                if (mDisplayMagns)
                {
                    double magn = magnValue*mGain;
                
                    mColorMap->GetColor(magn, &magnColor);
                }
                
                double phaseColor = phaseValue*phaseCoeff*(double)maxPixValue;
                if (phaseColor > maxPixValue)
                    phaseColor = maxPixValue;
                
                int imgIdx = (j + i*height)*4;
                
                buf[imgIdx] = (int)phaseColor;
                buf[imgIdx + 1] = ((unsigned char *)&magnColor)[1];
                buf[imgIdx + 2] = ((unsigned char *)&magnColor)[0];
                buf[imgIdx + 3] = 255;
            }
        }
    }
}

// NOTE: deque is very slow for direct access to elements,
// compared to vectors
//
void
BLSpectrogram2::UnwrapAllPhases(const deque<WDL_TypedBuf<double> > &inPhases,
                                vector<WDL_TypedBuf<double> > *outPhases,
                                bool horizontal, bool vertical)
{
    if (inPhases.empty())
        return;
    
    outPhases->clear();
    
    // Convert deque to vec
    for (int i = 0; i < inPhases.size(); i++)
    {
        WDL_TypedBuf<double> line = inPhases[i];
        outPhases->push_back(line);
    }
    
    if (horizontal)
    {
        // Unwrap for horizontal lines
        for (int i = 0; i < mHeight; i++)
        {
            WDL_TypedBuf<double> &phases0 = (*outPhases)[0];
            double prevPhase = phases0.Get()[i];
            
            FindNextPhase(&prevPhase, 0.0);
            
            double incr = 0.0;
            for (int j = 0; j < outPhases->size(); j++)
            {
                WDL_TypedBuf<double> &phases = (*outPhases)[j];
                
                double phase = phases.Get()[i];
                
                if (phase < prevPhase)
                    phase += incr;
                
                FindNextPhase(&phase, prevPhase);
                
#if 0
                while(phase < prevPhase)
                {
                    phase += 2.0*M_PI;
                    incr += 2.0*M_PI;
                }
#endif
                
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
            WDL_TypedBuf<double> &phases = (*outPhases)[j];
            
            UnwrapLineY(&phases);
        }
    }
}

void
BLSpectrogram2::UnwrapAllPhases(deque<WDL_TypedBuf<double> > *ioPhases,
                               bool horizontal, bool vertical)
{
    vector<WDL_TypedBuf<double> > phasesUnW;
    UnwrapAllPhases(*ioPhases, &phasesUnW, horizontal, vertical);
    
    StdVectorToPhases(phasesUnW, ioPhases);
}

// NOTE: deque is very slow for direct access to elements,
// compared to vectors
//
void
BLSpectrogram2::PhasesToStdVector(const deque<WDL_TypedBuf<double> > &inPhases,
                                  vector<WDL_TypedBuf<double> > *outPhases)
{
    if (inPhases.empty())
        return;
    
    outPhases->clear();
    
    // Convert deque to vec
    for (int i = 0; i < inPhases.size(); i++)
    {
        WDL_TypedBuf<double> line = inPhases[i];
        outPhases->push_back(line);
    }
}

void
BLSpectrogram2::StdVectorToPhases(const vector<WDL_TypedBuf<double> > &inPhases,
                                  deque<WDL_TypedBuf<double> > *outPhases)
{
    if (inPhases.empty())
        return;
    
    outPhases->clear();
    
    // Convert deque to vec
    for (int i = 0; i < inPhases.size(); i++)
    {
        WDL_TypedBuf<double> line = inPhases[i];
        outPhases->push_back(line);
    }
}

void
BLSpectrogram2::UnwrapLineX(WDL_TypedBuf<double> *phases)
{
    if (mUnwrappedPhases.size() > 0)
    {
        int lastIdx = mUnwrappedPhases.size() - 1;
        
        // Unwrap for horizontal lines
        WDL_TypedBuf<double> &prevPhases = mUnwrappedPhases[lastIdx];
        
        for (int i = 0; i < phases->GetSize(); i++)
        {
            double phase = phases->Get()[i];
            
            double prevPhase = prevPhases.Get()[i];
            
            // Just in case
            FindNextPhase(&prevPhase, 0.0);
            
            FindNextPhase(&phase, prevPhase);
            
            phases->Get()[i] = phase;
        }
    }
}

void
BLSpectrogram2::UnwrapLineY(WDL_TypedBuf<double> *phases)
{
    double prevPhase = phases->Get()[0];
    
    FindNextPhase(&prevPhase, 0.0);
    
    double incr = 0.0;
    for (int i = 0; i < phases->GetSize(); i++)
    {
        double phase = phases->Get()[i];
        
        if (phase < prevPhase)
            phase += incr;
        
        FindNextPhase(&phase, prevPhase);
#if 0
        while(phase < prevPhase)
        {
            phase += 2.0*M_PI;
            
            incr += 2.0*M_PI;
        }
#endif
        
        phases->Get()[i] = phase;
        
        prevPhase = phase;
    }
}

void
BLSpectrogram2::FillWithZeros()
{
    WDL_TypedBuf<double> zeros;
    zeros.Resize(mHeight);
    Utils::FillAllZero(&zeros);
    
    for (int i = 0; i < mMaxCols; i++)
    {
        AddLine(zeros, zeros);
    }
    
    mUnwrappedPhases = mPhases;
}

void
BLSpectrogram2::SavePPM(const char *filename, int maxValue)
{
    if (mMagns.empty())
        return;
    
    vector<WDL_TypedBuf<double> > phasesUnW;
    
    if (!mProgressivePhaseUnwrap)
        UnwrapAllPhases(mPhases, &phasesUnW, mDisplayPhasesX, mDisplayPhasesY);
    else
        PhasesToStdVector(mPhases, &phasesUnW);
        
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
            const WDL_TypedBuf<double> &magns = mMagns[j];
            const WDL_TypedBuf<double> &phases = phasesUnW[j];
            
            double magnValue = magns.Get()[i];
            
            // Increase
            magnValue = sqrt(magnValue);
            magnValue = sqrt(magnValue);
            
            double magnColor = magnValue*(double)maxValue;
            if (magnColor > maxValue)
                magnColor = maxValue;
            
            double phaseValue = phases.Get()[i];
            
            double phaseColor = phaseValue*(double)maxValue;
            if (phaseColor > maxValue)
                phaseColor = maxValue;
            
            fprintf(file, "%d %d 0\n", (int)magnColor, (int)phaseColor);
        }
        
        fprintf(file, "\n");
    }
    fclose(file);
}

BLSpectrogram2 *
BLSpectrogram2::ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits)
{
    double ppmMultiplier = is16Bits ? 65535.0 : 255.0;
    
    BLSpectrogram2 *result = new BLSpectrogram2(image->h);
    for (int i = 0; i < image->w; i++)
    {
        WDL_TypedBuf<double> magns;
        magns.Resize(image->h);
        
        WDL_TypedBuf<double> phases;
        phases.Resize(image->h);
        
        for (int j = 0; j < image->h; j++)
        {
            double magnColor;
            double phaseColor;
            
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
            double magnValue = magnColor/ppmMultiplier;
            
            magnValue = magnValue*magnValue;
            magnValue = magnValue*magnValue;
            
            magns.Get()[j] = magnValue;
            
            // Phase
            double phaseValue = phaseColor/ppmMultiplier;
            
            phases.Get()[j] = phaseValue;
        }
        
        result->AddLine(magns, phases);
    }
    
    free(image);
    
    return result;
}

BLSpectrogram2 *
BLSpectrogram2::ImagesToSpectrogram(PPMFile::PPMImage *magnsImage,
                                   PPMFile::PPMImage *phasesImage)
{
    BLSpectrogram2 *result = new BLSpectrogram2(magnsImage->h);
    
    for (int i = 0; i < magnsImage->w; i++)
    {
        WDL_TypedBuf<double> magns;
        magns.Resize(magnsImage->h);
        
        WDL_TypedBuf<double> phases;
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

double
BLSpectrogram2::ComputeMaxPhaseValue(const vector<WDL_TypedBuf<double> > &phasesUnW)
{
    if (mDisplayPhasesX && mDisplayPhasesY)
        // Wrap on both ?
        // Take the corner
    {
        if (phasesUnW.empty())
            return 0.0;
    
        const WDL_TypedBuf<double> &lastLine = phasesUnW[phasesUnW.size() - 1];
    
        if (lastLine.GetSize() == 0)
            return 0.0;
        
        // Maximum
        double lastVal = lastLine.Get()[lastLine.GetSize() - 1];
    
        return lastVal;
    }
    
    if (mDisplayPhasesX)
    {
        // Sum is in the last line
        
        if (phasesUnW.empty())
            return 0.0;
        const WDL_TypedBuf<double> &lastLine = phasesUnW[phasesUnW.size() - 1];
        
        double maxPhaseX = 0.0;
        for (int i = 0; i < lastLine.GetSize(); i++)
        {
            double phase = lastLine.Get()[i];
                
            if (phase > maxPhaseX)
                maxPhaseX = phase;
        }
        
        return maxPhaseX;
    }
    
    if (mDisplayPhasesY)
    {
        // Sum is at the end of each line
        
        double maxPhaseY = 0.0;
        for (int i = 0; i < phasesUnW.size(); i++)
        {
            const WDL_TypedBuf<double> &line = phasesUnW[i];
            
            if (line.GetSize() > 0)
            {
                double phase = line.Get()[line.GetSize() - 1];
                
                if (phase > maxPhaseY)
                    maxPhaseY = phase;
            }
        }
        
        return maxPhaseY;
    }
    
    return 0.0;
}

void
BLSpectrogram2::ComputeDPhases(vector<WDL_TypedBuf<double> > *phasesUnW)
{
    vector<WDL_TypedBuf<double> > dPhasesX;
    
    if (mDisplayPhasesX)
    {
        ComputeDPhasesX(*phasesUnW, &dPhasesX);
    }
    
    vector<WDL_TypedBuf<double> > dPhasesY;
    if (mDisplayPhasesY)
    {
        ComputeDPhasesY(*phasesUnW, &dPhasesY);
    }
    
    if (!dPhasesX.empty() && !dPhasesY.empty())
    {
        for (int i = 0; i < dPhasesX.size(); i++)
        {
            WDL_TypedBuf<double> phasesX = dPhasesX[i];
            WDL_TypedBuf<double> phasesY = dPhasesY[i];
            
#if 1
            WDL_TypedBuf<double> sumPhases;
            Utils::ComputeSum(phasesX, phasesY, &sumPhases);
            
            (*phasesUnW)[i] = sumPhases;
#endif
     
#if 0
            WDL_TypedBuf<double> prodPhases;
            Utils::ComputeProduct(phasesX, phasesY, &prodPhases);
            
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
BLSpectrogram2::ComputeDPhasesX(const vector<WDL_TypedBuf<double> > &phasesUnW,
                               vector<WDL_TypedBuf<double> > *dPhases)
{
    dPhases->resize(phasesUnW.size());
    
    for (int j = 0; j < phasesUnW.size(); j++)
    {
        const WDL_TypedBuf<double> &phases = phasesUnW[j];
        
        WDL_TypedBuf<double> dPhases0;
        dPhases0.Resize(phases.GetSize());
                       
        double prevPhase = 0;
        for (int i = 0; i < phases.GetSize(); i++)
        {
            double p = phases.Get()[i];
            
            double delta = p - prevPhase;
            delta = fabs(delta);
            
            dPhases0.Get()[i] = delta;
            
            prevPhase = p;
        }
        
        (*dPhases)[j] = dPhases0;
    }
}

void
BLSpectrogram2::ComputeDPhasesY(const vector<WDL_TypedBuf<double> > &phasesUnW,
                               vector<WDL_TypedBuf<double> > *dPhases)
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
        double prevPhase = phasesUnW[0].Get()[i];
        
        for (int j = 0; j < phasesUnW.size(); j++)
        {
            const WDL_TypedBuf<double> &dPhases0 = phasesUnW[j];
            
            double p = dPhases0.Get()[j];
            
            double delta = p - prevPhase;
            delta = fabs(delta);
            
            (*dPhases)[j].Get()[i] = delta;
            
            prevPhase = p;
        }
    }
}

void
BLSpectrogram2::FindNextPhase(double *phase, double refPhase)
{
    while(*phase < refPhase)
        *phase += 2.0*M_PI;
}

BLSpectrogram2 *
BLSpectrogram2::Load(const char *fileName)
{
    // TODO
    return NULL;
}

void
BLSpectrogram2::Save(const char *filename)
{
    // Magns
    char fullFilenameMagns[MAX_PATH];
    sprintf(fullFilenameMagns, "/Volumes/HDD/Share/%s-magns", filename);
    
    FILE *magnsFile = fopen(fullFilenameMagns, "w");
    
    for (int j = 0; j < mMagns.size(); j++)
    {
        const WDL_TypedBuf<double> &magns = mMagns[j];
        
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
        const WDL_TypedBuf<double> &phases = mPhases[j];
        
        for (int i = 0; i < mHeight; i++)
        {
            fprintf(phasesFile, "%g ", phases.Get()[i]);
        }
        
        fprintf(phasesFile, "\n");
    }
    
    fclose(phasesFile);
}

BLSpectrogram2 *
BLSpectrogram2::LoadPPM(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM(fullFilename);
    
    BLSpectrogram2 *result = ImageToSpectrogram(image, false);
    
    return result;
}

void
BLSpectrogram2::SavePPM(const char *filename)
{
    SavePPM(filename, 255);
}

BLSpectrogram2 *
BLSpectrogram2::LoadPPM16(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM16(fullFilename);
    
    BLSpectrogram2 *result = ImageToSpectrogram(image, true);
    
    return result;
}

void
BLSpectrogram2::SavePPM16(const char *filename)
{
    SavePPM(filename, 65535);
}

BLSpectrogram2 *
BLSpectrogram2::LoadPPM32(const char *filename)
{
    // Magns
    char magnsFullFilename[MAX_PATH];
    sprintf(magnsFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-magns.ppm", filename);
    
    PPMFile::PPMImage *magnsImage = PPMFile::ReadPPM16(magnsFullFilename);
    
    // Phases
    char phasesFullFilename[MAX_PATH];
    sprintf(phasesFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-phases.ppm", filename);
    
    PPMFile::PPMImage *phasesImage = PPMFile::ReadPPM16(phasesFullFilename);
    
    BLSpectrogram2 *result = ImagesToSpectrogram(magnsImage, phasesImage);
    
    return result;
}

void
BLSpectrogram2::SavePPM32(const char *filename)
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
            const WDL_TypedBuf<double> &magns = mMagns[j];
            const WDL_TypedBuf<double> &phases = mPhases[j];
            
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


