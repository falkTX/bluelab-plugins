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
//  SineSynth.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SineSynth__
#define __BL_SASViewer__SineSynth__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

// SineSynth: from SASFrame
//
class WavetableSynth;
class SineSynth
{
public:
    class Partial
    {
    public:
        Partial();
        
        Partial(const Partial &other);
        
        virtual ~Partial();

    public:
        long mId;
        
        BL_FLOAT mFreq;
        BL_FLOAT mAmpDB;
        BL_FLOAT mPhase;
    };
    
    SineSynth(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    virtual ~SineSynth();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    void SetPartials(const vector<PartialTracker3::Partial> &partials);
    
    void ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
protected:
#if 0
    void ComputeSamplesWin(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeSamplesPartials(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // Optim
    void ComputeSamplesSAS5(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeSamplesSASTable(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesSASTable2(WDL_TypedBuf<BL_FLOAT> *samples); // Better interpolation
    void ComputeSamplesSASOverlap(WDL_TypedBuf<BL_FLOAT> *samples);
#endif
    
    // Compute steps
    //

#if 0
    // Versions to interpolate over time
    bool FindPartial(BL_FLOAT freq);

    void GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t);
#endif
    
    int FindPrevPartialIdx(int currentPartialIdx);

    int FindPartialFromIdx(int partialIdx);

    
#if 0
    void GetPartial(Partial *result, int index, BL_FLOAT t);
#endif
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    vector<Partial> mPartials;
    vector<Partial> mPrevPartials;
    
    // For sample synth with table
    WavetableSynth *mTableSynth;
};

#endif /* defined(__BL_SASViewer__SineSynth__) */
