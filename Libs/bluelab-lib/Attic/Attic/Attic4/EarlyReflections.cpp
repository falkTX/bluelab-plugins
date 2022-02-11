//
//  EarlyReflections.cpp
//  BL-ReverbDepth
//
//  Created by applematuer on 9/1/20.
//
//

#include <DelayObj4.h>
#include <BLUtils.h>

#include "EarlyReflections.h"

// For early reflections reverb (in meters)
#define DEFAULT_ROOM_SIZE 10.0
#define DEFAULT_INTERMIC_DIST 0.1

#define SOUND_SPEED 343.0
#define DEPTH_COEFF 2.0

#define DEFAULT_DELAY 100

#if 0
TODO: if we need more reflections than 2x2, consider using delays/combfilters,
to get a kind of feedback
#endif

EarlyReflections::EarlyReflections(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mRoomSize = DEFAULT_ROOM_SIZE;
    mIntermicDist = DEFAULT_INTERMIC_DIST;
    mNormDepth = 1.0;
    
    for (int i = 0; i < NUM_DELAYS; i++)
        mDelays[i] = new DelayObj4(DEFAULT_DELAY);
    
    Init();
}

EarlyReflections::EarlyReflections(const EarlyReflections &other)
{
    mSampleRate = other.mSampleRate;
    
    mRoomSize = other.mRoomSize;
    mIntermicDist = other.mIntermicDist;
    mNormDepth = other.mNormDepth;
    
    for (int i = 0; i < NUM_DELAYS; i++)
        mDelays[i] = new DelayObj4(DEFAULT_DELAY);
    
    Init();
}

EarlyReflections::~EarlyReflections()
{
    for (int i = 0; i < NUM_DELAYS; i++)
        delete mDelays[i];
}

void
EarlyReflections::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < NUM_DELAYS; i++)
        mDelays[i]->Reset();
    
    Init();
}

void
EarlyReflections::Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                          WDL_TypedBuf<BL_FLOAT> outRevSamples[2])
{
    // Compute delays
    WDL_TypedBuf<BL_FLOAT> samplesL0 = samples;
    mDelays[0]->ProcessSamples(&samplesL0);
    
    WDL_TypedBuf<BL_FLOAT> samplesL1 = samples;
    mDelays[1]->ProcessSamples(&samplesL1);
    
    WDL_TypedBuf<BL_FLOAT> samplesR0 = samples;
    mDelays[2]->ProcessSamples(&samplesR0);
    
    WDL_TypedBuf<BL_FLOAT> samplesR1 = samples;
    mDelays[3]->ProcessSamples(&samplesR1);
    
    // Result
    outRevSamples[0].Resize(samples.GetSize());
    outRevSamples[1].Resize(samples.GetSize());
    
    BLUtils::FillAllZero(&outRevSamples[0]);
    BLUtils::AddValues(&outRevSamples[0], samplesL0);
    BLUtils::AddValues(&outRevSamples[0], samplesL1);
    
    BLUtils::FillAllZero(&outRevSamples[1]);
    BLUtils::AddValues(&outRevSamples[1], samplesR0);
    BLUtils::AddValues(&outRevSamples[1], samplesR1);
    
    // Ponderate with depth
    BLUtils::MultValues(&outRevSamples[0], mNormDepth*(BL_FLOAT)DEPTH_COEFF);
    BLUtils::MultValues(&outRevSamples[1], mNormDepth*(BL_FLOAT)DEPTH_COEFF);
}

void
EarlyReflections::Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                          WDL_TypedBuf<BL_FLOAT> *outRevSamplesL,
                          WDL_TypedBuf<BL_FLOAT> *outRevSamplesR)
{
    WDL_TypedBuf<BL_FLOAT> outRevSamples[2];
    Process(samples, outRevSamples);
    
    *outRevSamplesL = outRevSamples[0];
    *outRevSamplesR = outRevSamples[1];
}

void
EarlyReflections::SetRoomSize(BL_FLOAT roomSize)
{
    mRoomSize = roomSize;
}

void
EarlyReflections::SetIntermicDist(BL_FLOAT dist)
{
    mIntermicDist = dist;
}

void
EarlyReflections::SetNormDepth(BL_FLOAT depth)
{
    mNormDepth = depth;
}

// May have mistakes (see the picture in UST/Images)
void
EarlyReflections::Init()
{
    // Source coordinate
    BL_FLOAT S[2] = { (BL_FLOAT)(mRoomSize/2.0), (BL_FLOAT)(mRoomSize/2.0) };
    
#if 0
    BL_FLOAT normPan = (mPan + 1.0)/2.0;
    S[0] = normPan*mRoomSize;
#endif
    
    // Mics coordinates
    BL_FLOAT L[2] = { (BL_FLOAT)(mRoomSize/2.0 - mIntermicDist/2.0),
                      (BL_FLOAT)0.0 };
    BL_FLOAT R[2] = { (BL_FLOAT)(mRoomSize/2.0 + mIntermicDist/2.0),
                      (BL_FLOAT)0.0 };
    
    // Reflection points
    BL_FLOAT Lp0[2] = { (BL_FLOAT)((L[0] + S[0])/2.0), (BL_FLOAT)mRoomSize };
    BL_FLOAT Rp0[2] = { (BL_FLOAT)((R[0] + S[0])/2.0), (BL_FLOAT)mRoomSize };
    
    BL_FLOAT Lp1[2] = { (BL_FLOAT)0.0, (BL_FLOAT)((L[1] + S[1])/2.0) };
    //BL_FLOAT Rp1[2] = { 0.0, (R[1] + S[1])/2.0 };
    BL_FLOAT Rp1[2] = { (BL_FLOAT)mRoomSize, (BL_FLOAT)((R[1] + S[1])/2.0) }; // FIXED!
    
    // Distances
    BL_FLOAT L0 = BLUtils::ComputeDist(S, Lp0) + BLUtils::ComputeDist(Lp0, L);
    BL_FLOAT L1 = BLUtils::ComputeDist(S, Lp1) + BLUtils::ComputeDist(Lp1, L);
    
    BL_FLOAT R0 = BLUtils::ComputeDist(S, Rp0) + BLUtils::ComputeDist(Rp0, R);
    BL_FLOAT R1 = BLUtils::ComputeDist(S, Rp1) + BLUtils::ComputeDist(Rp1, R);
    
    // Times
    BL_FLOAT L0s = L0/SOUND_SPEED;
    BL_FLOAT L1s = L1/SOUND_SPEED;
    
    BL_FLOAT R0s = R0/SOUND_SPEED;
    BL_FLOAT R1s = R1/SOUND_SPEED;
    
    // Delays
    BL_FLOAT Ld0 = L0s*mSampleRate;
    BL_FLOAT Ld1 = L1s*mSampleRate;
    
    BL_FLOAT Rd0 = R0s*mSampleRate;
    BL_FLOAT Rd1 = R1s*mSampleRate;
    
    // Set delays
    mDelays[0]->SetDelay(Ld0);
    mDelays[1]->SetDelay(Ld1);
    mDelays[2]->SetDelay(Rd0);
    mDelays[3]->SetDelay(Rd1);
}
