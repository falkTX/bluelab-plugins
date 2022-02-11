//
//  USTProcess.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include <Utils.h>
#include <USTWidthAdjuster.h>
#include <DelayObj4.h>

#include "USTProcess.h"

// Stereo Widen
// For info, see: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
// and https://www.kvraudio.com/forum/viewtopic.php?t=212587
// and https://www.irisa.fr/prive/kadi/Sujets_CTR/Emmanuel/Vincent_sujet1_article_avendano.pdf

#ifndef M_PI
#define M_PI 3.141592653589
#endif

// Polar samples
//
#define CLIP_DISTANCE 0.95


// StereoWiden
//

// Stereo widen computation method

//
#define STEREO_WIDEN_GONIO 0
#define STEREO_WIDEN_VOL_ADJUSTED 0
#define STEREO_WIDEN_SQRT         0
#define STEREO_WIDEN_SQRT_CUSTOM  1 // VERY GOOD

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

// Correlation computation method
//

// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
#define COMPUTE_CORRELATION_ATAN 0

// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
// (may be more correct)
#define COMPUTE_CORRELATION_CROSS 1

// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
void
USTProcess::ComputePolarSamples(const WDL_TypedBuf<double> samples[2],
                                WDL_TypedBuf<double> polarSamples[2])
{
    polarSamples[0].Resize(samples[0].GetSize());
    polarSamples[1].Resize(samples[0].GetSize());
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        double l = samples[0].Get()[i];
        double r = samples[1].Get()[i];
        
        double angle = atan2(r, l);
        double dist = sqrt(l*l + r*r);
        
        //dist *= 0.5;
        
        // Clip
        if (dist > CLIP_DISTANCE)
            // Point outside the circle
            // => set it ouside the graph
        {
            polarSamples[0].Get()[i] = -1.0;
            polarSamples[1].Get()[i] = -1.0;
            
            continue;
        }
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
        // Compute (x, y)
        double x = dist*cos(angle);
        double y = dist*sin(angle);
        
        polarSamples[0].Get()[i] = x;
        polarSamples[1].Get()[i] = y;
    }
}

// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
#if COMPUTE_CORRELATION_ATAN
double
USTProcess::ComputeCorrelation(const WDL_TypedBuf<double> samples[2])
{
    double result = 0.0;
    double sumDist = 0.0;
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        double l = samples[0].Get()[i];
        double r = samples[1].Get()[i];
        
        double angle = atan2(r, l);
        double dist = sqrt(l*l + r*r);
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
        // Flip vertically, to keep only one half-circle
        if (angle < 0.0)
            angle = -angle;
        
        // Flip horizontally to keep only one quarter slice
        if (angle > M_PI/2.0)
            angle = M_PI - angle;
        
        // Normalize into [0, 1]
        double corr = angle/(M_PI/2.0);
        
        // Set into [-1, 1]
        corr = (corr * 2.0) - 1.0;
        
        // Ponderate with distance;
        corr *= dist;
        
        result += corr;
        sumDist += dist;
    }
    
    //if (samples[0].GetSize() > 0)
    //    result /= samples[0].GetSize();
    if (sumDist > 0.0)
        result /= sumDist;
    
    return result;
}
#endif

// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
#if COMPUTE_CORRELATION_CROSS
double
USTProcess::ComputeCorrelation(const WDL_TypedBuf<double> samples[2])
{
    double sumProduct = 0.0;
    double sumL2 = 0.0;
    double sumR2 = 0.0;
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        double l = samples[0].Get()[i];
        double r = samples[1].Get()[i];
        
        double product = l*r;
        sumProduct += product;
        
        sumL2 += l*l;
        sumR2 += r*r;
    }
    
    double correl = 0.0;
    int numValues = samples[0].GetSize();
    if (numValues > 0)
    {
        double num = sumProduct/numValues;
        
        double denom2 = (sumL2/numValues)*(sumR2/numValues);
        double denom = sqrt(denom2);
        
        correl = num/denom;
    }
    
    return correl;
}
#endif

void
USTProcess::StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples, double widthFactor)
{
    double width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double left = (*ioSamples)[0]->Get()[i];
        double right = (*ioSamples)[1]->Get()[i];
        
        StereoWiden(&left, &right, width);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}

void
USTProcess::StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples,
                        const USTWidthAdjuster *widthAdjuster)
{
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    //
    // https://www.musicdsp.org/en/latest/Effects/256-stereo-width-control-obtained-via-transfromation-matrix.html?highlight=stereo
    
    double widthFactor = widthAdjuster->GetWidth();
    double width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double left = (*ioSamples)[0]->Get()[i];
        double right = (*ioSamples)[1]->Get()[i];
        
        StereoWiden(&left, &right, width);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}

#if 1
// Correct formula for balance
// (Le livre des techniques du son - Tome 2 - p227)
//
// Balance do not use pan law
//
// Center position: 0dB atten for both channel
// Extrement positions: (+3dB, -inf)
//

// For implementation, see: https://www.kvraudio.com/forum/viewtopic.php?t=235347
void
USTProcess::Balance(vector<WDL_TypedBuf<double> * > *ioSamples,
                    double balance)
{
    double p = M_PI*(balance + 1.0)/4.0;
	double gl = cos(p);
	double gr = sin(p);
    
    // Coefficients to have no gain when center, and +3dB for extreme pan pos
    gl *= sqrt(2.0);
    gr *= sqrt(2.0);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double l = (*ioSamples)[0]->Get()[i];
        double r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
#endif

#if 1
// See: https://www.kvraudio.com/forum/viewtopic.php?t=235347
void
USTProcess::Balance0(vector<WDL_TypedBuf<double> * > *ioSamples,
                    double balance)
{
    double p = M_PI*(balance + 1.0)/4.0;
	double gl = cos(p);
	double gr = sin(p);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double l = (*ioSamples)[0]->Get()[i];
        double r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
#endif

#if 0
// Correct formula for balance
// (Le livre des techniques du son - Tome 2 - p227)
//
// Balance do not use pan law
//
// Center position: 0dB atten for both channel
// Extrement positions: (+3dB, -inf)
//
void
USTProcess::Balance(vector<WDL_TypedBuf<double> * > *ioSamples,
                    double balance)
{
    // NOTE: this code increases by 6dB instead of 3dB
    double pp = (balance + 1.0)*0.5;
    
    double coeff = sqrt(2.0); // same as 1.0/DBToAmp(-3.0)
    
    double gL = (1.0 - pp)*coeff;
    double gR = pp*coeff;
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double l = (*ioSamples)[0]->Get()[i];
        double r = (*ioSamples)[1]->Get()[i];
        
        l *= gL;
        r *= gR;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
#endif

#if 0
// Do like Locic and Reaper balance
// No gain increase on the main channel
void
USTProcess::Balance(vector<WDL_TypedBuf<double> * > *ioSamples,
                    double balance)
{
    double gL = 1.0;
    double gR = 1.0;
    
    if (balance < 0.0)
        gR = balance + 1.0;
    
    if (balance > 0.0)
        gL = 1.0 - balance;
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double l = (*ioSamples)[0]->Get()[i];
        double r = (*ioSamples)[1]->Get()[i];
        
        l *= gL;
        r *= gR;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
#endif

#if 0
// See: http://www.rs-met.com/documents/tutorials/PanRules.pdf
// (and maybe): https://www.kvraudio.com/forum/viewtopic.php?t=235347
void
USTProcess::Balance0(vector<WDL_TypedBuf<double> * > *ioSamples,
                     double balance)
{
    //balance = (balance + 1.0)*M_PI*0.5;
    //balance = cos(balance);
    
    double pp = (balance + 1.0)*0.5;

    double gL = 1.0 - pp;
    double gR = pp;
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double l = (*ioSamples)[0]->Get()[i];
        double r = (*ioSamples)[1]->Get()[i];
        
        l *= gL;
        r *= gR;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
#endif

void
USTProcess::Balance3(vector<WDL_TypedBuf<double> * > *ioSamples,
                     double balance)
{
    double p = M_PI*(balance + 1.0)/4.0;
	double gl = cos(p);
	double gr = sin(p);
    
    // TODO: check this
    gl = sqrt(gl);
    gr = sqrt(gr);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double l = (*ioSamples)[0]->Get()[i];
        double r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}

void
USTProcess::MonoToStereo(vector<WDL_TypedBuf<double> > *samplesVec,
                         DelayObj4 *delayObj)
{
    if (samplesVec->empty())
        return;
    
    StereoToMono(samplesVec);
    
    delayObj->ProcessSamples(&(*samplesVec)[1]);
}

void
USTProcess::StereoToMono(vector<WDL_TypedBuf<double> > *samplesVec)
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
        WDL_TypedBuf<double> mono;
        Utils::StereoToMono(&mono, (*samplesVec)[0], (*samplesVec)[1]);
        
        (*samplesVec)[0] = mono;
        (*samplesVec)[1] = mono;
    }
}

#if (!STEREO_WIDEN_SQRT || STEREO_WIDEN_SQRT_CUSTOM)
double
USTProcess::ComputeFactor(double normVal, double maxVal)
{
    double res = 0.0;
    if (normVal < 0.0)
        res = normVal + 1.0;
    else
        res = normVal*(maxVal - 1.0) + 1.0;
    
    return res;
}
#endif

#if STEREO_WIDEN_SQRT
double
USTProcess::ComputeFactor(double normVal, double maxVal)
{
    double res = (normVal + 1.0)*0.5;
    
    return res;
}

#endif

// See: https://www.musicdsp.org/en/latest/Effects/256-stereo-width-control-obtained-via-transfromation-matrix.html?highlight=stereo
// Initial version (goniometer)
#if STEREO_WIDEN_GONIO
void
USTProcess::StereoWiden(double *left, double *right, double widthFactor)
{
   // Initial version (goniometer)
    
    // Calculate scale coefficient
    double coef_S = widthFactor*0.5;
    
    double m = (*left + *right)*0.5;
    double s = (*right - *left )*coef_S;
    
    *left = m - s;
    *right = m + s;
    
#if 0 // First correction of volume (not the best)
    *left  /= 0.5 + coef_S;
    *right /= 0.5 + coef_S;
#endif
}
#endif

// See: https://www.musicdsp.org/en/latest/Effects/256-stereo-width-control-obtained-via-transfromation-matrix.html?highlight=stereo
// Volume adjusted version
// GOOD (loose a bit some volume when increasing width)
#if STEREO_WIDEN_VOL_ADJUSTED
void
USTProcess::StereoWiden(double *left, double *right, double widthFactor)
{
    // Volume adjusted version
    //
    
    // Calc coefs
    double tmp = 1.0/MAX(1.0 + widthFactor, 2.0);
    double coef_M = 1.0 * tmp;
    double coef_S = widthFactor * tmp;
    
    // Then do this per sample
    double m = (*left + *right)*coef_M;
    double s = (*right - *left )*coef_S;
    
    *left = m - s;
    *right = m + s;
}
#endif

// See: https://www.kvraudio.com/forum/viewtopic.php?t=212587
// and: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
#if STEREO_WIDEN_SQRT
void
USTProcess::StereoWiden(double *left, double *right, double widthFactor)
{
    double M = (*left + *right)/sqrt(2.0);   // obtain mid-signal from left and right
    double S = (*left - *right)/sqrt(2.0);   // obtain side-signal from left and right
    
    // amplify mid and side signal seperately:
    M *= 2.0*(1.0 - widthFactor);
    S *= 2.0*widthFactor;
    
    *left = (M + S)/sqrt(2.0);   // obtain left signal from mid and side
    *right = (M - S)/sqrt(2.0);   // obtain right signal from mid and side
}
#endif

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
#if STEREO_WIDEN_SQRT_CUSTOM
void
USTProcess::StereoWiden(double *left, double *right, double widthFactor)
{
    double l = *left;
    double r = *right;
    
    // Samples to polar
    double angle = atan2(r, l);
    double dist = sqrt(l*l + r*r);
    
    // Rotate
    angle -= M_PI/4.0;
    
    // Get x, y coordinates
    double x = dist*cos(angle);
    double y = dist*sin(angle);
    
    // NOTE: sounds is bad with normalization
    // Normalization (1)
    //double rad0 = sqrtf(x*x + y*y);
    
    // Scale over y
    y *= widthFactor;
    
    // Back to polar
    dist = sqrtf(x*x + y*y);
    angle = atan2(y, x);
    
    // Normalization (2)
    //dist = rad0;
    
    // Rotate back
    angle += M_PI/4.0;
    
    // Polar to sample
    l = dist*cos(angle);
    r = dist*sin(angle);
    
    // Result
    *left = l;
    *right = r;
}
#endif
