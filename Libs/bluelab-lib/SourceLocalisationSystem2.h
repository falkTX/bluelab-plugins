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
//  SourceLocalisationSystem2.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__SourceLocalisationSystem2__
#define __BL_Bat__SourceLocalisationSystem2__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// NOTE (SourceLocalisationSystem): This looks not very correct to implement delay
// on Fft samples using standard DelayLine...
//
// SourceLocalisationSystem2: use fft phase shift
//
class DelayLinePhaseShift;

class SourceLocalisationSystem2
{
public:
    SourceLocalisationSystem2(int bufferSize, BL_FLOAT sampleRate,
                             int numBands);
    
    virtual ~SourceLocalisationSystem2();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2]);
    
    void GetLocalization(WDL_TypedBuf<BL_FLOAT> *localization);
    
protected:
    void Init();
    
    void ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                             vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    void ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *diffSamps,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf0,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf1,
                     int index);

    
    void FindMinima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    void FindMinima2(WDL_TypedBuf<BL_FLOAT> *coincidence);
    void FindMinima3(WDL_TypedBuf<BL_FLOAT> *coincidence);
    //void FindMaxima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    void TimeIntegrate(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence);
    
    void Threshold(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence);

    void FreqIntegrate(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                       WDL_TypedBuf<BL_FLOAT> *localization);

    void FreqIntegrateStencil(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                              WDL_TypedBuf<BL_FLOAT> *localization);
    
    void InvertValues(WDL_TypedBuf<BL_FLOAT> *localization);

    
    void DeleteDelayLines();
    
    void DBG_DumpCoincidence(const char *fileName,
                             const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                             BL_FLOAT colorCoeff);
    
    void DBG_DumpCoincidenceLine(const char *fileName,
                                 int index,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence);

    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mNumBands;
    
    vector<vector<DelayLinePhaseShift *> > mDelayLines[2];
    
    WDL_TypedBuf<BL_FLOAT> mCurrentLocalization;
    
    // For time integrate
    vector<WDL_TypedBuf<BL_FLOAT> > mPrevCoincidence;
};

#endif /* defined(__BL_Bat__SourceLocalisationSystem2__) */
