//
//  ImageInpainting.cpp
//  BL-Ghost
//
//  Created by Pan on 09/07/18.
//
//

#include <stdlib.h>

#include <GtiInpaint.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <PPMFile.h>

#include "ImageInpaint.h"

#if 0
NOTE: there may be some adjustmes to do to retrive a working version after tests
(in particular memory size of out fft)
#endif

#define EP 1e-3
#define IMAX  100 //100 //10000
#define CONDW 3 //5

// For debugging
// Should always be 1 !
#define DEINTERLACE 1

void
ImageInpaint::Inpaint(BL_FLOAT *image, int width, int height,
                         BL_FLOAT borderRatio)
{
    // TODO: manage single channel instead of 3 channels
    
    // Extend the image to the next power of two
    int w2 = BLUtilsMath::NextPowerOfTwo(width);
    int h2 = BLUtilsMath::NextPowerOfTwo(height);
    BL_FLOAT *image2 = (BL_FLOAT*)malloc(sizeof(BL_FLOAT)*w2*h2);
    for (int j = 0; j < h2; j++)
    {
        for (int i = 0; i < w2; i++)
        {
            BL_FLOAT col = 0.0;
            if ((i < width) && (j < height))
            {
                col = image[i + j*width];
            }
            
            image2[i + j*w2] = col;
        }
    }
    
    // Create 3 channels image
    BL_FLOAT *image3 = (BL_FLOAT*)malloc(sizeof(BL_FLOAT)*w2*h2*3);
    for (int j = 0; j < h2; j++)
    {
        for (int i = 0; i < w2; i++)
        {
            BL_FLOAT col = image2[i + j*w2];
            
            for (int k = 0; k < 3; k++)
#if DEINTERLACE
                image3[w2*h2*k + i + j*w2] = col;
#else
                image3[(i + j*w2)*3 + k] = col;
#endif
        }
    }
    
    //PPMFile::SavePPM("image-bpp3.ppm", image3, w2, h2, 3, 255.0);
    
    // Create the mask
    BL_FLOAT *mask = (BL_FLOAT*)malloc(sizeof(BL_FLOAT)*w2*h2*3);
    for (int j = 0; j < h2; j++)
    {
        for (int i = 0; i < w2; i++)
        {
            BL_FLOAT col = 0.0;
            
            bool inside = ((i > borderRatio*width) && (i < (1.0 - borderRatio)*width) &&
                           (j > borderRatio*height) && (j < (1.0 - borderRatio)*height));

            if (inside)
                col = 255.0;
            
            for (int k = 0; k < 3; k++)
#if DEINTERLACE
                mask[w2*h2*k + i + j*w2] = col;
#else
                mask[(i + j*w2)*3 + k] = col;
#endif
        }
    }
    
    BL_FLOAT *bigMask = (BL_FLOAT*)malloc(sizeof(BL_FLOAT)*4*w2*h2*3);
    
    GtiInpaint inpaint(w2, h2);
    inpaint.SetMask(bigMask, mask);
    
    BL_FLOAT *bigRes = inpaint.Gausstexinpaint(image3, bigMask, EP, IMAX, CONDW);
    
    // Crop and get the result
    for (int i=0;i<h2;i++)
        for (int j=0;j<w2;j++)
            for (int c = 0; c < 3; c++)
        {
#if DEINTERLACE
            image3[c*h2*w2 + i*w2 + j] = bigRes[inpaint.Ic(i,j,c)];
#else
            image3[(i*w2 + j)*3 + c] = bigRes[inpaint.Ic(i,j,c)];
#endif
        }
    
    for (int j=0;j<h2;j++)
        for (int i=0;i<w2;i++)
        {
#if DEINTERLACE
            BL_FLOAT col = image3[i + j*w2] +
                         image3[w2*h2 + i + j*w2] +
                         image3[w2*h2*2 + i + j*w2];
#else
            BL_FLOAT col = image3[(i + j*w2)*3] +
                         image3[(i + j*w2)*3 + 1] +
                         image3[(i + j*w2)*3 + 2];
#endif
            col /= 3.0;
            
            image2[i + j*w2] = col;
        }
    
    // Copy the result (not power of two
    for (int j=0;j<height;j++)
        for (int i=0;i<width;i++)
        {
            BL_FLOAT col = image2[i + j*w2];
            image[i + j*width] = col;
        }
    
    free(image2);
    free(image3);
    free(bigRes);
    free(mask);
    free(bigMask);
}
