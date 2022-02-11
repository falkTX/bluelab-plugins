//
//  USTClipper2.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipper2__
#define __UST__USTClipper2__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class USTClipperDisplay2;
class ClipperOverObj2;

class USTClipper2
{
public:
    USTClipper2(GraphControl11 *graph, BL_FLOAT sampleRate);
    
    virtual ~USTClipper2();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
protected:
    friend class ClipperOverObj2;
    static void ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue);


    USTClipperDisplay2 *mClipperDisplay;
    
    BL_FLOAT mClipValue;
    
    ClipperOverObj2 *mClipObjs[2];
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__UST__USTClipper2__) */
