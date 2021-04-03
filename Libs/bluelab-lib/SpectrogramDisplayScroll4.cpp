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

// NEW: InfrasonicViewer port to iPlug2
#define FIX_JITTER_INFRASONIC_VIEWER 1

// Number of columns we hide on the left
#if !FIX_JITTER_INFRASONIC_VIEWER
// Do we need it for GhostVeiwer etc... ?
#define MARGIN_COEFF 8
#else
// Must be set to 1 for InfrasonicViewer!
#define MARGIN_COEFF 1
#endif

// Avoids black column of 1 pixel on the right
// (increase of 2 pixels on the right)
#define RIGHT_OFFSET 0.0025

// Test: to have simple behavior, without smooth scrolling
#define DBG_BYPASS_SMOOTH_SCROLL 0 //1

#define DEBUG_DUMP 1

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
    
    //mPrevSpectroLineNum = 0;
    
    //mPrevTimeMillis = UpTime::GetUpTime();
    
    //mLinesOffset = 0.0;
    
    //mAddLineRemainder = 0.0;
    
    // Avoid jump when restarting playback
    mPrevIsPlaying = false;
    //mPrevPixelOffset = 0.0;
    
    mIsPlaying = false;
    
    // Variable speed
    mSpeedMod = 1;

    RecomputeParams();
        
#if DEBUG_DUMP
    BLDebug::ResetFile("spectro-time0.txt");
    BLDebug::ResetFile("spectro-time1.txt");
    BLDebug::ResetFile("time.txt");
#endif
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
    
    //mPrevSpectroLineNum = 0;
    
    //mPrevTimeMillis = UpTime::GetUpTime();
    
    //mLinesOffset = 0.0;
    
    //mAddLineRemainder = 0.0;

    RecomputeParams();
    
#if DEBUG_DUMP
    BLDebug::ResetFile("spectro-time0.txt");
    BLDebug::ResetFile("spectro-time1.txt");
    BLDebug::ResetFile("time.txt");
#endif
}

void
SpectrogramDisplayScroll4::ResetScroll()
{
    //mLinesOffset = 0;
    
    mSpectroMagns.clear();
    mSpectroPhases.clear();

    ResetQueues(); //
    
    // Set to 0: no jump (but lag)
    // Set to mOverlapping: avoid very big lag
    //mAddLineRemainder = mOverlapping;

    RecomputeParams();
    
#if DEBUG_DUMP
    BLDebug::ResetFile("spectro-time0.txt");
    BLDebug::ResetFile("spectro-time1.txt");
    BLDebug::ResetFile("time.txt");
#endif
}

BL_FLOAT
SpectrogramDisplayScroll4::GetOffsetSec()
{
    long int millis = BLUtils::GetTimeMillis();
    BL_FLOAT currentTimeSec = (millis - mStartTimeMillis)*0.001;

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
        //return false;
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

    AddPendingSpectrogramLines();
    
    DoUpdateSpectrogram();
    
    if (!mShowSpectrogram)
        return;

    // Draw spectrogram first
    nvgSave(mVg);
    
    // New: set colormap only in the spectrogram state
    nvgSetColormap(mVg, mNvgColormapImage);

    //BL_FLOAT scrollOffsetPixels = ComputeScrollOffsetPixels(width);

    BL_FLOAT offsetSec = GetOffsetSec();
    BL_FLOAT offsetPixels = SecsToPixels(offsetSec, width);
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

#if DEBUG_DUMP // Debug 
    BLDebug::AppendValue("spectro-time0.txt", mSpectroTimeSec);
    BLDebug::AppendValue("spectro-time1.txt", mSpectroTimeSec + offsetSec);

    long int millis = BLUtils::GetTimeMillis();
    BL_FLOAT currentTimeSec = (millis - mStartTimeMillis)*0.001;

    BLDebug::AppendValue("time.txt", currentTimeSec);
#endif
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
    
    mSpectrogramBounds[0] = left - MARGIN_COEFF*normLineSize;
    mSpectrogramBounds[1] = top;
    mSpectrogramBounds[2] = right + RIGHT_OFFSET; // Avoids black column of 1 pixel on the right
    mSpectrogramBounds[3] = bottom;
    
    mShowSpectrogram = true;
    
    // Be sure to create the texture image in the right thread
    UpdateSpectrogram(true);
    
    // Avoid scrolling over time at launch
    // (until we get initial position)
    //mPrevSpectroLineNum = mSpectrogram->GetTotalLineNum();
    // Must set a value at the beginning
    // (otherwise scrolling will be very slow with overlapping 4)
    //mAddLineRemainder = mOverlapping;
    
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
    
    //if (overlappingChanged)
    //{
    // Must set a value at the beginning
    // (otherwise scrolling will be very slow with overlapping 4)
    //    mAddLineRemainder = mOverlapping;
    //}

    RecomputeParams();
}

void
SpectrogramDisplayScroll4::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                              const WDL_TypedBuf<BL_FLOAT> &phases)
{
#if DBG_BYPASS_SMOOTH_SCROLL
    mSpectrogram->AddLine(magns, phases);
    
    mSpectroTime += mSpectroLineDurationSec;
        
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
SpectrogramDisplayScroll4::SetIsPlaying(bool flag)
{
    mIsPlaying = flag;
}

// Variable speed
void
SpectrogramDisplayScroll4::SetSpeedMod(int speedMod)
{
    mSpeedMod = speedMod;
}

int
SpectrogramDisplayScroll4::GetSpeedMod()
{
    return mSpeedMod;
}

BL_FLOAT
SpectrogramDisplayScroll4::GetScaleRatio()
{
    int maxCols = mSpectrogram->GetMaxNumCols();
    BL_FLOAT ratio = ((BL_FLOAT)(maxCols - MARGIN_COEFF))/maxCols;
    
    return ratio;
}

void
SpectrogramDisplayScroll4::AddPendingSpectrogramLines()
{
#if DBG_BYPASS_SMOOTH_SCROLL
    return;
#endif
        
    long int millis = BLUtils::GetTimeMillis();
    BL_FLOAT currentTimeSec = (millis - mStartTimeMillis)*0.001;
        
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

#if 0
BL_FLOAT
SpectrogramDisplayScroll4::ComputeScrollOffsetPixels(int width)
{
#if DBG_BYPASS_SMOOTH_SCROLL
    return 0.0;
#endif
    
    // Elapsed time since last time
    unsigned long long currentTimeMillis = UpTime::GetUpTime();
    long long elapsedMillis = currentTimeMillis - mPrevTimeMillis;
    mPrevTimeMillis = currentTimeMillis;
    
    // Special case: playback stopped
    //
    // If not playing, stop do not make the scrolling process
    bool isPlaying = mIsPlaying;
    if (!isPlaying)
    {
        mPrevIsPlaying = false;
        
        return mPrevPixelOffset;
    }
    
    // Special case: playback just restarted
    //
    // Reset some things to avoid jumps in scrolling when restarting playback
    //
    if (!mPrevIsPlaying && isPlaying)
    {
        mLinesOffset = 0;
        
        // GOOD !
        // Flush the previous data if we stopped the playback and just restarted it
        // Avoids a big jump when restarting
        //mSpectroMagns.unfreeze();
        //mSpectroMagns.clear();

        //mSpectroPhases.unfreeze();
        //mSpectroPhases.clear();
        
        // Set to 0: no jump (but lag)
        // Set to mOverlapping: avoid very big lag
        mAddLineRemainder = mOverlapping;
    }
    
    // Update
    mPrevIsPlaying = isPlaying;
    
    // Do compute the offset
    //
    
    // How many lines by second ?
    BL_FLOAT lineSpeed = mOverlapping*mSampleRate/mBufferSize;
    
    // Variable speed
    lineSpeed /= mSpeedMod;
    
    // Compute the offset in units "line"
    BL_FLOAT offsetLine = lineSpeed*(((BL_FLOAT)elapsedMillis)/1000.0);
    mLinesOffset -= offsetLine;
    
    // How many lines added
    unsigned long long currentLineNum = mSpectrogram->GetTotalLineNum();
    long long diffLineNum = currentLineNum - mPrevSpectroLineNum;
    mPrevSpectroLineNum = currentLineNum;
    
    mLinesOffset += diffLineNum;
    if (mLinesOffset < 0.0)
        mLinesOffset = 0.0;
    
    // Compute the offset in units "pixels"
    int numCols = mSpectrogram->GetNumCols();
    
    BL_FLOAT lineNumPixels = 0.0;
    if (numCols > 0)
        lineNumPixels = ((BL_FLOAT)width)/numCols;
    
    BL_FLOAT offsetPixels = mLinesOffset*lineNumPixels;
    
    // Add new data if we are shifted enpough
    AddSpectrogramLines(mLinesOffset + 1);
    
    // Special case: scrolled too much
    //
    // Avoid scrolling too much on the right
    // (for example when keeping the spacebar pressed)
    // 1 col (as the spectrogram is scalled on the left to hide the black colums)
    //
    // Avoids time scrolling at startup with samplerate 88200Hz too.
    if (offsetPixels > MARGIN_COEFF*lineNumPixels)
    {
        // Reset
        offsetPixels = 0;
        mLinesOffset = 0;
        mAddLineRemainder = mOverlapping;
    }
    
    mPrevPixelOffset = offsetPixels;
    
    return offsetPixels;
}
#endif

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
    long int millis = BLUtils::GetTimeMillis();
    mStartTimeMillis = millis;
}

BL_FLOAT
SpectrogramDisplayScroll4::SecsToPixels(BL_FLOAT secs, BL_FLOAT width)
{
    BL_FLOAT pix = (secs/mSpectroTotalDurationSec)*width;

    return pix;
}

#endif // IGRAPHICS_NANOVG
