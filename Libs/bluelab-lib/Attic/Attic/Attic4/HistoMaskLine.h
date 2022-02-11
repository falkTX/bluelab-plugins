//
//  HistoMaskLine.h
//  BL-Panogram
//
//  Created by applematuer on 10/22/19.
//
//

#ifndef __BL_Panogram__HistoMaskLine__
#define __BL_Panogram__HistoMaskLine__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class HistoMaskLine
{
public:
    HistoMaskLine(int bufferSize);
    
    virtual ~HistoMaskLine();

    void AddValue(int index, int value);
    
    void Apply(WDL_TypedBuf<BL_FLOAT> *values,
               int startIndex, int endIndex);
    
protected:
    vector<vector<int> > mBuffer;
};

#endif /* defined(__BL_Panogram__HistoMaskLine__) */
