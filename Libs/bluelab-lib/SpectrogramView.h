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
//  SpectrogramView.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectrogramView__
#define __BL_Ghost__SpectrogramView__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class BLSpectrogram4;
class FftProcessObj16;
class SpectrogramView
{
public:
    SpectrogramView(BLSpectrogram4 *spectrogram,
                    FftProcessObj16 *fftObj,
                    int maxNumCols,
                    BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1,
                    BL_FLOAT sampleRate);
    
    virtual ~SpectrogramView();
    
    void Reset();

    // UNUSED
    void Resize(int prevWidth, int prevHeight,
                int newWidth, int newHeight);
    
    void SetData(vector<WDL_TypedBuf<BL_FLOAT> > *channels);
    
    void SetViewBarPosition(BL_FLOAT pos);
    
    void SetViewSelection(BL_FLOAT x0, BL_FLOAT y0,
                          BL_FLOAT x1, BL_FLOAT y1);
    
    void GetViewSelection(BL_FLOAT *x0, BL_FLOAT *y0,
                          BL_FLOAT *x1, BL_FLOAT *y1);
    
    void ViewToDataRef(BL_FLOAT *x0, BL_FLOAT *y0,
                       BL_FLOAT *x1, BL_FLOAT *y1);

    
    void DataToViewRef(BL_FLOAT *x0, BL_FLOAT *y0,
                       BL_FLOAT *x1, BL_FLOAT *y1);

    
    void ClearViewSelection();
    
    void GetViewDataBounds(BL_FLOAT *startDataPos, BL_FLOAT *endDataPos);
    
    // startDataPos and endDataPos are BL_FLOAT,
    // because we get the data bounds for spectrogram columns
    //
    // If we want it in samples, we must mult by BUFFER_SIZE
    // And to get precise sample data pos, we use BL_FLOAT here
    void GetViewDataBounds2(BL_FLOAT *startDataPos, BL_FLOAT *endDataPos,
                            BL_FLOAT minNormX, BL_FLOAT maxNormX);
    
    bool GetDataSelection(BL_FLOAT *x0, BL_FLOAT *y0, BL_FLOAT *x1, BL_FLOAT *y1);
    
    // Get the data selection, before the data has been recomputed
    bool GetDataSelection2(BL_FLOAT *x0, BL_FLOAT *y0, BL_FLOAT *x1, BL_FLOAT *y1,
                           BL_FLOAT minNormX, BL_FLOAT maxNormX);
    
    void SetDataSelection2(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1,
                           BL_FLOAT minNormX, BL_FLOAT maxNormX);
    
    bool GetNormDataSelection(BL_FLOAT *x0, BL_FLOAT *y0, BL_FLOAT *x1, BL_FLOAT *y1);
    
    // Return true if the zoom had actually be done
    // (otherwise, we can have reached the zoom limits)
    bool UpdateZoomFactor(BL_FLOAT zoomChange);
    BL_FLOAT GetZoomFactor();
    BL_FLOAT GetAbsZoomFactor();
    
    // Between 0 and 1 => for zoom between min and max
    BL_FLOAT GetNormZoom();
    
    void Translate(BL_FLOAT tX);
    
    BL_FLOAT GetTranslation();
    
    //
    void UpdateSpectrogramData(BL_FLOAT minNormX, BL_FLOAT maxNormX);
    
    void SetSampleRate(BL_FLOAT sampleRate);
    
protected:
    BLSpectrogram4 *mSpectrogram;
    int mMaxNumCols;
    
    FftProcessObj16 *mFftObj;
    
    // Use a pointer, to avoid duplicating the file data,
    // to save memory on big files
    vector<WDL_TypedBuf<BL_FLOAT> > *mChannels;
    
    // Use BL_FLOAT => avoid small translations due to rounging
    BL_FLOAT mViewBarPos;
    BL_FLOAT mStartDataPos;
    BL_FLOAT mEndDataPos;
    
    BL_FLOAT mZoomFactor;
    
    // Total zoom factor
    BL_FLOAT mAbsZoomFactor;
    
    // Total translation
    BL_FLOAT mTranslation;
    
    // View selection
    bool mSelectionActive;
    BL_FLOAT mSelection[4];
    
    // Bounds
    BL_FLOAT mBounds[4];
    
    // Just for resampling
    BL_FLOAT mSampleRate;
};

#endif

#endif /* defined(__BL_Ghost__SpectrogramView__) */
