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
 
#ifndef PITCH_SHIFTER_BL_SMB_H
#define PITCH_SHIFTER_BL_SMB_H

#include <vector>
using namespace std;

#include <PitchShifterInterface.h>

// Original code
// Moved from plugin main PitchShift.cpp to an external class
class PostTransientFftObj3;
class PitchShiftFftObj3;
class FftProcessObj16;
class StereoPhasesProcess;
class PitchShifterBLSmb : public PitchShifterInterface
{
 public:
    PitchShifterBLSmb();
    virtual ~PitchShifterBLSmb();

    void Reset(BL_FLOAT sampleRate, int blockSize) override;

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out) override;
    
    void SetNumChannels(int nchans) override {}
    void SetFactor(BL_FLOAT factor) override;
    // 0, 1, 2 or 3
    void SetQuality(int quality) override;

    void SetTransBoost(BL_FLOAT transBoost) override;

    int ComputeLatency(int blockSize) override;
    
protected:
    void InitFft(BL_FLOAT sampleRate);

    //
    BL_FLOAT mSampleRate;
    int mOversampling;

    // Shift factor
    BL_FLOAT mFactor;
    
    // FIX: was WDL_TypedBuf
    //vector<PitchShiftFftObj3 *> mPitchObjs;
    PitchShiftFftObj3 *mPitchObjs[2];
    
#if DEBUG_PITCH_OBJ
    FftProcessObj16 *mFftObj;
#else
    PostTransientFftObj3 *mFftObj;
#endif
    
    StereoPhasesProcess *mPhasesProcess;
};

#endif
