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
//  BL_BatFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Pano__BatFftObj__
#define __BL_Pano__BatFftObj__

#ifdef IGRAPHICS_NANOVG

#include "FftProcessObj16.h"

// From ChromaFftObj
//

class BLSpectrogram4;
class SpectrogramDisplay;
class HistoMaskLine2;

class BatFftObj : public MultichannelProcess
{
public:
    BatFftObj(int bufferSize, int oversampling, int freqRes,
                   BL_FLOAT sampleRate);
    
    virtual ~BatFftObj();
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay);
    
    void SetSharpness(BL_FLOAT sharpness);
    
    void SetTimeSmooth(BL_FLOAT smooth);
    
    void SetFftSmooth(BL_FLOAT smooth);
    
    void SetEnabled(bool flag);
    
protected:
    // Method1
    void ComputeCoords1(const WDL_TypedBuf<BL_FLOAT> magns[2],
                       WDL_TypedBuf<BL_FLOAT> *coords);

    void ComputeAmplitudes1(const WDL_TypedBuf<BL_FLOAT> &magns,
                           const WDL_TypedBuf<BL_FLOAT> &scMagns,
                           WDL_TypedBuf<BL_FLOAT> *amps);

    // Method2
    void ComputeCoords2(const WDL_TypedBuf<BL_FLOAT> magns[2],
                        WDL_TypedBuf<BL_FLOAT> *coords);
    
    void ComputeAmplitudes2(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &scMagns,
                            WDL_TypedBuf<BL_FLOAT> *amps);
    
    // Method3
    void ComputeCoords3(const WDL_TypedBuf<BL_FLOAT> magns[2],
                        const WDL_TypedBuf<BL_FLOAT> phases[2],
                        WDL_TypedBuf<BL_FLOAT> *coords);
    
    void ComputeAmplitudes3(const WDL_TypedBuf<BL_FLOAT> &magns,
                            //const WDL_TypedBuf<BL_FLOAT> &phases,
                            const WDL_TypedBuf<BL_FLOAT> &scMagns,
                            //const WDL_TypedBuf<BL_FLOAT> &scPhases,
                            WDL_TypedBuf<BL_FLOAT> *amps);
    
    void ApplySharpnessXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines);
    void ReverseXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines);
    
    void ApplySharpness(vector<WDL_TypedBuf<BL_FLOAT> > *lines);
    
    void ApplySharpness(WDL_TypedBuf<BL_FLOAT> *line);
    
    void TimeSmooth(vector<WDL_TypedBuf<BL_FLOAT> > *lines);

    
    BLSpectrogram4 *mSpectrogram;
    
    SpectrogramDisplay *mSpectroDisplay;
    
    long mLineCount;
    
    // Sharpness
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mSharpness;
    
    BL_FLOAT mTimeSmooth;
    vector<WDL_TypedBuf<BL_FLOAT> > mPrevLines;
    
    BL_FLOAT mFftSmooth;
    WDL_TypedBuf<BL_FLOAT> mPrevMagns[2];
    WDL_TypedBuf<BL_FLOAT> mPrevScMagns[2];
    
    //
    int mBufferSize;
    int mOverlapping;
    int mFreqRes;
    //int mSampleRate;
    BL_FLOAT mSampleRate;
    
    // For FIX_BAD_DISPLAY_HIGH_SAMPLERATES
    int mAddLineCount;
    
    bool mIsEnabled;
};

#endif

#endif /* defined(__BL_BL_Pano__BL_BatFftObj__) */
