//
//  SoftMaskingN.h
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#ifndef __BL_DUET__SoftMaskingN__
#define __BL_DUET__SoftMaskingN__

#include <deque>
#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Wiener soft masking
//
// See: https://github.com/TUIlmenauAMS/ASP/blob/master/MaskingMethods.py
// and: http://www.jonathanleroux.org/pdf/Erdogan2015ICASSP04.pdf
// and: https://www.researchgate.net/publication/220736985_Degenerate_Unmixing_Estimation_Technique_using_the_Constant_Q_Transform
// and: https://hal.inria.fr/inria-00544949/document
//

// SoftMasking2: from SoftMasking
// - code clean
//
// SoftMaskingN: from SoftMasking2
// - manage more than 1 mix and 1 mask
class SoftMaskingN
{
public:
    SoftMaskingN(int historySize);
    
    virtual ~SoftMaskingN();
  
    void Reset();
    
    void SetHistorySize(int size);
    
    // Mixture is the full sound
    // estimMagns is the estimated sound for a given mask
    void Process(const WDL_TypedBuf<BL_FLOAT> &mixtureMagns,
                 const vector<WDL_TypedBuf<BL_FLOAT> > &estimMagns,
                 vector<WDL_TypedBuf<BL_FLOAT> > *softMasks);
    
protected:
    void ComputeVariance(deque<WDL_TypedBuf<BL_FLOAT> > &history,
                         WDL_TypedBuf<BL_FLOAT> *outVariance);
    
    //
    int mHistorySize;
    
    
    // Mixture is the whole mixture, minus the sound corresponding to the mask
    //deque<WDL_TypedBuf<BL_FLOAT> > mMixtureHistory;
    // Must keep as many mixture history as number of masks
    // because mixture = full signal - mask signal
    vector<deque<WDL_TypedBuf<BL_FLOAT> > > mMixtureHistory;
    
    // The sound corresponding to the mask (estimMagns);
    vector<deque<WDL_TypedBuf<BL_FLOAT> > > mHistory;
};

#endif /* defined(__BL_DUET__SoftMaskingN__) */
