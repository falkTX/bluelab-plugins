//
//  USTProcess.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include <Utils.h>
#include <USTWidthAdjuster2.h>
#include <DelayObj4.h>

#include <Debug.h>

#include <FftProcessObj16.h>

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
#define CLIP_KEEP     1

// Lissajous
#define LISSAJOUS_CLIP_DISTANCE 1.0

// Polar levels
#define POLAR_LEVELS_NUM_BINS 64.0 //128.0
#define POLAR_LEVEL_SMOOTH_FACTOR 0.8 //0.9 //0.99
#define POLAR_LEVEL_MAX_SMOOTH_FACTOR 0.99

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

#define SQR2_INV 0.70710678118655


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
#if !CLIP_KEEP
            // Discard
            polarSamples[0].Get()[i] = -1.0;
            polarSamples[1].Get()[i] = -1.0;
            
            continue;
#else
            // Keep in the border
            dist = CLIP_DISTANCE;
#endif
        }
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
#if 0
        static double minAngle = 1000.0;
        if (angle < minAngle)
            minAngle = angle;
        static double maxAngle = -1000.0;
        if (angle > maxAngle)
            maxAngle = angle;
#endif
        
        // Compute (x, y)
        double x = dist*cos(angle);
        double y = dist*sin(angle);

#if 1 // NOTE: seems good !
        // Out of view samples add them by central symetry
        // Result: increase the directional perception (e.g when out of phase)
        if (y < 0.0)
        {
            x = -x;
            y = -y;
        }
#endif
        
        polarSamples[0].Get()[i] = x;
        polarSamples[1].Get()[i] = y;
    }
}

void
USTProcess::ComputePolarLevel(const WDL_TypedBuf<double> samples[2],
                              WDL_TypedBuf<double> polarLevelSamples[2],
                              WDL_TypedBuf<double> *prevLevels, bool smoothMinMax)
{
#define TEST_FFT 0
    
    // Accumulate levels depending on the angles
    WDL_TypedBuf<double> levels;
    levels.Resize(POLAR_LEVELS_NUM_BINS);
    Utils::FillAllZero(&levels);
    
    WDL_TypedBuf<double> levelSums;
    levelSums.Resize(POLAR_LEVELS_NUM_BINS);
    Utils::FillAllZero(&levelSums);
    
#if TEST_FFT
    FftProcessObj16::Init();
    
    WDL_TypedBuf<double> magns[2];
    WDL_TypedBuf<double> phases[2];
    FftProcessObj16::SamplesToMagnPhases(samples[0], &magns[0], &phases[0]);
    FftProcessObj16::SamplesToMagnPhases(samples[1], &magns[1], &phases[1]);
#endif

    for (int i = 0; i < samples[0].GetSize(); i++)
    {
#if !TEST_FFT
        double l = samples[0].Get()[i];
        double r = samples[1].Get()[i];
        
        double dist = sqrt(l*l + r*r);
        
        double angle = atan2(r, l);
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
#endif

#if TEST_FFT
        // Process only the first half, to avoid symetry
        if (i > samples[0].GetSize()/2)
            break;
        
        double angle = phases[0].Get()[i] - phases[1].Get()[i];
        double dist = sqrt(magns[0].Get()[i]*magns[0].Get()[i] +
                           magns[1].Get()[i]*magns[1].Get()[i]);
        
        angle += M_PI*0.5;
        
        angle = M_PI - angle;
#endif
        
        // Bound to [-Pi, Pi]
        angle = fmod(angle, 2.0*M_PI);
        if (angle < 0.0)
            angle += 2.0*M_PI;
        
        angle -= M_PI;
        
#if 1 // Looks better without
        if (angle < 0.0)
            angle += M_PI;
#endif
        
        // TEST
        //if (angle < 0.0)
        //    angle = M_PI - angle;
        
        //angle = ApplyAngleShape(angle, 2.0);
        
        // TEST
        int binNum = (angle/M_PI)*(POLAR_LEVELS_NUM_BINS - 1);
        
        if (binNum < 0)
            binNum = 0;
        if (binNum > POLAR_LEVELS_NUM_BINS - 1)
            binNum = POLAR_LEVELS_NUM_BINS - 1;
        
        levels.Get()[binNum] += dist;
        
        levelSums.Get()[binNum] = levelSums.Get()[binNum] + 1;;
    }
    
    // Rescale
#if !TEST_FFT
    double scaleFactor = ((double)levels.GetSize())/samples[0].GetSize();
#endif
    
#if TEST_FFT
    double scaleFactor = 2.0;
#endif
    
#if 1
    // Make average by bin
    for (int i = 0; i < levels.GetSize(); i++)
    {
        double level = levels.Get()[i];
        int levelSum = levelSums.Get()[i];
        
        if (levelSum > 0)
            level /= levelSum;
        
        levels.Get()[i] = level;
    }
    
    // Avoid scale fator
    scaleFactor = 1.0;
#endif
    
    for (int i = 0; i < levels.GetSize(); i++)
    {
        double level = levels.Get()[i];
        
        level *= scaleFactor;
        
        levels.Get()[i] = level;
    }
    
    // Clip
    for (int i = 0; i < levels.GetSize(); i++)
    {
        double level = levels.Get()[i];
        
        // Clip
        if (level > CLIP_DISTANCE)
        // Point outside the circle
        {
            // Keep in the border
            level = CLIP_DISTANCE;
        }
        
        levels.Get()[i] = level;
    }
    
    if (prevLevels != NULL)
    {
        if (prevLevels->GetSize() != levels.GetSize())
        {
            *prevLevels = levels;
        }
        else
        {
            if (!smoothMinMax)
                Utils::Smooth(&levels, prevLevels, POLAR_LEVEL_SMOOTH_FACTOR);
            else
                Utils::SmoothMax(&levels, prevLevels, POLAR_LEVEL_MAX_SMOOTH_FACTOR);
        }
    }
    
    // Convert to (x, y)
    polarLevelSamples[0].Resize(samples[0].GetSize());
    Utils::FillAllZero(&polarLevelSamples[0]);
    
    polarLevelSamples[1].Resize(samples[0].GetSize());
    Utils::FillAllZero(&polarLevelSamples[1]);
    
    for (int i = 0; i < levels.GetSize(); i++)
    {
        
        double angle = (((double)i)/(POLAR_LEVELS_NUM_BINS - 1))*M_PI;
        double dist = levels.Get()[i];
        
        //angle = ApplyAngleShape(angle, 2.0);
        
        // Compute (x, y)
        double x = dist*cos(angle);
        double y = dist*sin(angle);
        
        polarLevelSamples[0].Get()[i] = x;
        polarLevelSamples[1].Get()[i] = y;
    }
}

void
USTProcess::ComputeLissajous(const WDL_TypedBuf<double> samples[2],
                             WDL_TypedBuf<double> lissajousSamples[2],
                             bool fitInSquare)
{
    lissajousSamples[0].Resize(samples[0].GetSize());
    lissajousSamples[1].Resize(samples[0].GetSize());
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        double l = samples[0].Get()[i];
        double r = samples[1].Get()[i];
        
        if (fitInSquare)
        {
            if (l < -LISSAJOUS_CLIP_DISTANCE)
                l = -LISSAJOUS_CLIP_DISTANCE;
            if (l > LISSAJOUS_CLIP_DISTANCE)
                l = LISSAJOUS_CLIP_DISTANCE;
            
            if (r < -LISSAJOUS_CLIP_DISTANCE)
                r = -LISSAJOUS_CLIP_DISTANCE;
            if (r > LISSAJOUS_CLIP_DISTANCE)
                r = LISSAJOUS_CLIP_DISTANCE;
        }
        
        // Rotate
        double angle = atan2(r, l);
        double dist = sqrt(l*l + r*r);
        
        if (fitInSquare)
        {
            double coeff = SQR2_INV;
            dist *= coeff;
        }
        
        angle = -angle;
        angle -= M_PI/4.0;
        
        double x = dist*cos(angle);
        double y = dist*sin(angle);
        
#if 0 // Keep [-1, 1] bounds
        // Center
        x = (x + 1.0)*0.5;
        y = (y + 1.0)*0.5;
#endif
        
        lissajousSamples[0].Get()[i] = x;
        lissajousSamples[1].Get()[i] = y;
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
        if (denom2 > 0.0)
        {
            double denom = sqrt(denom2);
        
            correl = num/denom;
        }
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

// With width adjuster
void
USTProcess::StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples,
                        USTWidthAdjuster2 *widthAdjuster)
{
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        // Get current width
        widthAdjuster->Update();
        double widthFactor = widthAdjuster->GetLimitedWidth();
        double width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
        
#if FIX_WIDTH_PRECISION
        width /= WIDTH_PARAM_PRECISION;
        width = round(width);
        width *= WIDTH_PARAM_PRECISION;
#endif
        
        // Compute
        double left = (*ioSamples)[0]->Get()[i];
        double right = (*ioSamples)[1]->Get()[i];
        
        StereoWiden(&left, &right, width);
        
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

// TODO: remove this
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

#if 0 // "Haas effect" method
      // Problem: the sound is panned
void
USTProcess::MonoToStereo(vector<WDL_TypedBuf<double> > *samplesVec,
                         DelayObj4 *delayObj)
{
    if (samplesVec->empty())
        return;
    
    StereoToMono(samplesVec);
    
    delayObj->ProcessSamples(&(*samplesVec)[1]);
}
#endif

// See: http://www.native-instruments.com/forum/attachments/stereo-solutions-pdf.13883
// and: http://www.csounds.com/journal/issue14/PseudoStereo.html
//
// Result:
// - almost same level (mono2stereo, then out mono => the level is the same as mono only)
// - result seems like IZotope Ozone Imager
//
// NOTE: there are some improved algorithms in the paper
void
USTProcess::MonoToStereo(vector<WDL_TypedBuf<double> > *samplesVec,
                         DelayObj4 *delayObj)
{
    if (samplesVec->empty())
        return;
    
    StereoToMono(samplesVec);
    
    // Lauridsen
    
    WDL_TypedBuf<double> delayMono = (*samplesVec)[0];
    delayObj->ProcessSamples(&delayMono);
    
    // L = mono + delayMono
    Utils::AddValues(&(*samplesVec)[0], delayMono);
    
    // R = delayMono - mono
    Utils::MultValues(&(*samplesVec)[1], -1.0);
    Utils::AddValues(&(*samplesVec)[1], delayMono);
    
    // Adjust the width by default
#define LAURIDSEN_WIDTH_ADJUST -0.70
    vector<WDL_TypedBuf<double> * > samples;
    samples.push_back(&(*samplesVec)[0]);
    samples.push_back(&(*samplesVec)[1]);
    
    USTProcess::StereoWiden(&samples, LAURIDSEN_WIDTH_ADJUST);
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

double
USTProcess::ApplyAngleShape(double angle, double shape)
{
    // Angle is in [0, Pi]
    double angleShape = (angle - M_PI*0.5)/(M_PI*0.5);
    
    bool invert = false;
    if (angleShape < 0.0)
    {
        angleShape = -angleShape;
        
        invert = true;
    }
    
    angleShape = Utils::ApplyParamShape(angleShape, shape);
    
    if (invert)
        angleShape = -angleShape;
    
    angle = angleShape*M_PI*0.5 + M_PI*0.5;

    return angle;
}
