//
//  SoftMaskingComp2.h
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#ifndef __BL_DUET__SoftMaskingComp2__
#define __BL_DUET__SoftMaskingComp2__

#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// Wiener soft masking
//
// See: https://github.com/TUIlmenauAMS/ASP/blob/master/MaskingMethods.py
// and: http://www.jonathanleroux.org/pdf/Erdogan2015ICASSP04.pdf
// and: https://www.researchgate.net/publication/220736985_Degenerate_Unmixing_Estimation_Technique_using_the_Constant_Q_Transform
// and: https://hal.inria.fr/inria-00544949/document
//

// From SoftMasking, but with complex numbers. Returns a complex mask.
//

// SoftMaskingComp2: from SoftMaskingComp
// - code clean
class SoftMaskingComp2
{
public:
    SoftMaskingComp2(int historySize);
    
    virtual ~SoftMaskingComp2();
  
    void Reset();
    
    void SetHistorySize(int size);
    
    // Mixture is the full sound
    // magns is the estimated sound for a given mask
    void Process(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                 WDL_TypedBuf<WDL_FFT_COMPLEX> *softMask);
    
    // Return the centerd data values in ioMixtureValues and ioValues
    void ProcessCentered(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioMixtureValues,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *ioValues,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *softMask);
    
protected:
    void ComputeVariance(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *outVariance);
    
    //
    int mHistorySize;
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixtureHistory;
    
    // The sound corresponding to the mask
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mHistory;
};

#endif /* defined(__BL_DUET__SoftMaskingComp2__) */
