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
 
#ifndef EQHack_SmoothAvgHistogram2_h
#define EQHack_SmoothAvgHistogram2_h

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

// SmoothAvgHistogram2: from SmoothAvgHistogram
// - take the smoothness param as time in millis, and adapts to sample rate
//
// SmoothAvgHistogram3: same as SmoothAvgHistogram2 but with std::vector
class SmoothAvgHistogram3
{
public:
    SmoothAvgHistogram3(BL_FLOAT sampleRate, int size,
                        BL_FLOAT smoothTimeMs, BL_FLOAT defaultValue);
    
    virtual ~SmoothAvgHistogram3();
    
    void AddValue(int index, BL_FLOAT val);    
    void AddValues(const vector<BL_FLOAT> &values);
    
    void GetValues(vector<BL_FLOAT> *values);
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT sampleRate, const vector<BL_FLOAT> &values);
    
    void Resize(int newSize);

    void SetSmoothTimeMs(BL_FLOAT smoothTimeMs, bool reset = true);
    
protected:
    vector<BL_FLOAT> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;

    BL_FLOAT mSampleRate;
    BL_FLOAT mSmoothTimeMs;
    
    BL_FLOAT mDefaultValue;
};

#endif
