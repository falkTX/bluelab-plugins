//
//  USTVumeterProcess.cpp
//  UST
//
//  Created by applematuer on 8/14/19.
//
//

#include <LUFSMeter.h>

#include <BLUtils.h>
#include <ParamSmoother.h>

#include "USTVumeterProcess.h"

// Buffer size in millis for computing RMS
// Between 20ms and 50ms
#define WINDOW_SIZE 50.0 //20.0

#define MAX_NUM_SAMPLES 200000 //2048

#define DB_EPS 1e-15


// See: https://sonicscoop.com/2018/03/29/everything-need-know-audio-meteringand/


USTVumeterProcess::USTVumeterProcess(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mValue = VUMETER_MIN_GAIN;
    mPeakValue = VUMETER_MIN_GAIN;
    
#if USE_SMOOTHER
    mValueSmoother = new ParamSmoother(0.0, 0.9/*0.95*/);
#endif
}

USTVumeterProcess::~USTVumeterProcess()
{    
#if USE_SMOOTHERS
    delete mValueSmoother;
#endif
}

void
USTVumeterProcess::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
#if USE_SMOOTHERS
    mValueSmoother->Reset();
#endif
}

void
USTVumeterProcess::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mSamples.Add(samples.Get(), samples.GetSize());
                        
    int numToConsume = mSamples.GetSize() - MAX_NUM_SAMPLES;
    if (numToConsume > 0)
        BLUtils::ConsumeLeft(&mSamples, numToConsume);
    
    Update();
}

void
USTVumeterProcess::Update()
{
    ComputeValue(mSamples, &mValue);
    ComputePeakValue(mSamples, &mPeakValue);
    
#if USE_SMOOTHER
    mValueSmoother->SetNewValue(mValue);
    mValueSmoother->Update();
    mValue = mValueSmoother->GetCurrentValue();
#endif
        
    long numToConsume = ComputeNumToConsume();
    BLUtils::ConsumeLeft(&mSamples, numToConsume);
}

BL_FLOAT
USTVumeterProcess::GetValue()
{
    return mValue;
}

BL_FLOAT
USTVumeterProcess::GetPeakValue()
{
    return mPeakValue;
}

void
USTVumeterProcess::ComputeValue(const WDL_TypedBuf<BL_FLOAT> &samples,
                                BL_FLOAT *ioResult)
{
    long bufferSize = mSampleRate*WINDOW_SIZE/1000.0;
    
    BL_FLOAT sumAvg = 0.0;
    int numBuffers = 0;
    for (int i = 0; i < samples.GetSize(); i += bufferSize)
    {
        if (i + bufferSize > samples.GetSize())
            break;
        
        // FALSE !
        // See: https://stackoverflow.com/questions/4152201/calculate-decibels
        //BL_FLOAT avg = BLUtils::ComputeRMSAvg2(&samples.Get()[i], bufferSize);
        
        // GOOD
        BL_FLOAT avg = BLUtils::ComputeRMSAvg(&samples.Get()[i], bufferSize);
        
        // TEST
        //BL_FLOAT avg = BLUtils::ComputeAbsAvg(&samples.Get()[i], bufferSize);
        
        sumAvg += avg;
        numBuffers++;
    }
    
    if (numBuffers == 0)
        return;
    
    BL_FLOAT avg = sumAvg/numBuffers;
    
    BL_FLOAT gain = BLUtils::AmpToDB(avg, (BL_FLOAT)DB_EPS, (BL_FLOAT)VUMETER_MIN_GAIN);
    
    *ioResult = gain;
}

void
USTVumeterProcess::ComputePeakValue(const WDL_TypedBuf<BL_FLOAT> &samples,
                                    BL_FLOAT *ioResult)
{
    long bufferSize = mSampleRate*WINDOW_SIZE/1000.0;
    
    BL_FLOAT globalMax = 0.0;
    bool bufferProcessed =  false;
    for (int i = 0; i < samples.GetSize(); i += bufferSize)
    {
        if (i + bufferSize > samples.GetSize())
            break;
        
        BL_FLOAT max = BLUtils::ComputeMaxAbs(&samples.Get()[i], bufferSize);
        
        if (max > globalMax)
            globalMax = max;
        
        bufferProcessed = true;
    }
    
    if (!bufferProcessed)
        return;
    
    BL_FLOAT gain = BLUtils::AmpToDB(globalMax, (BL_FLOAT)DB_EPS, (BL_FLOAT)VUMETER_MIN_GAIN);
    
    *ioResult = gain;
}

long
USTVumeterProcess::ComputeNumToConsume()
{
    long bufferSize = mSampleRate*WINDOW_SIZE/1000.0;

    if (mSamples.GetSize() < bufferSize)
        return 0;
    
    long result = mSamples.GetSize() - bufferSize;
    
    return result;
}
