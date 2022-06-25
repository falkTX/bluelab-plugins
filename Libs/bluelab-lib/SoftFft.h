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
//  SoftFft.h
//  BL-PitchShift
//
//  Created by Pan on 03/04/18.
//
//

#ifndef __BL_PitchShift__SoftFft__
#define __BL_PitchShift__SoftFft__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

class SoftFft
{
public:
    static void Ifft(const WDL_TypedBuf<BL_FLOAT> &magns,
                     const WDL_TypedBuf<BL_FLOAT> &freqs,
                     const WDL_TypedBuf<BL_FLOAT> &phases,
                     BL_FLOAT sampleRate,
                     WDL_TypedBuf<BL_FLOAT> *outSamples);
};

#endif /* defined(__BL_PitchShift__SoftFft__) */
