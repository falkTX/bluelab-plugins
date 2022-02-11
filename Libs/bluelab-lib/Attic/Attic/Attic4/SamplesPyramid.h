//
//  SamplesPyramid.h
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#ifndef __BL_Ghost__SamplesPyramid__
#define __BL_Ghost__SamplesPyramid__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"


// Make "MipMaps" with samples
// (to optimize when displaying waveform of long sound)
class SamplesPyramid
{
public:
    SamplesPyramid();
    
    virtual ~SamplesPyramid();
    
    void Reset();
    
    void SetValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void PushValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void PopValues(long numSamples);
    
    void ReplaceValues(long start, const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void GetValues(long start, long end, long numValues,
                   WDL_TypedBuf<BL_FLOAT> *samples);
    
protected:
    vector<WDL_TypedBuf<BL_FLOAT> > mSamplesPyramid;
    
    // Keep a push buffer, to be able to push
    // by blocks of power of two (otherwise,
    // there are artefacts on the second half).
    WDL_TypedBuf<BL_FLOAT> mPushBuf;
    
    long mRemainToPop;
};

#endif /* defined(__BL_Ghost__SamplesPyramid__) */
