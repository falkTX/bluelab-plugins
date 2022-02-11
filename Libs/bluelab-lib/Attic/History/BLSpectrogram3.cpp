//
//  Spectrum.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

//#include "ColorMap3.h"

#include "ColorMap4.h"

#include <BLTypes.h>
#include <BLUtils.h>
#include <BLDebug.h>

#include "PPMFile.h"

#include "BLSpectrogram3.h"

//#define MAX_PATH 512


#define DPHASE_MULT_X 0.001
#define DPHASE_MULT_Y 0.01

#define NIKO_COLORMAP 1

// Very good colors with that
// Avoids too dark spectrograms
//
// (was 0.5 at the origin)
//
//#define DEFAULT_BLACK_LIMIT 0.125

#define GHOST_OPTIM 1

// NOTE: this is just in case, should not be necessary
//
// FIX: when resetting with a big mMaxNumCols, then resetting
// with a smaller mMaxNumCols, the size was not diminished
#define FIX_INFRASONIC_VIEWER_RESET 0

unsigned char BLSpectrogram3::mPhasesColor[4] = { 255, 255, 255, 255 };


BLSpectrogram3::BLSpectrogram3(int height, int maxCols)
{
    mHeight = height;
    mMaxCols = maxCols;
    
    mRange = 0.0;
    mContrast = 0.5;
    
    mDisplayMagns = true;
    
    mYLogScale = false;
    mYLogScaleFactor = 1.0;
    
    mDisplayPhasesX = false;
    mDisplayPhasesY = false;
    
    mDisplayDPhases = false;
    
    mColorMap = NULL;
    
    SetColorMap(0);
    
    if (mMaxCols > 0)
    {
        // At the beginning, fill with zero values
        // This avoid the effect of "reversed scrolling",
        // when the spectrogram is not totally full
        FillWithZeros();
    }
    
    mProgressivePhaseUnwrap = true;
    
    mTotalLineNum = 0;
}

BLSpectrogram3::~BLSpectrogram3()
{
    if (mColorMap != NULL)
        delete mColorMap;
}

void
BLSpectrogram3::SetRange(BL_FLOAT range)
{
    mRange = range;
    
    // NEW
    mColorMap->SetRange(mRange);
    mColorMap->Generate();
}

void
BLSpectrogram3::SetContrast(BL_FLOAT contrast)
{
    mContrast = contrast;
    
    // NEW
    mColorMap->SetContrast(mContrast);
    mColorMap->Generate();
}

void
BLSpectrogram3::SetDisplayMagns(bool flag)
{
    mDisplayMagns = flag;
}

void
BLSpectrogram3::SetYLogScale(bool flag, BL_FLOAT factor)
{
    mYLogScale = flag;
    mYLogScaleFactor = factor;
}

void
BLSpectrogram3::SetDisplayPhasesX(bool flag)
{
    if (mDisplayPhasesX != flag)
    {
        mUnwrappedPhases = mPhases;
        UnwrapAllPhases(&mUnwrappedPhases, flag, mDisplayPhasesY);
    }
    
    mDisplayPhasesX = flag;
}

void
BLSpectrogram3::SetDisplayPhasesY(bool flag)
{
    if (mDisplayPhasesY != flag)
    {
        mUnwrappedPhases = mPhases;
        UnwrapAllPhases(&mUnwrappedPhases, mDisplayPhasesX, flag);
    }
    
    mDisplayPhasesY = flag;
}

void
BLSpectrogram3::SetDisplayDPhases(bool flag)
{
    mDisplayDPhases = flag;
}

void
BLSpectrogram3::Reset()
{
    mMagns.clear();
    mPhases.clear();
    mUnwrappedPhases.clear();
    
    mTotalLineNum = 0;
}

void
BLSpectrogram3::Reset(int height, int maxCols)
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
BLSpectrogram3::GetNumCols()
{
    return mMagns.size();
}

int
BLSpectrogram3::GetMaxNumCols()
{
    return mMaxCols;
}

int
BLSpectrogram3::GetHeight()
{
    return mHeight;
}

#if 0
void
BLSpectrogram3::SetColorMap(int colorMapNum)
{
    if (mColorMap != NULL)
        delete mColorMap;
    
    switch(colorMapNum)
    {
        case 0:
        {
            // Blue and Pink
            unsigned char col0[4] = { 0, 0, 255, 255 };
            unsigned char col1[4] = { 255, 0, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;

        case 1:
        {
            // Green and red
            unsigned char col0[4] = { 0, 255, 0, 255 };
            unsigned char col1[4] = { 255, 0, 0, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
        
        case 2:
        {
            // Cyan to orange
            unsigned char col0[4] = { 0, 255, 255, 255 };
            unsigned char col1[4] = { 255, 128, 0, 255 };
            
            mColorMap = new ColorMap3(col0, col1, 0.5);
        }
        break;
            
        case 3:
        {
            // Blue and white
            unsigned char col0[4] = { 0, 0, 255, 255 };
            unsigned char col1[4] = { 255, 255, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
        
        case 4:
        {
            // Cyan and Pink
            unsigned char col0[4] = { 0, 255, 255, 255 };
            unsigned char col1[4] = { 255, 0, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
            
        case 5:
        {
            // Grey
            unsigned char col0[4] = { 128, 128, 128, 255 };
            unsigned char col1[4] = { 255, 255, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
        
        case 6:
        {
            // Cyan and Pink
            unsigned char col0[4] = { 0, 255, 255, 255 };
            unsigned char col1[4] = { 255, 0, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT, 0.75);
        }
        break;
            
#if 0
        case 1:
        {
            // Blue and Green
            unsigned char col0[4] = { 0, 0, 255, 255 };
            unsigned char col1[4] = { 0, 255, 0, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
#endif
            
#if 0
        case 4:
        {
            // Green and white
            unsigned char col0[4] = { 0, 255, 0, 255 };
            unsigned char col1[4] = { 255, 255, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
#endif
            
#if 0
        case 5: // Throw
        {
            // Red and white
            //unsigned char col0[4] = { 255, 0, 0, 255 };
            //unsigned char col1[4] = { 255, 255, 255, 255 };
            
            // Blue and white 2
            unsigned char col0[4] = { 0, 0, 255, 255 };
            unsigned char col1[4] = { 255, 255, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, 0.5/*0.125*/);
        }
        break;
#endif
        
#if 0
        case 7:
        {
            // Green and yellow
            unsigned char col0[4] = { 0, 255, 0, 255 };
            unsigned char col1[4] = { 255, 255, 0, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
#endif
            
#if 0
        case 8: // Throw
        {
            // Cyan to white
            unsigned char col0[4] = { 0, 255, 255, 255 };
            unsigned char col1[4] = { 255, 255, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
#endif
            
        default:
        {
            // Blue and Red
            unsigned char col0[4] = { 0, 0, 255, 255 };
            unsigned char col1[4] = { 255, 0, 255, 255 };
            
            mColorMap = new ColorMap3(col0, col1, DEFAULT_BLACK_LIMIT);
        }
        break;
    }
}
#endif

void
BLSpectrogram3::SetColorMap(int colorMapNum)
{
    if (mColorMap != NULL)
        delete mColorMap;
    
    switch(colorMapNum)
    {
        case 0:
        {
            // Blue and dark pink
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(68, 22, 68, 255, 0.25);
            mColorMap->AddColor(32, 122, 190, 255, 0.5);
            mColorMap->AddColor(172, 212, 250, 255, 0.75);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
          
        case 1:
        {
            // Green
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(32, 42, 26, 255, 0.0);
            mColorMap->AddColor(66, 108, 60, 255, 0.25);
            mColorMap->AddColor(98, 150, 82, 255, 0.5);
            mColorMap->AddColor(166, 206, 148, 255, 0.75);
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case 2:
        {
            // Grey
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(128, 128, 128, 255, 0.5);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case 3:
        {
            // Cyan and Pink
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(0, 150, 150, 255, 0.2);//
            mColorMap->AddColor(0, 255, 255, 255, 0.5/*0.2*/);
            mColorMap->AddColor(255, 0, 255, 255, 0.8);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case 4:
        {
            // Green and red
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(0, 255, 0, 255, 0.25);
            mColorMap->AddColor(255, 0, 0, 255, 1.0);
        }
        break;
            
        case 5:
        {
            // Multicolor ("jet")
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 128, 255, 0.0);
            mColorMap->AddColor(0, 0, 246, 255, 0.1);
            mColorMap->AddColor(0, 77, 255, 255, 0.2);
            mColorMap->AddColor(0, 177, 255, 255, 0.3);
            mColorMap->AddColor(38, 255, 209, 255, 0.4);
            mColorMap->AddColor(125, 255, 122, 255, 0.5);
            mColorMap->AddColor(206, 255, 40, 255, 0.6);
            mColorMap->AddColor(255, 200, 0, 255, 0.7);
            mColorMap->AddColor(255, 100, 0, 255, 0.8);
            mColorMap->AddColor(241, 8, 0, 255, 0.9);
            mColorMap->AddColor(132, 0, 0, 255, 1.0);
        }
        break;
            
        case 6:
        {
            // Cyan to orange
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(0, 255, 255, 255, 0.25);
            //mColorMap->AddColor(255, 128, 0, 255, 1.0);
            mColorMap->AddColor(255, 128, 0, 255, 0.75);
            mColorMap->AddColor(255, 228, 130, 255, 1.0);
        }
        break;
    
        case 7:
        {
            // Cyan and Orange (Wasp ?)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(22, 35, 68, 255, 0.25);
            mColorMap->AddColor(190, 90, 32, 255, 0.5);
            mColorMap->AddColor(250, 220, 96, 255, 0.75);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
        
        case 8:
        {
#if 0
            // Sky (parula / ice)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
            mColorMap->AddColor(62, 38, 168, 255, 0.004);
            
            mColorMap->AddColor(46, 135, 247, 255, 0.25);
            mColorMap->AddColor(11, 189, 189, 255, 0.5);
            mColorMap->AddColor(157, 201, 67, 255, 0.75);
            
            mColorMap->AddColor(249, 251, 21, 255, 1.0);
#endif
            
            // Not so bad, quite similar to original ice
            //
            // Sky (ice)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(4, 6, 19, 255, 0.0);
            
            mColorMap->AddColor(58, 61, 126, 255, 0.25);
            mColorMap->AddColor(67, 126, 184, 255, 0.5);
            
            //mColorMap->AddColor(112, 182, 205, 255, 0.75);
            mColorMap->AddColor(73, 173, 208, 255, 0.75);
            
            mColorMap->AddColor(232, 251, 252, 255, 1.0);
        }
        break;
            
        case 9:
        {
            // Dawn (thermal...)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(4, 35, 53, 255, 0.0);
            
            mColorMap->AddColor(90, 61, 154, 255, 0.25);
            mColorMap->AddColor(185, 97, 125, 255, 0.5);
            
            mColorMap->AddColor(245, 136, 71, 255, 0.75);
            
            mColorMap->AddColor(233, 246, 88, 255, 1.0);
            
        }
        break;
         
        // See: https://matplotlib.org/3.1.1/tutorials/colors/colormaps.html
        case 10:
        {
            // Rainbow2 (gist_rainbow)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(255, 48, 80, 255, 0.0);
            mColorMap->AddColor(255, 112, 0, 255, 0.114);
            mColorMap->AddColor(255, 245, 0, 255, 0.212);
            mColorMap->AddColor(115, 255, 0, 255, 0.318);
            mColorMap->AddColor(0, 255, 26, 255, 0.416);
            mColorMap->AddColor(0, 255, 246, 255, 0.581);
            mColorMap->AddColor(25, 0, 255, 255, 0.787);
            mColorMap->AddColor(231, 0, 255, 255, 0.935);
            mColorMap->AddColor(255, 0, 195, 255, 1.0);
        }
        break;
        
        case 11:
        {
            // Landscape (terrain)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(51, 51, 153, 255, 0.0);
            mColorMap->AddColor(2, 148, 250, 255, 0.150);
            mColorMap->AddColor(37, 111, 109, 255, 0.286);
            mColorMap->AddColor(253, 254, 152, 255, 0.494);
            mColorMap->AddColor(128, 92, 84, 255, 0.743);
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case 12:
        {
            // Fire (fire)
            mColorMap = new ColorMap4(true);
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            mColorMap->AddColor(143, 0, 0, 255, 0.200);
            mColorMap->AddColor(236, 0, 0, 255, 0.337);
            mColorMap->AddColor(255, 116, 0, 255, 0.535);
            mColorMap->AddColor(255, 234, 0, 255, 0.706);
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        default:
            return;
            break;
    }
    
    // NEW
    // Forward the current parameters
    mColorMap->SetRange(mRange);
    mColorMap->SetContrast(mContrast);
    
    mColorMap->Generate();
}

void
BLSpectrogram3::AddLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
        BLUtils::LogScaleX(&magns0, mYLogScaleFactor);
        BLUtils::LogScaleX(&phases0, mYLogScaleFactor);
    }
    
    if ((magns0.GetSize() > mHeight) ||
        (phases0.GetSize() > mHeight))
    {
        BLUtils::DecimateSamples(&magns0,
                               ((BL_FLOAT)mHeight)/magns0.GetSize());
        
        BLUtils::DecimateSamples(&phases0,
                               ((BL_FLOAT)mHeight)/phases0.GetSize());
    }
    
    // Convert amp to dB
    // (Works like a charm !)
    WDL_TypedBuf<BL_FLOAT> dbMagns;
    BLUtils::AmpToDBNorm(&dbMagns, magns0, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
    mMagns.push_back(dbMagns);
    
    //mMagns.push_back(magns0);
    mPhases.push_back(phases0);
    
    if (mProgressivePhaseUnwrap) // &&
        //(mDisplayPhasesX || mDisplayPhasesY))
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
    
    mTotalLineNum++;
}

bool
BLSpectrogram3::GetLine(int index,
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

#if !GHOST_OPTIM
// Original version, with possibly phase unwrap and display
void
BLSpectrogram3::GetImageDataFloat(int width, int height, unsigned char *buf)
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
        WDL_TypedBuf<BL_FLOAT> phases;
        
        if (mDisplayPhasesX || mDisplayPhasesY)
            phases = phasesUnW[j];
        
        for (int i = mHeight - 1; i >= 0 ; i--)
        {
            BL_FLOAT magnValue = magns.Get()[i];

            BL_FLOAT phaseValue = 0.0;
            if (mDisplayPhasesX || mDisplayPhasesY)
                phaseValue = phases.Get()[i];
            
            // Apply colormap
        
#if !NIKO_COLORMAP
            int maxPixValue = 255;
            
            // Magns
            ColorMap3::CmColor magnColor = 0;
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
            
            int pixIdx = (i*width + j)*4;
            
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
#else
            if (mDisplayMagns)
            {
                if (magnValue > 1.0)
                    magnValue = 1.0;
                
                int pixIdx = i*width + j;
                ((float *)buf)[pixIdx] = (float)magnValue;
            }
#endif
        }
    }
}
#endif

#if GHOST_OPTIM
// Optimized version: keep the strict minimum
void
BLSpectrogram3::GetImageDataFloat(int width, int height, unsigned char *buf)
{
    mColorMap->SetRange(mRange);
    mColorMap->SetContrast(mContrast);
    
    // Empty the buffer
    // Because the spectrogram may be not totally full
    memset(buf, 0, width*height*4);
    
    // Data
    for (int j = 0; j < mMagns.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &magns = mMagns[j];
        
        BL_FLOAT *magnsBuf = magns.Get();
        
        // TEST: try to copy the memory in blocks to optimize
        // doesn't work: width and height are reversed.
        //memcpy(&buf[j*magns.GetSize()*sizeof(float)],
        //       magns.Get(),
        //       magns.GetSize()*sizeof(float));
        //continue;
        
        //for (int i = mHeight - 1; i >= 0 ; i--)
        for (int i = 0; i < mHeight; i++)
        {
            BL_FLOAT magnValue = magnsBuf[i];
            if (magnValue > 1.0)
                magnValue = 1.0;
                
            // Fix for iPlug2
            //int pixIdx = i*width + j;
            int pixIdx = (mHeight - 1 - i)*width + j;
            ((float *)buf)[pixIdx] = (float)magnValue;
        }
    }
}
#endif

void
BLSpectrogram3::GetColormapImageDataRGBA(WDL_TypedBuf<unsigned int> *colormapImageData)
{
    mColorMap->GetDataRGBA(colormapImageData);
}

// NOTE: deque is very slow for direct access to elements,
// compared to vectors
//
void
BLSpectrogram3::UnwrapAllPhases(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
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
BLSpectrogram3::UnwrapAllPhases(deque<WDL_TypedBuf<BL_FLOAT> > *ioPhases,
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
BLSpectrogram3::PhasesToStdVector(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                                  vector<WDL_TypedBuf<BL_FLOAT> > *outPhases)
{
    if (inPhases.empty())
        return;
    
    outPhases->clear();
    
    // Convert deque to vec
#if !GHOST_OPTIM
    for (int i = 0; i < inPhases.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> line = inPhases[i];
        outPhases->push_back(line);
    }
#else
    long numPhases = inPhases.size();
    
    if (outPhases->size() != numPhases)
        outPhases->resize(numPhases);
    
    for (int i = 0; i < numPhases; i++)
    {
        WDL_TypedBuf<BL_FLOAT> line = inPhases[i];
        (*outPhases)[i] = line;
    }
#endif
}

void
BLSpectrogram3::StdVectorToPhases(const vector<WDL_TypedBuf<BL_FLOAT> > &inPhases,
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
BLSpectrogram3::UnwrapLineX(WDL_TypedBuf<BL_FLOAT> *phases)
{
    if (mUnwrappedPhases.size() > 0)
    {
        int lastIdx = mUnwrappedPhases.size() - 1;
        
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
BLSpectrogram3::UnwrapLineY(WDL_TypedBuf<BL_FLOAT> *phases)
{
    BLUtils::UnwrapPhases(phases);
}

void
BLSpectrogram3::FillWithZeros()
{
#if FIX_INFRASONIC_VIEWER_RESET
    mMagns.clear();
    mPhases.clear();
#endif
    
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
BLSpectrogram3::SavePPM(const char *filename, int maxValue)
{
    if (mMagns.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > phasesUnW;
    
    if (!mProgressivePhaseUnwrap)
        UnwrapAllPhases(mPhases, &phasesUnW, mDisplayPhasesX, mDisplayPhasesY);
    else
        PhasesToStdVector(mPhases, &phasesUnW);
        
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

BLSpectrogram3 *
BLSpectrogram3::ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits)
{
    BL_FLOAT ppmMultiplier = is16Bits ? 65535.0 : 255.0;
    
    BLSpectrogram3 *result = new BLSpectrogram3(image->h);
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

BLSpectrogram3 *
BLSpectrogram3::ImagesToSpectrogram(PPMFile::PPMImage *magnsImage,
                                   PPMFile::PPMImage *phasesImage)
{
    BLSpectrogram3 *result = new BLSpectrogram3(magnsImage->h);
    
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
BLSpectrogram3::ComputeMaxPhaseValue(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW)
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
BLSpectrogram3::ComputeDPhases(vector<WDL_TypedBuf<BL_FLOAT> > *phasesUnW)
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
BLSpectrogram3::ComputeDPhasesX(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
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
BLSpectrogram3::ComputeDPhasesY(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
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

BLSpectrogram3 *
BLSpectrogram3::Load(const char *fileName)
{
    // TODO
    return NULL;
}

void
BLSpectrogram3::Save(const char *filename)
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

BLSpectrogram3 *
BLSpectrogram3::LoadPPM(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM(fullFilename);
    
    BLSpectrogram3 *result = ImageToSpectrogram(image, false);
    
    return result;
}

void
BLSpectrogram3::SavePPM(const char *filename)
{
    SavePPM(filename, 255);
}

BLSpectrogram3 *
BLSpectrogram3::LoadPPM16(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s", filename);
    
    PPMFile::PPMImage *image = PPMFile::ReadPPM16(fullFilename);
    
    BLSpectrogram3 *result = ImageToSpectrogram(image, true);
    
    return result;
}

void
BLSpectrogram3::SavePPM16(const char *filename)
{
    SavePPM(filename, 65535);
}

BLSpectrogram3 *
BLSpectrogram3::LoadPPM32(const char *filename)
{
    // Magns
    char magnsFullFilename[MAX_PATH];
    sprintf(magnsFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-magns.ppm", filename);
    
    PPMFile::PPMImage *magnsImage = PPMFile::ReadPPM16(magnsFullFilename);
    
    // Phases
    char phasesFullFilename[MAX_PATH];
    sprintf(phasesFullFilename, "/Volumes/HDD/Share/BlueLabAudio-Debug/%s-phases.ppm", filename);
    
    PPMFile::PPMImage *phasesImage = PPMFile::ReadPPM16(phasesFullFilename);
    
    BLSpectrogram3 *result = ImagesToSpectrogram(magnsImage, phasesImage);
    
    return result;
}

void
BLSpectrogram3::SavePPM32(const char *filename)
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

unsigned long long
BLSpectrogram3::GetTotalLineNum()
{
    return mTotalLineNum;
}

