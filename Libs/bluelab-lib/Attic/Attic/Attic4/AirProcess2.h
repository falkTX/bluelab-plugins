//
//  AirProcess2.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Air__AirProcess2__
#define __BL_Air__AirProcess2__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include <PartialTracker3.h>

//#include "FftProcessObj15.h"
#include "FftProcessObj16.h"

#define AIR_PROCESS_PROFILE 0

#if AIR_PROCESS_PROFILE
#include <BlaTimer.h>
#endif

// From AirProcess
// - code clean (removed transient stuff)

class PartialTracker3;
class AirProcess2 : public ProcessObj
{
public:
    AirProcess2(int bufferSize,
                BL_FLOAT overlapping, BL_FLOAT oversampling,
                BL_FLOAT sampleRate);
    
    virtual ~AirProcess2();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void SetThreshold(BL_FLOAT threshold);
    void SetMix(BL_FLOAT mix);
    
protected:
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases);
    
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    PartialTracker3 *mPartialTracker;
    
    BL_FLOAT mMix;
    BL_FLOAT mTransientSP;
    
    bool mDebugFreeze;
    
#if AIR_PROCESS_PROFILE
    BlaTimer mTimer;
    long mCount;
#endif
};

#endif /* defined(__BL_Air__AirProcess2__) */
