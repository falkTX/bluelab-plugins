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
//  Timer.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/09/17.
//
//

#ifndef Transient_Timer_h
#define Transient_Timer_h

class BlaTimer
{
public:
    BlaTimer();
    
    virtual ~BlaTimer();
    
    void Reset();
    
    void Start();
    
    void Stop();
    
    unsigned long int Get();
    
    // Helpers
    static void Reset(BlaTimer *timer, long *count);
    
    static void Start(BlaTimer *timer);
    
    static void Stop(BlaTimer *timer);
    
    static void StopAndDump(BlaTimer *timer, long *count,
                            const char *fileName,
                            const char *message);

    
protected:
    unsigned long int mPrevTime;
    
    unsigned long int mAccumTime;
    
    bool mIsStarted;
};

#endif
