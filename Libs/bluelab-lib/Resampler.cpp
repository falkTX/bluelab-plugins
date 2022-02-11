//
//  Resample.cpp
//  PitchShift
//
//  Created by Apple m'a Tuer on 02/11/17.
//  Copyright (c) 2017 Apple m'a Tuer. All rights reserved.
//

#include "samplerate.h"

#include "Resampler.h"

int
Resampler::Resample(float  *data_in, float *data_out, long   input_frames, long output_frames,
                    BL_FLOAT src_ratio)
{
    SRC_DATA data;
    
    data.data_in = data_in;
    data.data_out = data_out;
    
    data.input_frames = input_frames;
    data.output_frames = output_frames;
    
    data.end_of_input = 1;
    
    data.src_ratio = src_ratio;
    
    int channels = 1;
    int res = src_simple(&data, SRC_SINC_BEST_QUALITY/*SRC_LINEAR*/, channels);
    
    if (res != 0)
        // Error;
        return -1;
    
    return 0;
}
