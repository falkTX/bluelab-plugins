//
//  DebugDataDumper.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 25/05/17.
//
//

#include <Debug.h>

#include "DebugDataDumper.h"

DebugDataDumper::DebugDataDumper(int maxCount)
{
    mMaxCount = maxCount;
    
    mCurrentCount = 0;
}

DebugDataDumper::~DebugDataDumper() {}

void
DebugDataDumper::AddData(const char *fileName, double *data, int dataSize)
{
    if (mCurrentCount >= mMaxCount)
        return;
    
    for (int i = 0; i < dataSize; i++)
        mData[fileName].push_back(data[i]);
}

void
DebugDataDumper::AddData(const char *fileName, const WDL_TypedBuf<double> *data)
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
    for (map<string, vector<double> >::iterator it = mData.begin(); it != mData.end(); it++)
    {
        const string &fileName = it->first;
        vector<double> &data = it->second;
        int dataSize = data.size();
        
        Debug::DumpData(fileName.c_str(), &data[0], dataSize);
    }
}

