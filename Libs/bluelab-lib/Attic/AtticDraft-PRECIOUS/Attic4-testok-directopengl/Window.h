//
//  Window.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 25/04/17.
//
//

#ifndef __Denoiser__Window__
#define __Denoiser__Window__

#include "../../WDL/IPlug/Containers.h"

class Window
{
public:
    static void MakeHanning(int size, WDL_TypedBuf<double> *result);
    
    // 2D window
    static void MakeHanningKernel(int size, WDL_TypedBuf<double> *result);
    
    static void MakeRootHanning(int size, WDL_TypedBuf<double> *result);
    
    static void MakeRootHanning2(int size, WDL_TypedBuf<double> *result);
    
    static void MakeRoot2Hanning(int size, WDL_TypedBuf<double> *result);
    
    static void MakeAnalysisHanning(int size, WDL_TypedBuf<double> *result);
    
    static void MakeSynthesisHanning(int size, WDL_TypedBuf<double> *result);
    
    static void MakeTriangular(int size, WDL_TypedBuf<double> *result);
    
    static double Gaussian(double sigma, double x);
    
    static void MakeGaussian(double sigma, int size, WDL_TypedBuf<double> *result);
    
protected:
    static void makehanning( double *H, double *A, double *S, int Nw, int N, int I, int odd );
};

#endif /* defined(__Denoiser__Window__) */
