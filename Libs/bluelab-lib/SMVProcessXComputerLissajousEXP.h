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
//  SMVProcessXComputerLissajousEXP.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputerLissajousEXP__
#define __BL_SoundMetaViewer__SMVProcessXComputerLissajousEXP__

#include <SMVProcessXComputer.h>

// Derived from Lissajous (but trick to get it in the frequency domain)
// => Makes very nice patters with Sine + Precedence (when varying the delay)
// => Looks a bit like V-Jaying / Artistic sound visualizer
//
// The basic mode (TOTAL_PSYCHE_EXPE=0) is like ifft(Lissajous)
class Axis3DFactory2;
class SMVProcessXComputerLissajousEXP : public SMVProcessXComputer
{
public:
    SMVProcessXComputerLissajousEXP(Axis3DFactory2 *axisFactory, BL_FLOAT sampleRate);
    
    virtual ~SMVProcessXComputerLissajousEXP();
    
    virtual void Reset(BL_FLOAT sampleRate) override;
    
    void ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                  const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  WDL_TypedBuf<BL_FLOAT> *resultX,
                  WDL_TypedBuf<BL_FLOAT> *resultY = NULL,
                  bool *isPolar = NULL,
                  BL_FLOAT polarCenter[2] = NULL,
                  bool *isScalable = NULL) override;
    
    Axis3D *CreateAxis() override;
    
protected:
    // Here just for tests
    void ComputeLissajous(const WDL_TypedBuf<BL_FLOAT> samples[2],
                          WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                          bool fitInSquare);

    // Used !
    void ComputeLissajousFft(const WDL_TypedBuf<BL_FLOAT> magns[2],
                             const WDL_TypedBuf<BL_FLOAT> phases[2],
                             WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                             bool fitInSquare);
    
    //
    Axis3DFactory2 *mAxisFactory;
    
    BL_FLOAT mSampleRate;

};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerLissajousEXP__) */
