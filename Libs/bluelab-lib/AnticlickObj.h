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
//  AnticlickObj.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#ifndef __Denoiser__AnticlickObj__
#define __Denoiser__AnticlickObj__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

// Logic X has a bug: when it restarts after stop,
// sometimes it provides full volume directly, making a sound crack.
// So we need to make a smooth transition by hand,
// each time we restart the play.
// We use an half Hanning on inputs for that, as Logic does (when it doesn't bug).
class AnticlickObj
{
public:
    enum Direction
    {
        OFF_TO_ON,
        ON_TO_OFF
    };
    
    AnticlickObj();
    
    virtual ~AnticlickObj();
    
    void Reset(Direction dir);
    
    bool MustProcessOnSignal();
    
    void SetOffSignal(const vector<WDL_TypedBuf<BL_FLOAT> > &bufs);
    void SetOnSignal(const vector<WDL_TypedBuf<BL_FLOAT> > &bufs);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *bufs);
    
protected:
    vector<WDL_TypedBuf<BL_FLOAT> > mOnSignal;
    vector<WDL_TypedBuf<BL_FLOAT> > mOffSignal;
    
    bool mNeedProcess;
    
    Direction mDirection;
};

#endif /* defined(__Denoiser__AnticlickObj__) */
