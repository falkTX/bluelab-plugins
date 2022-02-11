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

#include "IPlug_include_in_plug_hdr.h"
//#include "IControl.h"


// See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/

// FreqAdjustObj2: manage oversampling (renamed overlapping),
// and also FREQ_RES (named oversampling)
class FreqAdjustObj2
{
public:
    FreqAdjustObj2(int bufferSize, int overlapping, int oversampling,
                   BL_FLOAT sampleRate);
    
    virtual ~FreqAdjustObj2();
    
    void Reset(int bufferSize, int overlapping, int oversampling);
    
    void ComputeRealFrequencies(const WDL_TypedBuf<BL_FLOAT> &ioPhases, WDL_TypedBuf<BL_FLOAT> *outRealFreqs);
    
    void ComputePhases(WDL_TypedBuf<BL_FLOAT> *ioPhases, const WDL_TypedBuf<BL_FLOAT> &realFreqs);
    
protected:
    static BL_FLOAT MapToPi(BL_FLOAT val);
    
    int mBufferSize;
    int mOverlapping;
    int mOversampling;
    BL_FLOAT mSampleRate;
    
    // For correct phase computation
    WDL_TypedBuf<BL_FLOAT> mLastPhases;
    WDL_TypedBuf<BL_FLOAT> mSumPhases;
};

#endif /* defined(__PitchShift__FreqAdjustObj__) */
