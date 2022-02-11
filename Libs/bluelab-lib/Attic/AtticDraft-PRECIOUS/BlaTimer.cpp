//
//  Timer.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/09/17.
//
//

#include "BlaTimer.h"

#if 0

// https://stackoverflow.com/questions/30095439/how-do-i-get-system-up-time-in-milliseconds-in-c

// Includes for get_uptime()
#if defined(WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__linux) || defined(linux)
#include <sys/sysinfo.h>
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <time.h>
#include <errno.h>
#include <sys/sysctl.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#include <time.h>
#endif


static long int
get_uptime()
{
#if defined(BOOST_WINDOWS)
    return GetTickCount() / 1000u;
#elif defined(__linux__) || defined(__linux) || defined(linux)
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        return -1;
    return info.uptime;
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    struct timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0)
        return -1;
    return time(NULL) - boottime.tv_sec;
#elif (defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)) && defined(CLOCK_UPTIME)
    struct timespec ts;
    if (clock_gettime(CLOCK_UPTIME, &ts) != 0)
        return -1;
    return ts.tv_sec;
#else
    return 0;
#endif
}
#endif

#if 0

#if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <time.h>
#include <errno.h>
#include <sys/sysctl.h>

static unsigned long long
GetUptime()
{
    unsigned long long uptime(0u);
    struct timeval ts;
    size_t len = sizeof(ts);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &ts, &len, NULL, 0) == 0)
    {
        uptime = static_cast<unsigned long long>(ts.tv_sec)*1000ULL +
                 static_cast<unsigned long long>(ts.tv_usec)/1000ULL;
    }
    
    return uptime;
}
#else
NOT IMPLEMENTED
#endif
#endif

#if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


static unsigned long long
GetUptime()
{
#ifdef WIN32
    CHECK THIS !!!
    
    return GetTickCount() / 1000u;
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
#endif
    
    return uptime;
}

BlaTimer::BlaTimer()
{
    Reset();
}

BlaTimer::~BlaTimer() {}

void
BlaTimer::Reset()
{
    //mPrevTime = get_uptime();
    
    mPrevTime = GetUptime();
}

unsigned long int
BlaTimer::Get()
{
    //unsigned long int t = get_uptime();
    
    unsigned long int t = GetUptime();
    
    return t - mPrevTime;
}
