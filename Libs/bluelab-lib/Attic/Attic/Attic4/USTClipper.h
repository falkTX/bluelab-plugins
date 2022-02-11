//
//  USTClipper.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipper__
#define __UST__USTClipper__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class USTClipperDisplay;
class ClipperOverObj;

class USTClipper
{
public:
    USTClipper(GraphControl11 *graph, BL_FLOAT sampleRate);
    
    virtual ~USTClipper();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
protected:
    friend class ClipperOverObj;
    static void ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue);


    USTClipperDisplay *mClipperDisplay;
    
    BL_FLOAT mClipValue;
    
    ClipperOverObj *mClipObjs[2];
};

#endif /* defined(__UST__USTClipper__) */
