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
//  DebugDataDumper.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 25/05/17.
//
//

#include <BLDebug.h>

#include "DebugDataDumper.h"

DebugDataDumper::DebugDataDumper(int maxCount)
{
    mMaxCount = maxCount;
    
    mCurrentCount = 0;
}

DebugDataDumper::~DebugDataDumper() {}

void
DebugDataDumper::AddData(const char *fileName, BL_FLOAT *data, int dataSize)
{
    if (mCurrentCount >= mMaxCount)
        return;
    
    for (int i = 0; i < dataSize; i++)
        mData[fileName].push_back(data[i]);
}

void
DebugDataDumper::AddData(const char *fileName, const WDL_TypedBuf<BL_FLOAT> *data)
{
    AddData(fileName, data->Get(), data->GetSize());
}

void
DebugDataDumper::NextFrame()
{
    mCurrentCount++;
    
    if (mCurrentCount == mMaxCount)
        Dump();
}


void
DebugDataDumper::Dump()
{
    for (map<string, vector<BL_FLOAT> >::iterator it = mData.begin(); it != mData.end(); it++)
    {
        const string &fileName = it->first;
        vector<BL_FLOAT> &data = it->second;
        int dataSize = data.size();
        
        BLDebug::DumpData(fileName.c_str(), &data[0], dataSize);
    }
}

