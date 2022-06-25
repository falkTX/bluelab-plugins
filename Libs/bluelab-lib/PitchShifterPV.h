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
 
#ifndef PITCH_SHIFTER_PV_H
#define PITCH_SHIFTER_PV_H

#include <vector>
using namespace std;

#include <../PhaseVocoder-DSP/PitchShifter.h>
//#include <PhaseVocoder-DSP/PeakShifter.h> // Was a test

class PitchShiftPVFftObj;
class PitchShifterPV
{
 public:
    PitchShifterPV();
    virtual ~PitchShifterPV();

    void Reset(BL_FLOAT sampleRate, int blockSize);

    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                 vector<WDL_TypedBuf<BL_FLOAT> > *out);

    void SetFactor(BL_FLOAT factor);

    int ComputeLatency(int blockSize);
    
    void SetNumChannels(int nchans) {}

    // 0, 1, 2 or 3
    void SetQuality(int quality) {}

    void SetTransBoost(BL_FLOAT transBoost) {}
    
protected:
    stekyne::PitchShifter<BL_FLOAT> *mPitchObjs[2];
    //stekyne::PeakShifter<BL_FLOAT> *mPitchObjs[2];
};

#endif
