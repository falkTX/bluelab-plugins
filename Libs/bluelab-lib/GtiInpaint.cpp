//
//  GtiInpaint.cpp
//  BL-Ghost
//
//  Created by Pan on 09/07/18.
//
//

#include <stdlib.h>
#include <math.h>

#include <cmath>

#include <PPMFile.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "GtiInpaint.h"

// See: http://www.math-info.univ-paris5.fr/~aleclair/papers/gti_ipol.pdf
// and: http://www.ipol.im/pub/art/2017/198/

GtiInpaint::GtiInpaint(int w, int h)
{
    width = w;
    height = h;
    width2 = width*2;
    height2 = height*2;
    
    // TEST
    ftsize = (width+1)*height2;
    //ftsize = width2*height2;
    
    fftIn = NULL;
    fftOut = NULL;
        
    re = NULL;
    im = NULL;
    ret = NULL;
    imt = NULL;
    retmp = NULL;
    imtmp = NULL;
}

GtiInpaint::~GtiInpaint() {}


// Initialize FFT variables and compute the FFT plan.
void
GtiInpaint::InitFft()
{
    fftIn  = (BL_FLOAT *)malloc(sizeof(BL_FLOAT)*width2*height2);
    
    // TEST
    //fftOut = (PbFft::COMPLEX *)malloc(sizeof(PbFft::COMPLEX)*ftsize);
    fftOut = (PbFft::COMPLEX *)malloc(sizeof(PbFft::COMPLEX)*width2*height2);
    
    // The two following lines compute an optimal FFT plan for the current image size.
    //fftw_plan_direct  = fftwf_plan_dft_r2c_2d(height2,width2, fftw_in, fftw_out, FFTW_MEASURE);
    //fftw_plan_inverse = fftwf_plan_dft_c2r_2d(height2,width2, fftw_out, fftw_in, FFTW_MEASURE);
}

//Compute direct FFT
void
GtiInpaint::FftFft2d(BL_FLOAT *inRe, BL_FLOAT *outRe, BL_FLOAT *outIm)
{
    int i;
    
    for (i=0;i<width2*height2;i++)
        fftIn[i] = inRe[i];
    
    //PPMFile::SavePPM("in-fft.ppm", inRe, width2, height2, 1, 255.0*255*255);
                     
    //fftwf_execute(fftw_plan_direct);
    
    int res = PbFft::FFT2D(fftIn, fftOut, width2, height2, 1);
    if (res == 0)
        return;
    
    if (outRe)
        for (i=0;i<ftsize;i++)
            outRe[i] = fftOut[i].real;
    
    //PPMFile::SavePPM("out-fft.ppm", outRe, width2, height2, 1, 255.0*255.0*255);

    if (outIm)
        for (i=0;i<ftsize;i++)
            outIm[i] = fftOut[i].imag;
}


// Compute inverse FFT
void
GtiInpaint::FftFft2dInv(BL_FLOAT *inRe, BL_FLOAT *inIm, BL_FLOAT *outRe)
{
    int i;
    BL_FLOAT norm;
    
    for (i=0;i<ftsize;i++)
        fftOut[i].real = inRe[i];
    
    if (inIm)
        for (i=0;i<ftsize;i++)
            fftOut[i].imag = inIm[i];
    
    //PPMFile::SavePPM("in-fft.ppm", inRe, width2, height2, 1, 255.0*255*255);
    //BLDebug::DumpData("in-fft-re.txt", inRe, width2*height2);
    
    //fftwf_execute(fftw_plan_inverse);
    int res = PbFft::FFT2D(fftOut, fftIn, width2, height2, -1);
    if (res == 0)
        return;
    
    norm = 1./(BL_FLOAT)(width2*height2);
    for (i=0;i<width2*height2;i++)
        fftIn[i] *= norm;
                    
    if (outRe)
        for (i=0;i<width2*height2;i++)
            outRe[i] = fftIn[i];
    
    //PPMFile::SavePPM("out-fft.ppm", outRe, width2, height2, 1, 255.0*255*255);
}


// Compute inverse FFT
void
GtiInpaint::TermFft()
{
    free(fftIn);
    free(fftOut);
    //fftwf_destroy_plan(fftw_plan_direct); fftwf_destroy_plan(fftw_plan_inverse);
}



// Calculates the position on a torus.
// i The y coordinate
// j The x coordinate
// c The channel number
// Return the position in an array of the coordinate (i,j) on channel c
int
GtiInpaint::Ic(int i, int j, int c)
{
    if(i < 0)
        i += height2;
    else if( i >= height2)
        i = i - height2;
    if(j < 0)
        j += width2;
    else if( j >= width2)
        j = j - width2;
    return c*height2*width2 + i*width2 + j;
}




// Sets the mask from an image.
//
// The mask will be obtained by looking at the area that has RGB values (254,254,254) or higher.
// - mask The mask that will be extracted.
// - im The image from which the mask will be extracted
// NB: mask must be allocated with a twice larger size than im
void
GtiInpaint::SetMask(BL_FLOAT *mask, const BL_FLOAT *im)
{
    int i, j, c;
    int val;
    for(i = 0; i < height2; i++)
    {
        for(j = 0; j < width2; j++)
        {
            if (i<height && j<width)
            {
                if(im[i*width + j] > 253 && im[height*width + i*width + j] > 253  && im[2*height*width + i*width + j] > 253)
                    val = 0; // 0 value for a masked pixel
                else
                    val = 1;
                for(c = 0; c < 3; c++)
                {
                    int ic = Ic(i,j,c);
                    mask[ic] = val;
                }
            }
            else
            {
                for(c = 0; c < 3; c++)
                {
                    int ic = Ic(i,j,c);
                    mask[ic] = 0;
                }
            }
        }
    }
}



// Sets the value of an array to a given value.
//
// A The array to be written on
// value The value to write to the array
// size The size of the array
void
GtiInpaint::SetValue(BL_FLOAT *A, BL_FLOAT value, int size)
{
    int i;
    for(i = 0; i < size; i++)
        A[i] = value;
}


// Draw an ADSN realization
//
// meanv  3-channel mean (0 if NULL)
// v 3-channel output
//
// See [Algorithm 2], [Equation (2)] of the IPOL paper.
void
GtiInpaint::Adsn(BL_FLOAT *meanv, BL_FLOAT *v)
{
    int adr,ch;
    BL_FLOAT a,b,c,d;
    BL_FLOAT *w, *rew, *imw;
    
    w = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2);
    rew = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    imw = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    
    // Draw a white noise
    for (adr=0;adr<width2*height2;adr++)
    {
        a = ((BL_FLOAT)rand())/RAND_MAX; b = ((BL_FLOAT)rand())/RAND_MAX;
        w[adr] = std::sqrt(-2.0*log(a))*cos(2.0*M_PI*b);
    }
    
    // Convolve with texton (whose FFT has been precomputed)
    FftFft2d(w,rew,imw);
    
    for (ch=0;ch<3;ch++)
    {
        for (adr=0;adr<ftsize;adr++)
        {
            a = ret[adr+ch*ftsize]; b = imt[adr+ch*ftsize];
            c = rew[adr]; d = imw[adr];
            re[adr] = a*c-b*d;
            im[adr] = b*c+a*d;
        }
        FftFft2dInv(re,im,v+ch*height2*width2);
    }
    
    // Add the mean component
    if (meanv!=NULL)
        for (ch=0;ch<3;ch++)
            //#pragma omp parallel for shared(v) private(adr)
            for (adr=0;adr<width2*height2;adr++)
                v[adr+ch*width2*height2] += meanv[ch];
    
    // free(re); free(im);
    free(w);
    free(rew);
    free(imw);
}


// Convolve single-channel u with monochannel w
//
// u 1-channel input
// w 1-channel input
// v 1-channel output
void
GtiInpaint::ConvolSingleChannel(BL_FLOAT *u, BL_FLOAT *w, BL_FLOAT *v)
{
    int adr;
    BL_FLOAT a,b,c,d;
    BL_FLOAT *rew, *imw;
    
    rew = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    imw = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    
    FftFft2d(w,rew,imw);
    FftFft2d(u,re,im);

    for (adr=0;adr<ftsize;adr++)
    {
        a = re[adr]; b = im[adr];
        c = rew[adr]; d = imw[adr];
        re[adr] = a*c-b*d;
        im[adr] = b*c+a*d;
    }
    
    FftFft2dInv(re,im,v);
    
    free(rew);
    free(imw);
}

// Convolve with covariance (with possible restriction)
//
// u 3-channel input image
// v 3-channel output
// dom 1-channel indicator function for restriction
//
// This function allows to convolve the input image u with
// the matrix texton associated to the Gaussian model
// [see equations (4) and (6)]
// and then restrict the result on the domain {dom=1}
// (i.e. set the values to 0 outside this domain)
//
//In the inpainting context, {dom=1} is the set of conditioning points.
//
//The texton FFT is precomputed in the global variables ret, imt
//and thus is not given as an argument of this function.
//
//See [Algorithm 4] of the IPOL paper.
void
GtiInpaint::Convcov(BL_FLOAT *u,BL_FLOAT *v,int *dom)
{
    // NB: can be used with u=v
    int adr,ch;
    BL_FLOAT a,b,c,d;
    
    SetValue(retmp,0.,ftsize);
    SetValue(imtmp,0.,ftsize);
    
    /* The covariance operator is given by the BL_FLOAT convolution of [Equation (6)].
     See also [Algorithm 3] */
    
    // First convolution with transposed texton [ \tilde{t}_v^T ]
    for (ch=0;ch<3;ch++)
    {
        FftFft2d(u+ch*height2*width2,re,im);
        for (adr=0;adr<ftsize;adr++)
        {
            a = re[adr]; b = im[adr];
            c = ret[adr+ch*ftsize]; d = imt[adr+ch*ftsize];
            retmp[adr] += a*c+b*d;
            imtmp[adr] += b*c-a*d;
        }
    }
    
    // Second convolution with texton [ t_v ]
    for (ch=0;ch<3;ch++)
    {
        for (adr=0;adr<ftsize;adr++)
        {
            a = retmp[adr]; b = imtmp[adr];
            c = ret[adr+ch*ftsize]; d = imt[adr+ch*ftsize];
            re[adr] = a*c-b*d;
            im[adr] = b*c+a*d;
        }
        FftFft2dInv(re,im,v+ch*height2*width2);
    }
    
    // Extract the values on the domain dom.
    if (dom != NULL)
        for (ch=0;ch<3;ch++)
            for (adr=0;adr<width2*height2;adr++)
                if (dom[adr]==0)
                    v[adr+ch*height2*width2] = 0.;
}


// Get conditioning points
//
// mask Input Mask (0 for masked pixels)
// cond Output Conditioning points (1 for conditioning points)
// w Thickness of conditioning border
void
GtiInpaint::GetConditioningPoints(const BL_FLOAT *mask,int *cond,int condw)
{
    int i,j;
    BL_FLOAT *maskc,*tmp;
    tmp = (BL_FLOAT *) malloc(sizeof(BL_FLOAT)*width2*height2);
    maskc = (BL_FLOAT *) malloc(sizeof(BL_FLOAT)*width2*height2);
    
    for (i=0;i<height2;i++){
        for (j=0;j<width2;j++){
            tmp[Ic(i,j,0)] = 0.;
            if (i<height && j<width)
                maskc[Ic(i,j,0)] = 1-mask[Ic(i,j,0)];
            else
                maskc[Ic(i,j,0)] = 0;
        }
    }
    
    for (i=-condw;i<=condw;i++)
        for (j=-condw;j<=condw;j++)
            tmp[Ic(i,j,0)] = 1.;
    
    ConvolSingleChannel(maskc,tmp,tmp);
    for (i=0;i<height;i++)
        for (j=0;j<width;j++)
            if (i<height && j<width)
                cond[Ic(i,j,0)] = (int)((tmp[Ic(i,j,0)]>0.5)&&(mask[Ic(i,j,0)]>0.5));
            else
                cond[Ic(i,j,0)] = 0;
}


// Estimate ADSN model
//
// See [Algorithm 1] and [Equation (1)] in the IPOL paper.
//
// u 3-channel input
// mask Mask (0 for masked pixels)
// meanu Image mean color (3 values)
// t Output texton
void
GtiInpaint::EstimateAdsnModel(BL_FLOAT *u, const BL_FLOAT *mask, BL_FLOAT *meanu, BL_FLOAT *t)
{
    int i,j,c;
    BL_FLOAT cardm,cardcm;
    
    // Number of pixels in the mask
    cardm = 0.;
    for (i=0;i<height;i++)
        for (j=0;j<width;j++)
            if (mask[Ic(i,j,0)]<0.5)
                cardm++;
    cardcm = width*height-cardm;
    //printf("nb masked pixels=%f\n",cardm);
    
    // Mean value
    for (c=0;c<3;c++) {
        meanu[c] = 0.;
        for (i=0;i<height;i++)
            for (j=0;j<width;j++)
                if (mask[Ic(i,j,c)]>0.5)
                    meanu[c] += u[Ic(i,j,c)];
        meanu[c] /= cardcm;
    }
    //printf("meanu = (%f,%f,%f)\n",meanu[0],meanu[1],meanu[2]);
    // Texton
    for (c=0;c<3;c++)
        for (i=0;i<height2;i++)
            for (j=0;j<width2;j++)
                if (mask[Ic(i,j,c)]>0.5)
		  t[Ic(i,j,c)] = (u[Ic(i,j,c)]-meanu[c])/std::sqrt(cardcm);
                else
                    t[Ic(i,j,c)] = 0.;
}


// Conjugate gradient descent on normal equations for kriging system
//
// cond Conditioning points (1 for conditioning points)
// rhs Right-hand side of the system
// v  Output
// ep Stopping criterion on L^2 norm of residual
// imax Maximum number of iterations
//
// See [Algorithm 5] of the IPOL paper.
//
void
GtiInpaint::Cgd(int *cond,BL_FLOAT *rhs,BL_FLOAT *x,BL_FLOAT ep,int imax)
{
    int iter = 0, adr;
    BL_FLOAT *p,*q,*r;
    BL_FLOAT rn,rn2,rn2old,alpha,beta;
    
    p = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    q = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    r = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    
    // initialization
    SetValue(x,0.,width2*height2*3);
    Convcov(rhs,r,cond);
    rn2 = 0.;
    for (adr=0;adr<width2*height2*3;adr++)
    {
        rn2 += r[adr]*r[adr];
        p[adr] = r[adr];
    }
    rn = std::sqrt(rn2);
    
    while (iter++<imax && rn>ep)
    {
        //printf("Iteration %d, res norm = %f\n",iter,rn);
        // [compute A^T A d_k ]
        Convcov(p,q,cond);
        Convcov(q,q,cond);
        // [compute \alpha_k ]
        alpha = 0;

        for (adr=0;adr<width2*height2*3;adr++)
            alpha += p[adr]*q[adr];
        alpha = rn2/alpha;
        
        /* Update variables. Correspondence with the notation of IPOL paper:
         [ \psi_k ] <-> x
         [ r_k ] <-> r
         [ \|r_k\|^2 ] <-> rn2
         */
        rn2old = rn2; rn2 = 0.;

        for (adr=0;adr<width2*height2*3;adr++)
        {
            x[adr] += alpha*p[adr];
            r[adr] -= alpha*q[adr];
            rn2 += r[adr]*r[adr];
        }
        rn = std::sqrt(rn2);
        
        // Update [d_{k+1}] <-> p
        beta = rn2/rn2old;

        for (adr=0;adr<width2*height2*3;adr++)
            p[adr] = r[adr] + beta*p[adr];
    }
    
    free(p);
    free(q);
    free(r);
}


// Gaussian texture inpainting
//
// image The image to inpaint
// mask The mask which indicates the inpainting domain (0 for masked values)
// Returns the inpainted image for which a memory has been dynamically allocated.
//
// THRESHOLD MAXITER
//
// See [Algorithm 6] of the IPOL paper.
//
// NB: re,im,ret,imt,retmp,imtmp are global variables that contain
// the output of Fourier transforms (with separate real and imaginary parts)
//
BL_FLOAT*
GtiInpaint::Gausstexinpaint(const BL_FLOAT *image, const BL_FLOAT *mask, BL_FLOAT ep, int imax, int condw)
{
    BL_FLOAT *u, *v, *z, *meanu, *t, *rhs;
    int *cond;
    int i,j,c;
    
    u = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    v = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    z = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    meanu = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*3);
    t = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    ret = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize*3);
    imt = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize*3);
    re = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    im = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    retmp = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    imtmp = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*ftsize);
    
    rhs = (BL_FLOAT*) malloc(sizeof(BL_FLOAT)*width2*height2*3);
    cond = (int*) malloc(sizeof(int)*width2*height2);
    
    // initialize FFT structures
    InitFft();
    
    // copy initial image
    for (i=0;i<height;i++)
        for (j=0;j<width;j++)
            for (c=0;c<3;c++)
                u[Ic(i,j,c)] = image[c*height*width + i*width + j];
    
    /* Estimate Gaussian model [Algorithm 1]  */
    EstimateAdsnModel(u,mask,meanu,t);
    
    /* Compute Fourier transform of the texton (once and for all)
     It is stored in the global variables ret, imt */
    for (c=0;c<3;c++)
        FftFft2d(t+c*height2*width2,ret+c*ftsize,imt+c*ftsize);
    
    /* Get Conditioning Points */
    GetConditioningPoints(mask,cond,condw);
    
    /* Compute ADSN realization (the texton FFT must be precomputed!)
     [Algorithm 2], [Equation (2)] */
    Adsn(NULL,z);
    
    /* Right-hand side of kriging system */
    SetValue(rhs,0.,width2*height2*3);
    for (c=0;c<3;c++)
        for (i=0;i<height2;i++)
            for (j=0;j<width2;j++)
                if (cond[Ic(i,j,0)]==1)
                    rhs[Ic(i,j,c)] = u[Ic(i,j,c)] - meanu[c] - z[Ic(i,j,c)];
    
    /* Conjugate gradient descent [Algorithm 5] */
    Cgd(cond,rhs,v,ep,imax);
    
    /* Final steps:
     apply covariance operator, add mean value,
     and reimpose initial known values outside the mask. */
    Convcov(v,v,NULL);
    for (c=0;c<3;c++)
        for (i=0;i<height2;i++)
            for (j=0;j<width2;j++)
                if (mask[Ic(i,j,0)]<0.5)
                    v[Ic(i,j,c)] += meanu[c] + z[Ic(i,j,c)];
                else
                    v[Ic(i,j,c)] = u[Ic(i,j,c)];
    
    /* Cleanup */
    free(u);
    free(z);
    free(meanu);
    free(t);
    
    free(ret);
    free(imt);
    free(re);
    free(im);
    free(retmp);
    free(imtmp);
    
    free(rhs);
    free(cond);
    
    // Terminate FFT structures
    TermFft();
    
    return v; // WARNING! OUTPUT must be of size width2*height2*3
    
}

#if 0 // Main, for example
int
main(int argc, char **argv){
    
    if(argc > 1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h"))){
        //printHelp();
        return EXIT_FAILURE;
    }
    if(argc < 4){
        printf("Too few arguments.\n\n");
        printf("Syntax: gti [image] [mask] [output] [ep] [niter] [w]\n");
        // printf("  try --help for more\n");
        return EXIT_FAILURE;
    }
    
    //numth = MIN(8,omp_get_max_threads());
    ////atoi(getenv("OMP_NUM_THREADS"));
    
    BL_FLOAT ep = 1e-3;
    int imax = 100;
    int condw = 3;
    if (argc>4)
        ep = (BL_FLOAT) atof(argv[4]);
    if (argc>5)
        imax = (int) atoi(argv[5]);
    if (argc>6)
        condw = (int) atoi(argv[6]);
    
    BL_FLOAT *im = NULL;
    size_t swidth, sheight;
    im = io_png_read_f32_rgb(argv[1], &swidth, &sheight);
    if(im == NULL){
        printf("Are you sure %s is a png image?\n", argv[1]);
        return 0;
    }
    
    /* The following global variables are set once and for all
     and contain the dimensions of the image domain,
     the dimensions of the duplicated image domain (that serves
     for convolution), and the size of Fourier transforms. */
    width = (int) swidth;
    height = (int) sheight;
    width2 = width*2;
    height2 = height*2;
    ftsize = (width+1)*height2;
    
    BL_FLOAT *m,*mask;
    m = io_png_read_f32_rgb(argv[2], &swidth, &sheight);
    if(m == NULL){
        printf("Are you sure %s is a png image?\n", argv[1]);
        free(im);
        return EXIT_FAILURE;
    }
    if(width != (int) swidth || height != (int) sheight){
        printf("Dimensions of %s and %s don't agree!\n",argv[1],argv[2]);
        free(im);
        free(m);
        return EXIT_FAILURE;
    }
    mask = (BL_FLOAT*) xmalloc(sizeof(BL_FLOAT)*width2*height2*3);
    setMask(mask,m);
    int i,j,c;
    for(i = 0; i < width*height*3; i++)
        im[i] /= 255;
    
    BL_FLOAT *bigres = gausstexinpaint(im,mask,ep,imax,condw);
    BL_FLOAT *res;
    res = (BL_FLOAT*) xmalloc(sizeof(BL_FLOAT)*width*height*3);
    // crop the result on the initial image domain
    for (i=0;i<height;i++)
        for (j=0;j<width;j++)
            for (c=0;c<3;c++)
                res[c*height*width + i*width + j] = 255*bigres[Ic(i,j,c)];
    io_png_write_f32(argv[3], res, swidth, sheight, 3);
    
    free(im);
    free(m); free(mask);
    free(res);
    return EXIT_SUCCESS;
}
#endif
