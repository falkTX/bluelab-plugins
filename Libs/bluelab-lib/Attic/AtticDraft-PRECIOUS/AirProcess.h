//
//  AirProcess.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Air__AirProcess__
#define __BL_Air__AirProcess__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include <PartialTracker.h>

#include <BlaTimer.h>

#include "FftProcessObj14.h"


#define AIR_PROCESS_PROFILE 0


class PartialTracker;

class AirProcess : public ProcessObj
{
public:
    AirProcess(int bufferSize,
               double overlapping, double oversampling,
               double sampleRate);
    
    virtual ~AirProcess();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, double sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void SetThreshold(double threshold);
        
    // Amp to dB
    static double AmpToDBNorm(double val);
    static double DBToAmpNorm(double val);
    
protected:
    int GetDisplayRefreshRate();
    
    void ScaleFreqs(WDL_TypedBuf<double> *values);
    void AmpsToDb(WDL_TypedBuf<double> *magns);
    
    // Apply freq scale to freq id
    int ScaleFreq(int idx);
    
    void DetectPartials(const WDL_TypedBuf<double> &magns,
                        const WDL_TypedBuf<double> &phases);
    
    void AmpsToDBNorm(WDL_TypedBuf<double> *amps);
    
    void IdToColor(int idx, unsigned char color[3]);
    
    void PartialToColor(const PartialTracker::Partial &partial,
                        unsigned char color[3]);

    // Display
    void DisplayAmplitude();
    void DisplayFrequency();
    void DisplayColor();
    void DisplayWarping();
    
    int mBufferSize;
    double mOverlapping;
    double mOversampling;
    double mSampleRate;
    
    WDL_TypedBuf<double> mValues;
    
    PartialTracker *mPartialTracker;
    
#if AIR_PROCESS_PROFILE
    BlaTimer mTimer0;
    long mTimerCount0;
#endif
};

#endif /* defined(__BL_Air__AirProcess__) */
