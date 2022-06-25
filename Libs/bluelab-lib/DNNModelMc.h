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
//  DNNModelMc.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef BL_Rebalance_DNNModelMc_h
#define BL_Rebalance_DNNModelMc_h

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug::igraphics;

class DNNModelMc
{
public:
    DNNModelMc() {}
    
    virtual ~DNNModelMc() {};

    virtual bool Load(const char *modelFileName,
                      const char *resourcePath) = 0;
                
    // For WIN32
    //virtual bool LoadWin(IGraphics *pGraphics,
    //                     int modelRcId, int weightsRcId) = 0;
    virtual bool LoadWin(IGraphics &pGraphics, 
                         const char* modelRcName,
                         const char* weightsRcName) = 0;

    // Returns several masks at once
    virtual void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                         vector<WDL_TypedBuf<BL_FLOAT> > *masks) = 0;
    
    // TESTS
    //virtual void SetDbgThreshold(BL_FLOAT thrs) = 0;
    //virtual void SetMaskScale(int maskNum, BL_FLOAT scale) = 0;
};

#endif
