//
//  BbFft.cpp
//  BL-Ghost
//
//  Created by Pan on 09/07/18.
//
//

#include <stdlib.h>
#include <math.h>
#include <cmath>

#include "PbFft.h"

// From http://paulbourke.net/miscellaneous/dft/

#define TRUE 1
#define FALSE 0

/*
 Direct fourier transform
 */
int
PbFft::DFT(int dir,int m,BL_FLOAT *x1,BL_FLOAT *y1)
{
    long i,k;
    BL_FLOAT arg;
    BL_FLOAT cosarg,sinarg;
    BL_FLOAT *x2=NULL,*y2=NULL;
    
    x2 = (BL_FLOAT *)malloc(m*sizeof(BL_FLOAT));
    y2 = (BL_FLOAT *)malloc(m*sizeof(BL_FLOAT));
    if (x2 == NULL || y2 == NULL)
        return(FALSE);
    
    for (i=0;i<m;i++) {
        x2[i] = 0;
        y2[i] = 0;
        arg = - dir * 2.0 * 3.141592654 * (BL_FLOAT)i / (BL_FLOAT)m;
        for (k=0;k<m;k++) {
	  cosarg = std::cos(k * arg);
	  sinarg = std::sin(k * arg);
            x2[i] += (x1[k] * cosarg - y1[k] * sinarg);
            y2[i] += (x1[k] * sinarg + y1[k] * cosarg);
        }
    }
    
    /* Copy the data back */
    if (dir == 1) {
        for (i=0;i<m;i++) {
            x1[i] = x2[i] / (BL_FLOAT)m;
            y1[i] = y2[i] / (BL_FLOAT)m;
        }
    } else {
        for (i=0;i<m;i++) {
            x1[i] = x2[i];
            y1[i] = y2[i];
        }
    }
    
    free(x2);
    free(y2);
    return(TRUE);
}

/*
 This computes an in-place complex-to-complex FFT
 x and y are the real and imaginary arrays of 2^m points.
 dir =  1 gives forward transform
 dir = -1 gives reverse transform
 */
short
PbFft::FFT(short int dir,long m,BL_FLOAT *x,BL_FLOAT *y)
{
    long n,i,i1,j,k,i2,l,l1,l2;
    BL_FLOAT c1,c2,tx,ty,t1,t2,u1,u2,z;
    
    /* Calculate the number of points */
    n = 1;
    for (i=0;i<m;i++)
        n *= 2;
    
    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;
    for (i=0;i<n-1;i++) {
        if (i < j) {
            tx = x[i];
            ty = y[i];
            x[i] = x[j];
            y[i] = y[j];
            x[j] = tx;
            y[j] = ty;
        }
        k = i2;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
    
    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l=0;l<m;l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j=0;j<l1;j++) {
            for (i=j;i<n;i+=l2) {
                i1 = i + l1;
                t1 = u1 * x[i1] - u2 * y[i1];
                t2 = u1 * y[i1] + u2 * x[i1];
                x[i1] = x[i] - t1;
                y[i1] = y[i] - t2;
                x[i] += t1;
                y[i] += t2;
            }
            z =  u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = std::sqrt((1.0 - c1) / 2.0);
        if (dir == 1)
            c2 = -c2;
        c1 = std::sqrt((1.0 + c1) / 2.0);
    }
    
    /* Scaling for forward transform */
    if (dir == 1) {
        for (i=0;i<n;i++) {
            x[i] /= n;
            y[i] /= n;
        }
    }
    
    return(TRUE);
}

/*-------------------------------------------------------------------------
 Perform a 2D FFT inplace given a complex 2D array
 The direction dir, 1 for forward, -1 for reverse
 The size of the array (nx,ny)
 Return false if there are memory problems or
 the dimensions are not powers of 2
 */
int
PbFft::FFT2D(COMPLEX **c,int nx,int ny,int dir)
{
    int i,j;
    int m,twopm;
    BL_FLOAT *real,*imag;
    
    /* Transform the rows */
    real = (BL_FLOAT *)malloc(nx * sizeof(BL_FLOAT));
    imag = (BL_FLOAT *)malloc(nx * sizeof(BL_FLOAT));
    if (real == NULL || imag == NULL)
        return(FALSE);
    if (!Powerof2(nx,&m,&twopm) || twopm != nx)
    {
        free(real);
        free(imag);
    
        return(FALSE);
    }
    for (j=0;j<ny;j++) {
        for (i=0;i<nx;i++) {
            real[i] = c[i][j].real;
            imag[i] = c[i][j].imag;
        }
        FFTAux(dir,m,real,imag);
        for (i=0;i<nx;i++) {
            c[i][j].real = real[i];
            c[i][j].imag = imag[i];
        }
    }
    free(real);
    free(imag);
    
    /* Transform the columns */
    real = (BL_FLOAT *)malloc(ny * sizeof(BL_FLOAT));
    imag = (BL_FLOAT *)malloc(ny * sizeof(BL_FLOAT));
    if (real == NULL || imag == NULL)
        return(FALSE);
    if (!Powerof2(ny,&m,&twopm) || twopm != ny)
    {
        free(real);
        free(imag);
    
        return(FALSE);
    }
    for (i=0;i<nx;i++) {
        for (j=0;j<ny;j++) {
            real[j] = c[i][j].real;
            imag[j] = c[i][j].imag;
        }
        FFTAux(dir,m,real,imag);
        for (j=0;j<ny;j++) {
            c[i][j].real = real[j];
            c[i][j].imag = imag[j];
        }
    }
    free(real);
    free(imag);
    
    return(TRUE);
}

int
PbFft::FFT2D(const COMPLEX **inC, COMPLEX **outC, int nx,int ny,int dir)
{
    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            outC[i][j] = inC[i][j];
        }
    }
    
    int res = FFT2D(outC, nx, ny, dir);
    
    return res;
}

int
PbFft::FFT2D(const BL_FLOAT **inD, COMPLEX **outC, int nx,int ny,int dir)
{
    //COMPLEX inC[nx][ny];
    COMPLEX **inC = (COMPLEX **)malloc(sizeof(COMPLEX *)*nx);
    for (int i = 0; i < nx; i++)
        inC[i] = (COMPLEX *)malloc(sizeof(COMPLEX)*ny);
    
    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            inC[i][j].real = inD[i][j];
            inC[i][j].imag = 0.0;
        }
    }
    
    int res = FFT2D((const COMPLEX **)inC, outC, nx, ny, dir);
    
    for (int i = 0; i < nx; i++)
        free(inC[i]);
    free(inC);
        
    return res;
}

int
PbFft::FFT2D(const COMPLEX **inC, BL_FLOAT **outD, int nx,int ny,int dir)
{
    //COMPLEX outC[nx][ny];
    COMPLEX **outC = (COMPLEX **)malloc(sizeof(COMPLEX *)*nx);
    for (int i = 0; i < nx; i++)
        outC[i] = (COMPLEX *)malloc(sizeof(COMPLEX)*ny);
    
    int res = FFT2D(inC, (COMPLEX **)outC, nx, ny, dir);
    
    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            outD[i][j] = outC[i][j].real;
        }
    }
    
    for (int i = 0; i < nx; i++)
        free(outC[i]);
    free(outC);
    
    return res;
}

int
PbFft::FFT2D(const BL_FLOAT *inD, COMPLEX *outC, int nx,int ny,int dir)
{
    //BL_FLOAT inD2[nx][ny];
    BL_FLOAT **inD2 = (BL_FLOAT **)malloc(sizeof(BL_FLOAT *)*nx);
    for (int i = 0; i < nx; i++)
        inD2[i] = (BL_FLOAT *)malloc(sizeof(BL_FLOAT)*ny);
    
    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            inD2[i][j] = inD[i + j*nx];
        }
    }
    
    //COMPLEX outC2[nx][ny];
    COMPLEX **outC2 = (COMPLEX **)malloc(sizeof(COMPLEX *)*nx);
    for (int i = 0; i < nx; i++)
        outC2[i] = (COMPLEX *)malloc(sizeof(COMPLEX)*ny);

    int ret = FFT2D((const BL_FLOAT **)inD2, (COMPLEX **)outC2, nx, ny, dir);
    
    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            outC[i + j*nx] = outC2[i][j];
        }
    }
    
    for (int i = 0; i < nx; i++)
        free(inD2[i]);
    free(inD2);
    
    for (int i = 0; i < nx; i++)
        free(outC2[i]);
    free(outC2);
    
    return ret;
}

int
PbFft::FFT2D(const COMPLEX *inC, BL_FLOAT *outD, int nx,int ny,int dir)
{
    //COMPLEX inC2[nx][ny];
    COMPLEX **inC2 = (COMPLEX **)malloc(sizeof(COMPLEX *)*nx);
    for (int i = 0; i < nx; i++)
        inC2[i] = (COMPLEX *)malloc(sizeof(COMPLEX)*ny);
    
    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            inC2[i][j] = inC[i + j*nx];
        }
    }
    
    //BL_FLOAT outD2[nx][ny];
    BL_FLOAT **outD2 = (BL_FLOAT **)malloc(sizeof(BL_FLOAT *)*nx);
    for (int i = 0; i < nx; i++)
        outD2[i] = (BL_FLOAT *)malloc(sizeof(BL_FLOAT)*ny);
    
    int ret = FFT2D((const COMPLEX **)inC2, (BL_FLOAT **)outD2, nx, ny, dir);
    
    for (int j = 0; j < ny; j++)
    {
        for (int i = 0; i < nx; i++)
        {
            outD[i + j*nx] = outD2[i][j];
        }
    }
    
    for (int i = 0; i < nx; i++)
        free(inC2[i]);
    free(inC2);
    
    for (int i = 0; i < nx; i++)
        free(outD2[i]);
    free(outD2);
    
    return ret;
}

/*-------------------------------------------------------------------------
 This computes an in-place complex-to-complex FFT
 x and y are the real and imaginary arrays of 2^m points.
 dir =  1 gives forward transform
 dir = -1 gives reverse transform
 
 Formula: forward
 N-1
 ---
 1   \          - j k 2 pi n / N
 X(n) = ---   >   x(k) e                    = forward transform
 N   /                                n=0..N-1
 ---
 k=0
 
 Formula: reverse
 N-1
 ---
 \          j k 2 pi n / N
 X(n) =       >   x(k) e                    = forward transform
 /                                n=0..N-1
 ---
 k=0
 */
int
PbFft::FFTAux(int dir,int m,BL_FLOAT *x,BL_FLOAT *y)
{
    long nn,i,i1,j,k,i2,l,l1,l2;
    BL_FLOAT c1,c2,tx,ty,t1,t2,u1,u2,z;
    
    /* Calculate the number of points */
    nn = 1;
    for (i=0;i<m;i++)
        nn *= 2;
    
    /* Do the bit reversal */
    i2 = nn >> 1;
    j = 0;
    for (i=0;i<nn-1;i++) {
        if (i < j) {
            tx = x[i];
            ty = y[i];
            x[i] = x[j];
            y[i] = y[j];
            x[j] = tx;
            y[j] = ty;
        }
        k = i2;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
    
    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l=0;l<m;l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j=0;j<l1;j++) {
            for (i=j;i<nn;i+=l2) {
                i1 = i + l1;
                t1 = u1 * x[i1] - u2 * y[i1];
                t2 = u1 * y[i1] + u2 * x[i1];
                x[i1] = x[i] - t1;
                y[i1] = y[i] - t2;
                x[i] += t1;
                y[i] += t2;
            }
            z =  u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = std::sqrt((1.0 - c1) / 2.0);
        if (dir == 1)
            c2 = -c2;
        c1 = std::sqrt((1.0 + c1) / 2.0);
    }
    
    /* Scaling for forward transform */
    if (dir == 1) {
        for (i=0;i<nn;i++) {
            x[i] /= (BL_FLOAT)nn;
            y[i] /= (BL_FLOAT)nn;
        }
    }
    
    return(TRUE);
}

/*-------------------------------------------------------------------------
 Calculate the closest but lower power of two of a number
 twopm = 2**m <= n
 Return TRUE if 2**m == n
 */
int
PbFft::Powerof2(int n,int *m,int *twopm)
{
    if (n <= 1) {
        *m = 0;
        *twopm = 1;
        return(FALSE);
    }
    
    *m = 1;
    *twopm = 2;
    do {
        (*m)++;
        (*twopm) *= 2;
    } while (2*(*twopm) <= n);
    
    if (*twopm != n)
        return(FALSE);
    else
        return(TRUE);
}
