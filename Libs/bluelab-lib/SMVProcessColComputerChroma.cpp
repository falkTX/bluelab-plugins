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
//  SMVProcessColComputerChroma.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include "SMVProcessColComputerChroma.h"

#define LOG_SCALE_FACTOR 64.0 //128.0


SMVProcessColComputerChroma::SMVProcessColComputerChroma(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

SMVProcessColComputerChroma::~SMVProcessColComputerChroma() {}

void
SMVProcessColComputerChroma::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
SMVProcessColComputerChroma::ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                        const WDL_TypedBuf<BL_FLOAT> phases[2],
                                        const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                        WDL_TypedBuf<BL_FLOAT> *resultCol)
{
#define A_TUNE 440.0
    
    // Init
    resultCol->Resize(magns[0].GetSize());
    
    WDL_TypedBuf<BL_FLOAT> chromaBins;
    BLUtils::BinsToChromaBins(magns[0].GetSize(), &chromaBins, mSampleRate, (BL_FLOAT)A_TUNE);
    
    // Compute
    for (int i = 0; i < chromaBins.GetSize(); i++)
    {
        BL_FLOAT binVal = chromaBins.Get()[i];
        
        BL_FLOAT col = 0.0;
        if (chromaBins.GetSize() > 1)
            //col = binVal/(chromaBins.GetSize() - 1) - 0.5;
            col = binVal/(chromaBins.GetSize() - 1);
            
        resultCol->Get()[i] = col;
    }
    
#if 0 // Log scale
    BLUtils::LogScaleNorm2(resultCol, LOG_SCALE_FACTOR);
#endif
}
