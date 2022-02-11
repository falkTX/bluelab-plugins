//
//  FreqAdjustObj.h
//  PitchShift
//
//  Created by Apple m'a Tuer on 30/10/17.
//
//

#ifndef __PitchShift__FreqAdjustObj__
#define __PitchShift__FreqAdjustObj__

#include "IControl.h"

// See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
class FreqAdjustObj
{
public:
    FreqAdjustObj(int bufferSize, int oversampling, double sampleRate);
    
    virtual ~FreqAdjustObj();
    
    void Reset();
    
    void ComputeRealFrequencies(const WDL_TypedBuf<double> &ioPhases, WDL_TypedBuf<double> *outRealFreqs);
    
    void ComputePhases(WDL_TypedBuf<double> *ioPhases, const WDL_TypedBuf<double> &realFreqs);
    
protected:
    static double MapToPi(double val);
    
    int mBufferSize;
    int mOversampling;
    double mSampleRate;
    
    // For correct phase computation
    WDL_TypedBuf<double> mLastPhases;
    WDL_TypedBuf<double> mSumPhases;
};

#endif /* defined(__PitchShift__FreqAdjustObj__) */
