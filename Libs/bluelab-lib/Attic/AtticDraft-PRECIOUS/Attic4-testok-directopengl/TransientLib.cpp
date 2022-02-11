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

void
TransientLib::DetectTransients(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf, WDL_TypedBuf<double> *transients, double threshold)
{
    // Compute transients probability
    
    int bufSize = fftBuf->GetSize();
    
    for (int i = 0; i < bufSize; i++)
        transients->Get()[i] = 0.0;
    
    double prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = fftBuf->Get()[i];
        double r = c.re;
        double j = c.im;
        
        double phase = 0.0;
        if (fabs(r) > 0.0)
            phase = atan2(j, r);
        
        double phaseDiff = phase - prev;
        prev = phase;
        
        // TEST NIKO
        
        // Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // TEST NIKO: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        double transPos = ((double)bufSize)*phaseDiff/(2.0*M_PI);
        
        //double weight = log(1.0 + j*j + r*r);
        
        double weight = log(1.0 + sqrt(j*j + r*r));
        
        if (weight > threshold)
            transients->Get()[(int)transPos] += weight;
    }
}
