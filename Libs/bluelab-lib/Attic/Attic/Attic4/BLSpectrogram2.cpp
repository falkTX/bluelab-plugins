//
//  Spectrum.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#include <BLUtils.h>
#include "ColorMap2.h"
#include "PPMFile.h"

#include "BLSpectrogram2.h"

#define MAX_PATH 512


#define DPHASE_MULT_X 0.001
#define DPHASE_MULT_Y 0.01

// TODO: glsl colormap

unsigned char BLSpectrogram2::mPhasesColor[4] = { 255, 255, 255, 255 };


BLSpectrogram2::BLSpectrogram2(int height, int maxCols)
{
    mHeight = height;
    mMaxCols = maxCols;
    
    mRange = 0.0;
    mContrast = 0.5;
    
    mDisplayMagns = true;
    mYLogScale = false;
    
    mDisplayPhasesX = false;
    mDisplayPhasesY = false;
    
    mDisplayDPhases = false;
    
#if 0
    // Blue
    unsigned char col0[4] = { 0, 0, 255, 255 };
    unsigned char col1[4] = { 255, 255, 255, 255 };
#endif

#if 1 // Blue and Red
    //
    unsigned char col0[4] = { 0, 0, 255, 255 };
    unsigned char col1[4] = { 255, 0, 255, 255 };
#endif

#if 0
    // Grey
    unsigned char col0[4] = { 0, 0, 0, 255 };
    unsigned char col1[4] = { 255, 255, 255, 255 };
#endif

#if 0
    // Green and Yellow
    unsigned char col0[4] = { 0, 255, 0, 255 };
    unsigned char col1[4] = { 255, 255, 0, 255 };
#endif
    
    mColorMap = new ColorMap2(col0, col1);
    
    // At the beginning, fill with zero values
    // This avoid the effect of "reversed scrolling",
    // when the spectrogram is not totally full
    FillWithZeros();
    
    mProgressivePhaseUnwrap = true;
}

BLSpectrogram2::~BLSpectrogram2() {}

void
BLSpectrogram2::SetRange(BL_FLOAT range)
{
    mRange = range;
}

void
BLSpectrogram2::SetContrast(BL_FLOAT contrast)
{
    mContrast = contrast;
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
BLSpectrogram2::AddLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
BLSpectrogram2::GetImageDataRGBA(int width, int height, unsigned char *buf)
{
    mColorMap->SetRange(mRange);
    mColorMap->SetContrast(mContrast);
    
    // Empty the buffer
    // Because the spectrogram may be not totally full
    memset(buf, 0, width*height*4);
    
    // Unwrap phases ?
    vector<WDL_TypedBuf<BL_FLOAT> > phasesUnW;
    
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
        
        // NOTE: could compute several time, to get the second or third derivative...
    }
    
    int maxPixValue = 255;
    BL_FLOAT maxPhaseValue = ComputeMaxPhaseValue(phasesUnW);
    
    // Phases
    BL_FLOAT phaseCoeff = 0.0;
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
        const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
        const WDL_TypedBuf<BL_FLOAT> &phases = phasesUnW[j];
        
        for (int i = mHeight - 1; i >= 0 ; i--)
        {
            BL_FLOAT magnValue = magns.Get()[i];
            BL_FLOAT phaseValue = phases.Get()[i];
            
            // Apply colormap
        
            // Magns
            ColorMap2::CmColor magnColor = 0;
            if (mDisplayMagns)
            {
                mColorMap->GetColor(magnValue, &magnColor);
            }
                
            BL_FLOAT phaseColorCoeff = 1.0;
            if (mDisplayPhasesX || mDisplayPhasesY)
            {
                phaseColorCoeff = phaseValue*phaseCoeff;
                if (phaseColorCoeff > 1.0)
                    phaseColorCoeff = 1.0;
            }
                
            int pixIdx = (j + i*height)*4;
                
            if ((mDisplayPhasesX || mDisplayPhasesY) &&
                !mDisplayMagns)
                // Phases only
            {
                buf[pixIdx] = phaseColorCoeff*mPhasesColor[2]; // Blue
                buf[pixIdx + 1] = phaseColorCoeff*mPhasesColor[1]; // Green
                buf[pixIdx + 2] = phaseColorCoeff*mPhasesColor[0]; // Red
                buf[pixIdx + 3] = mPhasesColor[3]; // Alpha
            }
                
            if (mDisplayMagns)
            {
                // Magns color
                buf[pixIdx] = ((unsigned char *)&magnColor)[2];
                buf[pixIdx + 1] = ((unsigned char *)&magnColor)[1];
                buf[pixIdx + 2] = ((unsigned char *)&magnColor)[0];
                buf[pixIdx + 3] = 255; //((unsigned char *)&magnColor)[3];
                    
                if (mDisplayPhasesX || mDisplayPhasesY)
                    // Overlay phases
                {
                    int magnColor[3] = { buf[pixIdx + 2], buf[pixIdx + 1], buf[pixIdx] };

                    for (int k = 0; k < 3; k++)
                    {
                        magnColor[k] += phaseColorCoeff*mPhasesColor[k];
                            
                        if (magnColor[k] > maxPixValue)
                            magnColor[k] = maxPixValue;
                    }
                        
                    buf[pixIdx] = magnColor[2];
                    buf[pixIdx + 1] = magnColor[1];
                    buf[pixIdx + 2] = magnColor[0];
                }
            }
        }
    }
}

// NOTE: deque is very slow for direct access to elements,
// compared to vectors
//
void
BLSpectrogram2::UnwrapAllPhases(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
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
    
    if (horizontal)
    {
        // Unwrap for horizontal lines
        for (int i = 0; i < mHeight; i++)
        {
            WDL_TypedBuf<BL_FLOAT> &phases0 = (*outPhases)[0];
            BL_FLOAT prevPhase = phases0.Get()[i];
            
            BLUtils::FindNextPhase(&prevPhase, (BL_FLOAT)0.0);
            
            for (int j = 0; j < outPhases->size(); j++)
            {
                WDL_TypedBuf<BL_FLOAT> &phases = (*outPhases)[j];
                
                BL_FLOAT phase = phases.Get()[i];
                
                BLUtils::FindNextPhase(&phase, prevPhase);
                
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
            
            UnwrapLineY(&phases);
        }
    }
}

void
BLSpectrogram2::UnwrapAllPhases(deque<WDL_TypedBuf<BL_FLOAT> > *ioPhases,
                               bool horizontal, bool vertical)
{
    vector<WDL_TypedBuf<BL_FLOAT> > phasesUnW;
    UnwrapAllPhases(*ioPhases, &phasesUnW, horizontal, vertical);
    
    StdVectorToPhases(phasesUnW, ioPhases);
}

// NOTE: deque is very slow for direct access to elements,
// compared to vectors
//
void
BLSpectrogram2::PhasesToStdVector(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                                  vector<WDL_TypedBuf<BL_FLOAT> > *outPhases)
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
}

void
BLSpectrogram2::StdVectorToPhases(const vector<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                                  deque<WDL_TypedBuf<BL_FLOAT> > *outPhases)
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
}

void
BLSpectrogram2::UnwrapLineX(WDL_TypedBuf<BL_FLOAT> *phases)
{
    if (mUnwrappedPhases.size() > 0)
    {
        int lastIdx = (int)mUnwrappedPhases.size() - 1;
        
        // Unwrap for horizontal lines
        WDL_TypedBuf<BL_FLOAT> &prevPhases = mUnwrappedPhases[lastIdx];
        
        for (int i = 0; i < phases->GetSize(); i++)
        {
            BL_FLOAT phase = phases->Get()[i];
            
            BL_FLOAT prevPhase = prevPhases.Get()[i];
            
            // Just in case
            BLUtils::FindNextPhase(&prevPhase, (BL_FLOAT)0.0);
            
            BLUtils::FindNextPhase(&phase, prevPhase);
            
            phases->Get()[i] = phase;
        }
    }
}

void
BLSpectrogram2::UnwrapLineY(WDL_TypedBuf<BL_FLOAT> *phases)
{
    BLUtils::UnwrapPhases(phases);
}

void
BLSpectrogram2::FillWithZeros()
{
    WDL_TypedBuf<BL_FLOAT> zeros;
    zeros.Resize(mHeight);
    BLUtils::FillAllZero(&zeros);
    
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
    
    vector<WDL_TypedBuf<BL_FLOAT> > phasesUnW;
    
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
            const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
            const WDL_TypedBuf<BL_FLOAT> &phases = phasesUnW[j];
            
            BL_FLOAT magnValue = magns.Get()[i];
            
            // Increase
            magnValue = std::sqrt(magnValue);
            magnValue = std::sqrt(magnValue);
            
            BL_FLOAT magnColor = magnValue*(BL_FLOAT)maxValue;
            if (magnColor > maxValue)
                magnColor = maxValue;
            
            BL_FLOAT phaseValue = phases.Get()[i];
            
            BL_FLOAT phaseColor = phaseValue*(BL_FLOAT)maxValue;
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
    BL_FLOAT ppmMultiplier = is16Bits ? 65535.0 : 255.0;
    
    BLSpectrogram2 *result = new BLSpectrogram2(image->h);
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
            BL_FLOAT magnValue = magnColor/ppmMultiplier;
            
            magnValue = magnValue*magnValue;
            magnValue = magnValue*magnValue;
            
            magns.Get()[j] = magnValue;
            
            // Phase
            BL_FLOAT phaseValue = phaseColor/ppmMultiplier;
            
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

BL_FLOAT
BLSpectrogram2::ComputeMaxPhaseValue(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW)
{
    if (mDisplayPhasesX && mDisplayPhasesY)
        // Wrap on both ?
        // Take the corner
    {
        if (phasesUnW.empty())
            return 0.0;
    
        const WDL_TypedBuf<BL_FLOAT> &lastLine = phasesUnW[phasesUnW.size() - 1];
    
        if (lastLine.GetSize() == 0)
            return 0.0;
        
        // Maximum
        BL_FLOAT lastVal = lastLine.Get()[lastLine.GetSize() - 1];
    
        return lastVal;
    }
    
    if (mDisplayPhasesX)
    {
        // Sum is in the last line
        
        if (phasesUnW.empty())
            return 0.0;
        const WDL_TypedBuf<BL_FLOAT> &lastLine = phasesUnW[phasesUnW.size() - 1];
        
        BL_FLOAT maxPhaseX = 0.0;
        for (int i = 0; i < lastLine.GetSize(); i++)
        {
            BL_FLOAT phase = lastLine.Get()[i];
                
            if (phase > maxPhaseX)
                maxPhaseX = phase;
        }
        
        return maxPhaseX;
    }
    
    if (mDisplayPhasesY)
    {
        // Sum is at the end of each line
        
        BL_FLOAT maxPhaseY = 0.0;
        for (int i = 0; i < phasesUnW.size(); i++)
        {
            const WDL_TypedBuf<BL_FLOAT> &line = phasesUnW[i];
            
            if (line.GetSize() > 0)
            {
                BL_FLOAT phase = line.Get()[line.GetSize() - 1];
                
                if (phase > maxPhaseY)
                    maxPhaseY = phase;
            }
        }
        
        return maxPhaseY;
    }
    
    return 0.0;
}

void
BLSpectrogram2::ComputeDPhases(vector<WDL_TypedBuf<BL_FLOAT> > *phasesUnW)
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
BLSpectrogram2::ComputeDPhasesX(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
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
BLSpectrogram2::ComputeDPhasesY(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
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


