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
