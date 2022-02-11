//
//  UpTime.cpp
//  BL-PitchShift
//
//  Created by Pan on 30/07/18.
//
//

#include "UpTime.h"

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


unsigned long long
UpTime::GetUpTime()
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