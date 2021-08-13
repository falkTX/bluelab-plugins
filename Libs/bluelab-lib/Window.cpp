//
//  Window.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 25/04/17.
//
//

#include <stdlib.h>
#include <math.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "Window.h"

#ifndef M_PI
#define M_PI 3.141592653589793115997963468544185161590576171875
#endif

#if 1 // ORIG
void
Window::MakeHanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
      hanning[i] = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * ((BL_FLOAT)i) / (size - 1))));
}
#endif

// NIKO-TESTS
#if 0
// old version
void
Window::MakeHanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *hanning = result->Get();
    
    for (int i = 0; i < size - 1; i++)
      hanning[i] = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * ((BL_FLOAT)i) / (size - 2))));
    
    hanning[size - 1] = 0.0;
}
#endif

// Buggy
void
Window::MakeHanningKernel(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size*size);
    
    BL_FLOAT *hanning = result->Get();
    
    BL_FLOAT maxDist = std::sqrt((BL_FLOAT)((size / 2)*(size / 2) + (size / 2)*(size / 2)));
    
    for (int j = -size/2; j <= size/2; j++)
        for (int i = -size/2; i <= size/2; i++)
        {
	  BL_FLOAT dist = 0.5 * (1.0 - (std::sqrt((BL_FLOAT)((i*i + j*j) / maxDist))));
            
	  BL_FLOAT val = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * dist)));
            hanning[(i + size/2) + (j + size/2)*size] = val;
        }
}

// Fixed
void
Window::MakeHanningKernel2(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size*size);
    
    BL_FLOAT *hanning = result->Get();
    
	BL_FLOAT maxDist = size/2 + 1;
    
    for (int j = -size/2; j <= size/2; j++)
        for (int i = -size/2; i <= size/2; i++)
        {
	  BL_FLOAT dist = std::sqrt((BL_FLOAT)(i*i + j*j));
            if (dist > maxDist)
                continue;
            
            BL_FLOAT val = std::cos((BL_FLOAT)(0.5*M_PI * dist/maxDist));
            hanning[(i + size/2) + (j + size/2)*size] = val;
        }
}

void
Window::MakeRootHanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
    {
      hanning[i] = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * ((BL_FLOAT)i) / (size - 1))));
      hanning[i] = 0.5*std::sqrt(hanning[i]); // 0.5 ? => BUG ?
    }
}

void
Window::MakeRootHanning2(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
    {
      hanning[i] = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * ((BL_FLOAT)i) / (size - 1))));
      hanning[i] = std::sqrt(hanning[i]);
    }
}

void
Window::MakeRoot2Hanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
    {
      hanning[i] = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * ((BL_FLOAT)i) / (size - 1))));
      hanning[i] = /*0.5**/std::sqrt(hanning[i]);
        
        // Make root another time
        // Used a synthesis window, to avoid vertical bars in the spectrum,
        // at each buffer overlap bound
        // Doesn't make the sound vibrate as RootHanning
        
        // Skipped the first 0.5 multiplicator, so we seem to keep the same volume as without plugin
      hanning[i] = std::sqrt(hanning[i]);
    }
}

#if 1
void
Window::MakeAnalysisHanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *hanning = result->Get();

    for (int i = 0; i < size; i++)
      hanning[i] = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * ((BL_FLOAT)i) / (size - 1))));
    
    BL_FLOAT sum = 0.0;
    for (int i = 0; i < size; i++)
        sum += hanning[i];
    
    BL_FLOAT fract = 2.0 / sum;
    
    for (int i = 0; i < size; i++)
        hanning[i] *= fract;
}

void
Window::MakeSynthesisHanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *hanning = result->Get();

    for (int i = 0; i < size; i++)
      hanning[i] = 0.5 * (1.0 - std::cos((BL_FLOAT)(2.0*M_PI * ((BL_FLOAT)i) / (size - 1))));
    
    BL_FLOAT sum = 0.0;
    for (int i = 0; i < size; i++)
        sum += hanning[i];
    
    BL_FLOAT fract = 2.0 / sum;
    
    for (int i = 0; i < size; i++)
        hanning[i] *= fract;
    
    BL_FLOAT sum2 = 0.0;
    for (int i = 0; i < size; i++)
        sum2 += hanning[i]*hanning[i];

    BL_FLOAT fract2 = 1.0/sum2;
    
    for (int i = 0; i < size; i++)
        hanning[i] *= fract2;
}
#endif

#if 0

#ifndef PI
#define PI 3.141592653589793115997963468544185161590576171875
#endif

#ifndef TWOPI
#define TWOPI 6.28318530717958623199592693708837032318115234375
#endif

void
Window::makehanning( BL_FLOAT *H, BL_FLOAT *A, BL_FLOAT *S, int Nw, int N, int I, int odd )
{
    int i;
    BL_FLOAT sum ;
    
    
    if (odd) {
        for ( i = 0 ; i < Nw ; i++ )
	  H[i] = A[i] = S[i] = std::sqrt((BL_FLOAT)(0.5 * (1. + std::cos((BL_FLOAT)(PI + TWOPI * i / (Nw - 1))))));
    }
	
    else {
        
        for ( i = 0 ; i < Nw ; i++ )
	  H[i] = A[i] = S[i] = 0.5 * (1. + std::cos((BL_FLOAT)(PI + TWOPI * i / (Nw - 1))));
        
    }
 	
    if ( Nw > N ) {
        BL_FLOAT x ;
        
        x = -(Nw - 1)/2. ;
        for ( i = 0 ; i < Nw ; i++, x += 1. )
            if ( x != 0. ) {
	      A[i] *= N*std::sin( (BL_FLOAT)(PI*x/N )/(PI*x) ;
                if ( I )
		  S[i] *= I*std::sin( (BL_FLOAT)(PI*x/I) )/(PI*x) ;
            }
    }
    for ( sum = i = 0 ; i < Nw ; i++ )
        sum += A[i] ;
    
    for ( i = 0 ; i < Nw ; i++ ) {
        BL_FLOAT afac = 2./sum ;
        BL_FLOAT sfac = Nw > N ? 1./afac : afac ;
        A[i] *= afac ;
        S[i] *= sfac ;
    }
    
    if ( Nw <= N && I ) {
        for ( sum = i = 0 ; i < Nw ; i += I )
            sum += S[i]*S[i] ;
        for ( sum = 1./sum, i = 0 ; i < Nw ; i++ )
            S[i] *= sum ;
    }
}

void
Window::MakeAnalysisHanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    WDL_TypedBuf<BL_FLOAT> hanningBuf;
    hanningBuf.result(size);
    BL_FLOAT *hanning = hanningBuf.Get();
    
    WDL_TypedBuf<BL_FLOAT> ahanningBuf;
    ahanningBuf.result(size);
    BL_FLOAT *ahanning = ahanningBuf.Get();
    
    WDL_TypedBuf<BL_FLOAT> shanningBuf;
    shanningBuf.result(size);
    BL_FLOAT *shanning = shanningBuf.Get();
    
    makehanning(hanning, ahanning, shanning, size, size, 0, 0);
    
    *result = ahanningBuf;
}

void
Window::MakeSynthesisHanning(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    WDL_TypedBuf<BL_FLOAT> hanningBuf;
    hanningBuf.result(size);
    BL_FLOAT *hanning = hanningBuf.Get();
    
    WDL_TypedBuf<BL_FLOAT> ahanningBuf;
    ahanningBuf.result(size);
    BL_FLOAT *ahanning = ahanningBuf.Get();
    
    WDL_TypedBuf<BL_FLOAT> shanningBuf;
    shanningBuf.result(size);
    BL_FLOAT *shanning = shanningBuf.Get();
    
    makehanning(hanning, ahanning, shanning, size, size, 0, 0);
    
    *result = shanningBuf;
}
#endif

void
Window::MakeTriangular(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *triangular = result->Get();

    for (int i = 0; i < size; i++)
    {
        if (i < size/2)
            triangular[i] = i/(BL_FLOAT)(size/2);
        else
            triangular[i] = (size - i)/(BL_FLOAT)(size/2);
    }
}

// This version seems buggy...
BL_FLOAT
Window::Gaussian(BL_FLOAT sigma, BL_FLOAT x)
{
    BL_FLOAT result = (1.0/((sqrt((BL_FLOAT)(2.0*M_PI))*sigma)))*exp((BL_FLOAT)(-x*x/(2.0*sigma*sigma)));
    
    return result;
}

// This version is good!
// (Tested with SASViewer, and FftProcessObj16 for ana and resynth)
//
// The peak of the gaussian has a value of 1
// See: https://en.wikipedia.org/wiki/Window_function#Gaussian_window
static BL_FLOAT
_gaussian(int i, int N, BL_FLOAT sigma)
{
    BL_FLOAT a = (i - N/2)/(sigma*N/2);
    BL_FLOAT g = exp(-0.5*a*a);
        
    return g;
}

// This version is good!
// (Tested with SASViewer, and FftProcessObj16 for ana and resynth)
void
Window::MakeGaussian(int size, BL_FLOAT sigma, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *gaussian = result->Get();

    int N = size;
    for (int i = 0; i < size; i++)
    {
        gaussian[i] = _gaussian(i, N, sigma);
    }
}

// See: https://en.wikipedia.org/wiki/Window_function#Gaussian_window
// "Approximate confined Gaussian window"
//
// And: https://www.recordingblogs.com/wiki/gaussian-window
void
Window::MakeGaussianConfined(int size, BL_FLOAT sigma,
                             WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *gaussian = result->Get();

    int N = size;
    for (int i = 0; i < N; i++)
    {
        BL_FLOAT L = N;
        
        BL_FLOAT gn = _gaussian(i, N, sigma);
        BL_FLOAT gm05 = _gaussian(-0.5, N, sigma);
        BL_FLOAT gnpL = _gaussian(i + L, N, sigma);
        BL_FLOAT gnmL = _gaussian(i - L, N, sigma);
        BL_FLOAT gm05pL = _gaussian(-0.5 + L, N, sigma);
        BL_FLOAT gm05mL = _gaussian(-0.5 - L, N, sigma);

        BL_FLOAT w = gn - (gm05*(gnpL + gnmL))/(gm05pL + gm05mL);

        gaussian[i] = w;
    }
}

// Not used anymore
void
Window::MakeGaussianWholeDomain(int size, BL_FLOAT sigma,
                                WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    BL_FLOAT *gaussian = result->Get();

    for (int i = 0; i < size; i++)
    {
        BL_FLOAT t = ((((BL_FLOAT)i)/size)*2.0 - 1.0);
        
        // Change t to cover the "whole" domain of a gaussian with sigma == 1
        t *= 4.0;
        
        gaussian[i] = Gaussian(sigma, t);
    }
}

// FIXED !
// The function above seems false
void
Window::MakeGaussian2(int size, BL_FLOAT sigma, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    
    for (int i = 0; i < size; i++)
    {
        BL_FLOAT val0 = ((i - (size-1)/2.0)/(2.0*size*sigma));
        BL_FLOAT val = std::exp(-val0*val0);
        
        result->Get()[i] = val;
    }
}


#define DUMP_COLA 0
#if DUMP_COLA
#include "Debug.h"
#endif

BL_FLOAT
Window::CheckCOLA(const WDL_TypedBuf<BL_FLOAT> *window,
                  int overlap, BL_FLOAT outTimeStretchFactor)
{
#if DUMP_COLA
    BLDebug::DumpData("win.txt", window->Get(), window->GetSize());
#endif
    
    BL_FLOAT winSum = 0.0;
    for (int i = 0; i < window->GetSize(); i++)
    {
        BL_FLOAT val = window->Get()[i];
        winSum += val;
    }
    
    int shift = ((BL_FLOAT)window->GetSize())/overlap;

    shift = bl_round(shift*outTimeStretchFactor);
                     
    WDL_TypedBuf<BL_FLOAT> sum;
    sum.Resize(window->GetSize());
    for (int i = 0; i < window->GetSize(); i++)
        sum.Get()[i] = 0.0;
    
    for (int j = 0; j < window->GetSize(); j += shift)
    {
#if DUMP_COLA
        // Marker
#endif
        for (int i = 0; i < window->GetSize(); i++)
        {
            int index = (i + j) % window->GetSize();
            
            BL_FLOAT val = window->Get()[index];
            sum.Get()[i] += val;
            
#if DUMP_COLA
            //BLDebug::BLDebug::DumpSingleValue("ola.txt", val);
#endif
        }
    }
    
#if DUMP_COLA
    BLDebug::DumpData("ola-sum.txt", sum.Get(), sum.GetSize());
#endif

    // Check that the cola condition is respected
#define COLA_EPS 1e-3
    BL_FLOAT avg = BLUtils::ComputeAvg(sum.Get(), sum.GetSize());

    if (fabs(outTimeStretchFactor - 1.0) < BL_EPS)
        // Check COLA only if outTimeStretchFactor is 1.0
    {
        for (int i = 0; i < sum.GetSize(); i++)
        {
            BL_FLOAT val = sum.Get()[i];
            if (std::fabs(val - avg) > COLA_EPS)
                return -1.0;
        }
    }
    
    return avg;
}

void
Window::MakeHanningPow(int size, BL_FLOAT factor, WDL_TypedBuf<BL_FLOAT> *result)
{
    MakeHanning(size, result);
                
    for (int i = 0; i < size; i++)
    {
        BL_FLOAT val = result->Get()[i];
        val = std::pow(val, factor);
        
        result->Get()[i] = val;
    }
}

void
Window::MakeSquare(int size, BL_FLOAT value, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    for (int i = 0; i < size; i++)
    {
        result->Get()[i] = value;
    }
}

void
Window::MakeNormSinc(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    for (int i = 0; i < size; i++)
    {
        int x = i - size/2;
        BL_FLOAT val = 1.0;
        if (x != 0)
            // For x == 0, sinc = 1
        {
	  val = std::sin((BL_FLOAT)(M_PI*x))/((BL_FLOAT)(M_PI*x));
        }
        
        result->Get()[i] = val;
    }
}

void
Window::MakeNormSincFilter(int size, BL_FLOAT fcSr, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    for (int i = 0; i < size; i++)
    {
        int x = i - size/2;
        BL_FLOAT val = 1.0;
        if (x != 0)
            // For x == 0, sinc = 1
        {
            BL_FLOAT xx = x*2.0*fcSr;
            
            val = std::sin((BL_FLOAT)(M_PI*xx))/(M_PI*xx);
        }
        
        val *= 2.0*fcSr;
        
        result->Get()[i] = val;
    }
}

void
Window::MakeBlackman(int size, WDL_TypedBuf<BL_FLOAT> *result)
{
    result->Resize(size);
    for (int i = 0; i < size; i++)
    {
      BL_FLOAT wn = 0.42 - 0.5*std::cos((BL_FLOAT)((2.0*M_PI*i)/(size - 1))) + 0.08*std::cos((BL_FLOAT)((4.0*M_PI*i)/(size - 1)));
        result->Get()[i] = wn;
    }
}

void
Window::NormalizeWindow(WDL_TypedBuf<BL_FLOAT> *window, int oversampling,
                        BL_FLOAT outTimeStretchFactor)
{
    BL_FLOAT colaCoeff = CheckCOLA(window, oversampling, outTimeStretchFactor);
    
    NormalizeWindow(window, colaCoeff);
}

void
Window::NormalizeWindow(WDL_TypedBuf<BL_FLOAT> *window, BL_FLOAT factor)
{
    for (int i = 0; i < window->GetSize(); i++)
    {
        BL_FLOAT value = window->Get()[i];
        
        value /= factor;
        
        window->Get()[i] = value;
    }
}

void
Window::Apply(const WDL_TypedBuf<BL_FLOAT> &window, WDL_TypedBuf<BL_FLOAT> *buf)
{
    if (buf->GetSize() != window.GetSize())
        return;
    
    for (int i = 0; i < buf->GetSize(); i++)
    {
        BL_FLOAT val = buf->Get()[i];
        BL_FLOAT w = window.Get()[i];
        
        val *= w;
        
        buf->Get()[i] = val;
    }
}
