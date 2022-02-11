//
//  InstantCompressor.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include <math.h>

#include <Debug.h>

#include "InstantCompressor.h"

// Fix the gain computation, so we can use this class as limiter,
// when using a slope at 100%
#define NIKO_FIX_GAIN_COMPUTATION 1

// Impelmentation of soft knee
#define NIKO_SOFT_KNEE 1


InstantCompressor::InstantCompressor(double sampleRate)
{
    mSampleRate = sampleRate;
    
    mThreshold = 100.0;
    mSlope = 100.0;
    mAttack = 0.0;
    mRelease = 0.0;
    
    //
    mKneePercent = 0.0;
    
    // envelope
    mEnv = 0.0;
    
    mGain = 1.0;
}

InstantCompressor::~InstantCompressor() {}

void
InstantCompressor::Reset(double sampleRate)
{
    mSampleRate = sampleRate;
    
    mEnv = 0.0;
    
    mGain = 1.0;
}

void
InstantCompressor::SetParameters(double threshold, double  slope,
                                 double  tatt, double  trel)
{
    mThreshold = threshold;
    mSlope = slope;
    mAttack = tatt;
    mRelease = trel;
}

void
InstantCompressor::SetThreshold(double threshold)
{
    mThreshold = threshold;
}

void
InstantCompressor::SetSlope(double slope)
{
    mSlope = slope;
}

void
InstantCompressor::SetAttack(double tatt)
{
    mAttack = tatt;
}

void
InstantCompressor::SetRelease(double trel)
{
     mRelease = trel;
}

void
InstantCompressor::SetKnee(double kneePercent)
{
    mKneePercent = kneePercent;
}

void
InstantCompressor::Process(double *rmsAmp)
{
#define EPS 1e-15
    
    // See: https://www.musicdsp.org/en/latest/Effects/169-compressor.html
    //
    
    // threshold to unity (0...1)
    double threshold = mThreshold*0.01;
    
    // slope to unity
    double slope = mSlope*0.01;
    
    // attack time to seconds
    double tatt = mAttack*1e-3;
    
    // release time to seconds
    double trel = mRelease*1e-3;
    
    // attack and release "per sample decay"
    double  att = (tatt == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * tatt));
    double  rel = (trel == 0.0) ? (0.0) : exp (-1.0 / (mSampleRate * trel));
    
    // for each sample...
    
    double rms = *rmsAmp;
        
    // dynamic selection: attack or release?
    double  theta = rms > mEnv ? att : rel;
        
    // smoothing with capacitor, envelope extraction...
    // here be aware of pIV denormal numbers glitch
    mEnv = (1.0 - theta) * rms + theta * mEnv;
    
    // DEBUG
    mEnv = rms;
    
    double  gain = 1.0;
    
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
     //slope = 0.5; // TEST
    
    if (mKneePercent < EPS)
        // No knee
    {
        if (mEnv > threshold)
        {
            if (mEnv > EPS)
                gain = (threshold/mEnv) * slope + (1.0 - slope)*1.0;
        }
    }
    else
    {
        // Use knee
        //
        // See: https://dsp.stackexchange.com/questions/28548/differences-between-soft-knee-and-hard-knee-in-dynamic-range-compression-drc
        
        double w = mKneePercent*0.01*2.0*threshold;
        if (mEnv - threshold < -w/2.0)
        {
            gain = 1.0;
        }
        
        if (fabs(mEnv - threshold) <= w/2.0)
        {
            double t = (mEnv - threshold + w/2.0)/w;
            
            // Round curve
            //t = t*t;
            //t = sqrt(t);
            
            double a = 0.0;
            //double b = threshold*1.5; // for slope 1.0
            //threshold*1.25; // Hack for slope 0.5
            
            double b = threshold*(1.0 + slope/2.0);
            double c = 1.0;
            
            // NOTE: this is not optimal at all, could be optimized later
            // See: https://math.stackexchange.com/questions/2132213/interpolation-of-3-points
            t = 2.0*a*t*t - 3.0*a*t + a - 4.0*b*t*t + 4.0*b*t + 2.0*c*t*t - c*t;
            
            //double gain0 = 1.0;
            double gain0 = threshold - w/2.0;
            
            double gain1 = 1.0;
            if (mEnv > EPS)
                //gain1 = (threshold/mEnv) * slope + (1.0 - slope)*1.0;
                //gain1 = (threshold/(threshold + w/2.0)) * slope + (1.0 - slope)*1.0;
                //gain1 = threshold + w*0.5*slope; // TEST
                gain1 = threshold + w*0.5*(1.0 - slope);
            
            gain = (1.0 - t)*gain0 + t*gain1;
            
            // TEST
            *rmsAmp = gain;
            //*rmsAmp = t;
            //*rmsAmp = mEnv;
            return;
        }
        
        if (mEnv - threshold > w/2.0)
        {
            if (mEnv > EPS)
                gain = (threshold/mEnv) * slope + (1.0 - slope)*1.0;
        }
    }
#endif
    
    // result - hard kneed compressed channels...
    double result = *rmsAmp*gain;
    
#if 0 // Debug
    Debug::AppendValue("rms.txt", rms);
    Debug::AppendValue("env.txt", mEnv);
    Debug::AppendValue("gain.txt", gain);
    Debug::AppendValue("result.txt", result);
#endif
    
    *rmsAmp = result;
    
    mGain = gain;
}

double
InstantCompressor::GetGain()
{
    return mGain;
}
