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
 
#ifndef EQHACK_FFT_OBJ2_H
#define EQHACK_FFT_OBJ2_H

#include <FftProcessObj16.h>

#include <EQHackPluginInterface.h> // For Mode

#include "IGraphics_include_in_plug_hdr.h"

class FftProcessBufObj;
class EQHackFftObj2 : public ProcessObj
{
public:
    EQHackFftObj2(int bufferSize, int oversampling,
                  int freqRes, BL_FLOAT sampleRate);
  
    virtual ~EQHackFftObj2();
  
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);

    //void SetMode(EQHackPluginInterface::Mode mode);
    
    void GetDiffBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
  
protected:
    //EQHackPluginInterface::Mode mMode;
  
    WDL_TypedBuf<BL_FLOAT> mDiffBuffer;

private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
};

#endif
