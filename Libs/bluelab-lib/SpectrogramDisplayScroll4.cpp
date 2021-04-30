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
#include <BLTransport.h>

#include "SpectrogramDisplayScroll4.h"

#define USE_SPECTRO_NEAREST 0

#define DEBUG_DUMP 0 //1

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
    
    // Variable speed
    mSpeedMod = 1;

    mPrevOffsetSec = 0.0;

    mTransport = NULL;

    RecomputeParams();

#if DEBUG_DUMP
    BLDebug::ResetFile("offset.txt");
#endif
}

SpectrogramDisplayScroll4::~SpectrogramDisplayScroll4()
{
    if (mTransport != NULL)
        mTransport->SetListener(NULL);
    
    if (mVg == NULL)
        return;
    
    if (mNvgSpectroImage != 0)
        nvgDeleteImage(mVg, mNvgSpectroImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
}

void
SpectrogramDisplayScroll4::SetTransport(BLTransport *transport)
{
    mTransport = transport;
    
    mTransport->SetListener(this);
}

void
SpectrogramDisplayScroll4::Reset()
{
    mSpectroImageData.Resize(0);
    
    mNeedUpdateSpectrogram = true;
    
    mNeedUpdateSpectrogramData = true;
    
    mNeedUpdateColormapData = true;
    
    RecomputeParams();
}

void
SpectrogramDisplayScroll4::ResetScroll()
{    
    RecomputeParams();
}

BL_FLOAT
SpectrogramDisplayScroll4::GetOffsetSec()
{
    if (mTransport == NULL)
        return 0.0;
    
    BL_FLOAT currentTimeSec = mTransport->GetTransportElapsedSecTotal();
    
    if (currentTimeSec < 0.0)
        return 0.0;
    
    BL_FLOAT offset = mSpectroTimeSec - currentTimeSec;

#if DEBUG_DUMP
    BLDebug::AppendValue("offset.txt", offset);
#endif
    
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
    
    BL_FLOAT offsetSec = GetOffsetSec();
    mPrevOffsetSec = offsetSec;
    
    BL_FLOAT offsetPixels = SecsToPixels(offsetSec, width);

    //
    DoUpdateSpectrogram();

    if (!mShowSpectrogram)
        return;
    
    // Draw spectrogram first
    nvgSave(mVg);

    // New: set colormap only in the spectrogram state
    nvgSetColormap(mVg, mNvgColormapImage);
    
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


    // Scale and translate the spectrogram image
    // in order to hide the borders (that are blinking black as data arrives)
    BL_FLOAT leftDelayPix = SecsToPixels(mDelayTimeSecLeft, width);
    BL_FLOAT rightDelayPix = SecsToPixels(mDelayTimeSecRight, width);
    BL_FLOAT scale = ((BL_FLOAT)(width + leftDelayPix + rightDelayPix))/width;

    nvgTranslate(mVg, -leftDelayPix, 1.0);
    nvgScale(mVg, scale, 1.0);

    
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
SpectrogramDisplayScroll4::PushData()
{    
    mLockFreeQueues[1].push(mLockFreeQueues[0]);
    mLockFreeQueues[0].clear();
}

void
SpectrogramDisplayScroll4::PullData()
{   
    mLockFreeQueues[2].push(mLockFreeQueues[1]);
    mLockFreeQueues[1].clear();
}

void
SpectrogramDisplayScroll4::ApplyData()
{
    for (int i = 0; i < mLockFreeQueues[2].size(); i++)
    {
        SpectrogramLine &line = mTmpBuf1;
        mLockFreeQueues[2].get(i, line);

        LFAddSpectrogramLine(line.mMagns, line.mPhases);
    }

    mLockFreeQueues[2].clear();
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
        
    mSpectrogramBounds[0] = left;
    mSpectrogramBounds[1] = top;
     // Avoids black column of 1 pixel on the right
    mSpectrogramBounds[2] = right;
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
    SpectrogramLine &line = mTmpBuf2;
    line.mMagns = magns;
    line.mPhases = phases;
    
    mLockFreeQueues[0].push(line);
}
    
void
SpectrogramDisplayScroll4::LFAddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                                const WDL_TypedBuf<BL_FLOAT> &phases)
{    
    mSpectrogram->AddLine(magns, phases);
    
    mSpectroTimeSec += mSpectroLineDurationSec;
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
SpectrogramDisplayScroll4::TransportPlayingChanged()
{
    RecomputeParams();
}

// Variable speed
void
SpectrogramDisplayScroll4::SetSpeedMod(int speedMod)
{
    mSpeedMod = speedMod;

    if ((mTransport != NULL) && (mTransport->IsTransportPlaying()))
    {
        Reset();
    
        RecomputeParams();
    }
}

int
SpectrogramDisplayScroll4::GetSpeedMod()
{
    return mSpeedMod;
}

void
SpectrogramDisplayScroll4::GetTimeTransform(BL_FLOAT *timeOffsetSec,
                                            BL_FLOAT *timeScale)
{
    *timeOffsetSec = -mDelayTimeSecLeft;

    *timeScale = mSpectroTotalDurationSec/(mSpectroTotalDurationSec +
                                           mDelayTimeSecLeft +
                                           mDelayTimeSecRight);
}

void
SpectrogramDisplayScroll4::RecomputeParams()
{
    if (mSpectrogram == NULL)
        return;

    int spectroNumCols = mSpectrogram->GetNumCols();
    
    mSpectroLineDurationSec =
        mSpeedMod*((double)mBufferSize/mOverlapping)/mSampleRate;

    mSpectroTotalDurationSec = spectroNumCols*mSpectroLineDurationSec;

    mDelayTimeSecRight = mSpectroTotalDurationSec*mDelayPercent*0.01;
    // 1 single row => sometimes fails... (in debug only?)
    //mDelayTimeSecLeft = mSpectroLineDurationSec;
    // Bigger offset
    mDelayTimeSecLeft = mDelayTimeSecRight;

    // HACK! So we are sure to avoid black line on the right
    // (and not to have too much delay when speed is slow)
    mDelayTimeSecRight *= 4.0/mSpeedMod;
    
    mSpectroTimeSec = 0.0;
    mPrevOffsetSec = 0.0;
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
