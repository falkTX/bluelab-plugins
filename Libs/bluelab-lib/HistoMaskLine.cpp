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
//  HistoMaskLine.cpp
//  BL-Panogram
//
//  Created by applematuer on 10/22/19.
//
//

#include <BLUtils.h>

#include "HistoMaskLine.h"


HistoMaskLine::HistoMaskLine(int bufferSize)
{
    mBuffer.resize(bufferSize);
}

HistoMaskLine::~HistoMaskLine() {}

void
HistoMaskLine::AddValue(int index, int value)
{
    if (index >= mBuffer.size())
        return;
    
    mBuffer[index].push_back(value);
}

void
HistoMaskLine::Apply(WDL_TypedBuf<BL_FLOAT> *values,
                     int startIndex, int endIndex)
{
    if (startIndex < 0)
        return;
    if (startIndex >= mBuffer.size())
        return;
    if (endIndex >= mBuffer.size())
        return;
    
    WDL_TypedBuf<BL_FLOAT> newValues;
    newValues.Resize(values->GetSize());
    
    // Default value: 0.0
    BLUtils::FillAllZero(&newValues);
    
    // TEST: try to avoid musical noise
    //newValues = *values;
    //BLUtils::MultValues(&newValues, 0.25);
    
    for (int i = startIndex; i < endIndex; i++)
    {
        const vector<int> &indices = mBuffer[i];
        
        for (int j = 0; j < indices.size(); j++)
        {
            int idx = indices[j];
            if (idx >= values->GetSize())
                continue;
            
            BL_FLOAT val = values->Get()[idx];
            newValues.Get()[idx] = val;
        }
    }
    
    *values = newValues;
}
