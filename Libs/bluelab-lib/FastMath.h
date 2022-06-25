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
 
#ifndef FAST_MATH_H
#define FAST_MATH_H

class FastMath
{
 public:
    // Create and object to enable or disable fast math in the current scope
    FastMath(bool flag);
    virtual ~FastMath();
    
    static void SetFast(bool flag);

    static float log(float x);
    static double log(double x);

    static float log10(float x);
    static double log10(double x);
    
    static float exp(float x);
    static double exp(double x);

    static float pow(float x, float p);
    static double pow(double x, double p);
};

#endif
