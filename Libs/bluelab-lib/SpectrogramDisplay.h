/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  SpectrogramDisplay.h
//  BL-Ghost
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Ghost__SpectrogramDisplay__
#define __BL_Ghost__SpectrogramDisplay__

#include "IPlug_include_in_plug_hdr.h"

#include "resource.h"

#include <BLTypes.h>

class BLSpectrogram4;
class MiniView;
struct /*class*/ NVGcontext;

class SpectrogramDisplay
{
public:
    SpectrogramDisplay(NVGcontext *vg);
    
    virtual ~SpectrogramDisplay();
    
    void SetNvgContext(NVGcontext *vg);
    void ResetGfx();
    void RefreshGfx();
    
    void Reset();
    
    bool NeedUpdateSpectrogram();
    bool DoUpdateSpectrogram();
    
    void DrawSpectrogram(int width, int height);
    
    void DrawMiniView(int width, int height);

    bool PointInsideSpectrogram(int x, int y,
                                int width, int height);
    
    bool PointInsideMiniView(int x, int y,
                             int width, int height);
    
    void GetSpectroNormCoordinate(int x, int y, int width, int height,
                                  BL_FLOAT *nx, BL_FLOAT *ny);
    
    // Spectrogram
    void SetSpectrogram(BLSpectrogram4 *spectro,
                        BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom);
    
    void ShowSpectrogram(bool flag);
    
    void UpdateSpectrogram(bool updateData = true, bool updateFullData = false);
    
    // Reset all
    void ResetSpectrogramTransform();
    
    // Rest internal translation only
    void ResetSpectrogramTranslation();
    
    //
    void SetSpectrogramZoom(BL_FLOAT zoomX);
    //BL_FLOAT GetSpectrogramZoom();
    
    void SetSpectrogramAbsZoom(BL_FLOAT zoomX);
    
    void SetSpectrogramCenterPos(BL_FLOAT centerPos);
    
    // Return true if we are in bounds
    bool SetSpectrogramTranslation(BL_FLOAT tX);
    
    // Clamps
    void GetSpectrogramVisibleNormBounds(BL_FLOAT *minX, BL_FLOAT *maxX);
    
    // Allows result ouside of [0, 1]
    void GetSpectrogramVisibleNormBounds2(BL_FLOAT *minX, BL_FLOAT *maxX);
    
    void SetSpectrogramVisibleNormBounds2(BL_FLOAT minX, BL_FLOAT maxX);
    
    // Called after local data recomputation
    void ResetSpectrogramZoomAndTrans();
    
    void SetSpectrogramAlpha(BL_FLOAT alpha);

    // MiniView
    void CreateMiniView(int maxNumPoints,
                        BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    MiniView *GetMiniView();
    
    // For optimization
    // To be able to disable background spectrogram
    // will avoid to draw the spectrogram twice with Chroma and GhostViewer
    void SetDrawBGSpectrogram(bool flag);
    
protected:
    // NanoVG
    NVGcontext *mVg;
    
    // Spectrogram
    BLSpectrogram4 *mSpectrogram;
    BL_FLOAT mSpectrogramBounds[4];
    
    int mNvgSpectroImage;
    WDL_TypedBuf<unsigned char> mSpectroImageData;
    
    bool mNeedUpdateSpectrogram;
    bool mNeedUpdateSpectrogramData;
    
    int mNvgSpectroFullImage;
    bool mNeedUpdateSpectrogramFullData;
    
    // For local spectrogram
    BL_FLOAT mSpectroMinX;
    BL_FLOAT mSpectroMaxX;
    
    // For global spectrogram
    BL_FLOAT mSpectroAbsMinX;
    BL_FLOAT mSpectroAbsMaxX;
    
    BL_FLOAT mSpectroAbsTranslation;
    
    BL_FLOAT mSpectrogramAlpha;
    
    //
    bool mShowSpectrogram;
    
    BL_FLOAT mSpectroCenterPos;
    
    BL_FLOAT mSpectrogramGain;
    
    // Colormap
    int mNvgColormapImage;
    WDL_TypedBuf<unsigned int> mColormapImageData;
    
    // Mini view
    MiniView *mMiniView;
    
    // For optimization (Chroma)
    bool mDrawBGSpectrogram;
};

#endif /* defined(__BL_Ghost__SpectrogramDisplay__) */
