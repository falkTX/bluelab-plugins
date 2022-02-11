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

#define MAX_WIDTH_CHANGE 8.0

// FIX: On Protools, when width parameter was reset to 0,
// by using alt + click, the gain was increased compared to bypass
//
// On Protools, when alt + click to reset parameter,
// Protools manages all, and does not reset the parameter exactly to the default value
// (precision 1e-8).
// Then the coeff is not exactly 1, then the gain coeff is applied.
#define FIX_WIDTH_PRECISION 1
#define WIDTH_PARAM_PRECISION 1e-6

#define TEST_NIKO_ADJUST_WIDTH_VOLUME 0

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
USTProcess::StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples, double widthChange)
{
    double width = ComputeFactor(widthChange, MAX_WIDTH_CHANGE);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double in_left = (*ioSamples)[0]->Get()[i];
        double in_right = (*ioSamples)[1]->Get()[i];
        
#if 0 // Initial version (goniometer)
        
        // Calculate scale coefficient
        double coef_S = width*0.5;
        
        double m = (in_left + in_right)*0.5;
        double s = (in_right - in_left )*coef_S;
        
        double out_left = m - s;
        double out_right = m + s;
        
#if 0 // First correction of volume (not the best)
        out_left  /= 0.5 + coef_S;
        out_right /= 0.5 + coef_S;
#endif
        
#endif
        
        // GOOD (loose a bit some volume when increasing width)
#if 1 // Volume adjusted version
        // Calc coefs
        double tmp = 1.0/MAX(1.0 + width, 2.0);
        double coef_M = 1.0 * tmp;
        double coef_S = width * tmp;
        
        // Then do this per sample
        double m = (in_left + in_right)*coef_M;
        double s = (in_right - in_left )*coef_S;
        
        double out_left = m - s;
        double out_right = m + s;
#endif
        
#if TEST_NIKO_ADJUST_WIDTH_VOLUME // Test Niko (adjust final volume)
        if (width > 1.0)
        {
            // Works well with MAX_WIDTH = 4
            // Makes a small volume increase near 1,
            // just after 1
            double coeff = 1.3;
            
            out_left *= coeff;
            out_right *= coeff;
        }
#endif
        
        (*ioSamples)[0]->Get()[i] = out_left;
        (*ioSamples)[1]->Get()[i] = out_right;
    }
}

#if 0
void
USTProcess::StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples,
                        const USTWidthAdjuster *widthAdjuster)
{
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double widthChange = widthAdjuster->GetWidth();
        double width = ComputeFactor(widthChange, MAX_WIDTH_CHANGE);
        
#if FIX_WIDTH_PRECISION
        width /= WIDTH_PARAM_PRECISION;
        width = round(width);
        width *= WIDTH_PARAM_PRECISION;
#endif
        
        double in_left = (*ioSamples)[0]->Get()[i];
        double in_right = (*ioSamples)[1]->Get()[i];
        
        // GOOD (loose a bit some volume when increasing width)
#if 1 // Volume adjusted version
        // Calc coefs
        double tmp = 1.0/MAX(1.0 + width, 2.0);
        double coef_M = 1.0 * tmp;
        double coef_S = width * tmp;
        
        // Then do this per sample
        double m = (in_left + in_right)*coef_M;
        double s = (in_right - in_left )*coef_S;
        
        double out_left = m - s;
        double out_right = m + s;
#endif
        
#if TEST_NIKO_ADJUST_WIDTH_VOLUME // Test Niko (adjust final volume)
        if (width > 1.0)
        {
            // Works well with MAX_WIDTH = 4
            // Makes a small volume increase near 1,
            // just after 1
            double coeff = 1.3;
            
            out_left *= coeff;
            out_right *= coeff;
        }
#endif
        
        (*ioSamples)[0]->Get()[i] = out_left;
        (*ioSamples)[1]->Get()[i] = out_right;
    }
}
#endif

void
USTProcess::StereoWiden(vector<WDL_TypedBuf<double> * > *ioSamples,
                        const USTWidthAdjuster *widthAdjuster)
{
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    //
    // https://www.musicdsp.org/en/latest/Effects/256-stereo-width-control-obtained-via-transfromation-matrix.html?highlight=stereo
    
    double widthChange = widthAdjuster->GetWidth();
    double width = ComputeFactor(widthChange, MAX_WIDTH_CHANGE);
    
    fprintf(stderr, "width: %g\n", width);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        double in_left = (*ioSamples)[0]->Get()[i];
        double in_right = (*ioSamples)[1]->Get()[i];
        
        // GOOD (loose a bit some volume when increasing width)
#if 1
        // Volume adjusted version
        //
        
        // Calc coefs
        double tmp = 1.0/MAX(1.0 + width, 2.0);
        double coef_M = 1.0 * tmp;
        double coef_S = width * tmp;
        
        // Then do this per sample
        double m = (in_left + in_right)*coef_M;
        double s = (in_right - in_left )*coef_S;
        
        double out_left = m - s;
        double out_right = m + s;
#endif
        
        (*ioSamples)[0]->Get()[i] = out_left;
        (*ioSamples)[1]->Get()[i] = out_right;
    }
}

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

void
USTProcess::Balance3(vector<WDL_TypedBuf<double> * > *ioSamples,
                     double balance)
{
    double p= M_PI*(balance + 1.0)/4.0;
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
