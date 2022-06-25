/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
    // Change for launching on Windows Xp (For Lars, StereoWidth)
    // (Not integrated with the latest iPlug1 version of StereoWidth)
    //return GetTickCount64();
    return GetTickCount();
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

double
UpTime::GetUpTimeF()
{
#ifdef WIN32
 // TODO: use higher performance counter instead!
 // (we must be accurate near 1us for spectrogram/graphaxis smooth scroll
//        See: https://docs.microsoft.com/en-us/windows/win32/winmsg/about-timers
//    and https://docs.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
    
    // Change for launching on Windows Xp (For Lars, StereoWidth)
    // (Not integrated with the latest iPlug1 version of StereoWidth)

    // GetTickCount64() is just not limited to "up to 49.7 days"
    // Otherwise this should be the same
    //return GetTickCount64();

    // Use GetTickCount() not 64, to stay compatible with Windows Xp
    return GetTickCount();
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
    
    double uptime = tv_sec*1000.0 + tv_nsec/1000000.0;
    
	return uptime;
#endif
}
