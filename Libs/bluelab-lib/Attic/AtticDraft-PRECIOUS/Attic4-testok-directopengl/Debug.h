//
//  Debug.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 03/05/17.
//
//

#ifndef __Denoiser__Debug__
#define __Denoiser__Debug__

#include "../../WDL/fft.h"

class Debug
{
public:
    static void DumpMessage(const char *filename, const char *message);
    
    static void DumpData(const char *filename, double *data, int size);
    
    static void DumpData2D(const char *filename, double *data, int width, int height);
    
    static void DumpComplexData(const char *filenameMagn, const char *filenamePhase, WDL_FFT_COMPLEX *buf, int size);
    
    static void DumpRawComplexData(const char *filenameRe, const char *filenameImag, WDL_FFT_COMPLEX *buf, int size);
};

#endif /* defined(__Denoiser__Debug__) */
