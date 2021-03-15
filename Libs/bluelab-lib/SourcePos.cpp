//
//  SourcePos.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/18/18.
//
//

#include <BLUtils.h>
#include <BLUtilsPhases.h>
#include <BLUtilsMath.h>

#include "SourcePos.h"

// Let's say 1cm (Zoom H1)
//#define MICROPHONES_SEPARATION 0.01 // orig
//#define MICROPHONES_SEPARATION 0.1 // prev
//#define MICROPHONES_SEPARATION 1.0
//#define MICROPHONES_SEPARATION 4.0
// GOOD with float time delays
#define MICROPHONES_SEPARATION 10.0
// make identity work good / but the display is too narrow
//#define MICROPHONES_SEPARATION 50.0

// GOOD !
// The original algorithm works for half the cases
// So with the following, we invert the data at the beginning
// to manage the second case.
#define INVERT_ALGO 1

// GOOD !
// Sounds very good with float time delays
#define USE_TIME_DELAY 1 //0 //1

BL_FLOAT
SourcePos::GetDefaultMicSeparation()
{
    return MICROPHONES_SEPARATION;
}

void
SourcePos::MagnPhasesToSourcePos(WDL_TypedBuf<BL_FLOAT> *outSourceRs,
                                 WDL_TypedBuf<BL_FLOAT> *outSourceThetas,
                                 const WDL_TypedBuf<BL_FLOAT> &magnsL,
                                 const WDL_TypedBuf<BL_FLOAT> &magnsR,
                                 const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                 const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                 const WDL_TypedBuf<BL_FLOAT> &freqs,
                                 const WDL_TypedBuf<BL_FLOAT> &timeDelays)
{
    if (magnsL.GetSize() != magnsR.GetSize())
        // R can be empty in mono
        return;
    
    if (phasesL.GetSize() != phasesR.GetSize())
        // Just in case
        return;
    
    outSourceRs->Resize(phasesL.GetSize());
    outSourceThetas->Resize(phasesL.GetSize());
    
    for (int i = 0; i < phasesL.GetSize(); i++)
    {
        BL_FLOAT magnL = magnsL.Get()[i];
        BL_FLOAT magnR = magnsR.Get()[i];
        
        BL_FLOAT phaseL = phasesL.Get()[i];
        BL_FLOAT phaseR = phasesR.Get()[i];
        
        BL_FLOAT freq = freqs.Get()[i];
        
        BL_FLOAT timeDelay = timeDelays.Get()[i];
        
        BL_FLOAT sourceR;
        BL_FLOAT sourceTheta;
        bool computed = MagnPhasesToSourcePos(&sourceR, &sourceTheta,
                                              magnL, magnR, phaseL, phaseR, freq,
                                              timeDelay);
        
        if (computed)
        {
            outSourceRs->Get()[i] = sourceR;
            outSourceThetas->Get()[i] = sourceTheta;
        }
        else
        {
            // Set a negative value to the radius, to mark it as not computed
            outSourceRs->Get()[i] = UTILS_VALUE_UNDEFINED;
            outSourceThetas->Get()[i] = UTILS_VALUE_UNDEFINED;
        }
    }
}

bool
SourcePos::MagnPhasesToSourcePos(BL_FLOAT *sourceR, BL_FLOAT *sourceTheta,
                                 BL_FLOAT magnL, BL_FLOAT magnR,
                                 BL_FLOAT phaseL, BL_FLOAT phaseR,
                                 BL_FLOAT freq, BL_FLOAT timeDelay)
{
    //#define EPS 1e-15
    
    // Avoid numerical imprecision when magns are very small and similar
    // (was noticed by abnormal L/R phase inversions)
#define EPS 1e-8
    
    // See: https://dsp.stackexchange.com/questions/30736/how-to-calculate-an-azimuth-of-a-sound-source-in-stereo-signal
    // Thx :)
    //
    
    // NOTE: there were some DebugDrawer code here...
    
    // Init
    *sourceR = 0.0;
    *sourceTheta = 0.0;
    
    if (freq < EPS)
        // Just in case
    {
        // Null frequency
        // Can legitimately set the result to 0
        // and return true
        
        return true;
    }
    
    const BL_FLOAT d = MICROPHONES_SEPARATION;
    
    // Sound speed: 340 m/s
    const BL_FLOAT c = 340.0;
    
#if INVERT_ALGO
    // Test if we must invert left and right for the later computations
    // In fact, the algorithm is designed to have G < 1.0,
    // i.e magnL < magnR
    //
    bool invertFlag = false;
    
    if (magnL > magnR)
    {
        invertFlag = true;
        
        BL_FLOAT tmpMagn = magnL;
        magnL = magnR;
        magnR = tmpMagn;
        
        BL_FLOAT tmpPhase = phaseL;
        phaseL = phaseR;
        phaseR = tmpPhase;
        
        // timeDelay ?;
        
        // TEST
        timeDelay = -timeDelay;
    }
#endif
    
    // Test at least if we have sound !
    if ((magnR < EPS) || (magnL < EPS))
        // No sound !
    {
        // Can legitimately set the result to 0
        // and return true
        
        return true;
    }
    
    // Compute the delay from the phase difference
    BL_FLOAT phaseDiff = phaseR - phaseL;
    
    // Avoid negative phase diff (would make problems below)
    phaseDiff = BLUtilsPhases::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
    
    // Magns ratio
    BL_FLOAT G = magnL/magnR;
    //BLDebug::AppendValue("G0.txt", G);
    
    if (std::fabs((BL_FLOAT)(G - 1.0)) < EPS)
        // Similar magns: can't compute...
    {
        // We will be in mono
        
        return true;
    }
    
    // Time difference
    // (Unused)
    BL_FLOAT T = (1.0/freq)*(phaseDiff/(2.0*M_PI));
    
    // Distance to the right
    BL_FLOAT denomB = (1.0 - G);
    if (std::fabs(denomB) < EPS)
        // Similar magns (previously already tested)
    {
        // We will be in mono
        
        return true;
    }
    
#if !USE_TIME_DELAY
    BL_FLOAT b = G*T*c/denomB;
#else
    BL_FLOAT b = G*timeDelay*c/denomB;
#endif
    
#if !USE_TIME_DELAY
    BL_FLOAT a = T*c;
#else
    
    BL_FLOAT a = timeDelay*c;
    
#endif
    
    // r
    
    // Use the formula using a and b from he forum
    // (simpler for inversion later)
    
    // Formula from the forum (GOOD !)
    BL_FLOAT r2 = 4.0*b*b + 4.0*a*b + 2.0*a*a - d*d;
    if (r2 < 0.0)
        // No solution: stop
        return false;
    
    BL_FLOAT r = 0.5*std::sqrt(r2);
    
#if 1    // Method from the forum
    // => problem here because the two mics are too close,
    // and acos is > 1.0 so we can't get the angle
    
    // Theta
    BL_FLOAT cosTheta = r/d + d/(4.0*r) - b*b/(d*r);
    
    if ((cosTheta < -1.0) || (cosTheta > 1.0))
        // No solution: stop
    {        
        return false;
    }
    
    BL_FLOAT theta = std::acos(cosTheta);
#endif
    
#if 0 // Method from paper toni.heittola@tut.f
    BL_FLOAT theta = 2.0*M_PI*std::atan(G)/M_PI - M_PI/2.0;
#endif
    
#if INVERT_ALGO
    if (invertFlag)
        theta = M_PI - theta;
#endif
    
    *sourceR = r;
    *sourceTheta = theta;
    
    return true;
}

void
SourcePos::SourcePosToMagnPhases(const WDL_TypedBuf<BL_FLOAT> &sourceRs,
                                 const WDL_TypedBuf<BL_FLOAT> &sourceThetas,
                                 BL_FLOAT micsDistance,
                                 BL_FLOAT widthFactor,
                                 WDL_TypedBuf<BL_FLOAT> *ioMagnsL,
                                 WDL_TypedBuf<BL_FLOAT> *ioMagnsR,
                                 WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                                 WDL_TypedBuf<BL_FLOAT> *ioPhasesR,
                                 const WDL_TypedBuf<BL_FLOAT> &freqs,
                                 const WDL_TypedBuf<BL_FLOAT> &timeDelays)
{
    if (ioMagnsL->GetSize() != ioMagnsR->GetSize())
        // R can be empty in mono
        return;
    
    if (ioPhasesL->GetSize() != ioPhasesR->GetSize())
        // Just in case
        return;
    
    // Compute the new magns and phases (depending on the width factor)
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        // Retrieve the left and right distance
        BL_FLOAT sourceR = sourceRs.Get()[i];
        BL_FLOAT sourceTheta = sourceThetas.Get()[i];
        
        BL_FLOAT freq = freqs.Get()[i];
        
        // Init
        BL_FLOAT newMagnL = ioMagnsL->Get()[i];
        BL_FLOAT newMagnR = ioMagnsR->Get()[i];
        
        BL_FLOAT newPhaseL = ioPhasesL->Get()[i];
        BL_FLOAT newPhaseR = ioPhasesR->Get()[i];
        
        // Unused ?
        BL_FLOAT timeDelay = timeDelays.Get()[i];
        
        bool computed = SourcePosToMagnPhases(sourceR, sourceTheta,
                                              micsDistance,
                                              &newMagnL, &newMagnR,
                                              &newPhaseL, &newPhaseR, freq,
                                              timeDelay);
        
        // The result can be undfined for "noisy" parts of the signal
        // (difficult to locate a background noise...)
        if (!computed)
        {
            BL_FLOAT phaseFactor = widthFactor;
            
            // For these parts, simply narrow or enlarge the phase
            ModifyPhase(&newPhaseL, &newPhaseR, phaseFactor);
        }
        
        // Result
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
        
        ioMagnsL->Get()[i] = newMagnL;
        ioMagnsR->Get()[i] = newMagnR;
    }
}

bool
SourcePos::SourcePosToMagnPhases(BL_FLOAT sourceR, BL_FLOAT sourceTheta,
                                 BL_FLOAT d,
                                 BL_FLOAT *ioMagnL, BL_FLOAT *ioMagnR,
                                 BL_FLOAT *ioPhaseL, BL_FLOAT *ioPhaseR,
                                 BL_FLOAT freq,
                                 BL_FLOAT timeDelay /* unused ! */)
{
    //#define EPS 1e-15
    
    // Avoid numerical imprecision when magns are very small and similar
    // (was noticed by abnormal L/R phase inversions)
#define EPS 1e-8
    
    if (sourceR < EPS)
        // The source position was not previously computed (so not usable)
        // Do not modify the signal !
    {
        return false;
    }
    
#if INVERT_ALGO
    bool invertFlag = (*ioMagnL > *ioMagnR);
    if (invertFlag)
        sourceTheta = M_PI - sourceTheta;
#endif
    
    if (d < EPS)
        // The two mics are at the same position
        // => the signal should be mono !
    {
        BL_FLOAT middleMagn = (*ioMagnL + *ioMagnR)/2.0;
        *ioMagnL = middleMagn;
        *ioMagnR = middleMagn;
        
        // Take the left channel for the phase
        *ioPhaseR = *ioPhaseL;
        
        return true;
    }
    
    // Reverse process of the previous method
    
    // 340 m/s
    const BL_FLOAT c = 340.0;
    
    // b
    BL_FLOAT b2 = (sourceR/d + d/(4.0*sourceR) - std::cos(sourceTheta))*d*sourceR;
    if (b2 < 0.0)
    {
        // No solution
        return false;
    }
    
    BL_FLOAT b = std::sqrt(b2);
    
    // Second order equation resolution
    BL_FLOAT A0 = 0.5;
    BL_FLOAT B0 = b;
    BL_FLOAT C0 = -(sourceR*sourceR - b*b + d*d/4.0);
    
    BL_FLOAT a2[2];
    int numSol = BLUtilsMath::SecondOrderEqSolve(A0, B0, C0, a2);
    if (numSol == 0)
    {
        return false;
    }
    
    BL_FLOAT a;
    if (numSol == 1)
        a = a2[0];
    if (numSol == 2)
    {
        if ((a2[0] >= 0.0) && (a2[1] < 0.0))
            a = a2[0];
        else
            if ((a2[0] < 0.0) && (a2[1] >= 0.0))
                a = a2[1];
            else
            {
                // See: https://dsp.stackexchange.com/questions/30736/how-to-calculate-an-azimuth-of-a-sound-source-in-stereo-signal
                // Condition: a + b > r
                
                if (a2[0] + b > sourceR)
                {
                    a = a2[0];
                }
                else if (a2[1] + b > sourceR)
                {
                    a = a2[1];
                } else
                {
                    // No solution !
                    return false;
                }
                
            }
    }
    
    
    // Compute phaseDiff
    BL_FLOAT timeDelay0 = a/c;
    
    BL_FLOAT denomG = (timeDelay0*c + b);
    if (std::fabs(denomG) < EPS)
    {
        return false;
    }
    
    BL_FLOAT G = b/denomG;
    
    // Phase diff
    BL_FLOAT phaseDiff = timeDelay0*freq*2.0*M_PI;
    
#if INVERT_ALGO
    if (invertFlag)
    {
        if (G > 0.0)
            G = 1.0/G;
        
        phaseDiff = 2.0*M_PI - phaseDiff;
    }
#endif
    
    // Added after volume rendering tests
    phaseDiff = BLUtilsPhases::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
    
    // GOOD ... (old)
#if 0 // Method 1: take the left, and adjust for the right
    const BL_FLOAT origMagnL = *ioMagnL;
    const BL_FLOAT origPhaseL = *ioPhaseL;
    
    // Method "1"
    *ioMagnR = origMagnL/G;
    *ioPhaseR = origPhaseL + phaseDiff;
    
    *ioPhaseR = BLUtils::MapToPi(*ioPhaseR);
#endif
    
    // GOOD: for identity
#if 1 // Method 2: take the middle, and adjust for left and right
    
    // For magns, take the middle and compute the extremities
    BL_FLOAT middleMagn = (*ioMagnL + *ioMagnR)/2.0;
    
    *ioMagnR = 2.0*middleMagn/(G + 1.0);
    *ioMagnL = *ioMagnR*G;
    
    // For phases, take the left and compute the right
#if !INVERT_ALGO
    *ioPhaseR = *ioPhaseL + phaseDiff;
#else
    
    // NEW
    *ioPhaseR = *ioPhaseL + phaseDiff;
    
#if 0 //OLD
    if (invertFlag)
    {
        *ioPhaseR = *ioPhaseL + phaseDiff;
    }
    else
    {
        *ioPhaseL = *ioPhaseR + phaseDiff;
    }
#endif
    
    *ioPhaseL = BLUtilsPhases::MapToPi(*ioPhaseL);
    *ioPhaseR = BLUtilsPhases::MapToPi(*ioPhaseR);
#endif
    
#endif
    
    return true;
}

void
SourcePos::ModifyPhase(BL_FLOAT *phaseL,
                       BL_FLOAT *phaseR,
                       BL_FLOAT factor)
{
    BL_FLOAT phaseDiff = *phaseR - *phaseL;
    
    *phaseR = *phaseL + factor*phaseDiff;
}
