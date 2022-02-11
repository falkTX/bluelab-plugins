//
//  SoftMaskingComp.h
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#ifndef __BL_DUET__SoftMaskingComp__
#define __BL_DUET__SoftMaskingComp__

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
class SoftMaskingComp
{
public:
    SoftMaskingComp(int historySize);
    
    virtual ~SoftMaskingComp();
  
    void Reset();
    
    void SetHistorySize(int size);
    
    void Process(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                 WDL_TypedBuf<WDL_FFT_COMPLEX> *mask);
    
protected:
    void ProcessTime(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                     WDL_TypedBuf<WDL_FFT_COMPLEX> *mask);
    
    void ProcessFreq(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                     WDL_TypedBuf<WDL_FFT_COMPLEX> *mask);
    
    void ProcessTimeFreq(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixtureValues,
                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &values,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *mask);
    
    void ComputeVariance(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *variance);
    
    void ComputeVariance(const WDL_TypedBuf<WDL_FFT_COMPLEX> &data,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *variance);
    
    void ComputeVarianceWin(deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &history,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *variance);
    
    //
    int mHistorySize;
    int mFreqWinSize;
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixtureHistory;
    
    // The sound corresponding to the mask
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mHistory;
};

#endif /* defined(__BL_DUET__SoftMaskingComp__) */
