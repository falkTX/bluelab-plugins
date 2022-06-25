/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  BLDebug.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 03/05/17.
//
//

#ifndef __Denoiser__BLDebug__
#define __Denoiser__BLDebug__

// May be useful tu uncomment if BLDebug is included in WDL src files
//#define APP_API 1

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"
using namespace iplug;

#include "../../WDL/fft.h"

#ifdef WIN32
#include <windows.h>
#define BL_DBG_PRINTF(...) { char cad[512]; sprintf(cad, __VA_ARGS__); OutputDebugString(cad); }
#endif

class BLDebug
{
public:
    static const char *GetDebugBaseFile();
    
    static void AppendMessage(const char *filename, const char *message);
    
    template <typename FLOAT_TYPE>
    static void DumpData(const char *filename, const FLOAT_TYPE *data, int size);
    
    template <typename FLOAT_TYPE>
    static void DumpData(const char *filename, const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static void DumpData(const char *filename, const vector<FLOAT_TYPE> &buf);
    
    static void DumpData(const char *filename, const int *data, int size);
    
    static void DumpData(const char *filename, const WDL_TypedBuf<int> &buf);
    static void DumpData(const char *filename, const vector<int> &buf);
    
    static void DumpData(const char *filename,
                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf);

    static void DumpDataMagns(const char *filename,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf);
        
    template <typename FLOAT_TYPE>
    static void LoadData(const char *filename, FLOAT_TYPE *data, int size);
    
    template <typename FLOAT_TYPE>
    static void LoadData(const char *filename, WDL_TypedBuf<FLOAT_TYPE> *buf);
    
    template <typename FLOAT_TYPE>
    static void DumpValue(const char *filename, FLOAT_TYPE value);
    
    static void ResetFile(const char *filename);
    
    template <typename FLOAT_TYPE>
    static void AppendData(const char *filename, const FLOAT_TYPE *data, int size);
    
    template <typename FLOAT_TYPE>
    static void AppendData(const char *filename, const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static void AppendValue(const char *filename, FLOAT_TYPE value);
    
    static void AppendNewLine(const char *filename);
    
    static void DumpShortData(const char *filename, const short *data, int size);
  
    static void AppendShortData(const char *filename, const short data);
    
    template <typename FLOAT_TYPE>
    static void DumpData2D(const char *filename, const FLOAT_TYPE *data,
                           int width, int height);
    
    static void DumpComplexData(const char *filenameMagn,
                                const char *filenamePhase,
                                const WDL_FFT_COMPLEX *buf, int size);
    
    static void DumpRawComplexData(const char *filenameRe,
                                   const char *filenameImag,
                                   const WDL_FFT_COMPLEX *buf, int size);
    
    template <typename FLOAT_TYPE>
    static void DumpPhases(const char *filename,
                           const WDL_TypedBuf<FLOAT_TYPE> &data);
    
    template <typename FLOAT_TYPE>
    static void DumpPhases(const char *filename, const FLOAT_TYPE *data, int size);

    static bool ExitAfter(Plugin *plug, int numSeconds);

    // To be called from ProcessBlock
    static double ComputeRealSampleRate(double *prevTime, double *prevSR,
                                        int nFrames);

    static void MessageBox(const char *message);
};

#endif /* defined(__Denoiser__BLDebug__) */
