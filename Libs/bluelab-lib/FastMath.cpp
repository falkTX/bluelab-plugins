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
 
#include <math.h>

// fastapprox
#include <fastexp.h>
#include <fastlog.h>
#include <fastpow.h>

#include "FastMath.h"

// Make artifact with Air when set to 1
#define USE_FASTER 0 //1

#if !USE_FASTER
#define FAST_EXP fastexp
#define FAST_LOG fastlog
#define FAST_POW fastpow
#else
#define FAST_EXP fasterexp
#define FAST_LOG fasterlog
#define FAST_POW fasterpow
#endif

#define LN10_INV 0.434294482

bool _useFastMath = false;
bool _prevUseFastMath = false;

FastMath::FastMath(bool flag)
{
    _prevUseFastMath = _useFastMath;
    _useFastMath = flag;
}

FastMath::~FastMath()
{
    _useFastMath = _prevUseFastMath;
}

void
FastMath::SetFast(bool flag)
{
    _useFastMath = flag;
    _prevUseFastMath = flag;
}

float
FastMath::log(float x)
{
    if (_useFastMath)
        return FAST_LOG(x);
    else
        return ::logf(x);
}

double
FastMath::log(double x)
{
    if (_useFastMath)
        return (double)FAST_LOG((float)x);
    else
        return ::log(x);
}

float
FastMath::log10(float x)
{
    if (_useFastMath)
        return FAST_LOG(x)*LN10_INV;
    else
        return ::log10f(x);
}

double
FastMath::log10(double x)
{
    if (_useFastMath)
        return (double)FAST_LOG((float)x)*LN10_INV;
    else
        return ::log10(x);
}

float
FastMath::exp(float x)
{
    if (_useFastMath)
        return FAST_EXP(x);
    else
        return ::expf(x);
}

double
FastMath::exp(double x)
{
    if (_useFastMath)
        return (double)FAST_EXP((float)x);
    else
        return ::exp(x);
}

float
FastMath::pow(float x, float p)
{
    if (_useFastMath)
        return FAST_POW(x, p);
    else
        return ::powf(x, p);
}

double
FastMath::pow(double x, double p)
{
    if (_useFastMath)
        return (double)FAST_POW((float)x, (float)p);
    else
        return ::pow(x, p);
}
