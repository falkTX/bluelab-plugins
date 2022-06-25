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
//  SinLUT2.h
//  BL-SASViewer
//
//  Created by applematuer on 3/1/19.
//
//

#ifndef BL_SASViewer_SinLUT2_h
#define BL_SASViewer_SinLUT2_h

#include <cmath>

#define SIN_LUT_CREATE(__NAME__, __SIZE__)                              \
static BL_FLOAT __NAME__[__SIZE__];                                       \
static long __NAME__##_SIZE = __SIZE__;                                 \
static BL_FLOAT __NAME__##__TWOPI_INV__ = 1.0/(2.0*M_PI);                 \

#define SIN_LUT_INIT(__NAME__)                                          \
BL_FLOAT __COEFF__ = 2.0*M_PI/__NAME__##_SIZE;                            \
for (int __I__ = 0; __I__ < __NAME__##_SIZE; __I__++)                   \
  __NAME__[__I__] = std::sin(__I__*__COEFF__);

// OPTIM PROF Infra
// (avoid costly fmod)
#if 0 // ORIGINAL

#define SIN_LUT_GET(__NAME__, __VAR__, __ANGLE__)                       \
  __ANGLE__ = std::fmod(__ANGLE__, 2.0*M_PI);				\
int __INDEX__ = (__ANGLE__*__NAME__##__TWOPI_INV__)*__NAME__##_SIZE;    \
__VAR__ = __NAME__[__INDEX__];

#else // OPTIMIZED

#define SIN_LUT_GET(__NAME__, __VAR__, __ANGLE__)                       \
int __INDEX__ = (__ANGLE__*__NAME__##__TWOPI_INV__)*__NAME__##_SIZE;    \
while(__INDEX__ < 0)                                                    \
__INDEX__ += __NAME__##_SIZE;                                           \
__INDEX__ = __INDEX__%__NAME__##_SIZE;                                  \
__VAR__ = __NAME__[__INDEX__];

#endif

#endif
