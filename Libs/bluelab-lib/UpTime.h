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
//  UpTime.h
//  BL-PitchShift
//
//  Created by Pan on 30/07/18.
//
//

#ifndef __BL_PitchShift__UpTime__
#define __BL_PitchShift__UpTime__

// Niko
class UpTime
{
public:
    static unsigned long long GetUpTime();

    // Use double, not float
    // double has a precision of 16 decimals (float has only 7)
    // With double, we can store 1M seconds, with a precision on 1 ns
    static double GetUpTimeF();
};

#endif /* defined(__BL_PitchShift__UpTime__) */
