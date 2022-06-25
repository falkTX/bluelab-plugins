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
//  SpectroExpeFftObj.h
//  BL-SpectroExpe
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_SpectroExpe__SpectroExpeFftObj__
#define __BL_SpectroExpe__SpectroExpeFftObj__

#include <BLTypes.h>

#include <FftProcessObj16.h>

// From ChromaFftObj
//

// SpectrogramDisplayScroll => SpectrogramDisplayScroll3
// SpectroExpeFftObj: From GhostViewerFftObj
class BLSpectrogram4;
class SpectrogramDisplayScroll3;
class PanogramFftObj;
class ChromaFftObj2;
class SpectroExpeFftObj : public MultichannelProcess
{
public:
    enum Mode
    {
        SPECTROGRAM,
        PANOGRAM,
        PANOGRAM_FREQ,
        CHROMAGRAM,
        CHROMAGRAM_FREQ,
	STEREO_WIDTH,
	DUET_MAGNS,
	DUET_PHASES
    };
    
    SpectroExpeFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~SpectroExpeFftObj();

    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    BLSpectrogram4 *GetSpectrogram();
    
    void SetSpectrogramDisplay(SpectrogramDisplayScroll3 *spectroDisplay);
    
    void SetSpeedMod(int speedMod);
    
    void SetMode(Mode mode);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);

    void ComputePanoFreqLine(const WDL_TypedBuf<BL_FLOAT> magns[2],
                             WDL_TypedBuf<BL_FLOAT> *panoFreqLine);

    void ComputeChromaFreqLine(const WDL_TypedBuf<BL_FLOAT> magns[2],
                               const WDL_TypedBuf<BL_FLOAT> phases[2],
                               WDL_TypedBuf<BL_FLOAT> *chromaFreqLine);
    
    void ComputeDuetMagns(WDL_TypedBuf<BL_FLOAT> magns[2],
			  WDL_TypedBuf<BL_FLOAT> *duetMagnsLine);
    
    void ComputeDuetPhases(WDL_TypedBuf<BL_FLOAT> phases[2],
			   WDL_TypedBuf<BL_FLOAT> *duetPhasesLine);

    //
    BL_FLOAT mSampleRate;
    int mBufferSize;
    
    BLSpectrogram4 *mSpectrogram;
    SpectrogramDisplayScroll3 *mSpectroDisplay;
    
    long mLineCount;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mOverlapLines;
    
    int mSpeedMod;
    
    Mode mMode;

    PanogramFftObj *mPanogramObj;
    ChromaFftObj2 *mChromaObj;
};

#endif /* defined(__BL_SpectroExpe__SpectroExpeFftObj__) */
