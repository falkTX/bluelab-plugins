//
//  HistoMaskLine2.h
//  BL-Panogram
//
//  Created by applematuer on 10/22/19.
//
//

#ifndef __BL_Panogram__HistoMaskLine2__
#define __BL_Panogram__HistoMaskLine2__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// From HistoMaskLine
// - try to optimize by avoiding array of vectors
// => optim Panogram by 30%
class HistoMaskLine2
{
public:
    HistoMaskLine2(int bufferSize);
    HistoMaskLine2();
    
    virtual ~HistoMaskLine2();

    void Reset(int bufferSize);
    
    void AddValue(int index, int value);
    
    void Apply(WDL_TypedBuf<BL_FLOAT> *values,
               int startIndex, int endIndex);

    void GetValues(vector<int> *values);
    
protected:
    vector<int> mBuffer;
};

#endif /* defined(__BL_Panogram__HistoMaskLine2__) */
