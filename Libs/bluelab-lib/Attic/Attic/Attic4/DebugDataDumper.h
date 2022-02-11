//
//  DebugDataDumper.h
//  Transient
//
//  Created by Apple m'a Tuer on 25/05/17.
//
//

#ifndef __Transient__DebugDataDumper__
#define __Transient__DebugDataDumper__

#include <map>
#include <string>
#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

class DebugDataDumper
{
public:
    DebugDataDumper(int maxCount);
    
    virtual ~DebugDataDumper();
    
    void AddData(const char *fileName, BL_FLOAT *data, int dataSize);
    
    void AddData(const char *fileName, const WDL_TypedBuf<BL_FLOAT> *data);
    
    void NextFrame();
    
protected:
    void Dump();
    
    int mMaxCount;
    
    int mCurrentCount;
    
    map<string, vector<BL_FLOAT> > mData;
};

#endif /* defined(__Transient__DebugDataDumper__) */
