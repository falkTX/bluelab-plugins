//
//  Window.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 25/04/17.
//
//

#include <stdlib.h>
#include <math.h>

#include "Window.h"

#ifndef M_PI
#define M_PI 3.141592653589793115997963468544185161590576171875
#endif

void
Window::MakeHanning(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
        hanning[i] = 0.5 * (1.0 - cos(2.0*M_PI * ((double)i) / (size - 1)));
    
    //for (int i = 0; i < size; i++)
    //    hanning[i] *= 2.0/size;
}

void
Window::MakeHanningKernel(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size*size);
    
    double *hanning = result->Get();
    
	double maxDist = sqrt((double)((size / 2)*(size / 2) + (size / 2)*(size / 2)));
    
    for (int j = -size/2; j <= size/2; j++)
        for (int i = -size/2; i <= size/2; i++)
        {
			double dist = 0.5 * (1.0 - (sqrt((double)((i*i + j*j) / maxDist))));
            
            double val = 0.5 * (1.0 - cos(2.0*M_PI * dist));
            hanning[(i + size/2) + (j + size/2)*size] = val;
        }
}

void
Window::MakeRootHanning(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
    {
        hanning[i] = 0.5 * (1.0 - cos(2.0*M_PI * ((double)i) / (size - 1)));
        hanning[i] = 0.5*sqrt(hanning[i]); // 0.5 ? => BUG ?
    }
}

void
Window::MakeRootHanning2(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
    {
        hanning[i] = 0.5 * (1.0 - cos(2.0*M_PI * ((double)i) / (size - 1)));
        hanning[i] = sqrt(hanning[i]);
    }
}

void
Window::MakeRoot2Hanning(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *hanning = result->Get();
    
    for (int i = 0; i < size; i++)
    {
        hanning[i] = 0.5 * (1.0 - cos(2.0*M_PI * ((double)i) / (size - 1)));
        hanning[i] = /*0.5**/sqrt(hanning[i]);
        
        // Make root another time
        // Used a synthesis window, to avoid vertical bars in the spectrum,
        // at each buffer overlap bound
        // Doesn't make the sound vibrate as RootHanning
        
        // Skipped the first 0.5 multiplicator, so we seem to keep the same volume as without plugin
        hanning[i] = sqrt(hanning[i]);
    }
}

#if 1
void
Window::MakeAnalysisHanning(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *hanning = result->Get();

    for (int i = 0; i < size; i++)
        hanning[i] = 0.5 * (1.0 - cos(2.0*M_PI * ((double)i) / (size - 1)));
    
    double sum = 0.0;
    for (int i = 0; i < size; i++)
        sum += hanning[i];
    
    double fract = 2.0 / sum;
    
    for (int i = 0; i < size; i++)
        hanning[i] *= fract;
}

void
Window::MakeSynthesisHanning(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *hanning = result->Get();

    for (int i = 0; i < size; i++)
        hanning[i] = 0.5 * (1.0 - cos(2.0*M_PI * ((double)i) / (size - 1)));
    
    double sum = 0.0;
    for (int i = 0; i < size; i++)
        sum += hanning[i];
    
    double fract = 2.0 / sum;
    
    for (int i = 0; i < size; i++)
        hanning[i] *= fract;
    
    double sum2 = 0.0;
    for (int i = 0; i < size; i++)
        sum2 += hanning[i]*hanning[i];

    double fract2 = 1.0/sum2;
    
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
Window::makehanning( double *H, double *A, double *S, int Nw, int N, int I, int odd )
{
    int i;
    double sum ;
    
    
    if (odd) {
        for ( i = 0 ; i < Nw ; i++ )
            H[i] = A[i] = S[i] = sqrt(0.5 * (1. + cos(PI + TWOPI * i / (Nw - 1))));
    }
	
    else {
        
        for ( i = 0 ; i < Nw ; i++ )
            H[i] = A[i] = S[i] = 0.5 * (1. + cos(PI + TWOPI * i / (Nw - 1)));
        
    }
 	
    if ( Nw > N ) {
        double x ;
        
        x = -(Nw - 1)/2. ;
        for ( i = 0 ; i < Nw ; i++, x += 1. )
            if ( x != 0. ) {
                A[i] *= N*sin( PI*x/N )/(PI*x) ;
                if ( I )
                    S[i] *= I*sin( PI*x/I )/(PI*x) ;
            }
    }
    for ( sum = i = 0 ; i < Nw ; i++ )
        sum += A[i] ;
    
    for ( i = 0 ; i < Nw ; i++ ) {
        double afac = 2./sum ;
        double sfac = Nw > N ? 1./afac : afac ;
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
Window::MakeAnalysisHanning(int size, WDL_TypedBuf<double> *result)
{
    WDL_TypedBuf<double> hanningBuf;
    hanningBuf.result(size);
    double *hanning = hanningBuf.Get();
    
    WDL_TypedBuf<double> ahanningBuf;
    ahanningBuf.result(size);
    double *ahanning = ahanningBuf.Get();
    
    WDL_TypedBuf<double> shanningBuf;
    shanningBuf.result(size);
    double *shanning = shanningBuf.Get();
    
    makehanning(hanning, ahanning, shanning, size, size, 0, 0);
    
    *result = ahanningBuf;
}

void
Window::MakeSynthesisHanning(int size, WDL_TypedBuf<double> *result)
{
    WDL_TypedBuf<double> hanningBuf;
    hanningBuf.result(size);
    double *hanning = hanningBuf.Get();
    
    WDL_TypedBuf<double> ahanningBuf;
    ahanningBuf.result(size);
    double *ahanning = ahanningBuf.Get();
    
    WDL_TypedBuf<double> shanningBuf;
    shanningBuf.result(size);
    double *shanning = shanningBuf.Get();
    
    makehanning(hanning, ahanning, shanning, size, size, 0, 0);
    
    *result = shanningBuf;
}
#endif

void
Window::MakeTriangular(int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *triangular = result->Get();

    
    for (int i = 0; i < size; i++)
    {
        if (i < size/2)
            triangular[i] = i/(double)(size/2);
        else
            triangular[i] = (size - i)/(double)(size/2);
    }
}

double
Window::Gaussian(double sigma, double x)
{
    double result = (1.0/((sqrt(2.0*M_PI)*sigma)))*exp(-x*x/(2.0*sigma*sigma));
    
    return result;
}

void
Window::MakeGaussian(double sigma, int size, WDL_TypedBuf<double> *result)
{
    result->Resize(size);
    double *gaussian = result->Get();

    for (int i = 0; i < size; i++)
    {
        double t = ((((double)i)/size)*2.0 - 1.0);
        
        // change t to cover the "whole" domain of a gaussian with sigma == 1
        t *= 4.0;
        
        gaussian[i] = Gaussian(sigma, t);
    }
}