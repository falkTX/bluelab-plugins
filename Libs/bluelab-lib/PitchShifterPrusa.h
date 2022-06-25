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
 
#ifndef PITCH_SHIFTER_PRUSA_H
#define PITCH_SHIFTER_PRUSA_H

#include <vector>
using namespace std;

#include <PitchShifterInterface.h>

// Implementation of:
// "Phase Vocoder Done Right" by Zdenek Prusa and Nicki Holighaus.
// with improvements from:
// "Pitch-shifting algorithm design and applications in music" by THÉO ROYER
// (fix for high freqs using oversampling + pre-echo reduction for drums)
class PitchShiftPrusaFftObj;
class FftProcessObj16;
class StereoPhasesProcess;
class PitchShifterPrusa : public PitchShifterInterface
{
 public:
    PitchShifterPrusa();
    virtual ~PitchShifterPrusa();

    void Reset(BL_FLOAT sampleRate, int blockSize) override;

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out) override;
    
    void SetNumChannels(int nchans) override {}
    void SetFactor(BL_FLOAT factor) override;
    // Set the buffer size: 0 or 1
    void SetQuality(int quality) override;

    void SetTransBoost(BL_FLOAT transBoost) override {}

    int ComputeLatency(int blockSize) override;
    
protected:
    void InitFft(BL_FLOAT sampleRate);

    void UpdateQuality();
    
    //
    BL_FLOAT mSampleRate;
    // Not used anymore
    // It smeared traisients when increasing it
    // Change buffer size instead, to get better defined frequancies
    int mOversampling;

    // 2048 -> 4096: makes better defined frequencies
    int mBufferSize;
    
    // Shift factor
    BL_FLOAT mFactor;
    
    PitchShiftPrusaFftObj *mPitchObjs[2];
    
    FftProcessObj16 *mFftObj;
    
    StereoPhasesProcess *mPhasesProcess;

    int mQuality;
};

#endif
