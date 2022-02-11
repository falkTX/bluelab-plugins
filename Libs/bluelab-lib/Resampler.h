//
//  Resample.h
//  PitchShift
//
//  Created by Apple m'a Tuer on 02/11/17.
//  Copyright (c) 2017 Apple m'a Tuer. All rights reserved.
//

#ifndef __PitchShift__Resampler__
#define __PitchShift__Resampler__

#include <BLTypes.h>

// PROBLEM: only handles floats, not BL_FLOATs...
class Resampler
{
public:
    static int Resample(float  *data_in, float *data_out,
                        long   input_frames, long output_frames,
                        BL_FLOAT src_ratio);
};

#endif /* defined(__PitchShift__Resampler__) */
