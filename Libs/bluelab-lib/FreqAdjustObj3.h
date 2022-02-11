//
//  FreqAdjustObj.h
//  PitchShift
//
//  Created by Apple m'a Tuer on 30/10/17.
//
//

#ifndef __PitchShift__FreqAdjustObj__
#define __PitchShift__FreqAdjustObj__

#include <BLTypes.h>

#include "IControl.h"

// See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/

// FreqAdjustObj2: manage oversampling (renamed overlapping),
// and also FREQ_RES (named oversampling)
//
// Added BILL_FARMER_HACK (at least...)
//

#define BAD_COMPUTE_INITIAL_PHASES 0
#define DEBUG_STEREO_TEST 0

class FreqAdjustObj3
{
public:
    FreqAdjustObj3(int bufferSize, int overlapping,
                   int oversampling, BL_FLOAT sampleRate);
    
    virtual ~FreqAdjustObj3();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
#if DEBUG_STEREO_TEST
    void SetPhases(const WDL_TypedBuf<BL_FLOAT> &phases,
                   const WDL_TypedBuf<BL_FLOAT> &prevPhases);
#endif
    
    void ComputeRealFrequencies(const WDL_TypedBuf<BL_FLOAT> &ioPhases, WDL_TypedBuf<BL_FLOAT> *outRealFreqs);
    
    void ComputePhases(WDL_TypedBuf<BL_FLOAT> *ioPhases, const WDL_TypedBuf<BL_FLOAT> &realFreqs);
    
protected:
#if BAD_COMPUTE_INITIAL_PHASES
    void ComputeInitialLastPhases();
    void ComputeInitialSumPhases();
#endif
    
    static BL_FLOAT SMB_MapToPi(BL_FLOAT val);
    
    static BL_FLOAT MapToPi(BL_FLOAT val);
    static BL_FLOAT MapToPi2(BL_FLOAT val);
    
    int mBufferSize;
    int mOverlapping;
    int mOversampling;
    BL_FLOAT mSampleRate;
    
    // For correct phase computation
    WDL_TypedBuf<BL_FLOAT> mLastPhases;
    WDL_TypedBuf<BL_FLOAT> mSumPhases;
    
    bool mMustSetLastPhases;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
};

#endif /* defined(__PitchShift__FreqAdjustObj__) */
