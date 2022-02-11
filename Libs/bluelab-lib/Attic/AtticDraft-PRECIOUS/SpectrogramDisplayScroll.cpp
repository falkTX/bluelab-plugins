//
//  SpectrogramDisplayScroll.cpp
//  BL-Chroma
//
//  Created by Pan on 14/06/18.
//
//

// Opengl
//#include <GL/glew.h>

//#ifdef __APPLE__
//#include <OpenGL/OpenGL.h>
//#include <OpenGL/glu.h>
//#endif

#include "nanovg.h"

// Warning: Niko hack in NanoVg to support FBO even on GL2
//#define NANOVG_GL2_IMPLEMENTATION

//#include "nanovg_gl.h"
//#include "nanovg_gl_utils.h"
//

#include <BLSpectrogram3.h>
#include <UpTime.h>

#include "SpectrogramDisplayScroll.h"

// Spectrogram
#define GLSL_COLORMAP 1


#define USE_SPECTRO_NEAREST 0

// Number of columns we hide on the left
#define MARGIN_COEFF 8

// Avoids black column of 1 pixel on the right
// (increase of 2 pixels on the right)
#define RIGHT_OFFSET 0.0025


SpectrogramDisplayScroll::SpectrogramDisplayScroll(NVGcontext *vg)
{
    mVg = vg;
    
    // Spectrogram
    mSpectrogram = NULL;
    mNvgSpectroImage = 0;
    mMustUpdateSpectrogram = false;
    mMustUpdateSpectrogramData = false;
    
    mMustUpdateColormapData = false;
    mNvgColormapImage = 0;
    
    mShowSpectrogram = false;
    
    mSpectrogramGain = 1.0;
    
    mBufferSize = 2048;
    mSampleRate = 44100.0;
    
    mPrevSpectroLineNum = 0;
    
    mPrevTimeMillis = UpTime::GetUpTime();
    
    mLinesOffset = 0.0;
    
    mAddLineRemainder = 0.0;
    
    mIsPlaying = false;
    mPrevIsPlaying = false;
    //mPrevElapsedMillis = 0;
    mPrevPixelOffset = 0.0;
}

SpectrogramDisplayScroll::~SpectrogramDisplayScroll()
{
    if (mNvgSpectroImage != 0)
        nvgDeleteImage(mVg, mNvgSpectroImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
}

void
SpectrogramDisplayScroll::Reset()
{
    mSpectroImageData.Resize(0);
    
    mMustUpdateSpectrogram = true;
    
    mMustUpdateSpectrogramData = true;
    
    mMustUpdateColormapData = true;
    
    mPrevSpectroLineNum = 0;
    
    mPrevTimeMillis = UpTime::GetUpTime();
    
    mLinesOffset = 0.0;
    
    mAddLineRemainder = 0.0;
}

bool
SpectrogramDisplayScroll::DoUpdateSpectrogram()
{
    // Update first, before displaying
    if (!mMustUpdateSpectrogram)
        //return false;
        // Must return true, because the spectrogram scrolls over time
        // (for smooth scrolling), even if the data is not changed
        return true;
    
    int w = mSpectrogram->GetNumCols();
    int h = mSpectrogram->GetHeight();
    
#if 1 // Avoid white image when there is no data
    if (w == 0)
    {
        w = 1;
        int imageSize = w*h*4;
        
        mSpectroImageData.Resize(imageSize);
        memset(mSpectroImageData.Get(), 0, imageSize);
        
        // Spectrogram image
        if (mNvgSpectroImage != 0)
            nvgDeleteImage(mVg, mNvgSpectroImage);
        
        mNvgSpectroImage = nvgCreateImageRGBA(mVg,
                                              w, h,
#if USE_SPECTRO_NEAREST
                                              NVG_IMAGE_NEAREST |
#endif
                                              
#if GLSL_COLORMAP
                                              NVG_IMAGE_ONE_FLOAT_FORMAT,
#else
                                              0,
#endif
                                              //,
                                              mSpectroImageData.Get());
        
        mMustUpdateSpectrogram = false;
        mMustUpdateSpectrogramData = false;
        
        return true;
    }
#endif
    
    int imageSize = w*h*4;
    
    if (mMustUpdateSpectrogramData)
    {
        if (mSpectroImageData.GetSize() != imageSize)
        {
            mSpectroImageData.Resize(imageSize);
            
            memset(mSpectroImageData.Get(), 0, imageSize);
            mSpectrogram->GetImageDataFloat(w, h, mSpectroImageData.Get());
            
            // Spectrogram image
            if (mNvgSpectroImage != 0)
                nvgDeleteImage(mVg, mNvgSpectroImage);
            
            mNvgSpectroImage = nvgCreateImageRGBA(mVg,
                                                  w, h,
#if USE_SPECTRO_NEAREST
                                                  NVG_IMAGE_NEAREST |
#endif
                                                  
#if GLSL_COLORMAP
                                                  NVG_IMAGE_ONE_FLOAT_FORMAT,
#else
                                                  0,
#endif
                                                  
                                                  mSpectroImageData.Get());
            
			// No need since it has been better fixed in nanovg
#ifdef WIN32 // Hack: after having created the image, update it again
			 // FIX: spectrogram blinking between random pixels and correct pixels  
			//nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());
#endif
        }
        else
        {
            memset(mSpectroImageData.Get(), 0, imageSize);
            mSpectrogram->GetImageDataFloat(w, h, mSpectroImageData.Get());
            
            // Spectrogram image
            nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());
        }
    }
    
    if (mMustUpdateColormapData)
    {
        // Colormap
        WDL_TypedBuf<unsigned int> colorMapData;
        mSpectrogram->GetColormapImageDataRGBA(&colorMapData);
    
        if (colorMapData.GetSize() != mColormapImageData.GetSize())
        {
            mColormapImageData = colorMapData;
        
            if (mNvgColormapImage != 0)
                nvgDeleteImage(mVg, mNvgColormapImage);
        
            mNvgColormapImage = nvgCreateImageRGBA(mVg,
                                                   mColormapImageData.GetSize(), 1, NVG_IMAGE_NEAREST /*0*/,
                                                   (unsigned char *)mColormapImageData.Get());
        } else
        {
            mColormapImageData = colorMapData;
        
            nvgUpdateImage(mVg, mNvgColormapImage, (unsigned char *)mColormapImageData.Get());
        }
    }
    
    mMustUpdateSpectrogram = false;
    mMustUpdateSpectrogramData = false;
    mMustUpdateColormapData = false;
    
    return true;
}

void
SpectrogramDisplayScroll::DrawSpectrogram(int width, int height)
{
    if (!mShowSpectrogram)
        return;
    
    // Draw spectrogram first
    nvgSave(mVg);
    
    // New: set colormap only in the spectrogram state
#if GLSL_COLORMAP
    nvgSetColormap(mVg, mNvgColormapImage);
#endif

    double scrollOffsetPixels = ComputeScrollOffsetPixels(width);
    
    //
    // Spectrogram image
    //
    
    // Display the rightmost par in case of zoom
    //NVGpaint imgPaint = nvgImagePattern(mVg, 0.0, 0.0, width, height,
    //                                    0.0, mNvgSpectroImage, mSpectrogramAlpha);
    double alpha = 1.0;
    NVGpaint imgPaint = nvgImagePattern(mVg,
                                        mSpectrogramBounds[0]*width + scrollOffsetPixels,
                                        mSpectrogramBounds[1]*height,
                                        (mSpectrogramBounds[2] - mSpectrogramBounds[0])*width,
                                        (mSpectrogramBounds[3] - mSpectrogramBounds[1])*height,
                                        0.0, mNvgSpectroImage, alpha);
    
    nvgBeginPath(mVg);
    nvgRect(mVg,
            mSpectrogramBounds[0]*width + scrollOffsetPixels,
            mSpectrogramBounds[1]*height,
            (mSpectrogramBounds[2] - mSpectrogramBounds[0])*width,
            (mSpectrogramBounds[3] - mSpectrogramBounds[1])*height);
    
    
    nvgFillPaint(mVg, imgPaint);
    nvgFill(mVg);
    
    nvgRestore(mVg);
}

void
SpectrogramDisplayScroll::SetSpectrogram(BLSpectrogram3 *spectro,
                                         double left, double top, double right, double bottom)
{
    mSpectrogram = spectro;
    
    // Must "shift" the left of the spectrogram,
    // So we won't see the black column on the left
    int numCols = mSpectrogram->GetNumCols();
    double normLineSize = 0.0;
    if (numCols > 0)
        normLineSize = 1.0/((double)numCols);
    
    mSpectrogramBounds[0] = left - MARGIN_COEFF*normLineSize;
    mSpectrogramBounds[1] = top;
    mSpectrogramBounds[2] = right + RIGHT_OFFSET; // Avoids black column of 1 pixel on the right
    mSpectrogramBounds[3] = bottom;
    
    mShowSpectrogram = true;
    
    // Be sure to create the texture image in the right thread
    UpdateSpectrogram(true);
    
    // Avoid scrolling over time at launch
    // (until we get initial position)
    mPrevSpectroLineNum = mSpectrogram->GetTotalLineNum();
    // Must set a value at the beginning
    // (otherwise scrolling will be very slow with overlapping 4)
    mAddLineRemainder = mOverlapping;//4; // 16; //4;//37;
}

void
SpectrogramDisplayScroll::SetFftParams(int bufferSize,
                                       int overlapping,
                                       double sampleRate)
{
    bool overlappingChanged = (overlapping != mOverlapping);
    
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mSampleRate = sampleRate;
    
    if (overlappingChanged)
    {
        // Must set a value at the beginning
        // (otherwise scrolling will be very slow with overlapping 4)
        mAddLineRemainder = mOverlapping;
    }
}

void
SpectrogramDisplayScroll::AddSpectrogramLine(const WDL_TypedBuf<double> &magns,
                                             const WDL_TypedBuf<double> &phases)
{
    mSpectroMagns.push_back(magns);
    mSpectroPhases.push_back(phases);
}

void
SpectrogramDisplayScroll::ShowSpectrogram(bool flag)
{
    mShowSpectrogram = flag;
}

void
SpectrogramDisplayScroll::UpdateSpectrogram(bool flag)
{
    mMustUpdateSpectrogram = true;
    
    if (!mMustUpdateSpectrogramData)
        mMustUpdateSpectrogramData = flag;
}

void
SpectrogramDisplayScroll::UpdateColormap(bool flag)
{
    mMustUpdateSpectrogram = true;
    
    if (!mMustUpdateColormapData)
    {
        mMustUpdateColormapData = flag;
    }
}

void
SpectrogramDisplayScroll::SetIsPlaying(bool flag)
{
    mIsPlaying = flag;
    
    if (!flag)
        mPrevIsPlaying = false;
}

void
SpectrogramDisplayScroll::Update()
{
#if 0
    if (!mIsPlaying)
    {
        unsigned long long currentTimeMillis = UpTime::GetUpTime();
        mPrevTimeMillis = currentTimeMillis;
    }
#endif
}

void
SpectrogramDisplayScroll::AddSpectrogramLines(double numLines)
{
    // Keep the remainder, to add back later
    int numLines0 = numLines + mAddLineRemainder;
    
    int numLinesAdded = 0;
    while (mSpectroMagns.size() > 0)
    {
        const WDL_TypedBuf<double> &magns = mSpectroMagns[0];
        const WDL_TypedBuf<double> &phases = mSpectroPhases[0];
        
        mSpectrogram->AddLine(magns, phases);
        
        mSpectroMagns.pop_front();
        mSpectroPhases.pop_front();
        
        numLinesAdded++;
        
        if (numLinesAdded >= numLines0)
            break;
    }
    
    mAddLineRemainder += numLines - (int)numLines;
}

double
SpectrogramDisplayScroll::ComputeScrollOffsetPixels(int width)
{
    // Elapsed time since last time
    unsigned long long currentTimeMillis = UpTime::GetUpTime();
    long long elapsedMillis = currentTimeMillis - mPrevTimeMillis;
    mPrevTimeMillis = currentTimeMillis;
    
    // If not playing, stop do not make the scrolling process
    if (!mIsPlaying)
        return mPrevPixelOffset;
    
    // Playback just restarted
    // Reset elapsed time interval to the last one
    // (do not set to 0 in order to keep as much continuity
    // as possible)
    if (!mPrevIsPlaying && mIsPlaying)
    {
        // GOOD (but makes a very very small jump)
        // BUT USELESS: current elapsedMillis is correct when restarting
        //elapsedMillis = mPrevElapsedMillis;
        
        //fprintf(stderr, "elapsed: %lld\n", elapsedMillis);
        //fprintf(stderr, "prev elapsed: %lld\n", mPrevElapsedMillis);
        
        // does not work
        //elapsedMillis = 0;
        
        mLinesOffset = 0;
        
        // GOOD !
        // Flush the previous data if we stopped the playback and
        // just restarted it
        // Avoids a big jump when restarting
        mSpectroMagns.clear();
        mSpectroPhases.clear();
        
        // Set to 0: no jump
        // Set to mOverlapping: avoid very big lag
        //mAddLineRemainder = 0; //mOverlapping;
        mAddLineRemainder = mOverlapping;
    }
    
    //fprintf(stderr, "elapsed: %lld\n", elapsedMillis);
    //if (elapsedMillis > 80)
    //    elapsedMillis = mPrevElapsedMillis;
    
    mPrevIsPlaying = mIsPlaying;
    //mPrevElapsedMillis = elapsedMillis;
    
    // How many lines by second ?
    double lineSpeed = mOverlapping*mSampleRate/mBufferSize;
    
    // Compute the offset in units "line"
    double offsetLine = lineSpeed*(((double)elapsedMillis)/1000.0);
    
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
    double lineNumPixels = 0.0;
    if (numCols > 0)
        lineNumPixels = ((double)width)/numCols;
    
    double offsetPixels = mLinesOffset*lineNumPixels;
    
    // Add new data if we are shifted enpough
    AddSpectrogramLines(mLinesOffset + 1);
    
    // Fix some problems...
    //
    
    // Avoid scrolling too much on the right
    // (for example when keeping the spacebar pressed)
    // 1 col (as the spectrogram is scalled on the left to hide the black colums)
    //
    // Avoids time scrolling at startup with samplerate 88200Hz too.
    if (offsetPixels > MARGIN_COEFF*lineNumPixels)
    {
        //offsetPixels = maxNumCols*lineNumPixels;
        
        offsetPixels = 0;
        mLinesOffset = 0;
        mAddLineRemainder = mOverlapping; //0.0;
    }
    
    mPrevPixelOffset = offsetPixels;
    
    return offsetPixels;
}
