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
//  BL_DUETFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_DUET__DUETFftObj__
#define __BL_DUET__DUETFftObj__

#ifdef IGRAPHICS_NANOVG

#include "FftProcessObj16.h"

// From BatFftObj5 (directly)
//
class BLSpectrogram4;
class SpectrogramDisplay;
//class HistoMaskLine2;
class ImageDisplay;

class DUETSeparator;

class DUETFftObj : public MultichannelProcess
{
public:
    DUETFftObj(GraphControl11 *graph,
               int bufferSize, int oversampling, int freqRes,
               BL_FLOAT sampleRate);
    
    virtual ~DUETFftObj();
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay);
    
    void SetTimeSmooth(BL_FLOAT smoothFactor);
    
    //
    void SetThresholdFloor(BL_FLOAT threshold);
    void SetThresholdPeaks(BL_FLOAT threshold);
    void SetThresholdPeaksWidth(BL_FLOAT threshold);
    
    void SetDisplayThresholded(bool flag);
    void SetDisplayMaxima(bool flag);
    void SetDisplayMasks(bool flag);
    
    void SetUseSoftMasks(bool flag);
    void SetUseSoftMasksComp(bool flag);
    
    void SetSoftMaskSize(int size);
    
    //
    void SetKernelSmooth(bool kernelSmoothFlag);
    
    void SetExtractMax(bool extractMaxFlag);
    
    //
    void SetUseDuetSoftMasks(bool flag);
    void SetThresholdAll(bool flag);
    
    //
    void SetImageDisplay(ImageDisplay *imageDisplay);
    
    //
    void SetPickingActive(bool flag);
    void SetPickPosition(BL_FLOAT x, BL_FLOAT y);
    void SetPickModeBg(bool flag);
    
protected:
    void Process();
    
    //
    BLSpectrogram4 *mSpectrogram;
    
    SpectrogramDisplay *mSpectroDisplay;
    
    long mLineCount;
    
    //
    //BL_FLOAT mThreshold;
    
    //
    int mBufferSize;
    int mOverlapping;
    int mFreqRes;
    int mSampleRate;
    
    // For histogram
    ImageDisplay *mImageDisplay;
    
    //
    DUETSeparator *mSeparator;
    
    GraphControl11 *mGraph;
    
    bool mUseSoftMasksComp;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_BL_DUET__BL_DUETFftObj__) */
