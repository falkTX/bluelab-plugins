//
//  Timer.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/09/17.
//
//

#include "BlaTimer.h"

#ifdef WIN32

#include <Windows.h>

#else

#include <time.h>
#include <sys/time.h>

#endif

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


static unsigned long long
GetUptime()
{
#ifdef WIN32    
    return GetTickCount64();
#else
    
    struct timespec ts;

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;

#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif

    long tv_sec	= ts.tv_sec;
	long tv_nsec = ts.tv_nsec;
    
    unsigned long long uptime = static_cast<unsigned long long>(tv_sec)*1000ULL +
    static_cast<unsigned long long>(tv_nsec)/1000000ULL;

	return uptime;
#endif
}


BlaTimer::BlaTimer()
{
    Reset();
}

BlaTimer::~BlaTimer() {}

void
BlaTimer::Reset()
{
    mPrevTime = GetUptime();
    mAccumTime = 0;
    mIsStarted = false;
}

void
BlaTimer::Start()
{
    mPrevTime = GetUptime();
    mIsStarted = true;
}

void
BlaTimer::Stop()
{
    unsigned long int t = GetUptime();
    unsigned long int elapsed = t - mPrevTime;
    
    mAccumTime += elapsed;
    
    mPrevTime = GetUptime();
    
    mIsStarted = false;
}

unsigned long int
BlaTimer::Get()
{
    unsigned long int t = GetUptime();
    unsigned long int elapsed =  t - mPrevTime;
    
    if (mIsStarted)
        return elapsed + mAccumTime;
    
    return mAccumTime;
}
