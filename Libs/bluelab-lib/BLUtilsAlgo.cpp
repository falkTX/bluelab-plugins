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
 
#include "BLUtilsAlgo.h"

template <typename FLOAT_TYPE>
void
BLUtilsAlgo::Rotate90(int width, int height, FLOAT_TYPE *x, FLOAT_TYPE *y,
                      bool flipX, bool flipY)
{
    *x = *x / width;
    *y = *y / height;
    
    FLOAT_TYPE tmp = *x;
    *x = *y;
    *y = tmp;
    
    *x *= width;
    *y *= height;
    
    if (flipX)
        *x = width - *x;

    if (flipY)
        *y = height - *y;
}
template void BLUtilsAlgo::Rotate90(int width, int height,
                                    float *x, float *y,
                                    bool flipX, bool flipY);
template void BLUtilsAlgo::Rotate90(int width, int height,
                                    double *x, double *y,
                                    bool flipX, bool flipY);
    
template <typename FLOAT_TYPE>
void
BLUtilsAlgo::Rotate90Delta(int width, int height, FLOAT_TYPE *dx, FLOAT_TYPE *dy,
                           bool flipX, bool flipY)
{
    *dx = *dx / width;
    *dy = *dy / height;
    
    FLOAT_TYPE tmp = *dx;
    *dx = *dy;
    *dy = tmp;
    
    *dx *= width;
    *dy *= height;
    
    if (flipX)
        *dx = -(*dx);

    if (flipY)
        *dy = -(*dy);
}
template void BLUtilsAlgo::Rotate90Delta(int width, int height,
                                         float *dx, float *dy,
                                         bool flipX, bool flipY);
template void BLUtilsAlgo::Rotate90Delta(int width, int height,
                                         double *dx, double *dy,
                                         bool flipX, bool flipY);
