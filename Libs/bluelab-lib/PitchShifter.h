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
 
#ifndef PITCH_SHIFTER_H
#define PITCH_SHIFTER_H

#include <vector>
using namespace std;

// Which version of WDL pitch shifter ? (version 2 may not be finiehed)
#define USE_WDL_PITCHSHIFTER2 0 //1

#if USE_WDL_PITCHSHIFTER2
#define WDL_PITCHSHIFTER WDL_SimplePitchShifter2
#else
#define WDL_PITCHSHIFTER WDL_SimplePitchShifter
#endif

class WDL_PITCHSHIFTER;
class PitchShiftOversampObj;
class PitchShifter
{
 public:
    PitchShifter();
    virtual ~PitchShifter();

    void Reset(BL_FLOAT sampleRate, int blockSize);

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out);
    
    void SetNumChannels(int nchans);
    void SetFactor(BL_FLOAT factor);
    // 0, 1, 2 or 3
    void SetQuality(int quality);

    void SetTransBoost(BL_FLOAT transBoost) {}
    int ComputeLatency(int blockSize) { return 0; }
    
 protected:
    friend class PitchShiftOversampObj;
    void ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                        vector<WDL_TypedBuf<BL_FLOAT> > *out);
    
    // WDL
    WDL_PITCHSHIFTER *mWDLPitchShifter;
    int mNumChannels;
    WDL_TypedBuf<BL_FLOAT> mCurrentOutput;

    PitchShiftOversampObj *mOversampObj;

    BL_FLOAT mSampleRate;
    int mOversampling;
};

#endif
