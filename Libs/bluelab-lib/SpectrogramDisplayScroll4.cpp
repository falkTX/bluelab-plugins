//
//  SpectrogramDisplayScroll4.cpp
//  BL-Chroma
//
//  Created by Pan on 14/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <UpTime.h>
#include <BLDebug.h>
#include <BLUtils.h>

#include "SpectrogramDisplayScroll4.h"

#define USE_SPECTRO_NEAREST 0

// Avoids black column of 1 pixel on the right
// (increase of 2 pixels on the right)
#define RIGHT_OFFSET 0.0025

// Test: to have simple behavior, without smooth scrolling
#define DBG_BYPASS_SMOOTH_SCROLL 0 //1

SpectrogramDisplayScroll4::SpectrogramDisplayScroll4(Plugin *plug,
                                                     BL_FLOAT delayPercent)
{
    mPlug = plug;

    mDelayPercent = delayPercent;
    
    mVg = NULL;
    
    // Spectrogram
    mSpectrogram = NULL;
    mNvgSpectroImage = 0;
    mNeedUpdateSpectrogram = true;
    mNeedUpdateSpectrogramData = true;
    
    mNeedUpdateColormapData = true;
    mNvgColormapImage = 0;
    
    mShowSpectrogram = true;
    
    mBufferSize = 2048;
    mSampleRate = 44100.0;
    mOverlapping = 0;
    
    // Avoid jump when restarting playback
    mIsTransportPlaying = false;
    mIsMonitorOn = false;
    
    // Variable speed
    mSpeedMod = 1;

    mStartTransportTimeStamp = -1.0;
    
    RecomputeParams();
}

SpectrogramDisplayScroll4::~SpectrogramDisplayScroll4()
{
    if (mVg == NULL)
        return;
    
    if (mNvgSpectroImage != 0)
        nvgDeleteImage(mVg, mNvgSpectroImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
}

void
SpectrogramDisplayScroll4::Reset()
{
    mSpectroImageData.Resize(0);
    
    mNeedUpdateSpectrogram = true;
    
    mNeedUpdateSpectrogramData = true;
    
    mNeedUpdateColormapData = true;

    // NEW
    mStartTransportTimeStamp = BLUtils::GetTimeMillisF();
    
    RecomputeParams();
}

void
SpectrogramDisplayScroll4::ResetScroll()
{    
    mSpectroMagns.clear();
    mSpectroPhases.clear();

    ResetQueues();
    
    RecomputeParams();
}

BL_FLOAT
SpectrogramDisplayScroll4::GetOffsetSec(double drawTimeStamp)
{
    if ((mStartTransportTimeStamp < 0.0) || (drawTimeStamp < 0.0))
        return 0.0;
    
    BL_FLOAT currentTimeSec = (drawTimeStamp - mStartTransportTimeStamp)*0.001;
    BL_FLOAT offset = mSpectroTimeSec - currentTimeSec;
    
    return offset;
}

bool
SpectrogramDisplayScroll4::NeedUpdateSpectrogram()
{
    return mNeedUpdateSpectrogram;
}

bool
SpectrogramDisplayScroll4::DoUpdateSpectrogram()
{
    if (mVg == NULL)
        return true;
    
    // Update first, before displaying
    if (!mNeedUpdateSpectrogram)
        // Must return true, because the spectrogram scrolls over time
        // (for smooth scrolling), even if the data is not changed
        return true;
    
    int w = mSpectrogram->GetNumCols();
    int h = mSpectrogram->GetHeight();
    
    int imageSize = w*h*4;
    
    if (mNeedUpdateSpectrogramData || (mNvgSpectroImage == 0))
    {
        if ((mSpectroImageData.GetSize() != imageSize) || (mNvgSpectroImage == 0))
        {
            mSpectroImageData.Resize(imageSize);
            
            memset(mSpectroImageData.Get(), 0, imageSize);
            bool updated = mSpectrogram->GetImageDataFloat(mSpectroImageData.Get());
            
            if (updated)
            {
                // Spectrogram image
                if (mNvgSpectroImage != 0)
                    nvgDeleteImage(mVg, mNvgSpectroImage);
            
                mNvgSpectroImage = nvgCreateImageRGBA(mVg,
                                                      w, h,
#if USE_SPECTRO_NEAREST
                                                      NVG_IMAGE_NEAREST |
#endif
                                                      NVG_IMAGE_ONE_FLOAT_FORMAT,
                                                      mSpectroImageData.Get());
            }
			 // No need since it has been better fixed in nanovg
#ifdef WIN32 // Hack: after having created the image, update it again
			 // FIX: spectrogram blinking between random pixels and correct pixels  
			 //nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());
#endif
        }
        else
        {
                memset(mSpectroImageData.Get(), 0, imageSize);
                bool updated =
                        mSpectrogram->GetImageDataFloat(mSpectroImageData.Get());
                if (updated)
                {
                    // Spectrogram image
                    nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());
                }
        }
    }
    
    if (mNeedUpdateColormapData || (mNvgColormapImage == 0))
    {
        // Colormap
        WDL_TypedBuf<unsigned int> &colorMapData = mTmpBuf0;
        bool updated = mSpectrogram->GetColormapImageDataRGBA(&colorMapData);
        if (updated || (mNvgColormapImage == 0))
        {
            if ((colorMapData.GetSize() != mColormapImageData.GetSize()) ||
                (mNvgColormapImage == 0))
            {
                mColormapImageData = colorMapData;
        
                if (mNvgColormapImage != 0)
                    nvgDeleteImage(mVg, mNvgColormapImage);
        
                    mNvgColormapImage =
                        nvgCreateImageRGBA(mVg,
                                           mColormapImageData.GetSize(),
                                           1, NVG_IMAGE_NEAREST,
                                           (unsigned char *)mColormapImageData.Get());
            }
            else
            {
                mColormapImageData = colorMapData;
        
                nvgUpdateImage(mVg, mNvgColormapImage,
                               (unsigned char *)mColormapImageData.Get());
            }
        }
    }
    
    mNeedUpdateSpectrogram = false;
    mNeedUpdateSpectrogramData = false;
    mNeedUpdateColormapData = false;
    
    return true;
}

void
SpectrogramDisplayScroll4::PreDraw(NVGcontext *vg, int width, int height)
{   
    mVg = vg;

    double drawTimeStamp = BLUtils::GetTimeMillisF();
    AddPendingSpectrogramLines(drawTimeStamp);
    
    DoUpdateSpectrogram();
    
    if (!mShowSpectrogram)
        return;

    // Draw spectrogram first
    nvgSave(mVg);
    
    // New: set colormap only in the spectrogram state
    nvgSetColormap(mVg, mNvgColormapImage);
    
    BL_FLOAT offsetSec = 0.0;
    BL_FLOAT offsetPixels = 0.0;
    if (mIsTransportPlaying || mIsMonitorOn)
    {
        offsetSec = GetOffsetSec(drawTimeStamp);
        offsetPixels = SecsToPixels(offsetSec, width);
    }
    
    //
    // Spectrogram image
    //
    
    // Display the rightmost par in case of zoom
    BL_FLOAT alpha = 1.0;
    NVGpaint imgPaint =
        nvgImagePattern(mVg,
                        mSpectrogramBounds[0]*width + offsetPixels,
                        mSpectrogramBounds[1]*height,
                        (mSpectrogramBounds[2] - mSpectrogramBounds[0])*width,
                        (mSpectrogramBounds[3] - mSpectrogramBounds[1])*height,
                        0.0, mNvgSpectroImage, alpha);
    
    BL_GUI_FLOAT b1f = mSpectrogramBounds[1]*height;
    BL_GUI_FLOAT b3f = (mSpectrogramBounds[3] - mSpectrogramBounds[1])*height;

    // If ever we flip here (with GRAPH_CONTROL_FLIP_Y),
    // the spectrogram won't be displayed.
    
    nvgBeginPath(mVg);
    nvgRect(mVg,
            mSpectrogramBounds[0]*width + offsetPixels,
            b1f,
            (mSpectrogramBounds[2] - mSpectrogramBounds[0])*width, b3f);
    
    
    nvgFillPaint(mVg, imgPaint);
    nvgFill(mVg);
    
    nvgRestore(mVg);
}

void
SpectrogramDisplayScroll4::SetSpectrogram(BLSpectrogram4 *spectro,
                                          BL_FLOAT left, BL_FLOAT top,
                                          BL_FLOAT right, BL_FLOAT bottom)
{
    mSpectrogram = spectro;
    
    // Must "shift" the left of the spectrogram,
    // So we won't see the black column on the left
    int numCols = mSpectrogram->GetNumCols();
    
    BL_FLOAT normLineSize = 0.0;
    
    if (numCols > 0)
        normLineSize = 1.0/((BL_FLOAT)numCols);

    BL_FLOAT leftOffset = mDelayPercent*0.01;
        
    mSpectrogramBounds[0] = left - leftOffset;
    mSpectrogramBounds[1] = top;
     // Avoids black column of 1 pixel on the right
    mSpectrogramBounds[2] = right + RIGHT_OFFSET;
    mSpectrogramBounds[3] = bottom;
    
    mShowSpectrogram = true;
    
    // Be sure to create the texture image in the right thread
    UpdateSpectrogram(true);
    
    // Check that it will be updated well when displaying
    mSpectrogram->TouchData();
    mSpectrogram->TouchColorMap();
    
    RecomputeParams();
}

void
SpectrogramDisplayScroll4::SetFftParams(int bufferSize,
                                        int overlapping,
                                        BL_FLOAT sampleRate)
{
    bool overlappingChanged = (overlapping != mOverlapping);
    
    mBufferSize = bufferSize; 
    mOverlapping = overlapping;
    mSampleRate = sampleRate;
    
    RecomputeParams();
}

void
SpectrogramDisplayScroll4::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                              const WDL_TypedBuf<BL_FLOAT> &phases)
{
#if DBG_BYPASS_SMOOTH_SCROLL
    mSpectrogram->AddLine(magns, phases);
    
    mSpectroTimeSec += mSpectroLineDurationSec;
        
    return;
#endif
    
    // Add the new line to pending lines
    mSpectroMagns.push_back(magns);
    mSpectroPhases.push_back(phases);

    // Remove some pending lines if there are too many available
    //
    // FIX: If the plugin was hidden, and the host playing,
    // mSpectroMagns and mSpectroPhases continued to grow, without being ever flushed
    // (big memory leak)
    int maxCols = mSpectrogram->GetMaxNumCols();
    int bufferLimit = maxCols*2;
    while (mSpectroMagns.size() > bufferLimit)
    {
        mSpectroMagns.pop_front();
        mSpectroPhases.pop_front();
    }
}

void
SpectrogramDisplayScroll4::ShowSpectrogram(bool flag)
{
    mShowSpectrogram = flag;
}

void
SpectrogramDisplayScroll4::UpdateSpectrogram(bool flag)
{
    mNeedUpdateSpectrogram = true;
    
    if (!mNeedUpdateSpectrogramData)
        mNeedUpdateSpectrogramData = flag;
}

void
SpectrogramDisplayScroll4::UpdateColormap(bool flag)
{
    mNeedUpdateSpectrogram = true;
    
    if (!mNeedUpdateColormapData)
    {
        mNeedUpdateColormapData = flag;
    }
}

void
SpectrogramDisplayScroll4::SetTransportPlaying(bool transportPlaying,
                                               bool monitorOn)
{
    if ((transportPlaying != mIsTransportPlaying) ||
        (monitorOn != mIsMonitorOn))
        
    {        
        RecomputeParams();

        mStartTransportTimeStamp = BLUtils::GetTimeMillisF();
    }

    mIsTransportPlaying = transportPlaying;
    mIsMonitorOn = monitorOn;
}

// Variable speed
void
SpectrogramDisplayScroll4::SetSpeedMod(int speedMod)
{
    mSpeedMod = speedMod;

    // NEW
    Reset();
    
    RecomputeParams();
}

int
SpectrogramDisplayScroll4::GetSpeedMod()
{
    return mSpeedMod;
}

BL_FLOAT
SpectrogramDisplayScroll4::GetScaleRatio()
{
    BL_FLOAT ratio = 1.0 - mDelayPercent*0.01;

    return ratio;
}

void
SpectrogramDisplayScroll4::AddPendingSpectrogramLines(double drawTimeStamp)
{
#if DBG_BYPASS_SMOOTH_SCROLL
    return;
#endif

    if (mSpectroMagns.empty())
        return;

    if ((drawTimeStamp < 0.0) || (mStartTransportTimeStamp < 0.0))
        return;
    
    BL_FLOAT currentTimeSec = (drawTimeStamp - mStartTransportTimeStamp)*0.001;
        
    while(mSpectroTimeSec + mSpectroLineDurationSec < currentTimeSec + mDelayTimeSec)
    {
        if (mSpectroMagns.empty())
            break;
        
        const WDL_TypedBuf<BL_FLOAT> &magns = mSpectroMagns[0];
        const WDL_TypedBuf<BL_FLOAT> &phases = mSpectroPhases[0];
        
        mSpectrogram->AddLine(magns, phases);

        mSpectroMagns.pop_front();
        mSpectroPhases.pop_front();

        //
        mSpectroTimeSec += mSpectroLineDurationSec;
    }
}

void
SpectrogramDisplayScroll4::ResetQueues()
{
    // Resize
    int maxCols = mSpectrogram->GetMaxNumCols();
    int bufferLimit = maxCols*2;

    mSpectroMagns.resize(bufferLimit);
    mSpectroPhases.resize(bufferLimit);

    // Set zero value
    WDL_TypedBuf<BL_FLOAT> zeroLine;
    zeroLine.Resize(mBufferSize/2);
    BLUtils::FillAllZero(&zeroLine);

    for (int i = 0; i < bufferLimit; i++)
    {
        mSpectroMagns[i] = zeroLine;
        mSpectroPhases[i] = zeroLine;
    }
}

void
SpectrogramDisplayScroll4::RecomputeParams()
{
    if (mSpectrogram == NULL)
        return;
    
    mSpectroLineDurationSec =
        mSpeedMod*((BL_FLOAT)mBufferSize/mOverlapping)/mSampleRate;

    int numCols0 = mSpectrogram->GetNumCols();
    mSpectroTotalDurationSec = numCols0*mSpectroLineDurationSec;

    mDelayTimeSec = mSpectroTotalDurationSec*mDelayPercent*0.01;
    
    mSpectroTimeSec = 0.0;
}

BL_FLOAT
SpectrogramDisplayScroll4::SecsToPixels(BL_FLOAT secs, BL_FLOAT width)
{
    // NOTE: this coeff improves a small jittering!
    BL_FLOAT coeff = 1.0/(1.0 - mDelayPercent*0.01);
    
    BL_FLOAT pix = (secs/mSpectroTotalDurationSec)*width*coeff;

    return pix;
}

#endif // IGRAPHICS_NANOVG
