//
//  PhaseCorrectObj.h
//  BL-PitchShift
//
//  Created by Pan on 03/04/18.
//
//

#ifndef __BL_PitchShift__PhaseCorrectObj__
#define __BL_PitchShift__PhaseCorrectObj__

#include "IPlug_include_in_plug_hdr.h"

class PhaseCorrectObj
{
public:
    PhaseCorrectObj(int bufferSize, int oversampling, BL_FLOAT sampleRate);
    
    virtual ~PhaseCorrectObj();

    void Reset(int bufferSize, int oversampling, BL_FLOAT sampleRate);

    void Process(const WDL_TypedBuf<BL_FLOAT> &freqs,
                 WDL_TypedBuf<BL_FLOAT> *ioPhases);

protected:
    BL_FLOAT MapToPi(BL_FLOAT val);
    
    bool mFirstTime;

    int mBufferSize;

    int mOversampling;

    BL_FLOAT mSampleRate;

    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
};

#endif /* defined(__BL_PitchShift__PhaseCorrectObj__) */
