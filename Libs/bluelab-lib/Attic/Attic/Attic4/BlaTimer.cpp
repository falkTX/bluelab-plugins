//
//  Timer.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/09/17.
//
//

#include <stdio.h>

#include <BLDebug.h>
#include <UpTime.h>

#include "BlaTimer.h"

BlaTimer::BlaTimer()
{
    Reset();
}

BlaTimer::~BlaTimer() {}

void
BlaTimer::Reset()
{
    mPrevTime = UpTime::GetUpTime();
    mAccumTime = 0;
    mIsStarted = false;
}

void
BlaTimer::Start()
{
    mPrevTime = UpTime::GetUpTime();
    mIsStarted = true;
}

void
BlaTimer::Stop()
{
    unsigned long int t = UpTime::GetUpTime();
    unsigned long int elapsed = t - mPrevTime;
    
    mAccumTime += elapsed;
    
    mPrevTime = UpTime::GetUpTime();
    
    mIsStarted = false;
}

unsigned long int
BlaTimer::Get()
{
    unsigned long int t = UpTime::GetUpTime();
    unsigned long int elapsed =  t - mPrevTime;
    
    if (mIsStarted)
        return elapsed + mAccumTime;
    
    return mAccumTime;
}

// Helpers
void
BlaTimer::Reset(BlaTimer *timer, long *count)
{
    timer->Reset();
    *count = 0;
}

void
BlaTimer::Start(BlaTimer *timer)
{
    timer->Start();
}

void
BlaTimer::Stop(BlaTimer *timer)
{
    timer->Stop();
}

void
BlaTimer::StopAndDump(BlaTimer *timer, long *count,
                      const char *fileName,
                      const char *message)
{
    timer->Stop();

    if ((*count)++ > 50)
    {
        long t = timer->Get();
    
        char message0[1024];
        sprintf(message0, message, t);
    
        BLDebug::AppendMessage(fileName, message0);
    
        timer->Reset();
    
        *count = 0;
    }
}
