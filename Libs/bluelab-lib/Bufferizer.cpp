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
 
#include <BLUtils.h>

#include "Bufferizer.h"

Bufferizer::Bufferizer(int bufferSize)
{
  mBufferSize = bufferSize;
}

Bufferizer::~Bufferizer() {}

void
Bufferizer::Reset()
{
    mValues.Resize(0);
}

void
Bufferizer::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
  mValues.Add(values.Get(), values.GetSize());
}

bool
Bufferizer::GetBuffer(WDL_TypedBuf<BL_FLOAT> *buffer)
{
  if (mValues.GetSize() < mBufferSize)
    return false;

  //buffer->Add(mValues.Get(), mBufferSize);
  BLUtils::CopyBuf(buffer, mValues.Get(), mBufferSize);
  
  BLUtils::ConsumeLeft(&mValues, mBufferSize);
  
  return true;
}
