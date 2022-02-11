//
//  USTClipper3.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipper3__
#define __UST__USTClipper3__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class USTClipperDisplay3;
class USTClipperDisplay4;
class ClipperOverObj3;

class USTClipper3
{
public:
    USTClipper3(GraphControl11 *graph, BL_FLOAT sampleRate);
    
    virtual ~USTClipper3();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetEnabled(bool flag);
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void SetZoom(BL_FLOAT zoom);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
protected:
    friend class ClipperOverObj3;
    static void ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue);


    USTClipperDisplay4 *mClipperDisplay;
    
    BL_FLOAT mClipValue;
    
    ClipperOverObj3 *mClipObjs[2];
    
    bool mIsEnabled;
    BL_FLOAT mSampleRate;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__UST__USTClipper3__) */
