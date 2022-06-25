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
//  BLReverbViewer.h
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef __BL_Reverb__BLReverbViewer__
#define __BL_Reverb__BLReverbViewer__

#ifdef IGRAPHICS_NANOVG

class BLReverb;
//class MultiViewer;
class MultiViewer2;

class BLReverbViewer
{
public:
    BLReverbViewer(BLReverb *reverb, MultiViewer2 *viewer,
                   BL_FLOAT durationSeconds, BL_FLOAT sampleRate);
    
    virtual ~BLReverbViewer();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetDuration(BL_FLOAT durationSeconds);
    
    void Update();
    
protected:
    BLReverb *mReverb;
    MultiViewer2 *mViewer;
    
    BL_FLOAT mDurationSeconds;
    BL_FLOAT mSampleRate;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Reverb__BLReverbViewer__) */
