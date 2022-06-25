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
//  DNNModelDarknetMc.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelDarknetMc__
#define __BL_Rebalance__DNNModelDarknetMc__

#include <Rebalance_defs.h>
#include <DNNModelMc.h>

// Models trained directily inside Darknet
// DNNModelDarknetMc: from DNNModelDarknet
// - use mlutichannel (get all masks at once)
struct network;
class Scale;
class DNNModelDarknetMc : public DNNModelMc
{
public:
    DNNModelDarknetMc();
    
    virtual ~DNNModelDarknetMc();
    
    bool Load(const char *modelFileName,
              const char *resourcePath) override;
    
    // For WIN32
    bool LoadWin(IGraphics &pGraphics, const char* modelRcName,
                 const char* weightsRcName) override;
    
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 vector<WDL_TypedBuf<BL_FLOAT> > *masks) override;
    
    // TESTS
    void SetDbgThreshold(BL_FLOAT thrs);
    void SetMaskScale(int maskNum, BL_FLOAT scale);
    
protected:
    bool LoadWinTest(const char *modelFileName, const char *resourcePath);
    
    // Darknet model
    network *mNet;
    
    BL_FLOAT mDbgThreshold;
    
    BL_FLOAT mMaskScales[NUM_STEM_SOURCES];

    Scale *mScale;
};

#endif /* defined(__BL_Rebalance__DNNModelDarknetMc__) */
