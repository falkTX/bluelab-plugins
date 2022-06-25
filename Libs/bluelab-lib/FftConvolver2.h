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
//  FftConvolver.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolver__
#define __Spatializer__FftConvolver__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

// Inspired by FftprocessObj (5)

// FftConvolver2: manage buffers bigger than nFrames
// Used by Impulse
class FftConvolver2
{
public:
    enum AnalysisMethod
    {
        AnalysisMethodNone,
        AnalysisMethodWindow
    };
        
    enum SynthesisMethod
    {
        SynthesisMethodNone,
        SynthesisMethodWindow,
        SynthesisMethodInverseWindow
    };
        
    // Set skipFFT to true to skip fft and use only overlaping
    FftConvolver2(int bufferSize, int oversampling = 2, bool normalize = true,
                  enum AnalysisMethod aMethod = AnalysisMethodWindow,
                  enum SynthesisMethod sMethod = SynthesisMethodWindow);
        
    virtual ~FftConvolver2();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<BL_FLOAT> *response);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
    static void ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                BL_FLOAT sampleRate, BL_FLOAT respSampleRate);

    static void ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse, int newSize);
    
protected:
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &sampleBuf,
                          const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
                          int offsetSamples, int offsetResult);
        
    BL_FLOAT ComputeWinFactor(const WDL_TypedBuf<BL_FLOAT> &window);
        
    void ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                    bool noemalize);
        
    void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                       WDL_TypedBuf<BL_FLOAT> *samples);
        
    static void NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns);
        
    static void FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
        
    int mBufSize;
    int mOverlap;
    int mOversampling;
    int mShift;
        
    BL_FLOAT mAnalysisWinFactor;
    BL_FLOAT mSynthesisWinFactor;
    
    WDL_TypedBuf<BL_FLOAT> mSamplesBuf;
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
        
    WDL_TypedBuf<BL_FLOAT> mAnalysisWindow;
    WDL_TypedBuf<BL_FLOAT> mSynthesisWindow;
    
    // Need a member offset for the result
    // because it can remain processed results,
    // if we have just less than nFrames results
    // from one host processing loop to another
    int mOffsetResult;
    
    // Tweak parameters
    bool mNormalize;
        
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
        
    // Response transformed by fft
    WDL_TypedBuf<WDL_FFT_COMPLEX> mResponse;
    
    // Set to true when reset
    // If set to true, the left half of the OLA window will be 1.0
    // Avoids fade-in when starting
    // (particularly noticable with big buffers
    bool mFirstProcess;
};

#endif /* defined(__Spatializer__FftConvolver__) */
