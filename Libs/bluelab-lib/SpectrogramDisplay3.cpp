//
//  SpectrogramDisplay3.cpp
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>

#include "SpectrogramDisplay3.h"


#define USE_SPECTRO_NEAREST 0


SpectrogramDisplay3::SpectrogramDisplay3(SpectrogramDisplayState *state)
{
    mState = state;
    if (mState == NULL)
    {
        mState = new SpectrogramDisplayState();
        
        mState->mCenterPos = 0.5;
        mState->mAbsTranslation = 0.0;
        
        mState->mMinX = 0.0;
        mState->mMaxX = 1.0;
        
        mState->mAbsMinX = 0.0;
        mState->mAbsMaxX = 1.0;
        
        mState->mSpectroImageWidth = 0;
        mState->mSpectroImageHeight = 0;
    }
    
    mVg = NULL;
    
    // Spectrogram
    mSpectrogram = NULL;
    mNvgSpectroImage = 0;
    mNeedUpdateSpectrogram = false;
    mNeedUpdateSpectrogramData = false;
    mNvgColormapImage = 0;
    
    mNeedUpdateBGSpectrogramData = false;
    
    mNeedUpdateColormapData = false;
    
    mNvgBGSpectroImage = 0;
    
    mShowSpectrogram = false;
    
    mSpectrogramAlpha = 1.0;
    
    mDrawBGSpectrogram = true;
    
    mNeedRedraw = true;
}

SpectrogramDisplay3::~SpectrogramDisplay3()
{
    if (mVg == NULL)
        return;
  
    if (mNvgSpectroImage != 0)
        nvgDeleteImage(mVg, mNvgSpectroImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);
    
    if (mNvgBGSpectroImage != 0)
        nvgDeleteImage(mVg, mNvgBGSpectroImage);
}

SpectrogramDisplay3::SpectrogramDisplayState *
SpectrogramDisplay3::GetState()
{
    return mState;
}

void
SpectrogramDisplay3::Reset()
{
    mSpectroImageData.Resize(0);
    
    mNeedUpdateSpectrogram = true;   
    mNeedUpdateSpectrogramData = true;
    
    mNeedRedraw = true;
}

bool
SpectrogramDisplay3::NeedUpdateSpectrogram()
{
    return mNeedUpdateSpectrogram;
}

bool
SpectrogramDisplay3::DoUpdateSpectrogram()
{
    if (mVg == NULL)
        return true;
  
    // Update first, before displaying
    if (!mNeedUpdateSpectrogram)
        return false;
    
    int w = mSpectrogram->GetNumCols();
    int h = mSpectrogram->GetHeight();

    // The following lines avoid that: at startup when there is no data loaded,
    // all the background was white.
    if ((w == 0) || (h == 0))
    {
        w = 1;
        h = 1;
    }
    
    int imageSize = w*h*4;
    
    if ((mNeedUpdateSpectrogramData ||
         (mNvgSpectroImage == 0) ||
         (mNvgBGSpectroImage == 0)) ||
        (mSpectroImageData.GetSize() != imageSize))
    {
        bool imageCreated = false;
        if ((mSpectroImageData.GetSize() != imageSize) ||
            (mNvgSpectroImage == 0))
        {
            mSpectroImageData.Resize(imageSize);
            
            memset(mSpectroImageData.Get(), 0, imageSize);
            mSpectrogram->GetImageDataFloat(mSpectroImageData.Get());
            
            if (mState->mBGSpectroImageData.GetSize() == 0)
            {
                mState->mSpectroImageWidth = w;
                mState->mSpectroImageHeight = h;
                
                mState->mBGSpectroImageData = mSpectroImageData;
                
                mNeedUpdateBGSpectrogramData = true;
            }
            
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
            
            imageCreated = true;
        }
        
        bool bgImageCreated = false;
        if (mNeedUpdateBGSpectrogramData || (mNvgBGSpectroImage == 0))
        {
            // Spectrogram background image
            if (mNvgBGSpectroImage != 0)
                nvgDeleteImage(mVg, mNvgBGSpectroImage);
                
            mNvgBGSpectroImage = nvgCreateImageRGBA(mVg,
                                                    mState->mSpectroImageWidth,
                                                    mState->mSpectroImageHeight,
#if USE_SPECTRO_NEAREST
                                                    NVG_IMAGE_NEAREST |
#endif
                                                    NVG_IMAGE_ONE_FLOAT_FORMAT,
                                                    mState->mBGSpectroImageData.Get());
            
            
            bgImageCreated = true;

        }
        
        if (!imageCreated || !bgImageCreated)
        {
            memset(mSpectroImageData.Get(), 0, imageSize);
            bool updated = mSpectrogram->GetImageDataFloat(mSpectroImageData.Get());

            if (updated)
            {
                if (!imageCreated)
                {
                    if (mNvgSpectroImage != 0)
                    {
                        // Spectrogram image
                        nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());
                    }
                }
                
                if (!bgImageCreated)
                {
                    // Spectrogram background image
                    if (mNeedUpdateBGSpectrogramData)
                    {
                        if (mNvgBGSpectroImage != 0)
                            nvgUpdateImage(mVg,
                                           mNvgBGSpectroImage,
                                           mState->mBGSpectroImageData.Get());
                    }
                }
            }
        }
    }

    if (mNeedUpdateColormapData || (mNvgColormapImage == 0))
    {
        // Colormap
        bool updated = mSpectrogram->GetColormapImageDataRGBA(&mColormapImageData);
        if (mNvgColormapImage == 0)
        {        
            if (mNvgColormapImage != 0)
                nvgDeleteImage(mVg, mNvgColormapImage);
        
            mNvgColormapImage =
            nvgCreateImageRGBA(mVg,
                               mColormapImageData.GetSize(), 1,
                               NVG_IMAGE_NEAREST,
                               (unsigned char *)mColormapImageData.Get());
        }
        else
        { 
            if (updated)
                nvgUpdateImage(mVg, mNvgColormapImage,
                               (unsigned char *)mColormapImageData.Get());
        }
    }
    
    mNeedUpdateSpectrogram = false;
    mNeedUpdateSpectrogramData = false;
    mNeedUpdateBGSpectrogramData = false;
    
    return true;
}

void
SpectrogramDisplay3::PreDraw(NVGcontext *vg, int width, int height)
{
    mVg = vg;

    bool updated = DoUpdateSpectrogram();

#if 0 //////////////////////DEBUG
    if (mSpectrogram != NULL)
    {
        int maxNumCols = mSpectrogram->GetMaxNumCols();
        int numCols = mSpectrogram->GetNumCols();
        int height0 = mSpectrogram->GetHeight();

        fprintf(stderr, "maxc: %d [c = %d   h = %d]\n",
                maxNumCols, numCols, height0);
    }
    //////////////////////
#endif
    
    if (!mShowSpectrogram)
    {
        mNeedRedraw = false;
        
        return;
    }
    
    // Draw spectrogram first
    nvgSave(mVg);
    
    // New: set colormap only in the spectrogram state
    nvgSetColormap(mVg, mNvgColormapImage);
    
#define DEBUG_DISPLAY_FG 1
#define DEBUG_DISPLAY_BG 1
    
#if DEBUG_DISPLAY_BG
    if (mDrawBGSpectrogram)
    {
        //
        // Spectrogram background image
        //
        // Display the background image first, so it will be in background
        NVGpaint bgImgPaint = nvgImagePattern(mVg,
                                              mSpectrogramBounds[0]*width,
                                              mSpectrogramBounds[1]*height,
                                              (mSpectrogramBounds[2] -
                                               mSpectrogramBounds[0])*width,
                                              (mSpectrogramBounds[3] -
                                               mSpectrogramBounds[1])*height,
                                              0.0,
                                              mNvgBGSpectroImage, mSpectrogramAlpha);
        //
        nvgResetTransform(mVg);
    
        BL_FLOAT absZoom = mState->mAbsMaxX - mState->mAbsMinX;
    
        nvgTranslate(mVg, mState->mAbsMinX*width, 0.0);
        nvgScale(mVg, absZoom, 1.0);
        
    
        BL_GUI_FLOAT b1f = mSpectrogramBounds[1]*height;
        BL_GUI_FLOAT b3f = (mSpectrogramBounds[3] - mSpectrogramBounds[1])*height;
        
        nvgBeginPath(mVg);
    
        // Corner (x, y) => bottom-left
        nvgRect(mVg,
                mSpectrogramBounds[0]*width, b1f,
                (mSpectrogramBounds[2] - mSpectrogramBounds[0])*width, b3f);
    
        nvgFillPaint(mVg, bgImgPaint);
        nvgFill(mVg);

        //
        nvgResetTransform(mVg);
    }
#endif
    
#if DEBUG_DISPLAY_FG
    //
    // Spectrogram image
    //
    
    // Display the rightmost par in case of zoom
    NVGpaint imgPaint = nvgImagePattern(mVg,
                                        mSpectrogramBounds[0]*width,
                                        mSpectrogramBounds[1]*height,
                                        (mSpectrogramBounds[2] -
                                         mSpectrogramBounds[0])*width,
                                        (mSpectrogramBounds[3] -
                                         mSpectrogramBounds[1])*height,
                                        0.0, mNvgSpectroImage, mSpectrogramAlpha);
    
    //
    nvgResetTransform(mVg);
    
    BL_FLOAT zoom = mState->mMaxX - mState->mMinX;
    
    nvgTranslate(mVg, mState->mMinX*width, 0.0);
    nvgScale(mVg, zoom, 1.0);

    
    BL_GUI_FLOAT b1f = mSpectrogramBounds[1]*height;
    BL_GUI_FLOAT b3f = (mSpectrogramBounds[3] - mSpectrogramBounds[1])*height;
    
    //
    nvgBeginPath(mVg);
    nvgRect(mVg,
            mSpectrogramBounds[0]*width, b1f,
            (mSpectrogramBounds[2] - mSpectrogramBounds[0])*width, b3f);
    
    
    nvgFillPaint(mVg, imgPaint);
    nvgFill(mVg);
    
    //
    nvgResetTransform(mVg);
    
#endif
    
    nvgRestore(mVg);
    
    mNeedRedraw = updated;
}

bool
SpectrogramDisplay3::NeedRedraw()
{
    return mNeedRedraw;
}

bool
SpectrogramDisplay3::PointInsideSpectrogram(int x, int y, int width, int height)
{
    // Warning: y is reversed !
    BL_FLOAT nx = ((BL_FLOAT)x)/width;
    //BL_FLOAT ny = 1.0 - ((BL_FLOAT)y)/height;
    BL_FLOAT ny = ((BL_FLOAT)y)/height;
    
    if (nx < mSpectrogramBounds[0])
        return false;
    
    if (ny < mSpectrogramBounds[1])
        return false;
    
    if (nx > mSpectrogramBounds[2])
        return false;
    
    if (ny > mSpectrogramBounds[3])
        return false;
    
    return true;
}

void
SpectrogramDisplay3::GetSpectroNormCoordinate(int x, int y, int width, int height,
                                              BL_FLOAT *nx, BL_FLOAT *ny)
{
    BL_FLOAT nx0 = ((BL_FLOAT)x)/width;
    BL_FLOAT ny0 = ((BL_FLOAT)y)/height;
    
    *nx = (nx0 - mSpectrogramBounds[0])/
    (mSpectrogramBounds[2] - mSpectrogramBounds[0]);
    
    *ny = (ny0 - mSpectrogramBounds[1])/
    (mSpectrogramBounds[3] - mSpectrogramBounds[1]);
}

void
SpectrogramDisplay3::SetBounds(BL_FLOAT left, BL_FLOAT top,
                               BL_FLOAT right, BL_FLOAT bottom)
{
    mSpectrogramBounds[0] = left;
    mSpectrogramBounds[1] = top;
    mSpectrogramBounds[2] = right;
    mSpectrogramBounds[3] = bottom;
}

void
SpectrogramDisplay3::SetSpectrogram(BLSpectrogram4 *spectro)
{
    mSpectrogram = spectro;
    
    mShowSpectrogram = true;
    
    // Be sure to create the texture image in the right thread
    UpdateSpectrogram(true, true);
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::ShowSpectrogram(bool flag)
{
    mShowSpectrogram = flag;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::UpdateSpectrogram(bool updateData, bool updateBGData)
{
    mNeedUpdateSpectrogram = true;
    
    if (!mNeedUpdateSpectrogramData)
        mNeedUpdateSpectrogramData = updateData;
    
    if (!mNeedUpdateBGSpectrogramData)
    {
        if (updateBGData) // For debugging
            mNeedUpdateBGSpectrogramData = updateBGData;
    }
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::UpdateColormap(bool flag)
{
    mNeedUpdateSpectrogram = true;
    
    if (!mNeedUpdateColormapData)
    {
        mNeedUpdateColormapData = flag;
    }
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::ResetSpectrogramTransform()
{
    mState->mMinX = 0.0;
    mState->mMaxX = 1.0;
    
    mState->mAbsMinX = 0.0;
    mState->mAbsMaxX = 1.0;
    
    mState->mCenterPos = 0.5;
    mState->mAbsTranslation = 0.0;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::ResetSpectrogramTranslation()
{
    mState->mAbsTranslation = 0.0;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::SetSpectrogramZoom(BL_FLOAT zoomX)
{
    BL_FLOAT norm = (mState->mCenterPos - mState->mMinX)/
    (mState->mMaxX - mState->mMinX);
    
    mState->mMinX = mState->mCenterPos - norm*zoomX;
    mState->mMaxX = mState->mCenterPos + (1.0 - norm)*zoomX;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::SetSpectrogramAbsZoom(BL_FLOAT zoomX)
{
    BL_FLOAT norm = (mState->mCenterPos - mState->mAbsMinX)/
    (mState->mAbsMaxX - mState->mAbsMinX);
    
    mState->mAbsMinX = mState->mCenterPos - norm*zoomX;
    mState->mAbsMaxX = mState->mCenterPos + (1.0 - norm)*zoomX;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::SetSpectrogramCenterPos(BL_FLOAT centerPos)
{
    mState->mCenterPos = centerPos;
    
    mNeedRedraw = true;
}

bool
SpectrogramDisplay3::SetSpectrogramTranslation(BL_FLOAT tX)
{
    BL_FLOAT dX = tX - mState->mAbsTranslation;
    
    if (((mState->mAbsMinX > 1.0) && (dX > 0.0)) ||
        ((mState->mAbsMaxX < 0.0) && (dX < 0.0)))
        // Do not translate if the spectrogram gets out of view
        return false;
    
    mState->mAbsTranslation = tX;
    
    mState->mMinX += dX;
    mState->mMaxX += dX;
    
    mState->mAbsMinX += dX;
    mState->mAbsMaxX += dX;
    
    mNeedRedraw = true;
    
    return true;
}

void
SpectrogramDisplay3::GetSpectrogramVisibleNormBounds(BL_FLOAT *minX, BL_FLOAT *maxX)
{
    *minX = -mState->mAbsMinX/
    (mState->mAbsMaxX - mState->mAbsMinX);
    *maxX = 1.0 - (mState->mAbsMaxX - 1.0)/
    (mState->mAbsMaxX - mState->mAbsMinX);
}

void
SpectrogramDisplay3::SetSpectrogramVisibleNormBounds(BL_FLOAT minX, BL_FLOAT maxX)
{
    minX *= mState->mAbsMaxX - mState->mAbsMinX;
    BL_FLOAT spectroAbsMinX = -minX;
    
    maxX *= mState->mAbsMaxX - mState->mAbsMinX;
    
    BL_FLOAT spectroAbsMaxX = -maxX;
    
    BL_FLOAT trans = spectroAbsMinX - mState->mAbsMinX;
    
    mState->mAbsMinX = spectroAbsMinX;
    mState->mAbsMaxX = spectroAbsMaxX;
    
    //
    mState->mMinX += trans;
    mState->mMaxX += trans;
    
    mState->mCenterPos += trans;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::ResetSpectrogramZoomAndTrans()
{
    mState->mMinX = 0.0;
    mState->mMaxX = 1.0;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::SetDrawBGSpectrogram(bool flag)
{
    mDrawBGSpectrogram = flag;
    
    mNeedRedraw = true;
}

void
SpectrogramDisplay3::SetAlpha(BL_FLOAT alpha)
{
    mSpectrogramAlpha = alpha;
}

void
SpectrogramDisplay3::ClearBGSpectrogram()
{
    mState->mSpectroImageWidth = 0;
    mState->mSpectroImageHeight = 0;
    
    mState->mBGSpectroImageData.Resize(0);
}

#endif // IGRAPHICS_NANOVG
