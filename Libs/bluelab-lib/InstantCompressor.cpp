//
//  InstantCompressor.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include <math.h>
#include <cmath>

#include <BLDebug.h>

#include "InstantCompressor.h"

// Fix the gain computation, so we can use this class as limiter,
// when using a slope at 100%
#define NIKO_FIX_GAIN_COMPUTATION 1

// Impelmentation of soft knee
#define NIKO_SOFT_KNEE 1


InstantCompressor::InstantCompressor(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mThreshold = 100.0;
    mSlope = 100.0;
    mAttack = 0.0;
    mRelease = 0.0;
    
    //
    mKnee = 0.0;
    
    // envelope
    mEnv = 0.0;
    
    mGain = 1.0;
    
    // Optimization
    BL_FLOAT tatt = mAttack*1e-3;
    BL_FLOAT trel = mRelease*1e-3;
    mAttack0 = (tatt == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * tatt));
    mRelease0 = (trel == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * trel));
}

InstantCompressor::~InstantCompressor() {}

void
InstantCompressor::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mEnv = 0.0;
    
    mGain = 1.0;
    
    // Optimization
    BL_FLOAT tatt = mAttack*1e-3;
    BL_FLOAT trel = mRelease*1e-3;
    mAttack0 = (tatt == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * tatt));
    mRelease0 = (trel == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * trel));
}

void
InstantCompressor::SetParameters(BL_FLOAT threshold, BL_FLOAT slope,
                                 BL_FLOAT tatt, BL_FLOAT trel)
{
    mThreshold = threshold;
    mSlope = slope;
    mAttack = tatt;
    mRelease = trel;
    
    // Optimization
    BL_FLOAT tatt0 = mAttack*1e-3;
    BL_FLOAT trel0 = mRelease*1e-3;
    mAttack0 = (tatt0 == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * tatt0));
    mRelease0 = (trel0 == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * trel0));
}

void
InstantCompressor::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
}

void
InstantCompressor::SetSlope(BL_FLOAT slope)
{
    mSlope = slope;
}

void
InstantCompressor::SetAttack(BL_FLOAT tatt)
{
    mAttack = tatt;
    
    // Optimization
    BL_FLOAT tatt0 = mAttack*1e-3;
    mAttack0 = (tatt0 == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * tatt0));
}

void
InstantCompressor::SetRelease(BL_FLOAT trel)
{
     mRelease = trel;
    
    // Optimization
    BL_FLOAT trel0 = mRelease*1e-3;
    mRelease0 = (trel0 == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * trel0));
}

void
InstantCompressor::SetKnee(BL_FLOAT knee)
{
    mKnee = knee;
}

void
InstantCompressor::Process(BL_FLOAT *rmsAmp)
{
#define EPS 1e-15
    
    // See: https://www.musicdsp.org/en/latest/Effects/169-compressor.html
    //
    
    // threshold to unity (0...1)
    BL_FLOAT threshold = mThreshold*0.01;
    
    // slope to unity
    BL_FLOAT slope = mSlope*0.01;
    
    // attack time to seconds
    //BL_FLOAT tatt = mAttack*1e-3;
    
    // release time to seconds
    //BL_FLOAT trel = mRelease*1e-3;
    
    // attack and release "per sample decay"
    //BL_FLOAT  att = (tatt == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * tatt));
    //BL_FLOAT  rel = (trel == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * trel));
    BL_FLOAT att = mAttack0;
    BL_FLOAT rel = mRelease0;
    
    // for each sample...
    
    BL_FLOAT rms = *rmsAmp;
        
    // dynamic selection: attack or release?
    BL_FLOAT  theta = (rms > mEnv) ? att : rel;
        
    // smoothing with capacitor, envelope extraction...
    // here be aware of pIV denormal numbers glitch
    mEnv = (1.0 - theta) * rms + theta * mEnv;
    
    // Optim
    BL_FLOAT envInv = 1.0;
    if (mEnv > EPS)
        envInv = 1.0/mEnv;
    
    BL_FLOAT  gain = 1.0;
    
#if !NIKO_SOFT_KNEE
    // the very easy hard knee 1:N compressor
    if (mEnv > threshold)
    {
#if !NIKO_FIX_GAIN_COMPUTATION 
        // Origin
        gain = gain - (mEnv - threshold) * slope;
#endif
        
#if NIKO_FIX_GAIN_COMPUTATION
        if (mEnv > EPS)
            gain = (threshold/mEnv) * slope + (1.0 - slope)*1.0;
#endif
    }
#endif
    
#if NIKO_SOFT_KNEE
    if (mKnee < EPS)
        // No knee
    {
        if (mEnv > threshold)
        {
            if (mEnv > EPS)
                //gain = (threshold/mEnv) * slope + (1.0 - slope)*1.0;
                gain = (threshold*envInv) * slope + (1.0 - slope)*1.0;
        }
    }
    else
    {
        // Use knee
        //
        // See: https://dsp.stackexchange.com/questions/28548/differences-between-soft-knee-and-hard-knee-in-dynamic-range-compression-drc
        
        // Knee width
        BL_FLOAT w = mKnee*0.01*2.0*threshold;
        
        // Under threshold and knee = do nothing
        //if (mEnv - threshold < -w/2.0)
        if (mEnv - threshold < -w*0.5)
        {
            gain = 1.0;
        }
        
        // In knee
        //if (std::fabs(mEnv - threshold) <= w/2.0)
        if (std::fabs(mEnv - threshold) <= w*0.5)
        {
            //BL_FLOAT t = (mEnv - threshold + w/2.0)/w;
            BL_FLOAT t = (mEnv - threshold + w*0.5)/w;
            
            // Round curve
            //
            BL_FLOAT a = 0.0;
            //BL_FLOAT b = threshold*(1.0 + slope/2.0);
            BL_FLOAT b = threshold*(1.0 + slope*0.5);
            BL_FLOAT c = 1.0;
            
            // NOTE: this is not optimal at all, could be optimized later
            // See: https://math.stackexchange.com/questions/2132213/interpolation-of-3-points
            t = 2.0*a*t*t - 3.0*a*t + a - 4.0*b*t*t + 4.0*b*t + 2.0*c*t*t - c*t;
            
            //BL_FLOAT gain0 = threshold - w/2.0;
            BL_FLOAT gain0 = threshold - w*0.5;
            
            BL_FLOAT gain1 = 1.0;
            if (mEnv > EPS)
                gain1 = threshold + w*0.5*(1.0 - slope);
            
            gain = (1.0 - t)*gain0 + t*gain1;
            
            // (Necessary)
            //gain /= mEnv;
            gain *= envInv;
        }
        
        // After knee
        //if (mEnv - threshold > w/2.0)
        if (mEnv - threshold > w*0.5)
        {
            if (mEnv > EPS)
                //gain = (threshold/mEnv) * slope + (1.0 - slope)*1.0;
                gain = (threshold*envInv) * slope + (1.0 - slope)*1.0;
        }
    }
#endif
    
    // result - hard kneed compressed channels...
    BL_FLOAT result = *rmsAmp*gain;
        
    *rmsAmp = result;
    
    mGain = gain;
}

BL_FLOAT
InstantCompressor::GetGain()
{
    return mGain;
}

void
InstantCompressor::DBG_DumpCompressionCurve()
{
    // Skip envelope
    mAttack = 0.0;
    mRelease = 0.0;
    
    mSlope = 100.0; //50.0; //
    
    mKnee = 25.0;
    //mKnee = 0.0;
    
    for (int i = 0; i < 100; i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/99.0;
        
        Process(&t);
        
        BLDebug::AppendValue("comp.txt", t);
    }
}
