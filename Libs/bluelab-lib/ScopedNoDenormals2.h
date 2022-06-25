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
//  ScopedNoDenormals2.h
//  UST
//
//  Created by applematuer on 8/17/20.
//
//

#ifndef UST_ScopedNoDenormals2_h
#define UST_ScopedNoDenormals2_h

#include "bl_config.h"

#if BL_FIX_FLT_DENORMAL_OBJ

#define BL_USE_SSE_INTRINSICS 1 //0 //1
#define BL_INTEL 1 // Must also be set

#include <xmmintrin.h>

#ifdef WIN32
#include <cstdint>
#endif

// From Juce...
class ScopedNoDenormals2
{
public:
    ScopedNoDenormals2() /*noexcept*/
    {
#if BL_USE_SSE_INTRINSICS || (BL_USE_ARM_NEON || defined (__arm64__) || defined (__aarch64__))
#if BL_USE_SSE_INTRINSICS
        intptr_t mask = 0x8040;
#else /*BL_USE_ARM_NEON*/
        intptr_t mask = (1 << 24 /* FZ */);
#endif

        fpsr = GetFpStatusRegister();
        SetFpStatusRegister (fpsr | mask);
#endif
    }
    
    virtual ~ScopedNoDenormals2() /*noexcept*/
    {
#if BL_USE_SSE_INTRINSICS || (BL_USE_ARM_NEON || defined (__arm64__) || defined (__aarch64__))
        SetFpStatusRegister (fpsr);
#endif
    }
    
protected:
    intptr_t /*BL_CALLTYPE*/ GetFpStatusRegister() /*noexcept*/
    {
        intptr_t fpsr = 0;
#if BL_INTEL && BL_USE_SSE_INTRINSICS
        fpsr = static_cast<intptr_t> (_mm_getcsr());
#elif defined (__arm64__) || defined (__aarch64__) || BL_USE_ARM_NEON
#if defined (__arm64__) || defined (__aarch64__)
        asm volatile("mrs %0, fpcr" : "=r" (fpsr));
#elif BL_USE_ARM_NEON
        asm volatile("vmrs %0, fpscr" : "=r" (fpsr));
#endif
#else
#if ! (defined (BL_INTEL) || defined (BL_ARM))
        //jassertfalse; // No support for getting the floating point status register for your platform
#endif
#endif
        
        return fpsr;
    }
    
    void /*BL_CALLTYPE*/ SetFpStatusRegister (intptr_t fpsr) /*noexcept*/
    {
#if BL_INTEL && BL_USE_SSE_INTRINSICS
        auto fpsr_w = static_cast<uint32_t> (fpsr);
        _mm_setcsr (fpsr_w);
#elif defined (__arm64__) || defined (__aarch64__) || BL_USE_ARM_NEON
#if defined (__arm64__) || defined (__aarch64__)
        asm volatile("msr fpcr, %0" : : "ri" (fpsr));
#elif BL_USE_ARM_NEON
        asm volatile("vmsr fpscr, %0" : : "ri" (fpsr));
#endif
#else
#if ! (defined (BL_INTEL) || defined (BL_ARM))
        //jassertfalse; // No support for getting the floating point status register for your platform
#endif
        //ignoreUnused (fpsr);
#endif
    }

    
private:
#if BL_USE_SSE_INTRINSICS || (BL_USE_ARM_NEON || defined (__arm64__) || defined (__aarch64__))
    intptr_t fpsr;
#endif
};

#endif

#endif
