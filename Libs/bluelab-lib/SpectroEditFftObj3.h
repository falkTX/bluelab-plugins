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
//  SpectroEditFftObj3.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectroEditFftObj3__
#define __BL_Ghost__SpectroEditFftObj3__

#include <vector>
using namespace std;

#include <FftProcessObj16.h>

// From SpectroEditFftobj2
//
// Use mSamplesPos instead of mLineCount
// (count the number of samples instead of spectrogram lines)
class SpectroEditFftObj3 : public ProcessObj
{
public:
    enum Mode
    {
        BYPASS,
        PLAY,
        PLAY_RENDER,
        GEN_DATA,
        REPLACE_DATA
    };
    
    SpectroEditFftObj3(int bufferSize, int oversampling,
                       int freqRes, BL_FLOAT sampleRate);
    
    virtual ~SpectroEditFftObj3();
    
    // Set a reference to external samples
    void SetSamples(WDL_TypedBuf<BL_FLOAT> *samples);

    void SetForceMono(bool flag);
    void SetSamplesForMono(vector<WDL_TypedBuf<BL_FLOAT> > *samples);
    
    // In EDIT mode, replace the input samples buffer, provide by the app,
    // by the internal samples buffer, pointed at the mSamplesPos index
    void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 const WDL_TypedBuf<BL_FLOAT> *scBuffer) override;
    
    void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) override;
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;

    void ResetSamplesPos();
        
    void SetStep(BL_FLOAT step);
    
    void SetMode(Mode mode);
    Mode GetMode();

    // Set data selection area, in samples
    void SetDataSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    void SetSelectionEnabled(bool flag);
    bool IsSelectionEnabled();

    void RewindToStartSelection();
    
    // Normalized selection
    // x = 1 gives the last sample
    void GetNormSelection(BL_FLOAT selection[4]);
    void SetNormSelection(const BL_FLOAT selection[4]);

    void RewindToNormValue(BL_FLOAT value);
    
    bool SelectionPlayFinished();
    
    BL_FLOAT GetPlayPosition();
    
    // Get normalized pos of play selection,
    // normalization is done inside selection
    BL_FLOAT GetSelPlayPosition();
    
    // Can be use to save and restore states
    BL_FLOAT GetSamplesPos();
    void SetSamplesPos(BL_FLOAT samplePos);
    
    void GetGeneratedData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                          vector<WDL_TypedBuf<BL_FLOAT> > *phases);
    void ClearGeneratedData();
    
    void SetReplaceData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                        const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
    void ClearReplaceData();

    // Fill some parts with zeros if selection is partially out of bounds
    static void FillFromSelection(WDL_TypedBuf<BL_FLOAT> *result,
                                  const WDL_TypedBuf<BL_FLOAT> &inBuf,
                                  int selStartSamples,
                                  int selSizeSamples);

    // Set magns or phases to 0 outside of the y selection
    // NOTE: maybe it could be done internally
    void ApplyYSelection(WDL_TypedBuf<BL_FLOAT> *data);
        
protected:
    void GetData(const WDL_TypedBuf<BL_FLOAT> &currentData,
                 WDL_TypedBuf<BL_FLOAT> *data);

    // At each call, we process bs/ov samples,
    // and increment mSamplesPos by bs/ov
    BL_FLOAT mSamplesPos;
    
    WDL_TypedBuf<BL_FLOAT> *mSamples;
    
    vector<WDL_TypedBuf<BL_FLOAT> > *mSamplesForMono;
    bool mForceMono;
    
    bool mSelectionEnabled;
    BL_FLOAT mDataSelection[4];
    
    bool mSelectionPlayFinished;
    
    Mode mMode;
    
    // There are 4 magns and phases for one call to Process() if oversampling is 4
    
    // For GEN_DATA
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentMagns;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentPhases;
    
    // For REPLACE_DATA
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplaceMagns;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplacePhases;

    BL_FLOAT mStep;
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
};

#endif /* defined(__BL_Ghost__SpectroEditFftObj3__) */
