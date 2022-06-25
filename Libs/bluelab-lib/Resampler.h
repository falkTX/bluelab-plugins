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
