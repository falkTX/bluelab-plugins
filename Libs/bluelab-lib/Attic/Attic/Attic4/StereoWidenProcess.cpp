//
//  StereoWidenProcess.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include <BLUtils.h>
//#include <USTWidthAdjuster3.h>
#include <DelayObj4.h>

#include <FftProcessObj16.h>
#include <ParamSmoother.h>

#define STEREO_WIDEN_COMPLEX_OPTIM 1

#include "StereoWidenProcess.h"

// Stereo Widen
// For info, see: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
// and https://www.kvraudio.com/forum/viewtopic.php?t=212587
// and https://www.irisa.fr/prive/kadi/Sujets_CTR/Emmanuel/Vincent_sujet1_article_avendano.pdf

#ifndef M_PI
#define M_PI 3.141592653589
#endif

// StereoWiden
//

// Stereo widen computation method

#define MAX_WIDTH_FACTOR 2.0 //8.0 // not for sqrt method


// FIX: On Protools, when width parameter was reset to 0,
// by using alt + click, the gain was increased compared to bypass
//
// On Protools, when alt + click to reset parameter,
// Protools manages all, and does not reset the parameter exactly to the default value
// (precision 1e-8).
// Then the coeff is not exactly 1, then the gain coeff is applied.
#define FIX_WIDTH_PRECISION 1
#define WIDTH_PARAM_PRECISION 1e-6

// NOTE: does not work well, increase the volume when increasing the width
#define STEREO_WIDEN_COMPLEX_OPTIM2 0 //1

#if !STEREO_WIDEN_COMPLEX_OPTIM
void
StereoWidenProcess::StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                BL_FLOAT widthFactor)
{
    BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        BL_FLOAT left = (*ioSamples)[0]->Get()[i];
        BL_FLOAT right = (*ioSamples)[1]->Get()[i];
        
        StereoWiden(&left, &right, width);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}
#else
void
StereoWidenProcess::StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                BL_FLOAT widthFactor)
{
    BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    WDL_FFT_COMPLEX angle0 = StereoWidenProcess::StereoWidenComputeAngle0();
    WDL_FFT_COMPLEX angle1 = StereoWidenProcess::StereoWidenComputeAngle1();
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        BL_FLOAT left = (*ioSamples)[0]->Get()[i];
        BL_FLOAT right = (*ioSamples)[1]->Get()[i];
        
        StereoWidenProcess::StereoWiden(&left, &right, width, angle0, angle1);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}
#endif

// Same, with smoother
#if !STEREO_WIDEN_COMPLEX_OPTIM
void
StereoWidenProcess::StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                ParamSmoother *widthFactorSmoother)
{
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        widthFactorSmoother->Update();
        BL_FLOAT widthFactor = widthFactorSmoother->GetCurrentValue();
        
        BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
        
#if FIX_WIDTH_PRECISION
        width /= WIDTH_PARAM_PRECISION;
        width = bl_round(width);
        width *= WIDTH_PARAM_PRECISION;
#endif
        
        //
        BL_FLOAT left = (*ioSamples)[0]->Get()[i];
        BL_FLOAT right = (*ioSamples)[1]->Get()[i];
        
        StereoWiden(&left, &right, width);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}
#else
void
StereoWidenProcess::StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                ParamSmoother *widthFactorSmoother)
{
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    WDL_FFT_COMPLEX angle0 = StereoWidenProcess::StereoWidenComputeAngle0();
    WDL_FFT_COMPLEX angle1 = StereoWidenProcess::StereoWidenComputeAngle1();
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        widthFactorSmoother->Update();
        BL_FLOAT widthFactor = widthFactorSmoother->GetCurrentValue();
        
        BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
        
#if FIX_WIDTH_PRECISION
        width /= WIDTH_PARAM_PRECISION;
        width = bl_round(width);
        width *= WIDTH_PARAM_PRECISION;
#endif
        
        //
        BL_FLOAT left = (*ioSamples)[0]->Get()[i];
        BL_FLOAT right = (*ioSamples)[1]->Get()[i];
        
        StereoWidenProcess::StereoWiden(&left, &right, width, angle0, angle1);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}
#endif

void
StereoWidenProcess::StereoWiden(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioSamples,
                                BL_FLOAT widthFactor)
{
    BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        WDL_FFT_COMPLEX left = (*ioSamples)[0]->Get()[i];
        WDL_FFT_COMPLEX right = (*ioSamples)[1]->Get()[i];
        
        StereoWidenProcess::StereoWiden(&left, &right, width);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}

// Correct formula for balance
// (Le livre des techniques du son - Tome 2 - p227)
//
// Balance do not use pan law
//
// Center position: 0dB atten for both channel
// Extrement positions: (+3dB, -inf)
//
// Results:
// - center position: no gain
// - extreme position: (+3dB, -inf)
// - when balancing a mono signal, the line moves constantly on the vectorscope

// For implementation, see: https://www.kvraudio.com/forum/viewtopic.php?t=235347

void
StereoWidenProcess::Balance(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                            BL_FLOAT balance)
{
    BL_FLOAT p = M_PI*(balance + 1.0)/4.0;
    BL_FLOAT gl = std::cos(p);
    BL_FLOAT gr = std::sin(p);
    
    // Coefficients to have no gain when center, and +3dB for extreme pan pos
    gl *= std::sqrt(2.0);
    gr *= std::sqrt(2.0);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        BL_FLOAT l = (*ioSamples)[0]->Get()[i];
        BL_FLOAT r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}

// Same, with smoother
void
StereoWidenProcess::Balance(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                            ParamSmoother *balanceSmoother)
{
  BL_FLOAT sqrtTwo = std::sqrt(2.0);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        balanceSmoother->Update();
        BL_FLOAT balance = balanceSmoother->GetCurrentValue();
    
        BL_FLOAT p = M_PI*(balance + 1.0)/4.0;
        BL_FLOAT gl = std::cos(p);
        BL_FLOAT gr = std::sin(p);
  
        // Coefficients to have no gain when center, and +3dB for extreme pan pos
        gl *= sqrtTwo;
        gr *= sqrtTwo;
        
        //
        BL_FLOAT l = (*ioSamples)[0]->Get()[i];
        BL_FLOAT r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}

// See: http://www.native-instruments.com/forum/attachments/stereo-solutions-pdf.13883
// and: http://www.csounds.com/journal/issue14/PseudoStereo.html
//
// Result:
// - almost same level (mono2stereo, then out mono => the level is the same as mono only)
// - result seems like IZotope Ozone Imager
//
// NOTE: there are some improved algorithms in the paper
void
StereoWidenProcess::MonoToStereo(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec,
                                 DelayObj4 *delayObj)
{
    if (samplesVec->empty())
        return;
    
    StereoToMono(samplesVec);
    
    // Lauridsen
    
    WDL_TypedBuf<BL_FLOAT> delayMono = (*samplesVec)[0];
    delayObj->ProcessSamples(&delayMono);
    
    // L = mono + delayMono
    BLUtils::AddValues(&(*samplesVec)[0], delayMono);
    
    // R = delayMono - mono
    BLUtils::MultValues(&(*samplesVec)[1], (BL_FLOAT)-1.0);
    BLUtils::AddValues(&(*samplesVec)[1], delayMono);
    
    // Adjust the width by default
#define LAURIDSEN_WIDTH_ADJUST -0.70
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(&(*samplesVec)[0]);
    samples.push_back(&(*samplesVec)[1]);
    
    StereoWidenProcess::StereoWiden(&samples, LAURIDSEN_WIDTH_ADJUST);
}


void
StereoWidenProcess::StereoToMono(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec)
{
    // First, set to bi-mono channels
    if (samplesVec->size() == 1)
    {
        // Duplicate mono channel
        samplesVec->push_back((*samplesVec)[0]);
    }
    else if (samplesVec->size() == 2)
    {
        // Set to mono
        WDL_TypedBuf<BL_FLOAT> mono;
        BLUtils::StereoToMono(&mono, (*samplesVec)[0], (*samplesVec)[1]);
        
        (*samplesVec)[0] = mono;
        (*samplesVec)[1] = mono;
    }
}

BL_FLOAT
StereoWidenProcess::ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal)
{
    BL_FLOAT res = 0.0;
    if (normVal < 0.0)
        res = normVal + 1.0;
    else
        res = normVal*(maxVal - 1.0) + 1.0;
    
    return res;
}

// VERY GOOD:
// - presever loudness when decreasing stereo width
// - seems transparent if we set to mono after (no loudness change)
// - sound very well (comparable to IZotope Ozone)

// Custom version
//
// Algorithm:
// - samples to polar
// - rotate polar to have then on horizontal directino
// - scale them over y
// - rotate them back
// - polar to sample
//
// STEREO_WIDEN_SQRT_CUSTOM
void
StereoWidenProcess::StereoWiden(BL_FLOAT *left, BL_FLOAT *right, BL_FLOAT widthFactor)
{
    BL_FLOAT l = *left;
    BL_FLOAT r = *right;
    
    // Samples to polar
    BL_FLOAT angle = std::atan2(r, l);
    BL_FLOAT dist = std::sqrt(l*l + r*r);
    
    // Rotate
    angle -= M_PI/4.0;
    
    // Get x, y coordinates
    BL_FLOAT x = dist*std::cos(angle);
    BL_FLOAT y = dist*std::sin(angle);
    
    // NOTE: sounds is bad with normalization
    // Normalization (1)
    //BL_FLOAT rad0 = std::sqrt(x*x + y*y);
    
    // Scale over y
    y *= widthFactor;
    
    // Back to polar
    dist = std::sqrt(x*x + y*y);
    angle = std::atan2(y, x);
    
    // Normalization (2)
    //dist = rad0;
    
    // Rotate back
    angle += M_PI/4.0;
    
    // Polar to sample
    l = dist*std::cos(angle);
    r = dist*std::sin(angle);
    
    // Result
    *left = l;
    *right = r;
}

#if STEREO_WIDEN_COMPLEX_OPTIM
WDL_FFT_COMPLEX
StereoWidenProcess::StereoWidenComputeAngle0()
{
    WDL_FFT_COMPLEX result;
    
    BL_FLOAT magn = 1.0;
    BL_FLOAT phase = -M_PI/4.0;
    
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}

WDL_FFT_COMPLEX
StereoWidenProcess::StereoWidenComputeAngle1()
{
    WDL_FFT_COMPLEX result;
    
    BL_FLOAT magn = 1.0;
    BL_FLOAT phase = M_PI/4.0;
    
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}

void
StereoWidenProcess::StereoWiden(BL_FLOAT *left, BL_FLOAT *right, BL_FLOAT widthFactor,
                                const WDL_FFT_COMPLEX &angle0, WDL_FFT_COMPLEX &angle1)
{
    // Init
    WDL_FFT_COMPLEX signal0;
    signal0.re = *left;
    signal0.im = *right;
    
    // Rotate
    WDL_FFT_COMPLEX signal1;
    COMP_MULT(signal0, angle0, signal1);
    
    // Scale over y
    signal1.im *= widthFactor;
    
    // Rotate back
    WDL_FFT_COMPLEX signal2;
    COMP_MULT(signal1, angle1, signal2);
    
    // Result
    *left = signal2.re;
    *right = signal2.im;
}
#endif

// NOTE: does not work very well
void
StereoWidenProcess::StereoWiden(WDL_FFT_COMPLEX *left, WDL_FFT_COMPLEX *right, BL_FLOAT widthFactor)
{
    // Naive version, just to check
#if !STEREO_WIDEN_COMPLEX_OPTIM2
    BL_FLOAT l = COMP_MAGN((*left));
    BL_FLOAT lp = COMP_PHASE((*left));
    
    BL_FLOAT r = COMP_MAGN((*right));
    BL_FLOAT rp = COMP_PHASE((*right));
    
    StereoWiden(&l, &r, widthFactor);
    
    MAGN_PHASE_COMP(l, lp, (*left));
    MAGN_PHASE_COMP(r, rp, (*right));
    
    return;
#endif
    
#if 0 //1 // First optimized version, using only comlex numbers
    BL_FLOAT center[2] = { (left->re + right->re)*0.5, (left->im + right->im)*0.5 };
    BL_FLOAT dir[2] = { left->re - center[0], left->im - center[1] };
    BL_FLOAT dist = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
    
    if (dist > 0.0)
    {
        BL_FLOAT normDir[2] = { dir[0]/dist, dir[1]/dist };
        dist *= widthFactor;
    
        left->re = center[0] + dist*normDir[0];
        left->im = center[1] + dist*normDir[1];
        
        right->re = center[0] - dist*normDir[0];
        right->im = center[1] - dist*normDir[1];
    }
#endif
    
    // Second optimized version, using only comlex numbers
    // More optimized
#if STEREO_WIDEN_COMPLEX_OPTIM2
    BL_FLOAT center[2] = { (left->re + right->re)*0.5, (left->im + right->im)*0.5 };
    BL_FLOAT dir[2] = { left->re - center[0], left->im - center[1] };
    
    dir[0] *= widthFactor;
    dir[1] *= widthFactor;
        
    left->re = center[0] + dir[0];
    left->im = center[1] + dir[1];
        
    right->re = center[0] - dir[0];
    right->im = center[1] - dir[1];
#endif
}
