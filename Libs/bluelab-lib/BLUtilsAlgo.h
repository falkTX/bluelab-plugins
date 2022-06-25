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
 
#ifndef BL_UTILS_ALGO_H
#define BL_UTILS_ALGO_H

#include <BLTypes.h>

class BLUtilsAlgo
{
 public:
    // "swap" x and y, used when rotating a view
    template <typename FLOAT_TYPE>
    static void Rotate90(int width, int height, FLOAT_TYPE *x, FLOAT_TYPE *y,
                         bool flipX, bool flipY);

    template <typename FLOAT_TYPE>
    static void Rotate90Delta(int width, int height, FLOAT_TYPE *dx, FLOAT_TYPE *dy,
                              bool flipX, bool flipY);
};

#endif
