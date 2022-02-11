//
//  CMASmoother.h
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#ifndef __Transient__CMASmoother__
#define __Transient__CMASmoother__

#include "../../WDL/IPlug/Containers.h"

// Central moving average smoother
class CMASmoother
{
public:
    CMASmoother(int bufferSize, int windowSize);
    
    virtual ~CMASmoother();

    // Return true if nFrames has been returned
    bool Process(const double *data, double *smoothedData, int nFrames);

    void Reset();
    
    // Process one buffer, without managing streaming to next buffers
    // Fill the missing input data with zeros
    static bool ProcessOne(const double *data, double *smoothedData, int nFrames, int windowSize);
    
protected:
    bool ProcessInternal(const double *data, double *smoothedData, int nFrames);
    
    // Return true if something has been processed
    bool CentralMovingAverage(WDL_TypedBuf<double> &inData, WDL_TypedBuf<double> &outData, int windowSize);
    
    // Hack to avoid bad offset at the end of a signal
    // (This was detected in Transient, when the transient vu-meters didn't goes to 0 when no signal).
    //
    // When the signal goes to 0, an offset is remaining in the Smoother
    // (due to mPrevVal and the two points, left and right, who have the same value).
    // To avoid that, detect when the data value are constant, then update mPrevVal accordingly
    void ManageConstantValues(const double *data, int nFrames);

    
    int mBufferSize;
    int mWindowSize;
    
    bool mFirstTime;
    
    double mPrevVal;
    
    WDL_TypedBuf<double> mInData;
    WDL_TypedBuf<double> mOutData;
};

#endif /* defined(__Transient__CMASmoother__) */
