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

#include "FftProcessObj14.h"

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
                double overlapping, double oversampling,
                double sampleRate);
    
    virtual ~AirProcess2();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, double sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void SetThreshold(double threshold);
    void SetMix(double mix);
    
protected:
    void DetectPartials(const WDL_TypedBuf<double> &magns,
                        const WDL_TypedBuf<double> &phases);
    
    int mBufferSize;
    double mOverlapping;
    double mOversampling;
    double mSampleRate;
    
    PartialTracker3 *mPartialTracker;
    
    double mMix;
    double mTransientSP;
    
    bool mDebugFreeze;
    
#if AIR_PROCESS_PROFILE
    BlaTimer mTimer;
    long mCount;
#endif
};

#endif /* defined(__BL_Air__AirProcess2__) */
