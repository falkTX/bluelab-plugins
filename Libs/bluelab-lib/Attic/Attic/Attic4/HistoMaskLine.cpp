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
