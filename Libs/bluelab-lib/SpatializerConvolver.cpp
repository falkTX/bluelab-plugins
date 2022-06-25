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
 
#include <SpatializerConvolver.h>

SpatializerConvolver::SpatializerConvolver(int bufferSize)
: FftConvolver6(bufferSize, true, true, false),
  mResponse(NULL) {}

void
SpatializerConvolver::Reset()
{
    FftConvolver6::Reset();
}

void
SpatializerConvolver::Reset(int oversampling, int freqRes)
{
    FftConvolver6::Reset();
}

void
SpatializerConvolver::SetBufferSize(int bufferSize)
{
#if TEST_CONVOLVER_VARIABLE_BUFFER
    FftConvolver6::Reset(bufferSize);
#endif
}

void
SpatializerConvolver::Flush()
{
    FftConvolver6::Flush();
}
       
void
SpatializerConvolver::SetParameter(void *param)
{
    WDL_TypedBuf<BL_FLOAT> *response = (WDL_TypedBuf<BL_FLOAT> *)param;
  
    if (mResponse != NULL)
        delete mResponse;
  
    // Stock a copy of the response in the memeber variable
    // So the pointer will still be valid
    mResponse = new WDL_TypedBuf<BL_FLOAT>();
    *mResponse = *response;
  
    FftConvolver6::SetResponse(mResponse);
}


void *
SpatializerConvolver::GetParameter()
{
    return (void *)mResponse;
}

bool
SpatializerConvolver::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    return FftConvolver6::Process(input, output, nFrames);
}
