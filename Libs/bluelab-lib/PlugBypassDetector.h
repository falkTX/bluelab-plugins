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
 
#ifndef PLUG_BYPASS_DETECTOR_H
#define PLUG_BYPASS_DETECTOR_H

// No so many host seem to support detection of plug bypass
// So use a custom object, to be touched in ProcessBlock(),
// and to be asked in OnIdle()
class PlugBypassDetector
{
 public:
    // 200 ms should be ok for 22050Hz, buffer size 2048
    // NOTE: 200ms is not enough, we have false positives
    PlugBypassDetector(int delayMs = 500/*200*/);
    virtual ~PlugBypassDetector();

    void TouchFromAudioThread();
    void SetTransportPlaying(bool flag);

    // Detects if OnIdle() is called lately
    // as the audio thread is still processing
    void TouchFromIdleThread();
    bool PlugIsBypassed();

 protected:
    int mDelayMs;
    
    long int mPrevAudioTouchTimeStamp;
    long int mPrevIdleTouchTimeStamp;
    
    bool mIsPlaying;
};

#endif
