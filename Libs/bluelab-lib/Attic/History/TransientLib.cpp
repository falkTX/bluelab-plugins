//
//  TransientLib.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

#include <Smoother.h>

#include "TransientLib.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

#define SIGNAL_COEFF 1.0/50

void
TransientLib::DetectTransients(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf,
                               WDL_TypedBuf<BL_FLOAT> *transients, BL_FLOAT threshold)
{
    // Compute transients probability
    int bufSize = fftBuf->GetSize();
    
    for (int i = 0; i < bufSize; i++)
        transients->Get()[i] = 0.0;
    
    BL_FLOAT prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = fftBuf->Get()[i];
        BL_FLOAT r = c.re;
        BL_FLOAT j = c.im;
        
        BL_FLOAT phase = 0.0;
        if (std::fabs(r) > 0.0)
	  phase = std::atan2(j, r);
        
        BL_FLOAT phaseDiff = phase - prev;
        prev = phase;
        
        // TEST NIKO
        
        // Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // TEST NIKO: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        BL_FLOAT transPos = ((BL_FLOAT)bufSize)*phaseDiff/(2.0*M_PI);
        
        //BL_FLOAT weight = std::log(1.0 + j*j + r*r);
        
        BL_FLOAT weight = std::log(1.0 + std::sqrt(j*j + r*r));
        
        if (weight > threshold)
            transients->Get()[(int)transPos] += weight;
    }
}

void
TransientLib::DetectTransients2(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf,
                                BL_FLOAT threshold,
                                WDL_TypedBuf<BL_FLOAT> *strippedMagns,
                                WDL_TypedBuf<BL_FLOAT> *transientMagns,
                                WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns)
{
#if 0
    Debug::DumpComplexData("magns0.txt", "phases0.txt", fftBuf->Get(), fftBuf->GetSize());
    Debug::DumpSingleValue("threshold.txt", threshold);
#endif
    
    // Compute transients probability (and other things...)
    int bufSize = fftBuf->GetSize();
    
    // Set to zero
    for (int i = 0; i < bufSize; i++)
        strippedMagns->Get()[i] = 0.0;
    for (int i = 0; i < bufSize; i++)
        transientMagns->Get()[i] = 0.0;
    
    for (int i = 0; i < bufSize; i++)
        transientIntensityMagns->Get()[i] = 0.0;
    
    BL_FLOAT prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = fftBuf->Get()[i];
        BL_FLOAT re = c.re;
        BL_FLOAT im = c.im;
        
        BL_FLOAT magn = std::sqrt(re*re + im*im);
        
        BL_FLOAT phase = 0.0;
        if (std::fabs(re) > 0.0)
	  phase = std::atan2(im, re);
        
        BL_FLOAT phaseDiff = phase - prev;
        prev = phase;
        
        // TEST NIKO
        
        // Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // TEST NIKO: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        BL_FLOAT transPos = ((BL_FLOAT)bufSize)*phaseDiff/(2.0*M_PI);
        
        //BL_FLOAT weight = std::log(1.0 + j*j + r*r);
        
        BL_FLOAT weight = std::log(1.0 + magn);
        
        if (weight > threshold)
            transientIntensityMagns->Get()[(int)transPos] += weight;
        
        if (weight*SIGNAL_COEFF > threshold)
            transientMagns->Get()[i] = magn;
        else
            strippedMagns->Get()[i] = magn;
    }
    
#if 0
    Debug::DumpData("strippedMagns.txt", strippedMagns->Get(), strippedMagns->GetSize());
    Debug::DumpData("transientMagns.txt", transientMagns->Get(), transientMagns->GetSize());
    Debug::DumpData("transientIntensityMagns.txt",
                    transientIntensityMagns->Get(), transientIntensityMagns->GetSize());
#endif
}
