//
//  DUETHistogram.h
//  BL-DUET
//
//  Created by applematuer on 5/3/20.
//
//

#ifndef __BL_DUET__DUETHistogram__
#define __BL_DUET__DUETHistogram__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class DUETHistogram
{
public:
    DUETHistogram(int width, int height, BL_FLOAT maxValue);
    
    virtual ~DUETHistogram();
    
    void Reset();
    
    void Clear();
    
    void AddValue(BL_FLOAT u, BL_FLOAT v, BL_FLOAT value, int sampleIndex = -1);
    
    void Process();
    
    int GetWidth();
    int GetHeight();
    void GetData(WDL_TypedBuf<BL_FLOAT> *data);
    
    void GetIndices(int histoIndex, vector<int> *indices);
    
    void SetTimeSmooth(BL_FLOAT smoothFactor);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;
    
    int mWidth;
    int mHeight;
    
    BL_FLOAT mMaxValue;
    
    // Smooth
    BL_FLOAT mSmoothFactor;
    WDL_TypedBuf<BL_FLOAT> mPrevData;
    
    // Indices of the samples
    vector<vector<int> > mIndices;
};

#endif /* defined(__BL_DUET__DUETHistogram__) */
