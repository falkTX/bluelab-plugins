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
class FreqAdjustObj
{
public:
    FreqAdjustObj(int bufferSize, int oversampling, BL_FLOAT sampleRate);
    
    virtual ~FreqAdjustObj();
    
    void Reset();
    
    void ComputeRealFrequencies(const WDL_TypedBuf<BL_FLOAT> &ioPhases, WDL_TypedBuf<BL_FLOAT> *outRealFreqs);
    
    void ComputePhases(WDL_TypedBuf<BL_FLOAT> *ioPhases, const WDL_TypedBuf<BL_FLOAT> &realFreqs);
    
protected:
    static BL_FLOAT MapToPi(BL_FLOAT val);
    
    int mBufferSize;
    int mOversampling;
    BL_FLOAT mSampleRate;
    
    // For correct phase computation
    WDL_TypedBuf<BL_FLOAT> mLastPhases;
    WDL_TypedBuf<BL_FLOAT> mSumPhases;
};

#endif /* defined(__PitchShift__FreqAdjustObj__) */
