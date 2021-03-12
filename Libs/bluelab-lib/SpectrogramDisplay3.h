//
//  SpectrogramDisplay3.h
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Ghost__SpectrogramDisplay3__
#define __BL_Ghost__SpectrogramDisplay3__

#ifdef IGRAPHICS_NANOVG

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>
#include <GraphControl12.h>

class BLSpectrogram4;
class NVGcontext;
class SpectrogramDisplay3 : public GraphCustomDrawer
{
public:
    struct SpectrogramDisplayState
    {
        // For local spectrogram
        BL_FLOAT mMinX;
        BL_FLOAT mMaxX;
        
        // For global spectrogram
        BL_FLOAT mAbsMinX;
        BL_FLOAT mAbsMaxX;
        
        BL_FLOAT mAbsTranslation;
        
        BL_FLOAT mCenterPos;

        BL_FLOAT mZoomAdjustFactor;
        
        // Background spectrogram
        int mSpectroImageWidth;
        int mSpectroImageHeight;
        WDL_TypedBuf<unsigned char> mBGSpectroImageData;
    };
    
    SpectrogramDisplay3(SpectrogramDisplayState *spectroTransform);
    virtual ~SpectrogramDisplay3();
    
    SpectrogramDisplayState *GetState();
    
    void Reset();
    
    bool NeedUpdateSpectrogram();
    bool DoUpdateSpectrogram();
    
    void PreDraw(NVGcontext *vg, int width, int height) override;
    bool IsOwnedByGraph() override { return true; }
    bool NeedRedraw() override;
    
    //
    bool PointInsideSpectrogram(int x, int y,
                                int width, int height);
    
    bool PointInsideMiniView(int x, int y,
                             int width, int height);
    
    void GetNormCoordinate(int x, int y, int width, int height,
                           BL_FLOAT *nx, BL_FLOAT *ny);
    
    // Spectrogram
    void SetBounds(BL_FLOAT left, BL_FLOAT top,
                   BL_FLOAT right, BL_FLOAT bottom);
    void SetSpectrogram(BLSpectrogram4 *spectro);
    
    void ShowSpectrogram(bool flag);
    void UpdateSpectrogram(bool updateData = true, bool updateBgData = false);

    void UpdateColormap(bool flag);
    
    // Reset all
    void ResetTransform();
    
    // Rest internal translation only
    void ResetTranslation();
    
    //
    void SetZoom(BL_FLOAT zoomX);
    void SetAbsZoom(BL_FLOAT zoomX);
    // To be set first
    void SetZoomAdjust(BL_FLOAT zoomAdjust);
    
    void SetCenterPos(BL_FLOAT centerPos);
    
    // Return true if we are in bounds
    bool SetTranslation(BL_FLOAT tX);
    
    // Allows result ouside of [0, 1]
    void GetVisibleNormBounds(BL_FLOAT *minX, BL_FLOAT *maxX);
    
    void SetVisibleNormBounds(BL_FLOAT minX, BL_FLOAT maxX);
    
    // Called after local data recomputation
    void ResetZoomAndTrans();
    
    // For optimization
    // To be able to disable background spectrogram
    // will avoid to draw the spectrogram twice with Chroma and GhostViewer
    void SetDrawBGSpectrogram(bool flag);
    
    void SetAlpha(BL_FLOAT alpha);
    
    void ClearBGSpectrogram();
    
protected:
    // NanoVG
    NVGcontext *mVg;
    
    // Spectrogram
    BLSpectrogram4 *mSpectrogram;
    BL_FLOAT mSpectrogramBounds[4];

    BL_FLOAT mZoomAdjustFactor;
    
    int mNvgSpectroImage;
    WDL_TypedBuf<unsigned char> mSpectroImageData;
    
    bool mNeedUpdateSpectrogram;
    bool mNeedUpdateSpectrogramData;
    
    int mNvgBGSpectroImage;
    bool mNeedUpdateBGSpectrogramData;
    
    SpectrogramDisplayState *mState;
    
    BL_FLOAT mSpectrogramAlpha;

    bool mNeedUpdateColormapData;
    
    //
    bool mShowSpectrogram;
    
    // Colormap
    int mNvgColormapImage;
    WDL_TypedBuf<unsigned int> mColormapImageData;
    
    // For optimization (Chroma)
    bool mDrawBGSpectrogram;
    
    bool mNeedRedraw;
};

#endif

#endif /* defined(__BL_Ghost__SpectrogramDisplay3__) */
