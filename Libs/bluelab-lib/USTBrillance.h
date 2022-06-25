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
//  USTBrillance.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTBrillance__
#define __UST__USTBrillance__

#define MAX_NUM_CHANNELS 2

class CrossoverSplitterNBands4;

class USTBrillance
{
public:
    USTBrillance(BL_FLOAT sampleRate);
    
    virtual ~USTBrillance();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *samples,
                 BL_FLOAT brillance);
    
    void BypassProcess(vector<WDL_TypedBuf<BL_FLOAT> > *samples);
    
protected:
    CrossoverSplitterNBands4 *mSplitters[MAX_NUM_CHANNELS];
    
    CrossoverSplitterNBands4 *mBypassSplitters[MAX_NUM_CHANNELS];
};

#endif /* defined(__UST__USTBrillance__) */
