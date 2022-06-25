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
//  SpectroEditFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectroEditFftObj__
#define __BL_Ghost__SpectroEditFftObj__

#include <vector>
using namespace std;

#include "FftProcessObj16.h"

class SpectroEditFftObj : public ProcessObj
{
public:
    enum Mode
    {
        BYPASS,
        RECORD,
        PLAY
    };
    
    SpectroEditFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~SpectroEditFftObj();
    
    void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) override;
    
    void Reset(int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void ClearData();
    
    void SetMode(Mode mode);
    
    Mode GetMode();

    void SetDataSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    void SetSelectionEnabled(bool flag);
    
    bool IsSelectionEnabled();

    void GetNormSelection(BL_FLOAT selection[4]);
    
    void SetNormSelection(const BL_FLOAT selection[4]);
    
    
    void RewindToStartSelection();

    void RewindToNormValue(BL_FLOAT value);
    
    bool SelectionPlayFinished();
    
    BL_FLOAT GetPlayPosition();
    
    // Get normalized pos of play selection,
    // normalization is done inside selection
    BL_FLOAT GetSelPlayPosition();
    
    // BUGGY ?
    // Get normalized pos of play selection,
    // normalization is done on visible data only
    BL_FLOAT GetViewPlayPosition(BL_FLOAT startDataPos, BL_FLOAT endDataPos);

    // Acccess to full data
    void GetFullData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                     vector<WDL_TypedBuf<BL_FLOAT> > *phases);
    
    void SetFullData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                     const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
    
    // Can be use to save and restore states
    long GetLineCount();
    void SetLineCount(long lineCount);
    
protected:
    void GetData(const vector<WDL_TypedBuf<BL_FLOAT> > &fullData,
                 WDL_TypedBuf<BL_FLOAT> *data);

    
    long mLineCount;
    
    vector<WDL_TypedBuf<BL_FLOAT> > mLines;
    
    // Keep the phases too, to be able to reconstruct the signal
    vector<WDL_TypedBuf<BL_FLOAT> > mPhases;
    
    //BL_FLOAT mSelectionEnabled;
    bool mSelectionEnabled;
    BL_FLOAT mDataSelection[4];
    
    bool mSelectionPlayFinished;
    
    Mode mMode;
};

#endif /* defined(__BL_Ghost__SpectroEditFftObj__) */
