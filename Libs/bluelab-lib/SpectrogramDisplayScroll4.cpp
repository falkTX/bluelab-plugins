//
//  SpectrogramDisplayScroll4.cpp
//  BL-Chroma
//
//  Created by Pan on 14/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
//#include <UpTime.h>
#include <BLDebug.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLTransport.h>

#include "SpectrogramDisplayScroll4.h"

#define USE_SPECTRO_NEAREST 0

#define DEBUG_DUMP 0 //1

SpectrogramDisplayScroll4::
SpectrogramDisplayScroll4(SpectrogramDisplayScrollState *spectroState,
                          BL_FLOAT delayPercent)
{
    mState = spectroState;
    if (mState == NULL)
    {
        mState = new SpectrogramDisplayScrollState();
        
        mState->mDelayPercent = delayPercent;
        mState->mDelayTimeSecRight = 0.0;
        mState->mDelayTimeSecLeft = 0.0;

        mState->mSpectroLineDurationSec = 0.0;
        mState->mSpectroTotalDurationSec = 0.0;
        
        mState->mSpectroTimeSec = 0.0;        
        mState->mPrevOffsetSec = 0.0;

        mState->mSpectrogram = NULL;

        // Variable speed
        mState->mSpeedMod = 1;
        
        // Fft params
        mState->mBufferSize = 2048;
        mState->mSampleRate = 44100.0;
        mState->mOverlapping = 4; //0;
    
        //
        mState->mTransport = NULL;

        mState->mSmoothScrollDisabled = false;
    }

    if (mState->mTransport != NULL)
    {
        mState->mTransport->SetListener(this);
        mNeedRedraw = true;
    }
    
    mVg = NULL;
    
    // Spectrogram
    mNvgSpectroImage = 0;
    mNeedUpdateSpectrogram = true;
    mNeedUpdateSpectrogramData = true;
    
    mNeedUpdateColormapData = true;
    mNvgColormapImage = 0;
    
    mShowSpectrogram = true;

    mViewOrientation = HORIZONTAL;

    mIsBypassed = false;

    mUseLegacyLock = false;
    
    RecomputeParams(false);

    mNeedRedraw = true;
    
#if DEBUG_DUMP
    BLDebug::ResetFile("offset.txt");
#endif
}

SpectrogramDisplayScroll4::~SpectrogramDisplayScroll4()
{
    if (mState->mTransport != NULL)
        mState->mTransport->SetListener(NULL);
    
    if (mVg == NULL)
        return;
    
    if (mNvgSpectroImage != 0)
        nvgDeleteImage(mVg, mNvgSpectroImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
}

SpectrogramDisplayScroll4::SpectrogramDisplayScrollState *
SpectrogramDisplayScroll4::GetState()
{
    return mState;
}

void
SpectrogramDisplayScroll4::SetTransport(BLTransport *transport)
{
    mState->mTransport = transport;
    
    mState->mTransport->SetListener(this);

    mNeedRedraw = true;
}

void
SpectrogramDisplayScroll4::Reset()
{
    mSpectroImageData.Resize(0);
    
    mNeedUpdateSpectrogram = true;
    
    mNeedUpdateSpectrogramData = true;
    
    mNeedUpdateColormapData = true;
    
    RecomputeParams(true);

    mNeedRedraw = true;
}

void
SpectrogramDisplayScroll4::ResetScroll()
{    
    RecomputeParams(true);

    mNeedRedraw = true;
}

BL_FLOAT
SpectrogramDisplayScroll4::GetOffsetSec()
{
    if (mState->mSmoothScrollDisabled)
        return 0.0;
    
    if (mState->mTransport == NULL)
        return 0.0;

    // FIX: monitor, bypass => black margin the the right
    if (mIsBypassed)
        return 0.0;
    
    BL_FLOAT currentTimeSec = mState->mTransport->GetTransportElapsedSecTotal();
    if (currentTimeSec < 0.0)
        return 0.0;
    
    BL_FLOAT offset = mState->mSpectroTimeSec - currentTimeSec;

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
    
    int w = mState->mSpectrogram->GetNumCols();
    int h = mState->mSpectrogram->GetHeight();
    
    int imageSize = w*h*4;
    
    if (mNeedUpdateSpectrogramData || (mNvgSpectroImage == 0))
    {
        if ((mSpectroImageData.GetSize() != imageSize) || (mNvgSpectroImage == 0))
        {
            mSpectroImageData.Resize(imageSize);
            
            memset(mSpectroImageData.Get(), 0, imageSize);
            bool updated =
                mState->mSpectrogram->GetImageDataFloat(mSpectroImageData.Get());
            
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
                    mState->mSpectrogram->
                    GetImageDataFloat(mSpectroImageData.Get());
                if (updated)
                {
                    // Spectrogram image
                    nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());
                }
        }

         mNeedRedraw = true;
    }
    
    if (mNeedUpdateColormapData || (mNvgColormapImage == 0))
    {
        // Colormap
        WDL_TypedBuf<unsigned int> &colorMapData = mTmpBuf0;
        bool updated = mState->mSpectrogram->GetColormapImageDataRGBA(&colorMapData);
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

         mNeedRedraw = true;
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
    
    if (std::fabs(offsetSec - mState->mPrevOffsetSec) > BL_EPS)
        mNeedRedraw = true;
    
    mState->mPrevOffsetSec = offsetSec;

#if 0
    if ((mState->mPrevOffsetSec > mState->mDelayTimeSecLeft) ||
        (-mState->mPrevOffsetSec > mState->mDelayTimeSecRight))
#endif
#if 1
    if ((mState->mPrevOffsetSec > mState->mDelayTimeSecLeft) ||
        (-mState->mPrevOffsetSec > mState->mDelayTimeSecRight
         - mState->mSpectroLineDurationSec))
        // Take 1 spectrogram line additional bound
        // (will avoid think black border on the right sometimes)
#endif
    // Offset gets out of bounds, and will make black borders
    {
        // Hard reset the smooth scolling, so we will never have black borders
        // (and most of all, black borders that stay and would only disappear by
        // restarting playback   
        ResetScroll();
        
        if (mState->mTransport != NULL)
        {
            mState->mTransport->HardResynch();
            mState->mTransport->Reset();
        }

        // Recompute offset sec
        offsetSec = GetOffsetSec();
    
        if (std::fabs(offsetSec - mState->mPrevOffsetSec) > BL_EPS)
            mNeedRedraw = true;
    
        mState->mPrevOffsetSec = offsetSec;
    }
    
    BL_FLOAT offsetPixels = SecsToPixels(offsetSec, width);
    
    //
    DoUpdateSpectrogram();

    if (!mShowSpectrogram)
    {
        mNeedRedraw = false;
        
        return;
    }
    
    // Draw spectrogram first
    nvgSave(mVg);

    // New: set colormap only in the spectrogram state
    nvgSetColormap(mVg, mNvgColormapImage);
    
    //
    // Spectrogram image
    //
    
    // Display the rightmost part in case of zoom
    BL_FLOAT alpha = 1.0;
    NVGpaint imgPaint =
        nvgImagePattern(mVg,
                        mState->mSpectrogramBounds[0]*width + offsetPixels,
                        mState->mSpectrogramBounds[1]*height,
                        (mState->mSpectrogramBounds[2] -
                         mState->mSpectrogramBounds[0])*width,
                        (mState->mSpectrogramBounds[3] -
                         mState->mSpectrogramBounds[1])*height,
                        0.0, mNvgSpectroImage, alpha);
    
    BL_GUI_FLOAT b1f = mState->mSpectrogramBounds[1]*height;
    BL_GUI_FLOAT b3f = (mState->mSpectrogramBounds[3] -
                        mState->mSpectrogramBounds[1])*height;

    // If ever we flip here (with GRAPH_CONTROL_FLIP_Y),
    // the spectrogram won't be displayed.


    if (mViewOrientation == HORIZONTAL)
    {
        // Scale and translate the spectrogram image
        // in order to hide the borders (that are blinking black as data arrives)
        BL_FLOAT leftDelayPix = SecsToPixels(mState->mDelayTimeSecLeft, width);
        BL_FLOAT rightDelayPix = SecsToPixels(mState->mDelayTimeSecRight, width);
        BL_FLOAT scale = ((BL_FLOAT)(width + leftDelayPix + rightDelayPix))/width;
        
        nvgTranslate(mVg, -leftDelayPix, 1.0);
        nvgScale(mVg, scale, 1.0);
        
        nvgBeginPath(mVg);
        nvgRect(mVg,
                mState->mSpectrogramBounds[0]*width + offsetPixels,
                b1f,
                (mState->mSpectrogramBounds[2] -
                 mState->mSpectrogramBounds[0])*width, b3f);
        
        nvgFillPaint(mVg, imgPaint);
        nvgFill(mVg);
    }
    else
    {
        // Scale and translate the spectrogram image
        // in order to hide the borders (that are blinking black as data arrives)
        BL_FLOAT leftDelayPix = SecsToPixels(mState->mDelayTimeSecLeft, height);
        BL_FLOAT rightDelayPix = SecsToPixels(mState->mDelayTimeSecRight, height);
        BL_FLOAT scale = ((BL_FLOAT)(height + leftDelayPix + rightDelayPix))/height;
        
        nvgTranslate(mVg, width*0.5, height*0.5);
        nvgRotate(mVg, -M_PI*0.5);
        nvgTranslate(mVg, -width*0.5, -height*0.5);

        BL_FLOAT coeff = ((BL_FLOAT)width)/height;
        nvgTranslate(mVg, width*0.5, height*0.5);
        nvgScale(mVg, 1.0/coeff, coeff);
        nvgTranslate(mVg, -width*0.5, -height*0.5);
        
        nvgTranslate(mVg, -leftDelayPix, 1.0);
        nvgScale(mVg, scale, 1.0);
        
        nvgBeginPath(mVg);
        nvgRect(mVg,
                mState->mSpectrogramBounds[0]*width + offsetPixels,
                b1f,
                (mState->mSpectrogramBounds[2] -
                 mState->mSpectrogramBounds[0])*width, b3f);
        
        nvgFillPaint(mVg, imgPaint);
        nvgFill(mVg);
    }
    
    nvgRestore(mVg);

    mNeedRedraw = false;
}

bool
SpectrogramDisplayScroll4::NeedRedraw()
{
    return mNeedRedraw;
}

void
SpectrogramDisplayScroll4::PushData()
{
    // Must dirty this CustomDrawer if some data is pushed
    // Because GraphControl12::IsDirty() is called before PullAllData(),
    // which is called in GraphControl12::Draw()
    if (!mLockFreeQueues[0].empty())
        mNeedRedraw = true;
    
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
        Command &cmd = mTmpBuf1;
        mLockFreeQueues[2].get(i, cmd);

        if (cmd.mType == Command::ADD_SPECTROGRAM_LINE)
            AddSpectrogramLineLF(cmd.mMagns, cmd.mPhases);
        else if (cmd.mType == Command::SET_SPEED_MOD)
            SetSpeedModLF(cmd.mSpeedMod);
        else if (cmd.mType == Command::SET_BYPASSED)
            SetBypassedLF(cmd.mBypassed);
    }

    mLockFreeQueues[2].clear();
}

void
SpectrogramDisplayScroll4::SetSpectrogram(BLSpectrogram4 *spectro,
                                          BL_FLOAT left, BL_FLOAT top,
                                          BL_FLOAT right, BL_FLOAT bottom)
{
    // Just in case
    mSpectroImageData.Resize(0);
    
    mState->mSpectrogram = spectro;
    
    // Must "shift" the left of the spectrogram,
    // So we won't see the black column on the left
    int numCols = mState->mSpectrogram->GetNumCols();
    
    BL_FLOAT normLineSize = 0.0;
    
    if (numCols > 0)
        normLineSize = 1.0/((BL_FLOAT)numCols);
        
    mState->mSpectrogramBounds[0] = left;
    mState->mSpectrogramBounds[1] = top;
     // Avoids black column of 1 pixel on the right
    mState->mSpectrogramBounds[2] = right;
    mState->mSpectrogramBounds[3] = bottom;
    
    mShowSpectrogram = true;
    
    // Be sure to create the texture image in the right thread
    UpdateSpectrogram(true);
    
    // Check that it will be updated well when displaying
    mState->mSpectrogram->TouchData();
    mState->mSpectrogram->TouchColorMap();
    
    RecomputeParams(true);

    mNeedRedraw = true;
}

void
SpectrogramDisplayScroll4::SetFftParams(int bufferSize,
                                        int overlapping,
                                        BL_FLOAT sampleRate)
{
    mState->mBufferSize = bufferSize; 
    mState->mOverlapping = overlapping;
    mState->mSampleRate = sampleRate;
    
    RecomputeParams(true);

    mNeedRedraw = true;
}

void
SpectrogramDisplayScroll4::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                              const WDL_TypedBuf<BL_FLOAT> &phases)
{
    if (mUseLegacyLock)
    {
        AddSpectrogramLineLF(magns, phases);

        return;
    }
    
    Command &cmd = mTmpBuf2;
    cmd.mType = Command::ADD_SPECTROGRAM_LINE;
    cmd.mMagns = magns;
    cmd.mPhases = phases;
    
    mLockFreeQueues[0].push(cmd);
}
    
void
SpectrogramDisplayScroll4::AddSpectrogramLineLF(const WDL_TypedBuf<BL_FLOAT> &magns,
                                                const WDL_TypedBuf<BL_FLOAT> &phases)
{    
    mState->mSpectrogram->AddLine(magns, phases);
    
    mState->mSpectroTimeSec += mState->mSpectroLineDurationSec;

    mNeedRedraw = true;
}

void
SpectrogramDisplayScroll4::ShowSpectrogram(bool flag)
{
    if (flag != mShowSpectrogram)
        mNeedRedraw = true;
    
    mShowSpectrogram = flag;
}

void
SpectrogramDisplayScroll4::UpdateSpectrogram(bool flag)
{    
    if (!mNeedUpdateSpectrogram)
        mNeedRedraw = true;
    
    mNeedUpdateSpectrogram = true;
    
    if (flag && !mNeedUpdateSpectrogramData)
    {
        mNeedUpdateSpectrogramData = flag;

        mNeedRedraw = true;
    }
}

void
SpectrogramDisplayScroll4::UpdateColormap(bool flag)
{
    if (!mNeedUpdateSpectrogram)
        mNeedRedraw = true;
    
    mNeedUpdateSpectrogram = true;
    
    if (flag && !mNeedUpdateColormapData)
    {
        mNeedUpdateColormapData = flag;

        mNeedRedraw = true;
    }
}

void
SpectrogramDisplayScroll4::TransportPlayingChanged()
{
    RecomputeParams(true);

    mNeedRedraw = true;
}

void
SpectrogramDisplayScroll4::SetSpeedMod(int speedMod)
{
    //if (mState->mSpeedMod == speedMod)
    //    return;

    if (mUseLegacyLock)
    {
        SetSpeedModLF(speedMod);
        
        return;
    }
    
    Command &cmd = mTmpBuf3;
    cmd.mType = Command::SET_SPEED_MOD;
    cmd.mSpeedMod = speedMod;
    
    mLockFreeQueues[0].push(cmd);
}
    
// Variable speed
void
SpectrogramDisplayScroll4::SetSpeedModLF(int speedMod)
{    
    mState->mSpeedMod = speedMod;

#if 1
    if ((mState->mTransport != NULL) && (mState->mTransport->IsTransportPlaying()))
#endif
    {
        Reset();
    
        RecomputeParams(true);
    }

    mNeedRedraw = true;
}

int
SpectrogramDisplayScroll4::GetSpeedMod()
{
    return mState->mSpeedMod;
}

void
SpectrogramDisplayScroll4::GetTimeTransform(BL_FLOAT *timeOffsetSec,
                                            BL_FLOAT *timeScale)
{
    *timeOffsetSec = -mState->mDelayTimeSecLeft;

    *timeScale = mState->mSpectroTotalDurationSec/(mState->mSpectroTotalDurationSec +
                                                   mState->mDelayTimeSecLeft +
                                                   mState->mDelayTimeSecRight);
}

void
SpectrogramDisplayScroll4::GetTimeBoundsNorm(BL_FLOAT *tn0, BL_FLOAT *tn1)
{
    // Also take offset into account => more accurate!
    BL_FLOAT offsetSec = GetOffsetSec();

    *tn0 = (mState->mDelayTimeSecLeft - offsetSec)/mState->mSpectroTotalDurationSec;
    *tn1 = (mState->mSpectroTotalDurationSec -
            mState->mDelayTimeSecRight - offsetSec)/
        mState->mSpectroTotalDurationSec;
}

void
SpectrogramDisplayScroll4::SetViewOrientation(ViewOrientation orientation)
{
    if (orientation != mViewOrientation)
        mNeedRedraw = true;
    
    mViewOrientation = orientation;
}

void
SpectrogramDisplayScroll4::SetBypassed(bool flag)
{
    if (flag == mIsBypassed)
        // Nothing to do
        return;

    if (mUseLegacyLock)
    {
        SetBypassedLF(flag);
        
        return;
    }
    
    Command &cmd = mTmpBuf4;
    cmd.mType = Command::SET_BYPASSED;
    cmd.mBypassed = flag;
    
    mLockFreeQueues[0].push(cmd);
}

void
SpectrogramDisplayScroll4::SetBypassedLF(bool flag)
{
    if (flag != mIsBypassed)
        mNeedRedraw = true;
        
    mIsBypassed = flag;
}

void
SpectrogramDisplayScroll4::SetSmoothScrollDisabled(bool flag)
{
    if (!flag && mState->mSmoothScrollDisabled)
        // Just re-enabled
        ResetScroll();

    // Set the field
    mState->mSmoothScrollDisabled = flag;
}

void
SpectrogramDisplayScroll4::SetUseLegacyLock(bool flag)
{
    mUseLegacyLock = flag;
}

void
SpectrogramDisplayScroll4::RecomputeParams(bool resetAll)
{
    if (mState->mSpectrogram == NULL)
        return;
    
    int spectroNumCols = mState->mSpectrogram->GetNumCols();
    
    mState->mSpectroLineDurationSec =
        mState->mSpeedMod*((double)mState->mBufferSize/mState->mOverlapping)/
        mState->mSampleRate;
    
    mState->mSpectroTotalDurationSec = spectroNumCols*mState->mSpectroLineDurationSec;

    if (!mState->mSmoothScrollDisabled)
    {
        mState->mDelayTimeSecRight =
            mState->mSpectroTotalDurationSec*mState->mDelayPercent*0.01;
        // 1 single row => sometimes fails... (in debug only?)
        //mDelayTimeSecLeft = mSpectroLineDurationSec;
        // Bigger offset
        mState->mDelayTimeSecLeft = mState->mDelayTimeSecRight;
        
        // HACK! So we are sure to avoid black line on the right
        // (and not to have too much delay when speed is slow)
        mState->mDelayTimeSecRight *= 4.0/mState->mSpeedMod;
    }
    
#if 1
    // Ensure that the delay corresponds to enough spectro lines
    // (otherwise we would see black lines)
    BL_FLOAT delayNumColsL =
        (mState->mDelayTimeSecLeft/mState->mSpectroTotalDurationSec)*spectroNumCols;
    BL_FLOAT delayNumColsR =
        (mState->mDelayTimeSecRight/mState->mSpectroTotalDurationSec)*spectroNumCols;
    
    const BL_FLOAT minNumLinesL = 2.0;
    const BL_FLOAT minNumLinesR = 2.0;
    
#if 1 // Also adjust the left channel?
      // => Set to 1 to fix a bug: InfraViewer: set freq accuracy to max,
      // then resize gui => black column on the left
    if (delayNumColsL < minNumLinesL)
    {
        if (delayNumColsL > 0.0)
            mState->mDelayTimeSecLeft *= minNumLinesL/delayNumColsL;
    }
#endif
    
    if (delayNumColsR < minNumLinesR)
    {
        if (delayNumColsR > 0.0)
            mState->mDelayTimeSecRight *= minNumLinesR/delayNumColsR;
    }
#endif

    if (resetAll)
    {
        mState->mSpectroTimeSec = 0.0;
        mState->mPrevOffsetSec = 0.0;
    }
}

BL_FLOAT
SpectrogramDisplayScroll4::SecsToPixels(BL_FLOAT secs, BL_FLOAT width)
{
    // NOTE: this coeff improves a small jittering!
    BL_FLOAT coeff = 1.0/(1.0 - mState->mDelayPercent*0.01);
    BL_FLOAT pix = (secs/mState->mSpectroTotalDurationSec)*width*coeff;
    
    return pix;
}

#endif // IGRAPHICS_NANOVG
