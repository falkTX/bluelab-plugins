//
//  PPMFile.cpp
//  BL-Spectrogram
//
//  Created by Pan on 09/04/18.
//
//

#include <stdlib.h>
#include <stdio.h>

// For ntohs
#ifdef WIN32
#	include <winsock2.h>
//#	include <sys/param.h>
#endif

#include <portable_endian.h>

#include "PPMFile.h"

#define MAX_PATH 512

#define RGB_COMPONENT_COLOR 255
#define RGB_COMPONENT_COLOR16 65535

PPMFile::PPMImage *
PPMFile::ReadPPM(const char *filename)
{
    char buff[16];
    PPMImage *img;
    FILE *fp;
    int c, rgb_comp_color;
    
    //open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        return NULL;
    }
    
    //read image format
    if (!fgets(buff, sizeof(buff), fp))
    {
        return NULL;
    }
    
    //check the image format
    if (buff[0] != 'P' || buff[1] != '6')
    {
        fprintf(stderr, "Invalid image format (must be 'P6')\n");
        
        return NULL;
    }
    
    //alloc memory form image
    img = (PPMImage *)malloc(sizeof(PPMImage));
    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        
        return NULL;
    }
    
    img->data = NULL;
    img->data16 = NULL;
    
    //check for comments
    c = getc(fp);
    while (c == '#')
    {
        while (getc(fp) != '\n') ;
        
        c = getc(fp);
    }
    
    ungetc(c, fp);
    
    //read image size information
    if (fscanf(fp, "%d %d", &img->w, &img->h) != 2)
    {
        fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
        
        return NULL;
    }
    
    //read rgb component
    if (fscanf(fp, "%d", &rgb_comp_color) != 1)
    {
        fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
        
        return NULL;
    }
    
    //check rgb component depth
    if (rgb_comp_color!= RGB_COMPONENT_COLOR)
    {
        fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
        
        return NULL;
    }
    
    while (fgetc(fp) != '\n') ;
    //memory allocation for pixel data
    img->data = (PPMPixel*)malloc(img->w * img->h * sizeof(PPMPixel));
    
    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        
        return NULL;
    }
    
    //read pixel data from file
    if (fread(img->data, 3 * img->w, img->h, fp) != img->h)
    {
        fprintf(stderr, "Error loading image '%s'\n", filename);
        
        return NULL;
    }
    
    fclose(fp);
    
    return img;
}

PPMFile::PPMImage *
PPMFile::ReadPPM16(const char *filename)
{
    char buff[16];
    PPMImage *img;
    FILE *fp;
    int c, rgb_comp_color;
    
    //open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        return NULL;
    }
    
    //read image format
    if (!fgets(buff, sizeof(buff), fp))
    {
        return NULL;
    }
    
    //check the image format
    if (buff[0] != 'P' || buff[1] != '6')
    {
        fprintf(stderr, "Invalid image format (must be 'P6')\n");
        
        return NULL;
    }
    
    //alloc memory form image
    img = (PPMImage *)malloc(sizeof(PPMImage));
    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        
        return NULL;
    }
    
    img->data = NULL;
    img->data16 = NULL;
    
    //check for comments
    c = getc(fp);
    while (c == '#')
    {
        while (getc(fp) != '\n') ;
        
        c = getc(fp);
    }
    
    ungetc(c, fp);
    
    //read image size information
    if (fscanf(fp, "%d %d", &img->w, &img->h) != 2)
    {
        fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
        
        return NULL;
    }
    
    //read rgb component
    if (fscanf(fp, "%d", &rgb_comp_color) != 1)
    {
        fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
        
        return NULL;
    }
    
    //check rgb component depth
    if (rgb_comp_color!= RGB_COMPONENT_COLOR16)
    {
        fprintf(stderr, "'%s' does not have 16-bits components\n", filename);
        
        return NULL;
    }
    
    while (fgetc(fp) != '\n') ;
    //memory allocation for pixel data
    img->data16 = (PPMPixel16*)malloc(img->w * img->h * sizeof(PPMPixel16));
    
    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        
        return NULL;
    }
    
    //read pixel data from file
    if (fread(img->data16, sizeof(PPMPixel16) * img->w, img->h, fp) != img->h)
    {
        fprintf(stderr, "Error loading image '%s'\n", filename);
        
        return NULL;
    }
    
    // Endianess
    for (int i = 0; i < img->w*img->h; i++)
    {
        PPMPixel16 &pix = img->data16[i];
        
        pix.red = be16toh(pix.red);
        pix.green = be16toh(pix.green);
        pix.blue = be16toh(pix.blue);
    }
    
    fclose(fp);
    
    return img;
}

void
PPMFile::SavePPM(const char *filename, BL_FLOAT *image,
                 int width, int height, int bpp,
                 BL_FLOAT colorCoeff, bool deInterlace)
{
    char fullFilename[MAX_PATH];
#if __APPLE__
    sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", filename);
#endif
    
#if __linux__
    sprintf(fullFilename, "/home/niko/Documents/BlueLabAudio-Debug/%s", filename);
#endif
    
    FILE *file = fopen(fullFilename, "w");
    
    // Header
    fprintf(file, "P3\n");
    fprintf(file, "%d %d\n", width, height);
    fprintf(file, "%d\n", RGB_COMPONENT_COLOR);
    
    // Data
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            if (bpp > 1)
            {
                for (int k = 0; k < bpp; k++)
                {
                    BL_FLOAT value;
                    if (!deInterlace)
                        value = image[(i + j*width)*bpp + k];
                    else
                        value = image[width*height*k + i + j*width];
                    
                    unsigned int valI = value*colorCoeff;
                    if (valI > 255)
                        valI = 255;
                    
                    fprintf(file, "%d ", valI);
                }
                
                fprintf(file, "\n");
            }
            else
            {
                BL_FLOAT value = image[i + j*width];
                unsigned int valI = value*colorCoeff;
                if (valI > 255)
                    valI = 255;
                
                fprintf(file, "%d %d %d\n", valI, valI, valI);
            }
        }
        
        fprintf(file, "\n");
    }
    fclose(file);
}
