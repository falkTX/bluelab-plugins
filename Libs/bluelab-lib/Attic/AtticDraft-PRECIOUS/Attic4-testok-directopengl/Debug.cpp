//
//  Debug.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 03/05/17.
//
//

#include <stdio.h>
#include <math.h>

#include "Debug.h"

#define MAX_PATH 512

#define COMP_MAGN(__x__) (sqrt(__x__.re * __x__.re + __x__.im * __x__.im))

#ifdef __APPLE__
#define BASE_FILE "/Volumes/HDD/Share/BlueLabAudio-Debug/"
#else
#ifdef WIN32 
#define BASE_FILE "C:/Tmp/BlueLabAudio-Debug/"
#else
NOT IMPLEMENTED
#endif
#endif

void
Debug::DumpMessage(const char *filename, const char *message)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "a+");
    
    fprintf(file, "%s\n", message);
    
    fclose(file);
}

void
Debug::DumpData(const char *filename, double *data, int size)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < size; i++)
        fprintf(file, "%f ", data[i]);
    
    fclose(file);
}

void
Debug::DumpData2D(const char *filename, double *data, int width, int height)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
            fprintf(file, "%f ", data[i + j*width]);
        fprintf(file, "\n");
    }
    
    fclose(file);
}

void
Debug::DumpComplexData(const char *filenameMagn, const char *filenamePhase, WDL_FFT_COMPLEX *buf, int size)
{
    // Magnitudes
    char fullFilenameMagn[MAX_PATH];
    sprintf(fullFilenameMagn, BASE_FILE"%s", filenameMagn);
    
    FILE *fileMagn = fopen(fullFilenameMagn, "w");
    for (int i = 0; i < size; i++)
        fprintf(fileMagn, "%f ", COMP_MAGN(buf[i]));
    fclose(fileMagn);
    
    // Phases
    char fullFilenamePhase[MAX_PATH];
    sprintf(fullFilenamePhase, BASE_FILE"%s", filenamePhase);
    
    FILE *filePhase = fopen(fullFilenamePhase, "w");
    for (int i = 0; i < size; i++)
    {
        double phase = atan2(buf[i].im, buf[i].re);
        fprintf(filePhase, "%f ", phase);
    }
    
    fclose(filePhase);
}

void
Debug::DumpRawComplexData(const char *filenameRe, const char *filenameImag, WDL_FFT_COMPLEX *buf, int size)
{
    // Magnitudes
    char fullFilenameRe[MAX_PATH];
    sprintf(fullFilenameRe, BASE_FILE"%s", filenameRe);
    
    FILE *fileRe = fopen(fullFilenameRe, "w");
    for (int i = 0; i < size; i++)
        fprintf(fileRe, "%f ", buf[i].re);
    fclose(fileRe);
    
    // Phases
    char fullFilenameImag[MAX_PATH];
    sprintf(fullFilenameImag, BASE_FILE"%s", filenameImag);
    
    FILE *fileImag = fopen(fullFilenameImag, "w");
    for (int i = 0; i < size; i++)
    {
        fprintf(fileImag, "%f ", buf[i].im);
    }
    
    fclose(fileImag);
}