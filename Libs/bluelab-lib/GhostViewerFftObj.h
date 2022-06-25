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
//  GhostViewerFftObj.h
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_GhostViewer__GhostViewerFftObj__
#define __BL_GhostViewer__GhostViewerFftObj__

#include <bl_queue.h>

#include <BLTypes.h>

#include <FftProcessObj16.h>

// From ChromaFftObj
//

// SpectrogramDisplayScroll => SpectrogramDisplayScroll3

class BLSpectrogram4;
//class SpectrogramDisplayScroll3;
class SpectrogramDisplayScroll4;
class GhostViewerFftObj : public ProcessObj
{
public:
    GhostViewerFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~GhostViewerFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay);
    
    void SetSpeedMod(int speedMod);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);

    //
    BLSpectrogram4 *mSpectrogram;
    
    SpectrogramDisplayScroll4 *mSpectroDisplay;
    
    long mLineCount;
    
    //deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    int mSpeedMod;

private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
};

#endif /* defined(__BL_GhostViewer__GhostViewerFftObj__) */
