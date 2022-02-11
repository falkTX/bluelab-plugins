//
//  KalmanParamSmooth.h
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#ifndef __UST__KalmanParamSmooth__
#define __UST__KalmanParamSmooth__

#include <SimpleKalmanFilter.h>

// "How much do we expect to our measurement vary"
//
// e.g 200Hz
#define DEFAULT_KF_E_MEA 200.0
#define DEFAULT_KF_E_EST DEFAULT_KF_E_MEA

// "usually a small number between 0.001 and 1"
//
// If too low: predicted values move too slowly
// If too high: predicted values go straight
//
#define DEFAULT_KF_Q 5.0

class KalmanParamSmooth
{
public:
    KalmanParamSmooth(BL_FLOAT samplingRate);
    
    virtual ~KalmanParamSmooth();
    
    void Reset(BL_FLOAT samplingRate);
    
    void Reset(BL_FLOAT samplingRate, BL_FLOAT val);
    
    void SetKalmanParameters(BL_FLOAT mea, BL_FLOAT est, BL_FLOAT q);
    
    BL_FLOAT Process(BL_FLOAT inVal);
    
private:
    BL_FLOAT mSampleRate;
    
    SimpleKalmanFilter *mKf;
    
    BL_FLOAT mMea;
    BL_FLOAT mEst;
    BL_FLOAT mQ;
    
    bool mInitialized;
};

#endif /* defined(__UST__KalmanParamSmooth__) */
