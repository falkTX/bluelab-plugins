//
//  TransientLib.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__TransientLib__
#define __Transient__TransientLib__

// See: http://werner.yellowcouch.org/Papers/transients12/index.html
class TransientLib
{
public:
    static void DetectTransients(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf, WDL_TypedBuf<double> *transients, double threshold);
};
#endif /* defined(__Transient__TransientLib__) */
