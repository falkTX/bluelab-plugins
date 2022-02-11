//
//  PhasesUnwrapper.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/1/20.
//
//

#ifndef __BL_SoundMetaViewer__PhasesUnwrapper__
#define __BL_SoundMetaViewer__PhasesUnwrapper__

#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class PhasesUnwrapper
{
public:
    PhasesUnwrapper(long historySize);
    
    virtual ~PhasesUnwrapper();

    void Reset();
    
    void SetHistorySize(long historySize);
    
    // Along freqs
    static void UnwrapPhasesFreq(WDL_TypedBuf<BL_FLOAT> *phases);
    void NormalizePhasesFreq(WDL_TypedBuf<BL_FLOAT> *phases);
    
    // Must call UnwrapPhasesFreq before
    void ComputePhasesGradientFreqs(WDL_TypedBuf<BL_FLOAT> *phases);
    void NormalizePhasesGradientFreqs(WDL_TypedBuf<BL_FLOAT> *phases);
    
    // Along time
    void UnwrapPhasesTime(WDL_TypedBuf<BL_FLOAT> *phases);
    void NormalizePhasesTime(WDL_TypedBuf<BL_FLOAT> *phases);

    static void UnwrapPhasesTime(const WDL_TypedBuf<BL_FLOAT> &phases0,
                                 WDL_TypedBuf<BL_FLOAT> *phases1);

    // See: http://kth.diva-portal.org/smash/get/diva2:1381398/FULLTEXT01.pdf
    static void ComputeUwPhasesDiffTime(WDL_TypedBuf<BL_FLOAT> *diff,
                                        const WDL_TypedBuf<BL_FLOAT> &phases0,
                                        const WDL_TypedBuf<BL_FLOAT> &phases1,
                                        BL_FLOAT sampleRate, int bufferSize,
                                        int overlapping);
    
    // Must call UnwrapPhasesTime() before
    void ComputePhasesGradientTime(WDL_TypedBuf<BL_FLOAT> *phases);
    void NormalizePhasesGradientTime(WDL_TypedBuf<BL_FLOAT> *phases);
    
protected:
    long mHistorySize;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mUnwrappedPhasesTime;
    deque<WDL_TypedBuf<BL_FLOAT> > mUnwrappedPhasesFreqs;
    
    // For FREQ_GLOBAL_MIN_MAX
    BL_FLOAT mGlobalMinDiff;
    BL_FLOAT mGlobalMaxDiff;
};

#endif /* defined(__BL_SoundMetaViewer__PhasesUnwrapper__) */
