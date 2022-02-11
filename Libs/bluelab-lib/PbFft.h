//
//  BbFft.h
//  BL-Ghost
//
//  Created by Pan on 09/07/18.
//
//

#ifndef __BL_Ghost__BbFft__
#define __BL_Ghost__BbFft__

#include <BLTypes.h>

class PbFft
{
public:
    typedef struct
    {
        BL_FLOAT real;
        BL_FLOAT imag;
    } COMPLEX;
    
    static int DFT(int dir,int m,BL_FLOAT *x1,BL_FLOAT *y1);
  
    static short FFT(short int dir,long m,BL_FLOAT *x,BL_FLOAT *y);
    
    static int FFT2D(COMPLEX **c,int nx,int ny,int dir);
    
    // Niko
    static int FFT2D(const COMPLEX **inC, COMPLEX **outC, int nx,int ny,int dir);
    
    //
    static int FFT2D(const BL_FLOAT **inD, COMPLEX **outC, int nx,int ny,int dir);
    
    static int FFT2D(const COMPLEX **inC, BL_FLOAT **outD, int nx,int ny,int dir);
    
    static int FFT2D(const BL_FLOAT *inD, COMPLEX *outC, int nx,int ny,int dir);
    
    static int FFT2D(const COMPLEX *inC, BL_FLOAT *outD, int nx,int ny,int dir);
    
protected:
    // NOTE: very similar, maybe identical to FFT() above.
    static int FFTAux(int dir,int m,BL_FLOAT *x,BL_FLOAT *y);
    
    static int Powerof2(int n,int *m,int *twopm);
};


#endif /* defined(__BL_Ghost__BbFft__) */
