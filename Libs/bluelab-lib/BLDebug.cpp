//
//  BLDebug.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 03/05/17.
//
//

#include <stdio.h>
#include <math.h>

#include <BLUtils.h>
#include <BLUtilsMath.h> // For M_PI
#include <BLUtilsComp.h>

#include <BLDebug.h>

//#define COMP_MAGN(__x__) (std::sqrt(__x__.re * __x__.re + __x__.im * __x__.im))

#ifdef __APPLE__
#define BASE_FILE "/Users/applematuer/Documents/BlueLabAudio-Debug/"
#endif

#ifdef WIN32 
#define BASE_FILE "C:/Tmp/BlueLabAudio-Debug/"
#endif

#ifdef __linux__
#define BASE_FILE "/home/niko/Documents/BlueLabAudio-Debug/"
#endif

const char *
BLDebug::GetDebugBaseFile()
{
    return BASE_FILE;
}

void
BLDebug::AppendMessage(const char *filename, const char *message)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "a+");
    
    fprintf(file, "%s\n", message);
    
    fclose(file);
}

template <typename FLOAT_TYPE>
void
BLDebug::DumpData(const char *filename, const FLOAT_TYPE *data, int size)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < size; i++)
        fprintf(file, "%g ", data[i]);
    
    fclose(file);
}
template void BLDebug::DumpData(const char *filename, const float *data, int size);
template void BLDebug::DumpData(const char *filename, const double *data, int size);

template <typename FLOAT_TYPE>
void
BLDebug::DumpData(const char *filename, const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    DumpData(filename, buf.Get(), buf.GetSize());
}
template void BLDebug::DumpData(const char *filename, const WDL_TypedBuf<float> &buf);
template void BLDebug::DumpData(const char *filename, const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
void
BLDebug::DumpData(const char *filename, const vector<FLOAT_TYPE> &data)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < data.size(); i++)
        fprintf(file, "%g ", data[i]);
    
    fclose(file);
}
template void BLDebug::DumpData(const char *filename, const vector<float> &data);
template void BLDebug::DumpData(const char *filename, const vector<double> &data);

void
BLDebug::DumpData(const char *filename, const int *data, int size)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < size; i++)
        fprintf(file, "%d ", data[i]);
    
    fclose(file);
}

void
BLDebug::DumpData(const char *filename, const WDL_TypedBuf<int> &buf)
{
    DumpData(filename, buf.Get(), buf.GetSize());
}

void
BLDebug::DumpData(const char *filename, const vector<int> &buf)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < buf.size(); i++)
        fprintf(file, "%d ", buf[i]);
    
    fclose(file);
}

void
BLDebug::DumpData(const char *filename, const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < buf.GetSize(); i++)
    {
        fprintf(file, "%g ", buf.Get()[i].re);
        fprintf(file, "%g ", buf.Get()[i].im);
    }
    
    fclose(file);
}

void
BLDebug::DumpDataMagns(const char *filename,
                       const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < buf.GetSize(); i++)
    {
        BL_FLOAT magn = COMP_MAGN(buf.Get()[i]);
        fprintf(file, "%g ", magn);
    }
    
    fclose(file);
}

template <typename FLOAT_TYPE>
void
BLDebug::LoadData(const char *filename, FLOAT_TYPE *data, int size)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "rb");

    for (int i = 0; i < size; i++)
        fscanf(file, "%lf", &data[i]);
    
    fclose(file);
}
template void BLDebug::LoadData(const char *filename, float *data, int size);
template void BLDebug::LoadData(const char *filename, double *data, int size);

template <typename FLOAT_TYPE>
void
BLDebug::LoadData(const char *filename, WDL_TypedBuf<FLOAT_TYPE> *buf)
{
    LoadData(filename, buf->Get(), buf->GetSize());
}
template void BLDebug::LoadData(const char *filename, WDL_TypedBuf<float> *buf);
template void BLDebug::LoadData(const char *filename, WDL_TypedBuf<double> *buf);

template <typename FLOAT_TYPE>
void
BLDebug::DumpValue(const char *filename, FLOAT_TYPE value)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    
    fprintf(file, "%g ", value);
    
    fclose(file);
}
template void BLDebug::DumpValue(const char *filename, float value);
template void BLDebug::DumpValue(const char *filename, double value);

void
BLDebug::ResetFile(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
        
    fclose(file);
}

template <typename FLOAT_TYPE>
void
BLDebug::AppendData(const char *filename, const FLOAT_TYPE *data, int size)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "a+");
    for (int i = 0; i < size; i++)
        fprintf(file, "%g ", data[i]);
    
    fclose(file);
}
template void BLDebug::AppendData(const char *filename, const float *data, int size);
template void BLDebug::AppendData(const char *filename, const double *data, int size);

template <typename FLOAT_TYPE>
void
BLDebug::AppendData(const char *filename, const WDL_TypedBuf<FLOAT_TYPE> &buf)
{
    AppendData(filename, buf.Get(), buf.GetSize());
}
template void BLDebug::AppendData(const char *filename, const WDL_TypedBuf<float> &buf);
template void BLDebug::AppendData(const char *filename, const WDL_TypedBuf<double> &buf);

template <typename FLOAT_TYPE>
void
BLDebug::AppendValue(const char *filename, FLOAT_TYPE value)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "a+");
    
    fprintf(file, "%g ", value);
    
    fclose(file);
}
template void BLDebug::AppendValue(const char *filename, float value);
template void BLDebug::AppendValue(const char *filename, double value);

void
BLDebug::AppendNewLine(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "a+");
    
    fprintf(file, "\n");
    
    fclose(file);
}

void
BLDebug::DumpShortData(const char *filename, const short *data, int size)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int i = 0; i < size; i++)
        fprintf(file, "%d ", data[i]);
    
    fclose(file);
}

void
BLDebug::AppendShortData(const char *filename, short data)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "a");
    fprintf(file, "%d ", data);
    
    fclose(file);
}

template <typename FLOAT_TYPE>
void
BLDebug::DumpData2D(const char *filename, const FLOAT_TYPE *data, int width, int height)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
            fprintf(file, "%g ", data[i + j*width]);
        fprintf(file, "\n");
    }
    
    fclose(file);
}
template void BLDebug::DumpData2D(const char *filename, const float *data, int width, int height);
template void BLDebug::DumpData2D(const char *filename, const double *data, int width, int height);

void
BLDebug::DumpComplexData(const char *filenameMagn, const char *filenamePhase,
                       const WDL_FFT_COMPLEX *buf, int size)
{
    // Magnitudes
    char fullFilenameMagn[MAX_PATH];
    sprintf(fullFilenameMagn, BASE_FILE"%s", filenameMagn);
    
    FILE *fileMagn = fopen(fullFilenameMagn, "w");
    for (int i = 0; i < size; i++)
        fprintf(fileMagn, "%g ", COMP_MAGN(buf[i]));
    fclose(fileMagn);
    
    // Phases
    char fullFilenamePhase[MAX_PATH];
    sprintf(fullFilenamePhase, BASE_FILE"%s", filenamePhase);
    
    FILE *filePhase = fopen(fullFilenamePhase, "w");
    for (int i = 0; i < size; i++)
    {
      BL_FLOAT phase = std::atan2(buf[i].im, buf[i].re);
        fprintf(filePhase, "%g ", phase);
    }
    
    fclose(filePhase);
}

void
BLDebug::DumpRawComplexData(const char *filenameRe, const char *filenameImag,
                          const WDL_FFT_COMPLEX *buf, int size)
{
    // Magnitudes
    char fullFilenameRe[MAX_PATH];
    sprintf(fullFilenameRe, BASE_FILE"%s", filenameRe);
    
    FILE *fileRe = fopen(fullFilenameRe, "w");
    for (int i = 0; i < size; i++)
        fprintf(fileRe, "%g ", buf[i].re);
    fclose(fileRe);
    
    // Phases
    char fullFilenameImag[MAX_PATH];
    sprintf(fullFilenameImag, BASE_FILE"%s", filenameImag);
    
    FILE *fileImag = fopen(fullFilenameImag, "w");
    for (int i = 0; i < size; i++)
    {
        fprintf(fileImag, "%g ", buf[i].im);
    }
    
    fclose(fileImag);
}

template <typename FLOAT_TYPE>
void
BLDebug::DumpPhases(const char *filename, const WDL_TypedBuf<FLOAT_TYPE> &data)
{
    DumpPhases(filename, data.Get(), data.GetSize());
}
template void BLDebug::DumpPhases(const char *filename, const WDL_TypedBuf<float> &data);
template void BLDebug::DumpPhases(const char *filename, const WDL_TypedBuf<double> &data);

template <typename FLOAT_TYPE>
void
BLDebug::DumpPhases(const char *filename, const FLOAT_TYPE *data, int size)
{
    if (size < 1)
        return;
    
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    
    FLOAT_TYPE prevPhase = data[0];
    for (int i = 0; i < size; i++)
    {
        FLOAT_TYPE phase = data[i];
        
        while(phase < prevPhase)
            phase += 2.0*M_PI;
        
        fprintf(file, "%g ", phase);
        
        prevPhase = phase;
    }
    
    fclose(file);
}
template void BLDebug::DumpPhases(const char *filename, const float *data, int size);
template void BLDebug::DumpPhases(const char *filename, const double *data, int size);

bool
BLDebug::ExitAfter(Plugin *plug, int numSeconds)
{
    static int numSamples = 0;

    BL_FLOAT sampleRate = plug->GetSampleRate();
    int blockSize = plug->GetBlockSize();
    
    int maxNumSamples = numSeconds*sampleRate;
    if (numSamples >= maxNumSamples)
    {
        //plug->CloseAudio();
        //plug->GetUI()->CloseWindow();
        //delete plug->GetUI(); // segfault
        //exit(EXIT_SUCCESS);
        
        // This makes a segfault, but close everything...
        delete plug;

        //return true;
        return false;
    }
    numSamples += blockSize;

    return false;
}

double
BLDebug::ComputeRealSampleRate(double *prevTime, double *prevSR, int nFrames)
{
    
    double now = BLUtils::GetTimeMillisF();
    if (*prevTime < 0.0)
    {
        *prevTime = now;
        
        return 0.0;
    }
    
    double elapsed = now - *prevTime;
    *prevTime = now;
    double elapsedSec = elapsed*0.001;
    double rate = nFrames/elapsedSec;

    if (*prevSR < 0.0)
        *prevSR = rate;
    
#define SMOOTH_FAC 0.95
    rate = (*prevSR)*SMOOTH_FAC + rate*(1.0 - SMOOTH_FAC);
    *prevSR = rate;

    return rate;
}
