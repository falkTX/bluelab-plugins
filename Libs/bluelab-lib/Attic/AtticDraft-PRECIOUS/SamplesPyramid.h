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
    SamplesPyramid(/*long bufferSize*/);
    
    virtual ~SamplesPyramid();
    
    void SetValues(const WDL_TypedBuf<double> &samples);
    
    void PushValues(const WDL_TypedBuf<double> &samples);
    
    void PopValues(long numSamples);
    
    void GetValues(long start, long end, long numValues,
                   WDL_TypedBuf<double> *samples);
    
protected:
    //long mBufferSize;
    
    vector<WDL_TypedBuf<double> > mSamplesPyramid;
};

#endif /* defined(__BL_Ghost__SamplesPyramid__) */
