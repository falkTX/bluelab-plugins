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
 
#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "BLUtilsPhases.h"

#define USE_SIMD_OPTIM 1

// See: https://gist.github.com/arrai/451426
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsPhases::fmod_negative(FLOAT_TYPE x, FLOAT_TYPE y)
{
    // Move input to range 0.. 2*pi
    if (x < 0.0)
    {
        // fmod only supports positive numbers. Thus we have
        // to emulate negative numbers
        FLOAT_TYPE modulus = x * -1.0;
        modulus = std::fmod(modulus, y);
        modulus = -modulus + y;
        
        return modulus;
    }
    return std::fmod(x, y);
}
template float BLUtilsPhases::fmod_negative(float x, float y);
template double BLUtilsPhases::fmod_negative(double x, double y);

// See: http://kth.diva-portal.org/smash/get/diva2:1381398/FULLTEXT01.pdf
// and: http://ltfat.github.io/notes/ltfatnote050.pdf
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsPhases::princarg(FLOAT_TYPE x)
{
    FLOAT_TYPE result = BLUtilsPhases::fmod_negative(x + M_PI, TWO_PI) - M_PI;

    return result;
}
template float BLUtilsPhases::princarg(float x);
template double BLUtilsPhases::princarg(double x);

template <typename FLOAT_TYPE>
void
BLUtilsPhases::FindNextPhase(FLOAT_TYPE *phase, FLOAT_TYPE refPhase)
{
#if 0
    while(*phase < refPhase)
        *phase += 2.0*M_PI;
#endif
    
#if 0 // Optim (does not optimize a lot)
    while(*phase < refPhase)
        *phase += TWO_PI;
#endif
    
#if 1 // Optim2: very efficient !
      // Optim for SoundMetaViewer: gain about 10%
    if (*phase >= refPhase)
        return;
    
    FLOAT_TYPE refMod = fmod_negative(refPhase, (FLOAT_TYPE)TWO_PI);
    FLOAT_TYPE pMod = fmod_negative(*phase, (FLOAT_TYPE)TWO_PI);
    
    FLOAT_TYPE resPhase = (refPhase - refMod) + pMod;
    if (resPhase < refPhase)
        resPhase += TWO_PI;
    
    *phase = resPhase;
#endif
}
template void BLUtilsPhases::FindNextPhase(float *phase, float refPhase);
template void BLUtilsPhases::FindNextPhase(double *phase, double refPhase);

#if 0 // Quite costly version !
void
BLUtilsPhases::UnwrapPhases(WDL_TypedBuf<FLOAT_TYPE> *phases)
{
    FLOAT_TYPE prevPhase = phases->Get()[0];
    
    FindNextPhase(&prevPhase, 0.0);
    
    int phasesSize = phases->GetSize();
    FLOAT_TYPE *phasesData = phases->Get();
    
    for (int i = 0; i < phasesSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];
        
        FindNextPhase(&phase, prevPhase);
        
        phasesData[i] = phase;
        
        prevPhase = phase;
    }
}
#endif

#if !USE_SIMD_OPTIM
// Optimized version
void
BLUtilsPhases::UnwrapPhases(WDL_TypedBuf<FLOAT_TYPE> *phases,
                            bool adjustFirstPhase)
{
    if (phases->GetSize() == 0)
        // Empty phases
        return;
    
    FLOAT_TYPE prevPhase = phases->Get()[0];

    if (adjustFirstPhase)
        FindNextPhase(&prevPhase, 0.0);
    
    FLOAT_TYPE sum = 0.0;
    
    int phasesSize = phases->GetSize();
    FLOAT_TYPE *phasesData = phases->Get();
    
    for (int i = 0; i < phasesSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];
        phase += sum;
        
        while(phase < prevPhase)
        {
            phase += 2.0*M_PI;
            
            sum += 2.0*M_PI;
        }
        
        phasesData[i] = phase;
        
        prevPhase = phase;
    }
}
#else
// Optimized version 2
template <typename FLOAT_TYPE>
void
BLUtilsPhases::UnwrapPhases(WDL_TypedBuf<FLOAT_TYPE> *phases,
                            bool adjustFirstPhase)
{
    if (phases->GetSize() == 0)
        // Empty phases
        return;
    
    FLOAT_TYPE prevPhase = phases->Get()[0];

    if (adjustFirstPhase)
        FindNextPhase(&prevPhase, (FLOAT_TYPE)0.0);
    
    //FLOAT_TYPE sum = 0.0;
    
    int phasesSize = phases->GetSize();
    FLOAT_TYPE *phasesData = phases->Get();
    for (int i = 0; i < phasesSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];
        
        FindNextPhase(&phase, prevPhase);
        
        phasesData[i] = phase;
        
        prevPhase = phase;
    }
}
template void BLUtilsPhases::UnwrapPhases(WDL_TypedBuf<float> *phases,
                                          bool adjustFirstPhase);
template void BLUtilsPhases::UnwrapPhases(WDL_TypedBuf<double> *phases,
                                          bool adjustFirstPhase);
#endif

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsPhases::MapToPi(FLOAT_TYPE val)
{
    /* Map delta phase into +/- Pi interval */
    val =  std::fmod(val, (FLOAT_TYPE)(2.0*M_PI));
    if (val <= -M_PI)
        val += 2.0*M_PI;
    if (val > M_PI)
        val -= 2.0*M_PI;
    
    return val;
}
template float BLUtilsPhases::MapToPi(float val);
template double BLUtilsPhases::MapToPi(double val);

template <typename FLOAT_TYPE>
void
BLUtilsPhases::MapToPi(WDL_TypedBuf<FLOAT_TYPE> *values)
{
    int valuesSize = values->GetSize();
    FLOAT_TYPE *valuesData = values->Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        FLOAT_TYPE val = valuesData[i];
        
        val = MapToPi(val);
        
        valuesData[i] = val;
    }
}
template void BLUtilsPhases::MapToPi(WDL_TypedBuf<float> *values);
template void BLUtilsPhases::MapToPi(WDL_TypedBuf<double> *values);

template <typename FLOAT_TYPE>
void
BLUtilsPhases::FindClosestPhase(FLOAT_TYPE *phase, FLOAT_TYPE refPhase)
{
    FLOAT_TYPE refMod = BLUtilsPhases::fmod_negative(refPhase, (FLOAT_TYPE)TWO_PI);
    FLOAT_TYPE pMod = BLUtilsPhases::fmod_negative(*phase, (FLOAT_TYPE)TWO_PI);
    
    // Find closest
    if (std::fabs((pMod - (FLOAT_TYPE)TWO_PI) - refMod) <
        std::fabs(pMod - refMod))
        pMod -= TWO_PI;
    else
        if (std::fabs((pMod + (FLOAT_TYPE)TWO_PI) - refMod) <
            std::fabs(pMod - refMod))
            pMod += TWO_PI;
    
    *phase = (refPhase - refMod) + pMod;
}
template void BLUtilsPhases::FindClosestPhase(float *phase, float refPhase);
template void BLUtilsPhases::FindClosestPhase(double *phase, double refPhase);
    
// For debugging, use modulo PI insteaod of modulo TWO_PI
template <typename FLOAT_TYPE>
void
BLUtilsPhases::FindClosestPhase180(FLOAT_TYPE *phase, FLOAT_TYPE refPhase)
{
    FLOAT_TYPE refMod = BLUtilsPhases::fmod_negative(refPhase, (FLOAT_TYPE)M_PI);
    FLOAT_TYPE pMod = BLUtilsPhases::fmod_negative(*phase, (FLOAT_TYPE)M_PI);
    
    // Find closest
    if (std::fabs((pMod - (FLOAT_TYPE)M_PI) - refMod) <
        std::fabs(pMod - refMod))
        pMod -= M_PI;
    else
        if (std::fabs((pMod + (FLOAT_TYPE)M_PI) - refMod) <
            std::fabs(pMod - refMod))
            pMod += M_PI;
    
    *phase = (refPhase - refMod) + pMod;
}
template void BLUtilsPhases::FindClosestPhase180(float *phase,
                                                 float refPhase);
template void BLUtilsPhases::FindClosestPhase180(double *phase,
                                                 double refPhase);

// We must use "closest", not "next"
// See: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
//
// Use smooth coeff to avoid several successive jumps
// that finally gets the data very far from the begining values
//
// NOTE: this is normal to have some jumps by PI
// See: https://ccrma.stanford.edu/~jos/fp/Example_Zero_Phase_Filter_Design.html#fig:remezexb
//
// Set dbgUnwrap180 to true in order to make module PI instead of TWO_PI
// (just for debugging)
template <typename FLOAT_TYPE>
void
BLUtilsPhases::UnwrapPhases2(WDL_TypedBuf<FLOAT_TYPE> *phases,
                             bool dbgUnwrap180)
{
#define UNWRAP_SMOOTH_COEFF 0.9 //0.5
    
    if (phases->GetSize() == 0)
        // Empty phases
        return;
    
    FLOAT_TYPE prevPhaseSmooth = phases->Get()[0];
    
    int phasesSize = phases->GetSize();
    FLOAT_TYPE *phasesData = phases->Get();
    for (int i = 0; i < phasesSize; i++)
    {
        FLOAT_TYPE phase = phasesData[i];

        if (!dbgUnwrap180)
            // Correct dehaviour
            FindClosestPhase(&phase, prevPhaseSmooth);
        else
            FindClosestPhase180(&phase, prevPhaseSmooth);
        
        phasesData[i] = phase;
        
        prevPhaseSmooth =
            (1.0 - UNWRAP_SMOOTH_COEFF)*phase + UNWRAP_SMOOTH_COEFF*prevPhaseSmooth;
    }
}
template void BLUtilsPhases::UnwrapPhases2(WDL_TypedBuf<float> *phases,
                                           bool dbgUnwrap180);
template void BLUtilsPhases::UnwrapPhases2(WDL_TypedBuf<double> *phases,
                                           bool dbgUnwrap180);
