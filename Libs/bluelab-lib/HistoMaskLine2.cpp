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
//  HistoMaskLine2.cpp
//  BL-Panogram
//
//  Created by applematuer on 10/22/19.
//
//

#include <BLUtils.h>

#include "HistoMaskLine2.h"


HistoMaskLine2::HistoMaskLine2(int bufferSize)
{
    Reset(bufferSize);
}

HistoMaskLine2::HistoMaskLine2() {}

HistoMaskLine2::~HistoMaskLine2() {}

void
HistoMaskLine2::Reset(int bufferSize)
{
    mBuffer.resize(bufferSize/2);
    
#if 1 // Correct init => so if a value is not set, we won't take it in apply
    for (int i = 0; i < bufferSize/2; i++)
        mBuffer[i] = -1;
#endif
}

void
HistoMaskLine2::AddValue(int index, int value)
{
    if (value >= mBuffer.size())
        return;
    
    mBuffer[value] = index;
}

void
HistoMaskLine2::Apply(WDL_TypedBuf<BL_FLOAT> *values,
                     int startIndex, int endIndex)
{
    WDL_TypedBuf<BL_FLOAT> newValues;
    newValues.Resize(values->GetSize());
    
    // Default value: 0.0
    BLUtils::FillAllZero(&newValues);
    
    // TEST: try to avoid musical noise
    //newValues = *values;
    //BLUtils::MultValues(&newValues, 0.25);
    
    for (int i = 0; i < mBuffer.size(); i++)
    {
        int idx = mBuffer[i];
        
        if ((idx >= startIndex) && (idx <= endIndex))
        {
            BL_FLOAT val = values->Get()[i];
            newValues.Get()[i] = val;
        }
    }
    
    *values = newValues;
}

void
HistoMaskLine2::GetValues(vector<int> *values) 
{
  *values = mBuffer;
}
